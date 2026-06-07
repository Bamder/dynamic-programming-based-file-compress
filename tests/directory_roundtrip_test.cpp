#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "compressor/directory_compress.h"

namespace fs = std::filesystem;

void verifyDirectory(const fs::path& original_dir, const fs::path& restored_dir);

int main(int argc, char* argv[]) {
    using std::cerr;
    using std::cout;
    using std::endl;

    try {
        const fs::path fixture_root = fs::path("tests") / "fixtures" / "dpdc_roundtrip";
        const fs::path input_dir = fixture_root / "original";
        const fs::path archive_path = fixture_root / "sample.dpdc";
        const fs::path restored_dir = fixture_root / "restored";

        if (argc >= 4) {
            directoryCompress(fs::path(argv[1]), fs::path(argv[2]));
            directoryDecompress(fs::path(argv[2]), fs::path(argv[3]));
            verifyDirectory(fs::path(argv[1]), fs::path(argv[3]));
            cout << "PASS: roundtrip OK" << endl;
            return 0;
        }

        fs::create_directories(input_dir / "sub");
        {
            std::ofstream(input_dir / "readme.txt") << "hello dpdc";
            std::ofstream(input_dir / "sub" / "data.bin", std::ios::binary)
                << char(0x00) << char(0x01) << char(0x02) << char(0xff);
            std::ofstream(input_dir / "empty.txt");
        }

        if (fs::exists(restored_dir)) {
            fs::remove_all(restored_dir);
        }
        if (fs::exists(archive_path)) {
            fs::remove(archive_path);
        }

        cout << "Compress: " << input_dir << " -> " << archive_path << endl;
        directoryCompress(input_dir, archive_path);

        cout << "Decompress: " << archive_path << " -> " << restored_dir << endl;
        directoryDecompress(archive_path, restored_dir);

        verifyDirectory(input_dir, restored_dir);

        cout << "PASS: DPDC roundtrip" << endl;
        return 0;
    } catch (const std::exception& ex) {
        cerr << "FAIL: " << ex.what() << endl;
        return 1;
    }
}
