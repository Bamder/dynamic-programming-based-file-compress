#ifndef COMPRESSOR_STREAM_IO_H
#define COMPRESSOR_STREAM_IO_H

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

inline void writeU16LE(std::ofstream& out, uint16_t value) {
    const char bytes[2] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8) & 0xffu),
    };
    out.write(bytes, 2);
}

inline void writeU32LE(std::ofstream& out, uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8) & 0xffu),
        static_cast<char>((value >> 16) & 0xffu),
        static_cast<char>((value >> 24) & 0xffu),
    };
    out.write(bytes, 4);
}

inline void writeU64LE(std::ofstream& out, uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        const char byte = static_cast<char>((value >> shift) & 0xffu);
        out.write(&byte, 1);
    }
}

inline uint16_t readU16LE(std::ifstream& in) {
    unsigned char bytes[2] = {0, 0};
    in.read(reinterpret_cast<char*>(bytes), 2);
    if (!in) {
        throw std::runtime_error("读取 uint16 失败");
    }
    return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

inline uint32_t readU32LE(std::ifstream& in) {
    unsigned char bytes[4] = {0, 0, 0, 0};
    in.read(reinterpret_cast<char*>(bytes), 4);
    if (!in) {
        throw std::runtime_error("读取 uint32 失败");
    }
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

inline uint64_t readU64LE(std::ifstream& in) {
    uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        unsigned char byte = 0;
        in.read(reinterpret_cast<char*>(&byte), 1);
        if (!in) {
            throw std::runtime_error("读取 uint64 失败");
        }
        value |= static_cast<uint64_t>(byte) << shift;
    }
    return value;
}

inline std::vector<uint8_t> readBytes(std::ifstream& in, size_t count) {
    std::vector<uint8_t> data(count);
    if (count == 0) {
        return data;
    }
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(count));
    if (!in) {
        throw std::runtime_error("读取二进制数据不完整");
    }
    return data;
}

inline std::vector<uint8_t> readRemainingBytes(std::ifstream& in) {
    std::vector<uint8_t> data;
    char ch = 0;
    while (in.get(ch)) {
        data.push_back(static_cast<uint8_t>(ch));
    }
    return data;
}

#endif  // COMPRESSOR_STREAM_IO_H
