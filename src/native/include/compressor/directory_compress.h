#ifndef COMPRESSOR_DIRECTORY_COMPRESS_H
#define COMPRESSOR_DIRECTORY_COMPRESS_H

#include <filesystem>

void directoryCompress(const std::filesystem::path& dir_path,
                       const std::filesystem::path& output_path);

void directoryDecompress(const std::filesystem::path& compressed_file_path,
                         const std::filesystem::path& output_path);

#endif  // COMPRESSOR_DIRECTORY_COMPRESS_H
