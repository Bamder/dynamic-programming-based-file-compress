#ifndef COMPRESSOR_BIT_IO_H
#define COMPRESSOR_BIT_IO_H

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

class BitWriter {
public:
    void write(uint32_t value, int bits) {
        if (bits < 0 || bits > 32) {
            throw std::runtime_error("BitWriter 写入位数非法");
        }
        if (bits < 32 && value >= (1u << bits)) {
            throw std::runtime_error("BitWriter 写入值超出指定位宽");
        }

        for (int shift = bits - 1; shift >= 0; --shift) {
            const uint8_t bit = static_cast<uint8_t>((value >> shift) & 1u);
            current_byte_ = static_cast<uint8_t>((current_byte_ << 1) | bit);
            ++bit_count_;

            if (bit_count_ == 8) {
                data_.push_back(current_byte_);
                current_byte_ = 0;
                bit_count_ = 0;
            }
        }
    }

    std::vector<uint8_t> toBytes() {
        if (bit_count_ > 0) {
            current_byte_ = static_cast<uint8_t>(current_byte_ << (8 - bit_count_));
            data_.push_back(current_byte_);
            current_byte_ = 0;
            bit_count_ = 0;
        }
        return std::move(data_);
    }

private:
    std::vector<uint8_t> data_;
    uint8_t current_byte_ = 0;
    int bit_count_ = 0;
};

class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& data) : data_(data) {}

    uint32_t read(int bits) {
        if (bits < 0 || bits > 32) {
            throw std::runtime_error("BitReader 读取位数非法");
        }

        uint32_t value = 0;
        for (int i = 0; i < bits; ++i) {
            if (byte_index_ >= data_.size()) {
                throw std::runtime_error("压缩文件数据不完整");
            }

            const uint8_t current_byte = data_[byte_index_];
            const uint8_t bit = static_cast<uint8_t>((current_byte >> (7 - bit_index_)) & 1u);
            value = (value << 1) | bit;

            ++bit_index_;
            if (bit_index_ == 8) {
                bit_index_ = 0;
                ++byte_index_;
            }
        }
        return value;
    }

private:
    const std::vector<uint8_t>& data_;
    size_t byte_index_ = 0;
    int bit_index_ = 0;
};

#endif  // COMPRESSOR_BIT_IO_H
