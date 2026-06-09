#ifndef COMPRESSOR_IMAGE_COMPRESS_H
#define COMPRESSOR_IMAGE_COMPRESS_H

#include "compress_algorithm.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct RGBChannelMetrics {
    long long dp_model_bits = 0;
    double compression_ratio = 0.0;
    uint32_t segment_count = 0;
    std::map<int, int> bit_width_distribution;
    std::vector<Segment> segments;
};

struct GrayAnalyzeMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t pixel_count = 0;
    long long original_bits = 0;
    long long dp_model_bits = 0;
    double compression_ratio = 0.0;
    double saved_ratio = 0.0;
    double dp_seconds = 0.0;
    uint32_t segment_count = 0;
    std::map<int, int> bit_width_distribution;
    std::vector<Segment> segments;
};

struct GrayCompressMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t pixel_count = 0;
    long long original_bits = 0;
    long long dp_model_bits = 0;
    long long actual_file_bits = 0;
    uint64_t output_file_bytes = 0;
    double model_compression_ratio = 0.0;
    double file_compression_ratio = 0.0;
    uint32_t segment_count = 0;
    double dp_seconds = 0.0;
    double total_seconds = 0.0;
};

struct GrayDecompressMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t pixel_count = 0;
    double total_seconds = 0.0;
};

struct RGBAnalyzeMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    long long rgb_original_bits = 0;
    long long total_dp_model_bits = 0;
    double total_compression_ratio = 0.0;
    double saved_ratio = 0.0;
    double dp_seconds = 0.0;
    RGBChannelMetrics r;
    RGBChannelMetrics g;
    RGBChannelMetrics b;
};

struct RGBCompressMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t pixel_count = 0;
    long long original_bits = 0;
    long long dp_model_bits = 0;
    long long actual_file_bits = 0;
    uint64_t output_file_bytes = 0;
    double model_compression_ratio = 0.0;
    double file_compression_ratio = 0.0;
    double dp_seconds = 0.0;
    double total_seconds = 0.0;
    RGBChannelMetrics r;
    RGBChannelMetrics g;
    RGBChannelMetrics b;
};

struct RGBDecompressMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t pixel_count = 0;
    double total_seconds = 0.0;
};

GrayAnalyzeMetrics analyzeGrayImage(const std::string& input_path);
RGBAnalyzeMetrics analyzeRGBImage(const std::string& input_path);

GrayCompressMetrics compressGrayToFile(const std::string& input_path, const std::string& output_path);
GrayDecompressMetrics decompressGrayToImage(const std::string& input_path, const std::string& output_path);

RGBCompressMetrics compressRGBToFile(const std::string& input_path, const std::string& output_path);
RGBDecompressMetrics decompressRGBToImage(const std::string& input_path, const std::string& output_path);

void verifyGrayImage(const std::string& original_path, const std::string& recovered_path);
void verifyRGBImage(const std::string& original_path, const std::string& recovered_path);

#endif  // COMPRESSOR_IMAGE_COMPRESS_H
