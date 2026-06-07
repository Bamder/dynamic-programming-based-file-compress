#ifndef COMPRESSOR_COMPRESS_ALGORITHM_H
#define COMPRESSOR_COMPRESS_ALGORITHM_H

#include "bit_io.h"

#include <cstdint>
#include <vector>

struct Segment {
    int start = 0;     // 1-based inclusive
    int end = 0;       // 1-based inclusive
    int length = 0;
    int bit_width = 0;
};

struct DPResult {
    long long min_bits = 0;
    std::vector<Segment> segments;
    std::vector<int> l;
    std::vector<int> b;
};

// DP segment encoding for a sequence of symbols in [0, 255].
DPResult dpCompress(const std::vector<int>& symbols);

// Write/read the DP segment bitstream: per segment, 8-bit length header, 3-bit width
// header, then symbol_count symbols at the segment bit width.
void writeSegmentBitstream(BitWriter& writer, const std::vector<int>& symbols,
                           const std::vector<Segment>& segments);

std::vector<int> readSegmentBitstream(BitReader& reader, uint64_t symbol_count,
                                      uint32_t segment_count);

#endif  // COMPRESSOR_COMPRESS_ALGORITHM_H
