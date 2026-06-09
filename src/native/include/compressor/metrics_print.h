#ifndef COMPRESSOR_METRICS_PRINT_H
#define COMPRESSOR_METRICS_PRINT_H

#include "directory_compress.h"
#include "image_compress.h"

#include <filesystem>
#include <string>

void printDirectoryCompressMetrics(const DirectoryCompressMetrics& metrics,
                                   const std::filesystem::path& input_dir,
                                   const std::filesystem::path& output_path);

void printDirectoryDecompressMetrics(const DirectoryDecompressMetrics& metrics,
                                     const std::filesystem::path& input_path,
                                     const std::filesystem::path& output_dir);

void printGrayAnalyzeMetrics(const GrayAnalyzeMetrics& metrics, const std::string& input_path);
void printRGBAnalyzeMetrics(const RGBAnalyzeMetrics& metrics, const std::string& input_path);

void printGrayCompressMetrics(const GrayCompressMetrics& metrics,
                              const std::string& input_path,
                              const std::string& output_path);
void printGrayDecompressMetrics(const GrayDecompressMetrics& metrics,
                                const std::string& input_path,
                                const std::string& output_path);

void printRGBCompressMetrics(const RGBCompressMetrics& metrics,
                             const std::string& input_path,
                             const std::string& output_path);
void printRGBDecompressMetrics(const RGBDecompressMetrics& metrics,
                               const std::string& input_path,
                               const std::string& output_path);

#endif  // COMPRESSOR_METRICS_PRINT_H
