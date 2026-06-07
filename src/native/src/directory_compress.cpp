#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "../include/compressor/byte_io.h"
#include "../include/compressor/compress_algorithm.h"
#include "../include/compressor/directory_compress.h"
#include "../include/compressor/stream_io.h"

namespace fs = std::filesystem;

using std::runtime_error;
using std::string;
using std::vector;

constexpr uint8_t kDirectoryTreeFormatVersion = 1;
constexpr uint32_t kMaxDirectoryChildCount = 0xffffffffu;

struct DirNode {
    string name;
    bool is_directory = false;
    uint64_t file_size = 0;   // valid when is_directory == false
    vector<DirNode> children; // valid when is_directory == true
};

// -----------------------------------------------------------------------------
// Tree of directory
// Filesystem scan, in-memory DirNode tree, and binary tree-blob encode/decode.
// -----------------------------------------------------------------------------

// list all the contents of the target directory without entering the sub-directories.
static vector<fs::directory_entry> listSortedEntries(const fs::path& dir) {
    vector<fs::directory_entry> entries;
    for (const fs::directory_entry& entry : fs::directory_iterator(
             dir, fs::directory_options::skip_permission_denied)) {
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a,
                                                 const fs::directory_entry& b) {
        return a.path().filename().string() < b.path().filename().string();
    });
    return entries;
}

// build the DirNode subtree of directory recursively
DirNode scanDirectoryTree(const fs::path& path) {
    if (!fs::exists(path)) {
        throw runtime_error("[Directory Tree]路径不存在：" + path.string());
    }

    DirNode node;
    node.name = path.filename().string();
    if (node.name.empty()) {
        node.name = path.string();
    }

    if (fs::is_symlink(path)) {
        throw runtime_error("[Directory Tree]暂不支持符号链接：" + path.string());
    }

    // build the structure of current directrory and enter sub-directories recursively
    if (fs::is_directory(path)) {
        node.is_directory = true;
        for (const fs::directory_entry& entry : listSortedEntries(path)) {
            node.children.push_back(scanDirectoryTree(entry.path()));
        }
        return node;
    }

    // current node is not a directory
    if (!fs::is_regular_file(path)) {
        throw runtime_error("[Directory Tree]暂不支持非常规文件：" + path.string());
    }

    node.is_directory = false;
    node.file_size = fs::file_size(path);
    return node;
}

// turn directory tree into a formatted string for debugging
string formatDirectoryTree(const DirNode& node, int indent = 0) {
    const string pad(static_cast<size_t>(indent) * 2, ' ');
    string out = pad + node.name;

    if (node.is_directory) {
        out += "/\n";
        for (const DirNode& child : node.children) {
            out += formatDirectoryTree(child, indent + 1);
        }
        return out;
    }

    out += " (" + std::to_string(node.file_size) + " bytes)\n";
    return out;
}

// entrance of tree-building
DirNode buildDirectoryTree(const fs::path& root_path) {
    return scanDirectoryTree(root_path);
}

// recursively encode a DirNode subtree into ByteWriter as per the binary protocol.
static void writeNode(ByteWriter& writer, const DirNode& node) {
    writer.appendU8(node.is_directory ? 1 : 0);
    writer.appendString(node.name);

    if (node.is_directory) {
        if (node.children.size() > kMaxDirectoryChildCount) {
            throw runtime_error("[Directory Tree]目录子项超出限制，无法序列化目录树：" + node.name);
        }
        writer.appendU32LE(static_cast<uint32_t>(node.children.size()));
        for (const DirNode& child : node.children) {
            writeNode(writer, child);
        }
        return;
    }

    writer.appendU64LE(node.file_size);
}

// recursively decode a DirNode subtree from ByteReader as per the binary protocol.
static DirNode readNode(ByteReader& reader) {
    const uint8_t is_directory = reader.readU8();
    if (is_directory != 0 && is_directory != 1) {
        throw runtime_error("[Directory Tree]目录树节点类型非法");
    }

    DirNode node;
    node.name = reader.readString();
    node.is_directory = is_directory == 1;

    if (node.is_directory) {
        const uint32_t child_count = reader.readU32LE();
        node.children.reserve(child_count);
        for (uint32_t i = 0; i < child_count; ++i) {
            node.children.push_back(readNode(reader));
        }
        return node;
    }

    node.file_size = reader.readU64LE();
    return node;
}

/*
 * Tree binary layout (little-endian):
 *   [version: u8]
 *   node:
 *     [is_directory: u8]
 *     [name_len: u16][name bytes]
 *     if directory: [child_count: u32][child...]
 *     else:         [file_size: u64]
 */
vector<uint8_t> serializeDirectoryTree(const DirNode& root) {
    ByteWriter writer;
    writer.appendU8(kDirectoryTreeFormatVersion);
    writeNode(writer, root);
    return writer.take();
}

DirNode deserializeDirectoryTree(const vector<uint8_t>& data) {
    ByteReader reader(data);
    const uint8_t version = reader.readU8();
    if (version != kDirectoryTreeFormatVersion) {
        throw runtime_error("不支持的目录树格式版本");
    }
    return readNode(reader);
}

// -----------------------------------------------------------------------------
// Payload compression
// concatenating payload bytes, turning bytes to DP symbols and invoking DP.
// -----------------------------------------------------------------------------

// read a regular file and append its raw bytes to payload (binary, no text encoding).
static void appendFileToPayload(
    const fs::path& file_path, 
    uint64_t expected_size,
    vector<uint8_t>& payload
    ) {
    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        throw runtime_error("[Payload]无法打开文件：" + file_path.string());
    }

    // measure the actual size of the file and check it with expected size
    input.seekg(0, std::ios::end);
    const std::streamsize actual_size = input.tellg(); // file end position
    if (actual_size < 0 || static_cast<uint64_t>(actual_size) != expected_size) {
        throw runtime_error("[Payload]文件大小与目录树不一致：" + file_path.string());
    }

    // empty file
    if (expected_size == 0) {
        return;
    }

    // extend the container of payload
    const size_t offset = payload.size();
    payload.resize(offset + static_cast<size_t>(expected_size));

    // read file blob into payload in memory (start at base address + offset)
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(payload.data() + offset),
            static_cast<std::streamsize>(expected_size));
    
    if (!input) {
        throw runtime_error("[Payload]读取文件失败：" + file_path.string());
    }
}

void flattenDirectory(
    const DirNode &tree_root, 
    const fs::path &root_path,
    vector<uint8_t> &payload
    ) {
    if (!fs::exists(root_path)) {
        throw runtime_error("[Payload]路径不存在：" + root_path.string());
    }
        
    for (const DirNode& node : tree_root.children) {
        fs::path file_path = root_path / node.name;

        if(node.is_directory) {
            flattenDirectory(node, file_path, payload);
        } else {
            appendFileToPayload(file_path, node.file_size, payload);
        }
    }
}

static vector<int> payloadToSymbols(const vector<uint8_t> &raw_payload){
    vector<int> symbols(raw_payload.size());
    for (size_t i = 0; i < raw_payload.size(); i++) {
        symbols[i] = static_cast<int>(raw_payload[i]);
    }
    return symbols;
}

DPResult compressSymbols(const vector<int>& symbols) {
    return dpCompress(symbols);
}

// -----------------------------------------------------------------------------
// Compression
// process of directory compression and dpdc file write
// -----------------------------------------------------------------------------
/*
 * DPDC binary layout (little-endian):
 *
 * header: 
 *   [magic: 4 bytes "DPDC"]
 *   [tree_size: u32]
 *   [tree_blob: tree_size bytes]    
 *   [symbol_count: u64]              
 *   [segment_count: u32]
 *   [dp_blob: to EOF]
 *
 * tree_blob:
 *   [version: u8]
 *   node (recursive, DFS + lexicographic sibling order):
 *     [is_directory: u8]
 *     [name_len: u16][name bytes]
 *     if directory: [child_count: u32][child...]
 *     else:         [file_size: u64]
 *
 * payload (dp_blob: bitstream): 
 *   repeat segment_count times:
 *     [this_segment_len: 8 bits]       // how many symbols in this segment (1..256); stored as len - 1
 *     [this_segment_bit_width: 3 bits] // bits per symbol in this segment (1..8); stored as width - 1
 *     [this_segment_symbols: (len * bit_width) bits]
 *   Tip: tail may be zero-padded to a byte boundary
 *
 */
DirectoryCompressMetrics directoryCompress(const fs::path& dir_path, const fs::path& output_path) {
    // for metrics count
    const auto total_time_start = std::chrono::steady_clock::now();
    DirectoryCompressMetrics metrics;

    // 1) deal with tree_blob
    const DirNode dir_tree = buildDirectoryTree(dir_path);
    const vector<uint8_t> tree_blob = serializeDirectoryTree(dir_tree);
    metrics.tree_blob_bytes = static_cast<uint32_t>(tree_blob.size());

   // 2) deal with raw payload
    vector<uint8_t> payload;
    flattenDirectory(dir_tree, dir_path, payload);
    metrics.payload_bytes = payload.size();

    // 3) deal with dp_blob
    const auto dp_time_start = std::chrono::steady_clock::now(); // for dp time count
    vector<int> payload_symbols = payloadToSymbols(payload);     // only do type-casting
    const DPResult dp_result = compressSymbols(payload_symbols);
    BitWriter bit_writer;
    writeSegmentBitstream(bit_writer, payload_symbols, dp_result.segments);
    const vector<uint8_t> dp_blob = bit_writer.toBytes();
    // record dp metrics
    metrics.dp_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - dp_time_start).count();
    metrics.dp_blob_bytes = dp_blob.size();
    metrics.dp_model_bits = dp_result.min_bits;
    metrics.segment_count = static_cast<uint32_t>(dp_result.segments.size());

    // 4) write in dpdc file
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        throw runtime_error("[Compression]无法创建压缩文件：" + output_path.string());
    }

    // write magic
    output.write("DPDC", 4);

    // write tree header (raw data of directory structure)
    if (tree_blob.size() > 0xffffffffu) {
        throw runtime_error("[Compression]目录树大小超出范围(u32)");
    }
    writeU32LE(output, static_cast<uint32_t>(tree_blob.size())); // tree size
    if (!tree_blob.empty()) {
        output.write(reinterpret_cast<const char*>(tree_blob.data()), // tree blob
                     static_cast<std::streamsize>(tree_blob.size()));
    }

    // write dp payload metadata (for readSegmentBitstream on decompress)
    writeU64LE(output, static_cast<uint64_t>(payload_symbols.size())); // count of symbols (raw payload)
    if (dp_result.segments.size() > 0xffffffffu) {
        throw runtime_error("[Compression]负载分段数量超出范围(u32)");
    }
    writeU32LE(output, static_cast<uint32_t>(dp_result.segments.size())); // count of segments (payload processed by dp)

    // write compressed payload blob
    if (!dp_blob.empty()) {
        output.write(reinterpret_cast<const char*>(dp_blob.data()),
                     static_cast<std::streamsize>(dp_blob.size()));
    }

    // 5) write into metrics
    metrics.total_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - total_time_start).count();
    metrics.output_file_bytes = fs::file_size(output_path);
    // fallback for invalid division
    if (metrics.payload_bytes == 0) {
        metrics.model_compression_ratio = std::numeric_limits<double>::quiet_NaN();
        metrics.file_compression_ratio = std::numeric_limits<double>::quiet_NaN();
    } else {
        // calculate the ratios
        const double raw_payload_bits = static_cast<double>(metrics.payload_bytes * 8ULL);
        const double total_file_bits = static_cast<double>(metrics.output_file_bytes * 8ULL);
        metrics.model_compression_ratio = static_cast<double>(metrics.dp_model_bits) / raw_payload_bits;
        metrics.file_compression_ratio = total_file_bits / raw_payload_bits;
    }
    return metrics;
}

// -----------------------------------------------------------------------------
// Decompression
// process of directory decompression and dpdc file read and analysis
// -----------------------------------------------------------------------------
static vector<uint8_t> symbolsToPayload(const vector<int>& symbols) {
    vector<uint8_t> payload(symbols.size());
    for (size_t i = 0; i < symbols.size(); ++i) {
        payload[i] = static_cast<uint8_t>(symbols[i]);
    }
    return payload;
}

// write a payload slice to a regular file
static void writeFileFromPayload(
    const fs::path& file_path,
    uint64_t expected_size,
    const vector<uint8_t>& payload,
    size_t& offset) {
    std::ofstream output(file_path, std::ios::binary);
    if (!output) {
        throw runtime_error("[Decompression]无法创建文件：" + file_path.string());
    }

    // empty file
    if (expected_size == 0) {
        return;
    }

    // lack of data
    if (offset + expected_size > payload.size()) {
        throw runtime_error("[Decompression]数据缺失，无法还原文件：" + file_path.string());
    }

    output.write(reinterpret_cast<const char*>(payload.data() + offset),
                 static_cast<std::streamsize>(expected_size));
    if (!output) {
        throw runtime_error("[Decompression]写入文件失败：" + file_path.string());
    }

    // move offset
    offset += static_cast<size_t>(expected_size);
}

void expandToDirectory(
    const DirNode& tree_root,
    const fs::path& root_path,
    const vector<uint8_t>& payload,
    size_t& offset) {
    if (!fs::exists(root_path)) {
        throw runtime_error("[Decompression]路径不存在：" + root_path.string());
    }

    for (const DirNode& node : tree_root.children) {
        fs::path file_path = root_path / node.name;

        if (node.is_directory) {
            fs::create_directories(file_path);
            expandToDirectory(node, file_path, payload, offset);
        } else {
            writeFileFromPayload(file_path, node.file_size, payload, offset);
        }
    }
}

DirectoryDecompressMetrics directoryDecompress(const fs::path& compressed_file_path, const fs::path& output_path) {
    // for metrics count
    const auto total_time_start = std::chrono::steady_clock::now();
    DirectoryDecompressMetrics metrics;

    // 1) read header
    std::ifstream input(compressed_file_path, std::ios::binary);
    if (!input) {
        throw runtime_error("[Decompression]无法打开压缩文件：" + compressed_file_path.string());
    }

    // read magic
    char magic[4];
    input.read(magic, 4);
    if (!input || std::memcmp(magic, "DPDC", 4)) { // bad input or magic is not "DPDC"
        throw runtime_error("[Decompression]解压失败，不是合法的目录压缩文件");
    }

    // read tree header
    const uint32_t tree_size = readU32LE(input);
    const vector<uint8_t> tree_blob = readBytes(input, tree_size);
    metrics.tree_blob_bytes = tree_size;

    // read dp payload metadata
    const uint64_t symbol_count = readU64LE(input);   // count of symbols in raw payload
    const uint32_t segment_count = readU32LE(input);  // count of segments in dp-compressed payload

    // read compressed payload blob
    const vector<uint8_t> dp_blob = readRemainingBytes(input);

    // 2) rebuild directory tree
    const DirNode dir_tree = deserializeDirectoryTree(tree_blob);

    // 3) resume payload from dp_blob
    BitReader bit_reader(dp_blob);
    const vector<int> symbols = readSegmentBitstream(bit_reader, symbol_count, segment_count);
    const vector<uint8_t> payload = symbolsToPayload(symbols); // only do type-casting
    metrics.payload_bytes = payload.size();

    // 4) write back to disk
    fs::create_directories(output_path);  // create root directory
    size_t offset = 0;
    expandToDirectory(dir_tree, output_path, payload, offset);
    if (offset != payload.size()) {
        throw runtime_error("[Decompression]实际载荷大小与目录树记录不一致");
    }

    metrics.total_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - total_time_start).count();
    return metrics;
}

// -----------------------------------------------------------------------------
// Verify
// process to flatten directory into payload and api of verification
// -----------------------------------------------------------------------------
static vector<uint8_t> flattenDirectoryToPayload(const fs::path& dir_path) {
    const DirNode dir_tree = buildDirectoryTree(dir_path);
    vector<uint8_t> payload;
    flattenDirectory(dir_tree, dir_path, payload);
    return payload;
}

void verifyDirectory(const fs::path& original_dir, const fs::path& restored_dir) {
    const vector<uint8_t> original_payload = flattenDirectoryToPayload(original_dir);
    const vector<uint8_t> restored_payload = flattenDirectoryToPayload(restored_dir);
    const bool same_payload = original_payload == restored_payload;

    std::cout << "========== 目录还原验证 ==========" << std::endl;
    std::cout << "原始目录：" << original_dir.string() << std::endl;
    std::cout << "还原目录：" << restored_dir.string() << std::endl;
    std::cout << "原始 payload 字节数：" << original_payload.size() << std::endl;
    std::cout << "还原 payload 字节数：" << restored_payload.size() << std::endl;
    std::cout << "是否完全一致：" << (same_payload ? "True" : "False") << std::endl;

    if (!same_payload) {
        throw runtime_error("[Verify]目录 roundtrip 校验失败：payload 不一致");
    }
}