#include <filesystem>
#include <iostream>
#include <string>
#include<windows.h>
#include "../include/compressor/directory_compress.h"
#include "../include/compressor/image_compress.h"

namespace fs = std::filesystem;

using std::cerr;
using std::cout;
using std::endl;
using std::string;

void printGrayStats(const string& input_path);
void printRGBStats(const string& input_path);
void verifyGrayImage(const string& original_path, const string& recovered_path);
void verifyRGBImage(const string& original_path, const string& recovered_path);
void verifyDirectory(const fs::path& original_dir, const fs::path& restored_dir);

static void printUsage(const string& program) {
    cout << "用法：" << endl;
    cout << "  目录：" << endl;
    cout << "    " << program << " dir-compress <input_dir> <output.dpdc>" << endl;
    cout << "    " << program << " dir-decompress <input.dpdc> <output_dir>" << endl;
    cout << "    " << program << " dir-verify <original_dir> <restored_dir>" << endl;
    cout << "  灰度图像：" << endl;
    cout << "    " << program << " gray <input.png>" << endl;
    cout << "    " << program << " gray-compress <input.png> <output.dpgc>" << endl;
    cout << "    " << program << " gray-decompress <input.dpgc> <output.png>" << endl;
    cout << "    " << program << " gray-verify <original.png> <recovered.png>" << endl;
    cout << "  RGB 图像：" << endl;
    cout << "    " << program << " rgb <input.png>" << endl;
    cout << "    " << program << " rgb-compress <input.png> <output.dprc>" << endl;
    cout << "    " << program << " rgb-decompress <input.dprc> <output.png>" << endl;
    cout << "    " << program << " rgb-verify <original.png> <recovered.png>" << endl;
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

        if (command == "dir-compress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            directoryCompress(fs::path(argv[2]), fs::path(argv[3]));
        } else if (command == "dir-decompress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            directoryDecompress(fs::path(argv[2]), fs::path(argv[3]));
        } else if (command == "dir-verify") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            verifyDirectory(fs::path(argv[2]), fs::path(argv[3]));
        } else if (command == "gray") {
            if (argc != 3) {
                printUsage(argv[0]);
                return 1;
            }
            printGrayStats(argv[2]);
        } else if (command == "gray-compress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            compressGrayToFile(argv[2], argv[3]);
        } else if (command == "gray-decompress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            decompressGrayToImage(argv[2], argv[3]);
        } else if (command == "gray-verify") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            verifyGrayImage(argv[2], argv[3]);
        } else if (command == "rgb") {
            if (argc != 3) {
                printUsage(argv[0]);
                return 1;
            }
            printRGBStats(argv[2]);
        } else if (command == "rgb-compress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            compressRGBToFile(argv[2], argv[3]);
        } else if (command == "rgb-decompress") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            decompressRGBToImage(argv[2], argv[3]);
        } else if (command == "rgb-verify") {
            if (argc != 4) {
                printUsage(argv[0]);
                return 1;
            }
            verifyRGBImage(argv[2], argv[3]);
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
