#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "compressor/image_compress.h"

extern "C" int stbi_write_png(char const* filename, int w, int h, int comp, const void* data,
                              int stride_in_bytes);

namespace fs = std::filesystem;

using std::runtime_error;
using std::string;
using std::vector;

void verifyGrayImage(const string& original_path, const string& recovered_path);
void verifyRGBImage(const string& original_path, const string& recovered_path);

static void writeFixtureGrayPng(const fs::path& path) {
    constexpr int width = 32;
    constexpr int height = 32;
    vector<uint8_t> pixels(static_cast<size_t>(width * height));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            pixels[static_cast<size_t>(y * width + x)] =
                static_cast<uint8_t>((x * 7 + y * 11) & 0xff);
        }
    }

    if (!stbi_write_png(path.string().c_str(), width, height, 1, pixels.data(), width)) {
        throw runtime_error("[Test]无法写出灰度 fixture：" + path.string());
    }
}

static void writeFixtureRgbPng(const fs::path& path) {
    constexpr int width = 16;
    constexpr int height = 16;
    vector<uint8_t> pixels(static_cast<size_t>(width * height * 3));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t index = static_cast<size_t>((y * width + x) * 3);
            pixels[index + 0] = static_cast<uint8_t>((x * 13) & 0xff);
            pixels[index + 1] = static_cast<uint8_t>((y * 17) & 0xff);
            pixels[index + 2] = static_cast<uint8_t>(((x + y) * 5) & 0xff);
        }
    }

    if (!stbi_write_png(path.string().c_str(), width, height, 3, pixels.data(), width * 3)) {
        throw runtime_error("[Test]无法写出 RGB fixture：" + path.string());
    }
}

static void runGrayRoundtrip(const fs::path& fixture_root) {
    const fs::path input_png = fixture_root / "sample_gray.png";
    const fs::path archive = fixture_root / "sample.dpgc";
    const fs::path restored_png = fixture_root / "restored_gray.png";

    fs::create_directories(fixture_root);
    writeFixtureGrayPng(input_png);

    if (fs::exists(archive)) {
        fs::remove(archive);
    }
    if (fs::exists(restored_png)) {
        fs::remove(restored_png);
    }

    std::cout << "Gray compress: " << input_png << " -> " << archive << std::endl;
    compressGrayToFile(input_png.string(), archive.string());

    std::cout << "Gray decompress: " << archive << " -> " << restored_png << std::endl;
    decompressGrayToImage(archive.string(), restored_png.string());

    verifyGrayImage(input_png.string(), restored_png.string());
}

static void runRgbRoundtrip(const fs::path& fixture_root) {
    const fs::path input_png = fixture_root / "sample_rgb.png";
    const fs::path archive = fixture_root / "sample.dprc";
    const fs::path restored_png = fixture_root / "restored_rgb.png";

    fs::create_directories(fixture_root);
    writeFixtureRgbPng(input_png);

    if (fs::exists(archive)) {
        fs::remove(archive);
    }
    if (fs::exists(restored_png)) {
        fs::remove(restored_png);
    }

    std::cout << "RGB compress: " << input_png << " -> " << archive << std::endl;
    compressRGBToFile(input_png.string(), archive.string());

    std::cout << "RGB decompress: " << archive << " -> " << restored_png << std::endl;
    decompressRGBToImage(archive.string(), restored_png.string());

    verifyRGBImage(input_png.string(), restored_png.string());
}

int main() {
    try {
        const fs::path fixture_root = fs::path("tests") / "fixtures" / "image_roundtrip";

        runGrayRoundtrip(fixture_root);
        std::cout << "PASS: gray roundtrip" << std::endl;

        runRgbRoundtrip(fixture_root);
        std::cout << "PASS: rgb roundtrip" << std::endl;

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << std::endl;
        return 1;
    }
}
