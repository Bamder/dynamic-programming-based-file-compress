#include "../include/compressor/metrics_print.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

namespace fs = std::filesystem;

using std::cout;
using std::endl;
using std::map;
using std::string;

static void printDistributionMap(const string& title, const map<int, int>& dist) {
    cout << title << endl;
    for (const auto& [bit_width, count] : dist) {
        cout << bit_width << " bit：" << count << " 段" << endl;
    }
}

static void printFirstSegments(const std::vector<Segment>& segments, int limit = 10) {
    const int count = std::min<int>(limit, static_cast<int>(segments.size()));
    for (int i = 0; i < count; ++i) {
        const Segment& seg = segments[i];
        cout << "{start: " << seg.start
             << ", end: " << seg.end
             << ", length: " << seg.length
             << ", bit_width: " << seg.bit_width << "}" << endl;
    }
}

void printDirectoryCompressMetrics(const DirectoryCompressMetrics& metrics,
                                   const fs::path& input_dir,
                                   const fs::path& output_path) {
    cout << "========== 目录压缩完成 ==========" << endl;
    cout << "输入目录：" << input_dir.string() << endl;
    cout << "压缩文件：" << output_path.string() << endl;
    cout << "载荷字节数：" << metrics.payload_bytes << endl;
    cout << "目录树字节数：" << metrics.tree_blob_bytes << endl;
    cout << "DP 载荷字节数：" << metrics.dp_blob_bytes << endl;
    cout << "输出文件字节数：" << metrics.output_file_bytes << endl;
    cout << "DP 模型位数：" << metrics.dp_model_bits << " bit" << endl;
    cout << "分段数量：" << metrics.segment_count << endl;
    cout << std::fixed << std::setprecision(4);
    if (std::isnan(metrics.model_compression_ratio)) {
        cout << "DP 模型压缩率：N/A" << endl;
        cout << "实际文件压缩率：N/A" << endl;
    } else {
        cout << "DP 模型压缩率：" << metrics.model_compression_ratio << endl;
        cout << "实际文件压缩率：" << metrics.file_compression_ratio << endl;
    }
    cout << "DP 计算时间：" << metrics.dp_seconds << " 秒" << endl;
    cout << "总耗时：" << metrics.total_seconds << " 秒" << endl;
}

void printDirectoryDecompressMetrics(const DirectoryDecompressMetrics& metrics,
                                     const fs::path& input_path,
                                     const fs::path& output_dir) {
    cout << "========== 目录解压完成 ==========" << endl;
    cout << "压缩文件：" << input_path.string() << endl;
    cout << "输出目录：" << output_dir.string() << endl;
    cout << "目录树字节数：" << metrics.tree_blob_bytes << endl;
    cout << "载荷字节数：" << metrics.payload_bytes << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "总耗时：" << metrics.total_seconds << " 秒" << endl;
}

void printGrayAnalyzeMetrics(const GrayAnalyzeMetrics& metrics, const string& input_path) {
    (void)input_path;
    cout << "========== DP 灰度图像压缩结果 ==========" << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "像素数量：" << metrics.pixel_count << endl;
    cout << "原始存储空间：" << metrics.original_bits << " bit" << endl;
    cout << "压缩后存储空间：" << metrics.dp_model_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "压缩率：" << metrics.compression_ratio << endl;
    cout << std::setprecision(2);
    cout << "节省比例：" << metrics.saved_ratio * 100.0 << "%" << endl;
    cout << std::setprecision(4);
    cout << "执行时间：" << metrics.dp_seconds << " 秒" << endl;
    cout << "最优分段数量：" << metrics.segment_count << endl;
    cout << endl;
    printDistributionMap("各位宽分段数量统计：", metrics.bit_width_distribution);
    cout << endl;
    cout << "前 10 个分段结果：" << endl;
    printFirstSegments(metrics.segments);
}

void printRGBAnalyzeMetrics(const RGBAnalyzeMetrics& metrics, const string& input_path) {
    (void)input_path;
    cout << "========== RGB 彩色图像 DP 压缩结果 ==========" << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "原始 RGB 存储空间：" << metrics.rgb_original_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "R 通道压缩后：" << metrics.r.dp_model_bits << " bit，压缩率：" << metrics.r.compression_ratio << endl;
    cout << "G 通道压缩后：" << metrics.g.dp_model_bits << " bit，压缩率：" << metrics.g.compression_ratio << endl;
    cout << "B 通道压缩后：" << metrics.b.dp_model_bits << " bit，压缩率：" << metrics.b.compression_ratio << endl;
    cout << "RGB 总压缩后：" << metrics.total_dp_model_bits << " bit" << endl;
    cout << "RGB 总压缩率：" << metrics.total_compression_ratio << endl;
    cout << std::setprecision(2);
    cout << "节省比例：" << metrics.saved_ratio * 100.0 << "%" << endl;
    cout << std::setprecision(4);
    cout << "执行时间：" << metrics.dp_seconds << " 秒" << endl;
    cout << "R 通道分段数量：" << metrics.r.segment_count << endl;
    cout << "G 通道分段数量：" << metrics.g.segment_count << endl;
    cout << "B 通道分段数量：" << metrics.b.segment_count << endl;
    cout << endl;
    printDistributionMap("R 通道位宽分布统计：", metrics.r.bit_width_distribution);
    cout << endl;
    printDistributionMap("G 通道位宽分布统计：", metrics.g.bit_width_distribution);
    cout << endl;
    printDistributionMap("B 通道位宽分布统计：", metrics.b.bit_width_distribution);
}

void printGrayCompressMetrics(const GrayCompressMetrics& metrics,
                              const string& input_path,
                              const string& output_path) {
    cout << "========== 灰度压缩文件生成完成 ==========" << endl;
    cout << "输入图片：" << input_path << endl;
    cout << "压缩文件：" << output_path << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "原始像素数据：" << metrics.original_bits << " bit" << endl;
    cout << "DP 模型压缩后：" << metrics.dp_model_bits << " bit" << endl;
    cout << "实际压缩文件大小：" << metrics.actual_file_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "DP 模型压缩率：" << metrics.model_compression_ratio << endl;
    cout << "实际文件压缩率：" << metrics.file_compression_ratio << endl;
    cout << "分段数量：" << metrics.segment_count << endl;
    cout << "DP 计算时间：" << metrics.dp_seconds << " 秒" << endl;
}

void printGrayDecompressMetrics(const GrayDecompressMetrics& metrics,
                                const string& input_path,
                                const string& output_path) {
    cout << "========== 灰度解压完成 ==========" << endl;
    cout << "压缩文件：" << input_path << endl;
    cout << "还原图片：" << output_path << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "像素数量：" << metrics.pixel_count << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "执行时间：" << metrics.total_seconds << " 秒" << endl;
}

void printRGBCompressMetrics(const RGBCompressMetrics& metrics,
                             const string& input_path,
                             const string& output_path) {
    cout << "========== RGB 压缩文件生成完成 ==========" << endl;
    cout << "输入图片：" << input_path << endl;
    cout << "压缩文件：" << output_path << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "原始 RGB 像素数据：" << metrics.original_bits << " bit" << endl;
    cout << "DP 模型压缩后：" << metrics.dp_model_bits << " bit" << endl;
    cout << "实际压缩文件大小：" << metrics.actual_file_bits << " bit" << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "DP 模型压缩率：" << metrics.model_compression_ratio << endl;
    cout << "实际文件压缩率：" << metrics.file_compression_ratio << endl;
    cout << "R 通道分段数量：" << metrics.r.segment_count << endl;
    cout << "G 通道分段数量：" << metrics.g.segment_count << endl;
    cout << "B 通道分段数量：" << metrics.b.segment_count << endl;
    cout << "DP 计算时间：" << metrics.dp_seconds << " 秒" << endl;
}

void printRGBDecompressMetrics(const RGBDecompressMetrics& metrics,
                               const string& input_path,
                               const string& output_path) {
    cout << "========== RGB 解压完成 ==========" << endl;
    cout << "压缩文件：" << input_path << endl;
    cout << "还原图片：" << output_path << endl;
    cout << "图像尺寸：" << metrics.width << " x " << metrics.height << endl;
    cout << "像素数量：" << metrics.pixel_count << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "执行时间：" << metrics.total_seconds << " 秒" << endl;
}
