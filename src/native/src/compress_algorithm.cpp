#include "../include/compressor/compress_algorithm.h"

#include <algorithm>
#include <stdexcept>

namespace {

constexpr int kLmax = 256;
constexpr int kSegmentHeaderBits = 11;

int bitLength(int value) {
    if (value < 0 || value > 255) {
        throw std::runtime_error("符号值必须在 0~255 之间");
    }
    if (value == 0) {
        return 1;
    }

    int bits = 0;
    while (value > 0) {
        ++bits;
        value >>= 1;
    }
    return bits;
}

std::vector<Segment> tracebackSegments(const std::vector<int>& l, const std::vector<int>& b, int n) {
    std::vector<Segment> segments;
    int end = n;

    while (end > 0) {
        const int seg_len = l[end];
        const int seg_bit = b[end];
        if (seg_len <= 0 || seg_bit <= 0) {
            throw std::runtime_error("DP 回溯失败：分段长度或位宽非法");
        }

        const int start = end - seg_len + 1;
        segments.push_back(Segment{start, end, seg_len, seg_bit});
        end = start - 1;
    }

    std::reverse(segments.begin(), segments.end());
    return segments;
}

}  // namespace

DPResult dpCompress(const std::vector<int>& symbols) {
    const int n = static_cast<int>(symbols.size());
    std::vector<long long> cost(n + 1, 0);
    std::vector<int> seg_len(n + 1, 0);
    std::vector<int> seg_bit(n + 1, 0);

    for (int i = 1; i <= n; ++i) {
        int bmax = bitLength(symbols[i - 1]);
        cost[i] = cost[i - 1] + bmax + kSegmentHeaderBits;
        seg_len[i] = 1;
        seg_bit[i] = bmax;

        const int max_j = std::min(i, kLmax);
        for (int j = 2; j <= max_j; ++j) {
            const int current_bit = bitLength(symbols[i - j]);
            if (current_bit > bmax) {
                bmax = current_bit;
            }

            const long long candidate =
                cost[i - j] + 1LL * j * bmax + kSegmentHeaderBits;
            if (candidate < cost[i]) {
                cost[i] = candidate;
                seg_len[i] = j;
                seg_bit[i] = bmax;
            }
        }
    }

    DPResult result;
    result.min_bits = cost[n];
    result.l = std::move(seg_len);
    result.b = std::move(seg_bit);
    result.segments = tracebackSegments(result.l, result.b, n);
    return result;
}

void writeSegmentBitstream(BitWriter& writer, const std::vector<int>& symbols,
                           const std::vector<Segment>& segments) {
    for (const Segment& seg : segments) {
        if (seg.length < 1 || seg.length > 256 || seg.bit_width < 1 || seg.bit_width > 8) {
            throw std::runtime_error("分段长度或位宽非法，无法写入压缩数据");
        }

        writer.write(static_cast<uint32_t>(seg.length - 1), 8);
        writer.write(static_cast<uint32_t>(seg.bit_width - 1), 3);

        for (int index = seg.start - 1; index <= seg.end - 1; ++index) {
            writer.write(static_cast<uint32_t>(symbols[index]), seg.bit_width);
        }
    }
}

std::vector<int> readSegmentBitstream(BitReader& reader, uint64_t symbol_count,
                                      uint32_t segment_count) {
    std::vector<int> symbols;
    symbols.reserve(static_cast<size_t>(symbol_count));

    for (uint32_t seg_index = 0; seg_index < segment_count; ++seg_index) {
        const int seg_len = static_cast<int>(reader.read(8)) + 1;
        const int bit_width = static_cast<int>(reader.read(3)) + 1;
        if (bit_width < 1 || bit_width > 8) {
            throw std::runtime_error("压缩文件中的位宽非法");
        }

        for (int i = 0; i < seg_len; ++i) {
            if (symbols.size() >= symbol_count) {
                throw std::runtime_error("压缩文件中的符号数量超过文件头记录");
            }
            symbols.push_back(static_cast<int>(reader.read(bit_width)));
        }
    }

    if (symbols.size() != symbol_count) {
        throw std::runtime_error("解压后的符号数量不正确");
    }
    return symbols;
}
