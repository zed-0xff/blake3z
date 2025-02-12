#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

#include <blake3.h>

#define CACHE_BLOCK_SIZE (BLAKE3_CACHE_BLOCK_CHUNKS * BLAKE3_CHUNK_LEN)

typedef std::vector<std::pair<int64_t, int64_t>> SparseMap;

void blake3z_calc_file(const std::filesystem::path &file_path, uint8_t hash_output[BLAKE3_OUT_LEN]);
std::string blake3z_calc_file_str(const std::filesystem::path &file_path);

SparseMap build_sparse_map(const std::string& file_path, int64_t fileSize);
