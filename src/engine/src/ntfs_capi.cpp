// ntfs_capi.cpp - minimal C API helpers for extracting DATA runlists from an MFT record
#include "ntfs.h"
#include "ntfs_mft.h"
#include <fstream>
#include <vector>
#include <cstdint>

extern "C" {

// Extract data runs from an MFT record stored in `image_path` at `mft_offset`.
// Parameters:
//  - image_path: path to file
//  - mft_offset: byte offset of MFT record
//  - counts: caller-provided array of uint64_t to receive cluster counts
//  - lcns: caller-provided array of int64_t to receive LCNs (-1 for sparse)
//  - max_runs: capacity of the arrays
// Returns: number of runs written on success, -1 on error.
int ntfs_extract_data_runs(const char* image_path, uint64_t mft_offset,
                          uint64_t* counts, int64_t* lcns, size_t max_runs) {
    if (!image_path || !counts || !lcns || max_runs == 0) return -1;
    std::ifstream ifs(image_path, std::ios::binary);
    if (!ifs) return -1;
    const size_t MFT_RECORD_SIZE = 1024;
    std::vector<uint8_t> buf(MFT_RECORD_SIZE);
    ifs.seekg(mft_offset);
    if (!ifs.read(reinterpret_cast<char*>(buf.data()), buf.size())) return -1;

    // Find DATA attribute similarly to NTFSParser::read_mft_record
    uint16_t attribute_offset = 0;
    if (buf.size() >= 22) attribute_offset = buf[20] | (buf[21] << 8);
    if (attribute_offset == 0 || attribute_offset >= MFT_RECORD_SIZE) return -1;
    size_t attr_off = attribute_offset;
    while (attr_off + 8 < buf.size()) {
        uint32_t attr_type = 0;
        // read 4 bytes LE
        for (int i = 0; i < 4; ++i) attr_type |= (uint32_t)buf[attr_off + i] << (8 * i);
        uint32_t attr_len = 0;
        for (int i = 0; i < 4; ++i) attr_len |= (uint32_t)buf[attr_off + 4 + i] << (8 * i);
        if (attr_type == 0xFFFFFFFF) break;
        if (attr_len == 0) break;
        if (attr_type == 0x80) {
            uint8_t non_res = buf[attr_off + 8];
            if (non_res != 0) {
                uint16_t runlist_offset = buf[attr_off + 32] | (buf[attr_off + 33] << 8);
                size_t run_pos = attr_off + runlist_offset;
                if (run_pos < attr_off + attr_len && run_pos < buf.size()) {
                    size_t avail = attr_off + attr_len - run_pos;
                    std::vector<std::pair<uint64_t,int64_t>> runs_parsed;
                    // reuse decode_data_runs from ntfs_mft.cpp via declaration
                    if (decode_data_runs(buf.data() + run_pos, avail, runs_parsed)) {
                        normalize_data_runs(runs_parsed);
                        size_t take = std::min<size_t>(runs_parsed.size(), max_runs);
                        for (size_t i = 0; i < take; ++i) {
                            counts[i] = runs_parsed[i].first;
                            lcns[i] = runs_parsed[i].second;
                        }
                        return static_cast<int>(take);
                    }
                }
            }
        }
        attr_off += attr_len;
    }
    return -1;
}

} // extern "C"
