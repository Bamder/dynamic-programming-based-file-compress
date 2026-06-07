#ifndef COMPRESSOR_IMAGE_COMPRESS_H
#define COMPRESSOR_IMAGE_COMPRESS_H

#include <string>

void compressGrayToFile(const std::string& input_path, const std::string& output_path);
void decompressGrayToImage(const std::string& input_path, const std::string& output_path);

void compressRGBToFile(const std::string& input_path, const std::string& output_path);
void decompressRGBToImage(const std::string& input_path, const std::string& output_path);

#endif  // COMPRESSOR_IMAGE_COMPRESS_H
