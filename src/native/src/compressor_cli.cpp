#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include "../include/compressor/directory_compress.h"
#include "../include/compressor/image_compress.h"
#include "../include/compressor/metrics_print.h"

namespace fs = std::filesystem;

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

static void printUsage(const string& program);
struct CommandArgs {
    bool show_metrics = false;
    vector<string> positional;
};

static CommandArgs parseTrailingArgs(int argc, char* argv[], int start_index) {
    CommandArgs parsed;
    for (int i = start_index; i < argc; ++i) {
        const string arg = argv[i];
        if (arg == "--metrics") {
            parsed.show_metrics = true;
        } else {
            parsed.positional.push_back(arg);
        }
    }
    return parsed;
}

static bool expectPositionalCount(const CommandArgs& args, size_t expected, const string& program) {
    if (args.positional.size() != expected) {
        printUsage(program);
        return false;
    }
    return true;
}

static void printUsage(const string& program) {
    cout << "用法：" << endl;
    cout << "  目录：" << endl;
    cout << "    " << program << " dir-compress [--metrics] <input_dir> <output.dpdc>" << endl;
    cout << "    " << program << " dir-decompress [--metrics] <input.dpdc> <output_dir>" << endl;
    cout << "    " << program << " dir-verify <original_dir> <restored_dir>" << endl;
    cout << "  灰度图像：" << endl;
    cout << "    " << program << " gray <input.png>" << endl;
    cout << "    " << program << " gray-compress [--metrics] <input.png> <output.dpgc>" << endl;
    cout << "    " << program << " gray-decompress [--metrics] <input.dpgc> <output.png>" << endl;
    cout << "    " << program << " gray-verify <original.png> <recovered.png>" << endl;
    cout << "  RGB 图像：" << endl;
    cout << "    " << program << " rgb <input.png>" << endl;
    cout << "    " << program << " rgb-compress [--metrics] <input.png> <output.dprc>" << endl;
    cout << "    " << program << " rgb-decompress [--metrics] <input.dprc> <output.png>" << endl;
    cout << "    " << program << " rgb-verify <original.png> <recovered.png>" << endl;
    cout << endl;
    cout << "  --metrics  在压缩或解压完成后输出统计信息（gray/rgb 统计命令始终输出）" << endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    try {
        if (argc < 3) {
            printUsage(argc > 0 ? argv[0] : "./compressor");
            return 1;
        }

        const string command = argv[1];
        const CommandArgs args = parseTrailingArgs(argc, argv, 2);

        if (command == "dir-compress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const DirectoryCompressMetrics metrics =
                directoryCompress(fs::path(args.positional[0]), fs::path(args.positional[1]));
            if (args.show_metrics) {
                printDirectoryCompressMetrics(metrics, fs::path(args.positional[0]),
                                              fs::path(args.positional[1]));
            }
        } else if (command == "dir-decompress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const DirectoryDecompressMetrics metrics = directoryDecompress(fs::path(args.positional[0]),
                                                                           fs::path(args.positional[1]));
            if (args.show_metrics) {
                printDirectoryDecompressMetrics(metrics, fs::path(args.positional[0]),
                                                fs::path(args.positional[1]));
            }
        } else if (command == "dir-verify") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            verifyDirectory(fs::path(args.positional[0]), fs::path(args.positional[1]));
        } else if (command == "gray") {
            if (!expectPositionalCount(args, 1, argv[0])) {
                return 1;
            }
            const GrayAnalyzeMetrics metrics = analyzeGrayImage(args.positional[0]);
            printGrayAnalyzeMetrics(metrics, args.positional[0]);
        } else if (command == "gray-compress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const GrayCompressMetrics metrics =
                compressGrayToFile(args.positional[0], args.positional[1]);
            if (args.show_metrics) {
                printGrayCompressMetrics(metrics, args.positional[0], args.positional[1]);
            }
        } else if (command == "gray-decompress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const GrayDecompressMetrics metrics =
                decompressGrayToImage(args.positional[0], args.positional[1]);
            if (args.show_metrics) {
                printGrayDecompressMetrics(metrics, args.positional[0], args.positional[1]);
            }
        } else if (command == "gray-verify") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            verifyGrayImage(args.positional[0], args.positional[1]);
        } else if (command == "rgb") {
            if (!expectPositionalCount(args, 1, argv[0])) {
                return 1;
            }
            const RGBAnalyzeMetrics metrics = analyzeRGBImage(args.positional[0]);
            printRGBAnalyzeMetrics(metrics, args.positional[0]);
        } else if (command == "rgb-compress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const RGBCompressMetrics metrics =
                compressRGBToFile(args.positional[0], args.positional[1]);
            if (args.show_metrics) {
                printRGBCompressMetrics(metrics, args.positional[0], args.positional[1]);
            }
        } else if (command == "rgb-decompress") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            const RGBDecompressMetrics metrics =
                decompressRGBToImage(args.positional[0], args.positional[1]);
            if (args.show_metrics) {
                printRGBDecompressMetrics(metrics, args.positional[0], args.positional[1]);
            }
        } else if (command == "rgb-verify") {
            if (!expectPositionalCount(args, 2, argv[0])) {
                return 1;
            }
            verifyRGBImage(args.positional[0], args.positional[1]);
        } else {
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& ex) {
        cerr << "错误：" << ex.what() << endl;
        return 1;
    }

    return 0;
}
