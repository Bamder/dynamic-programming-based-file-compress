/*
 * C++17 DP image compression demo.
 *
 * Image IO:
 * - If OpenCV headers are available, this file uses OpenCV automatically.
 * - Otherwise it falls back to stb under src/native/third_party/stb/, or install
 *   OpenCV and compile without FORCE_STB.
 *
 * Size policy:
 * - USE_FIXED_SIZE = true  : convert input images to 512 x 512 before DP.
 * - USE_FIXED_SIZE = false : keep the original image size.
 *
 * Complexity:
 * - For one channel, the DP checks at most 256 possible last-segment lengths
 *   for every pixel, so the time complexity is O(256n), usually treated as O(n).
 * - RGB compression runs the same DP independently for R/G/B, so the time
 *   complexity is about O(3 * 256n).
 * - Space complexity is O(n) for one channel; RGB uses about O(3n) for pixels
 *   plus each channel's DP arrays.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/compressor/bit_io.h"
#include "../include/compressor/compress_algorithm.h"
#include "../include/compressor/image_compress.h"
#include "../include/compressor/stream_io.h"

#if !defined(FORCE_STB) && __has_include(<opencv2/opencv.hpp>)
#define DPIC_USE_OPENCV 1
#include <opencv2/opencv.hpp>
#else
#define DPIC_USE_STB 1
#if __has_include("../third_party/stb/stb_image.h") && __has_include("../third_party/stb/stb_image_write.h")
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"
#else
#error "OpenCV was not found. Add stb headers under src/native/third_party/stb/, or install OpenCV."
#endif
#endif

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::runtime_error;
using std::size_t;
using std::string;
using std::vector;

constexpr bool USE_FIXED_SIZE = true;
constexpr int FIXED_WIDTH = 512;
constexpr int FIXED_HEIGHT = 512;

struct GrayImage {
    int width = 0;
    int height = 0;
    vector<int> pixels;
};

struct RGBImage {
    int width = 0;
    int height = 0;
    vector<int> r;
    vector<int> g;
    vector<int> b;
};

DPResult compressPixels(const vector<int>& pixels) {
    return dpCompress(pixels);
}

static uint8_t toByte(int value) {
    if (value < 0 || value > 255) {
        throw runtime_error("像素值必须在 0~255 之间");
    }
    return static_cast<uint8_t>(value);
}

static int outputWidth(int width) {
    return USE_FIXED_SIZE ? FIXED_WIDTH : width;
}

static int outputHeight(int height) {
    return USE_FIXED_SIZE ? FIXED_HEIGHT : height;
}

#if DPIC_USE_OPENCV

GrayImage loadGrayImage(const string& path, bool apply_fixed_size) {
    cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
    if (image.empty()) {
        throw runtime_error("无法读取图像：" + path);
    }

    if (apply_fixed_size && USE_FIXED_SIZE &&
        (image.cols != FIXED_WIDTH || image.rows != FIXED_HEIGHT)) {
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(FIXED_WIDTH, FIXED_HEIGHT), 0, 0, cv::INTER_LINEAR);
        image = resized;
    }

    GrayImage out;
    out.width = image.cols;
    out.height = image.rows;
    out.pixels.reserve(static_cast<size_t>(out.width) * out.height);

    for (int y = 0; y < out.height; ++y) {
        const uint8_t* row = image.ptr<uint8_t>(y);
        for (int x = 0; x < out.width; ++x) {
            out.pixels.push_back(row[x]);
        }
    }
    return out;
}

RGBImage loadRGBImage(const string& path, bool apply_fixed_size) {
    cv::Mat bgr = cv::imread(path, cv::IMREAD_COLOR);
    if (bgr.empty()) {
        throw runtime_error("无法读取图像：" + path);
    }

    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);

    if (apply_fixed_size && USE_FIXED_SIZE &&
        (rgb.cols != FIXED_WIDTH || rgb.rows != FIXED_HEIGHT)) {
        cv::Mat resized;
        cv::resize(rgb, resized, cv::Size(FIXED_WIDTH, FIXED_HEIGHT), 0, 0, cv::INTER_LINEAR);
        rgb = resized;
    }

    RGBImage out;
    out.width = rgb.cols;
    out.height = rgb.rows;
    const size_t total = static_cast<size_t>(out.width) * out.height;
    out.r.reserve(total);
    out.g.reserve(total);
    out.b.reserve(total);

    for (int y = 0; y < out.height; ++y) {
        const cv::Vec3b* row = rgb.ptr<cv::Vec3b>(y);
        for (int x = 0; x < out.width; ++x) {
            out.r.push_back(row[x][0]);
            out.g.push_back(row[x][1]);
            out.b.push_back(row[x][2]);
        }
    }
    return out;
}

void writeGrayImage(const string& path, int width, int height, const vector<int>& pixels) {
    if (pixels.size() != static_cast<size_t>(width) * height) {
        throw runtime_error("灰度图像素数量与尺寸不匹配");
    }

    cv::Mat image(height, width, CV_8UC1);
    size_t index = 0;
    for (int y = 0; y < height; ++y) {
        uint8_t* row = image.ptr<uint8_t>(y);
        for (int x = 0; x < width; ++x) {
            row[x] = toByte(pixels[index++]);
        }
    }

    if (!cv::imwrite(path, image)) {
        throw runtime_error("无法写出图像：" + path);
    }
}

void writeRGBImage(const string& path, int width, int height,
                   const vector<int>& r, const vector<int>& g, const vector<int>& b) {
    const size_t total = static_cast<size_t>(width) * height;
    if (r.size() != total || g.size() != total || b.size() != total) {
        throw runtime_error("RGB 图像素数量与尺寸不匹配");
    }

    cv::Mat rgb(height, width, CV_8UC3);
    for (int y = 0; y < height; ++y) {
        cv::Vec3b* row = rgb.ptr<cv::Vec3b>(y);
        for (int x = 0; x < width; ++x) {
            const size_t index = static_cast<size_t>(y) * width + x;
            row[x] = cv::Vec3b(toByte(r[index]), toByte(g[index]), toByte(b[index]));
        }
    }

    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
    if (!cv::imwrite(path, bgr)) {
        throw runtime_error("无法写出图像：" + path);
    }
}

#else

vector<uint8_t> resizeInterleavedNearest(const vector<uint8_t>& input,
                                         int width,
                                         int height,
                                         int channels,
                                         int new_width,
                                         int new_height) {
    if (width == new_width && height == new_height) {
        return input;
    }

    vector<uint8_t> output(static_cast<size_t>(new_width) * new_height * channels);
    for (int y = 0; y < new_height; ++y) {
        const int src_y = std::min(height - 1, y * height / new_height);
        for (int x = 0; x < new_width; ++x) {
            const int src_x = std::min(width - 1, x * width / new_width);
            const size_t src_index = (static_cast<size_t>(src_y) * width + src_x) * channels;
            const size_t dst_index = (static_cast<size_t>(y) * new_width + x) * channels;
            for (int c = 0; c < channels; ++c) {
                output[dst_index + c] = input[src_index + c];
            }
        }
    }
    return output;
}

GrayImage loadGrayImage(const string& path, bool apply_fixed_size) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* raw = stbi_load(path.c_str(), &width, &height, &channels, 1);
    if (!raw) {
        throw runtime_error("无法读取图像：" + path);
    }

    vector<uint8_t> data(raw, raw + static_cast<size_t>(width) * height);
    stbi_image_free(raw);

    int final_width = width;
    int final_height = height;
    if (apply_fixed_size && USE_FIXED_SIZE) {
        final_width = outputWidth(width);
        final_height = outputHeight(height);
        data = resizeInterleavedNearest(data, width, height, 1, final_width, final_height);
    }

    GrayImage out;
    out.width = final_width;
    out.height = final_height;
    out.pixels.reserve(static_cast<size_t>(out.width) * out.height);
    for (uint8_t value : data) {
        out.pixels.push_back(value);
    }
    return out;
}

RGBImage loadRGBImage(const string& path, bool apply_fixed_size) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* raw = stbi_load(path.c_str(), &width, &height, &channels, 3);
    if (!raw) {
        throw runtime_error("无法读取图像：" + path);
    }

    vector<uint8_t> data(raw, raw + static_cast<size_t>(width) * height * 3);
    stbi_image_free(raw);

    int final_width = width;
    int final_height = height;
    if (apply_fixed_size && USE_FIXED_SIZE) {
        final_width = outputWidth(width);
        final_height = outputHeight(height);
        data = resizeInterleavedNearest(data, width, height, 3, final_width, final_height);
    }

    RGBImage out;
    out.width = final_width;
    out.height = final_height;
    const size_t total = static_cast<size_t>(out.width) * out.height;
    out.r.reserve(total);
    out.g.reserve(total);
    out.b.reserve(total);

    for (size_t i = 0; i < total; ++i) {
        out.r.push_back(data[i * 3 + 0]);
        out.g.push_back(data[i * 3 + 1]);
        out.b.push_back(data[i * 3 + 2]);
    }
    return out;
}

void writeGrayImage(const string& path, int width, int height, const vector<int>& pixels) {
    if (pixels.size() != static_cast<size_t>(width) * height) {
        throw runtime_error("灰度图像素数量与尺寸不匹配");
    }

    vector<uint8_t> data(pixels.size());
    for (size_t i = 0; i < pixels.size(); ++i) {
        data[i] = toByte(pixels[i]);
    }

    if (!stbi_write_png(path.c_str(), width, height, 1, data.data(), width)) {
        throw runtime_error("无法写出图像：" + path);
    }
}

void writeRGBImage(const string& path, int width, int height,
                   const vector<int>& r, const vector<int>& g, const vector<int>& b) {
    const size_t total = static_cast<size_t>(width) * height;
    if (r.size() != total || g.size() != total || b.size() != total) {
        throw runtime_error("RGB 图像素数量与尺寸不匹配");
    }

    vector<uint8_t> data(total * 3);
    for (size_t i = 0; i < total; ++i) {
        data[i * 3 + 0] = toByte(r[i]);
        data[i * 3 + 1] = toByte(g[i]);
        data[i * 3 + 2] = toByte(b[i]);
    }

    if (!stbi_write_png(path.c_str(), width, height, 3, data.data(), width * 3)) {
        throw runtime_error("无法写出图像：" + path);
    }
}

#endif

uint16_t checkedU16(int value, const string& name) {
    if (value < 0 || value > std::numeric_limits<uint16_t>::max()) {
        throw runtime_error(name + " 超出 uint16 范围");
    }
    return static_cast<uint16_t>(value);
}

uint32_t checkedU32(size_t value, const string& name) {
    if (value > std::numeric_limits<uint32_t>::max()) {
        throw runtime_error(name + " 超出 uint32 范围");
    }
    return static_cast<uint32_t>(value);
}

long long fileSizeBits(const string& path) {
    ifstream in(path, ios::binary | ios::ate);
    if (!in) {
        throw runtime_error("无法获取文件大小：" + path);
    }
    return static_cast<long long>(in.tellg()) * 8LL;
}

map<int, int> bitWidthDistribution(const vector<Segment>& segments) {
    map<int, int> dist;
    for (const Segment& seg : segments) {
        ++dist[seg.bit_width];
    }
    return dist;
}

void printDistribution(const string& title, const vector<Segment>& segments) {
    cout << title << endl;
    const map<int, int> dist = bitWidthDistribution(segments);
    for (const auto& [bit_width, count] : dist) {
        cout << bit_width << " bit：" << count << " 段" << endl;
    }
}

void printFirstSegments(const vector<Segment>& segments, int limit = 10) {
    const int count = std::min<int>(limit, static_cast<int>(segments.size()));
    for (int i = 0; i < count; ++i) {
        const Segment& seg = segments[i];
        cout << "{start: " << seg.start
             << ", end: " << seg.end
             << ", length: " << seg.length
             << ", bit_width: " << seg.bit_width << "}" << endl;
    }
}

void printGrayStats(const string& input_path) {
    GrayImage image = loadGrayImage(input_path, true);
    const long long original_bits = static_cast<long long>(image.pixels.size()) * 8LL;

    const auto start = std::chrono::high_resolution_clock::now();
    DPResult result = compressPixels(image.pixels);
    const auto end = std::chrono::high_resolution_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();

    const double compression_ratio = static_cast<double>(result.min_bits) / original_bits;
    const double saved_ratio = 1.0 - compression_ratio;

    cout << "========== DP 灰度图像压缩结果 ==========" << endl;
    cout << "图像尺寸：" << image.width << " x " << image.height << endl;
    cout << "像素数量：" << image.pixels.size() << endl;
    cout << "原始存储空间：" << original_bits << " bit" << endl;
    cout << "压缩后存储空间：" << result.min_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "压缩率：" << compression_ratio << endl;
    cout << std::setprecision(2);
    cout << "节省比例：" << saved_ratio * 100.0 << "%" << endl;
    cout << std::setprecision(4);
    cout << "执行时间：" << seconds << " 秒" << endl;
    cout << "最优分段数量：" << result.segments.size() << endl;
    cout << endl;
    printDistribution("各位宽分段数量统计：", result.segments);
    cout << endl;
    cout << "前 10 个分段结果：" << endl;
    printFirstSegments(result.segments);
}

void printRGBStats(const string& input_path) {
    RGBImage image = loadRGBImage(input_path, true);
    const size_t pixel_count = image.r.size();
    const long long channel_original_bits = static_cast<long long>(pixel_count) * 8LL;
    const long long rgb_original_bits = static_cast<long long>(pixel_count) * 24LL;

    const auto start = std::chrono::high_resolution_clock::now();
    DPResult r_result = compressPixels(image.r);
    DPResult g_result = compressPixels(image.g);
    DPResult b_result = compressPixels(image.b);
    const auto end = std::chrono::high_resolution_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();

    const long long total_bits = r_result.min_bits + g_result.min_bits + b_result.min_bits;
    const double r_ratio = static_cast<double>(r_result.min_bits) / channel_original_bits;
    const double g_ratio = static_cast<double>(g_result.min_bits) / channel_original_bits;
    const double b_ratio = static_cast<double>(b_result.min_bits) / channel_original_bits;
    const double total_ratio = static_cast<double>(total_bits) / rgb_original_bits;
    const double saved_ratio = 1.0 - total_ratio;

    cout << "========== RGB 彩色图像 DP 压缩结果 ==========" << endl;
    cout << "图像尺寸：" << image.width << " x " << image.height << endl;
    cout << "原始 RGB 存储空间：" << rgb_original_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "R 通道压缩后：" << r_result.min_bits << " bit，压缩率：" << r_ratio << endl;
    cout << "G 通道压缩后：" << g_result.min_bits << " bit，压缩率：" << g_ratio << endl;
    cout << "B 通道压缩后：" << b_result.min_bits << " bit，压缩率：" << b_ratio << endl;
    cout << "RGB 总压缩后：" << total_bits << " bit" << endl;
    cout << "RGB 总压缩率：" << total_ratio << endl;
    cout << std::setprecision(2);
    cout << "节省比例：" << saved_ratio * 100.0 << "%" << endl;
    cout << std::setprecision(4);
    cout << "执行时间：" << seconds << " 秒" << endl;
    cout << "R 通道分段数量：" << r_result.segments.size() << endl;
    cout << "G 通道分段数量：" << g_result.segments.size() << endl;
    cout << "B 通道分段数量：" << b_result.segments.size() << endl;
    cout << endl;
    printDistribution("R 通道位宽分布统计：", r_result.segments);
    cout << endl;
    printDistribution("G 通道位宽分布统计：", g_result.segments);
    cout << endl;
    printDistribution("B 通道位宽分布统计：", b_result.segments);
}

void compressGrayToFile(const string& input_path, const string& output_path) {
    GrayImage image = loadGrayImage(input_path, true);
    const auto start = std::chrono::high_resolution_clock::now();
    DPResult result = compressPixels(image.pixels);
    const auto end = std::chrono::high_resolution_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();

    BitWriter writer;
    writeSegmentBitstream(writer, image.pixels, result.segments);
    vector<uint8_t> payload = writer.toBytes();

    ofstream out(output_path, ios::binary);
    if (!out) {
        throw runtime_error("无法创建压缩文件：" + output_path);
    }

    out.write("DPGC", 4);
    writeU16LE(out, checkedU16(image.width, "width"));
    writeU16LE(out, checkedU16(image.height, "height"));
    writeU32LE(out, checkedU32(image.pixels.size(), "pixel_count"));
    writeU32LE(out, checkedU32(result.segments.size(), "segment_count"));
    out.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    out.close();

    const long long original_bits = static_cast<long long>(image.pixels.size()) * 8LL;
    const long long actual_file_bits = fileSizeBits(output_path);

    cout << "========== 灰度压缩文件生成完成 ==========" << endl;
    cout << "输入图片：" << input_path << endl;
    cout << "压缩文件：" << output_path << endl;
    cout << "图像尺寸：" << image.width << " x " << image.height << endl;
    cout << "原始像素数据：" << original_bits << " bit" << endl;
    cout << "DP 模型压缩后：" << result.min_bits << " bit" << endl;
    cout << "实际压缩文件大小：" << actual_file_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "DP 模型压缩率：" << static_cast<double>(result.min_bits) / original_bits << endl;
    cout << "实际文件压缩率：" << static_cast<double>(actual_file_bits) / original_bits << endl;
    cout << "分段数量：" << result.segments.size() << endl;
    cout << "DP 计算时间：" << seconds << " 秒" << endl;
}

void decompressGrayToImage(const string& input_path, const string& output_path) {
    ifstream in(input_path, ios::binary);
    if (!in) {
        throw runtime_error("无法打开压缩文件：" + input_path);
    }

    char magic[4] = {0, 0, 0, 0};
    in.read(magic, 4);
    if (!in || std::memcmp(magic, "DPGC", 4) != 0) {
        throw runtime_error("不是合法的 DPGC 压缩文件");
    }

    const uint16_t width = readU16LE(in);
    const uint16_t height = readU16LE(in);
    const uint32_t pixel_count = readU32LE(in);
    const uint32_t segment_count = readU32LE(in);
    if (static_cast<uint64_t>(width) * height != pixel_count) {
        throw runtime_error("文件头中的尺寸与 pixel_count 不一致");
    }

    const vector<uint8_t> payload = readRemainingBytes(in);
    BitReader reader(payload);
    vector<int> pixels = readSegmentBitstream(reader, pixel_count, segment_count);
    writeGrayImage(output_path, width, height, pixels);

    cout << "========== 灰度解压完成 ==========" << endl;
    cout << "压缩文件：" << input_path << endl;
    cout << "还原图片：" << output_path << endl;
    cout << "图像尺寸：" << width << " x " << height << endl;
    cout << "像素数量：" << pixels.size() << endl;
}

bool sameVector(const vector<int>& a, const vector<int>& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

void verifyGrayImage(const string& original_path, const string& recovered_path) {
    GrayImage original = loadGrayImage(original_path, true);
    GrayImage recovered = loadGrayImage(recovered_path, false);

    const bool same_size = original.width == recovered.width && original.height == recovered.height;
    const bool same_pixels = same_size && sameVector(original.pixels, recovered.pixels);

    cout << "========== 灰度还原验证 ==========" << endl;
    cout << "原始图片：" << original_path << endl;
    cout << "还原图片：" << recovered_path << endl;
    cout << "原始灰度尺寸：" << original.width << " x " << original.height << endl;
    cout << "还原图像尺寸：" << recovered.width << " x " << recovered.height << endl;
    cout << "是否完全一致：" << (same_pixels ? "True" : "False") << endl;

    if (!same_pixels) {
        throw runtime_error("[Verify]灰度 roundtrip 校验失败：像素不一致");
    }
}

void compressRGBToFile(const string& input_path, const string& output_path) {
    RGBImage image = loadRGBImage(input_path, true);

    const auto start = std::chrono::high_resolution_clock::now();
    DPResult r_result = compressPixels(image.r);
    DPResult g_result = compressPixels(image.g);
    DPResult b_result = compressPixels(image.b);
    const auto end = std::chrono::high_resolution_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();

    BitWriter writer;
    writeSegmentBitstream(writer, image.r, r_result.segments);
    writeSegmentBitstream(writer, image.g, g_result.segments);
    writeSegmentBitstream(writer, image.b, b_result.segments);
    vector<uint8_t> payload = writer.toBytes();

    ofstream out(output_path, ios::binary);
    if (!out) {
        throw runtime_error("无法创建压缩文件：" + output_path);
    }

    out.write("DPRC", 4);
    writeU16LE(out, checkedU16(image.width, "width"));
    writeU16LE(out, checkedU16(image.height, "height"));
    writeU32LE(out, checkedU32(image.r.size(), "pixel_count"));
    writeU32LE(out, checkedU32(r_result.segments.size(), "R segment_count"));
    writeU32LE(out, checkedU32(g_result.segments.size(), "G segment_count"));
    writeU32LE(out, checkedU32(b_result.segments.size(), "B segment_count"));
    out.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    out.close();

    const long long original_bits = static_cast<long long>(image.r.size()) * 24LL;
    const long long model_bits = r_result.min_bits + g_result.min_bits + b_result.min_bits;
    const long long actual_file_bits = fileSizeBits(output_path);

    cout << "========== RGB 压缩文件生成完成 ==========" << endl;
    cout << "输入图片：" << input_path << endl;
    cout << "压缩文件：" << output_path << endl;
    cout << "图像尺寸：" << image.width << " x " << image.height << endl;
    cout << "原始 RGB 像素数据：" << original_bits << " bit" << endl;
    cout << "DP 模型压缩后：" << model_bits << " bit" << endl;
    cout << "实际压缩文件大小：" << actual_file_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "DP 模型压缩率：" << static_cast<double>(model_bits) / original_bits << endl;
    cout << "实际文件压缩率：" << static_cast<double>(actual_file_bits) / original_bits << endl;
    cout << "R 通道分段数量：" << r_result.segments.size() << endl;
    cout << "G 通道分段数量：" << g_result.segments.size() << endl;
    cout << "B 通道分段数量：" << b_result.segments.size() << endl;
    cout << "DP 计算时间：" << seconds << " 秒" << endl;
}

void decompressRGBToImage(const string& input_path, const string& output_path) {
    ifstream in(input_path, ios::binary);
    if (!in) {
        throw runtime_error("无法打开压缩文件：" + input_path);
    }

    char magic[4] = {0, 0, 0, 0};
    in.read(magic, 4);
    if (!in || std::memcmp(magic, "DPRC", 4) != 0) {
        throw runtime_error("不是合法的 DPRC 压缩文件");
    }

    const uint16_t width = readU16LE(in);
    const uint16_t height = readU16LE(in);
    const uint32_t pixel_count = readU32LE(in);
    const uint32_t r_segment_count = readU32LE(in);
    const uint32_t g_segment_count = readU32LE(in);
    const uint32_t b_segment_count = readU32LE(in);
    if (static_cast<uint64_t>(width) * height != pixel_count) {
        throw runtime_error("文件头中的尺寸与 pixel_count 不一致");
    }

    const vector<uint8_t> payload = readRemainingBytes(in);
    BitReader reader(payload);
    vector<int> r = readSegmentBitstream(reader, pixel_count, r_segment_count);
    vector<int> g = readSegmentBitstream(reader, pixel_count, g_segment_count);
    vector<int> b = readSegmentBitstream(reader, pixel_count, b_segment_count);
    writeRGBImage(output_path, width, height, r, g, b);

    cout << "========== RGB 解压完成 ==========" << endl;
    cout << "压缩文件：" << input_path << endl;
    cout << "还原图片：" << output_path << endl;
    cout << "图像尺寸：" << width << " x " << height << endl;
    cout << "像素数量：" << pixel_count << endl;
}

void verifyRGBImage(const string& original_path, const string& recovered_path) {
    RGBImage original = loadRGBImage(original_path, true);
    RGBImage recovered = loadRGBImage(recovered_path, false);

    const bool same_size = original.width == recovered.width && original.height == recovered.height;
    const bool same_pixels = same_size &&
                             sameVector(original.r, recovered.r) &&
                             sameVector(original.g, recovered.g) &&
                             sameVector(original.b, recovered.b);

    cout << "========== RGB 还原验证 ==========" << endl;
    cout << "原始图片：" << original_path << endl;
    cout << "还原图片：" << recovered_path << endl;
    cout << "原始 RGB 尺寸：" << original.width << " x " << original.height << endl;
    cout << "还原图像尺寸：" << recovered.width << " x " << recovered.height << endl;
    cout << "是否完全一致：" << (same_pixels ? "True" : "False") << endl;

    if (!same_pixels) {
        throw runtime_error("[Verify]RGB roundtrip 校验失败：像素不一致");
    }
}
