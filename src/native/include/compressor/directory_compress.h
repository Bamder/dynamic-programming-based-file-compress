#ifndef COMPRESSOR_DIRECTORY_COMPRESS_H
#define COMPRESSOR_DIRECTORY_COMPRESS_H

#include <cstdint>
#include <filesystem>

struct DirectoryCompressMetrics {
    uint64_t payload_bytes = 0;
    uint32_t tree_blob_bytes = 0;
    uint64_t dp_blob_bytes = 0;
    uint64_t output_file_bytes = 0;
    long long dp_model_bits = 0;
    uint32_t segment_count = 0;
    // Denominator: payload_bytes * 8. NaN when payload_bytes == 0.
    double model_compression_ratio = 0.0;
    double file_compression_ratio = 0.0;
    double dp_seconds = 0.0;
    double total_seconds = 0.0;
};

struct DirectoryDecompressMetrics {
    uint64_t payload_bytes = 0;
    uint32_t tree_blob_bytes = 0;
    double total_seconds = 0.0;
};

DirectoryCompressMetrics directoryCompress(const std::filesystem::path& dir_path,
                                           const std::filesystem::path& output_path);

DirectoryDecompressMetrics directoryDecompress(const std::filesystem::path& compressed_file_path,
                                                 const std::filesystem::path& output_path);

#endif  // COMPRESSOR_DIRECTORY_COMPRESS_H
