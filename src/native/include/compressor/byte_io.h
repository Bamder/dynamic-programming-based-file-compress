#ifndef COMPRESSOR_BYTE_IO_H
#define COMPRESSOR_BYTE_IO_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class ByteWriter {
public:
    void appendU8(uint8_t value) {
        data_.push_back(value);
    }

    void appendU16LE(uint16_t value) {
        data_.push_back(static_cast<uint8_t>(value & 0xff));
        data_.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    }

    void appendU32LE(uint32_t value) {
        data_.push_back(static_cast<uint8_t>(value & 0xff));
        data_.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
        data_.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
        data_.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
    }

    void appendU64LE(uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8) {
            data_.push_back(static_cast<uint8_t>((value >> shift) & 0xff));
        }
    }

    void appendString(const std::string& value) {
        if (value.size() > 0xffff) {
            throw std::runtime_error("字符串过长，无法用 u16 表示长度");
        }
        appendU16LE(static_cast<uint16_t>(value.size()));
        for (unsigned char ch : value) {
            data_.push_back(ch);
        }
    }

    std::vector<uint8_t> take() {
        return std::move(data_);
    }

private:
    std::vector<uint8_t> data_;
};

class ByteReader {
public:
    explicit ByteReader(const std::vector<uint8_t>& data) : data_(data) {}

    uint8_t readU8() {
        return readRaw(1)[0];
    }

    uint16_t readU16LE() {
        const std::vector<uint8_t> raw = readRaw(2);
        return static_cast<uint16_t>(raw[0] | (static_cast<uint16_t>(raw[1]) << 8));
    }

    uint32_t readU32LE() {
        const std::vector<uint8_t> raw = readRaw(4);
        return static_cast<uint32_t>(raw[0] | (static_cast<uint32_t>(raw[1]) << 8) |
                                       (static_cast<uint32_t>(raw[2]) << 16) |
                                       (static_cast<uint32_t>(raw[3]) << 24));
    }

    uint64_t readU64LE() {
        uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 8) {
            value |= static_cast<uint64_t>(readU8()) << shift;
        }
        return value;
    }

    std::string readString() {
        const uint16_t length = readU16LE();
        const std::vector<uint8_t> raw = readRaw(length);
        return std::string(raw.begin(), raw.end());
    }

private:
    std::vector<uint8_t> readRaw(size_t count) {
        if (offset_ + count > data_.size()) {
            throw std::runtime_error("二进制数据不完整");
        }
        std::vector<uint8_t> raw(data_.begin() + static_cast<std::ptrdiff_t>(offset_),
                                 data_.begin() + static_cast<std::ptrdiff_t>(offset_ + count));
        offset_ += count;
        return raw;
    }

    const std::vector<uint8_t>& data_;
    size_t offset_ = 0;
};

#endif  // COMPRESSOR_BYTE_IO_H
