#include "blake3z_file.hpp"

#include <fcntl.h>   // for open
#include <unistd.h>  // for lseek

#include <fstream>
#include <sstream>

#ifdef O_BINARY
#define OPEN_MODE O_RDONLY|O_BINARY
#else
#define OPEN_MODE O_RDONLY
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

SparseMap build_sparse_map(const std::filesystem::path& file_path, int64_t fileSize) {
    SparseMap result;
#ifdef SEEK_HOLE
    int fd = open(file_path.c_str(), OPEN_MODE);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file: " + file_path.string() + " (" + std::to_string(errno) + ")");
    }
    for (int64_t pos = 0; pos < fileSize;) {
        int64_t nextHole = lseek(fd, pos, SEEK_HOLE);
        if (nextHole == -1) {
            break;
        }
        int64_t nextData = lseek(fd, nextHole, SEEK_DATA);
        if (nextData == -1) {
            break;
        }
        result.emplace_back(nextHole, nextData);
        pos = nextData;
    }
    close(fd);
#else
    HANDLE hFile = CreateFileW(
        file_path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        nullptr
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open file: " + file_path.string());
    }

    FILE_ALLOCATED_RANGE_BUFFER inRange = { 0 };
    inRange.Length.QuadPart = fileSize;

    FILE_ALLOCATED_RANGE_BUFFER outRanges[1024]; // TODO: correct size
    DWORD bytesReturned;

    if (DeviceIoControl(
            hFile,
            FSCTL_QUERY_ALLOCATED_RANGES,
            &inRange,
            sizeof(inRange),
            outRanges,
            sizeof(outRanges),
            &bytesReturned,
            nullptr)) {

        DWORD rangeCount = bytesReturned / sizeof(FILE_ALLOCATED_RANGE_BUFFER);
        int64_t pos = 0;

        for (DWORD i = 0; i < rangeCount; ++i) {
            const auto& r = outRanges[i];
            if (r.FileOffset.QuadPart > pos) {
                result.emplace_back(pos, r.FileOffset.QuadPart); // sparse region
            }
            pos = r.FileOffset.QuadPart + r.Length.QuadPart;
        }

        if (pos < fileSize) {
            result.emplace_back(pos, fileSize); // trailing sparse region
        }

    } else {
        CloseHandle(hFile);
        throw std::runtime_error("DeviceIoControl failed: " + std::to_string(GetLastError()));
    }

    CloseHandle(hFile);
#endif
    return result;
}

void blake3z_calc_file(const std::filesystem::path &file_path, uint8_t hash_output[BLAKE3_OUT_LEN]){
    const std::vector<char> zeroes(CACHE_BLOCK_SIZE, 0);

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path.string());
    }

    size_t fsize = 0;
    file.seekg(0, std::ios::end);
    fsize = file.tellg();
    SparseMap sparseMap = build_sparse_map(file_path, fsize);
    file.seekg(0, std::ios::beg);

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Read the file in chunks and update the hash
    std::vector<char> buf(CACHE_BLOCK_SIZE);
    size_t pos = 0;
    auto hole = sparseMap.begin();
    
    while (pos < fsize){
        if( hole != sparseMap.end() && fsize - pos >= CACHE_BLOCK_SIZE ){
            if( pos >= hole->second ){
                hole++;
            }
            if( hole != sparseMap.end() ){
                while( pos >= hole->first && pos + CACHE_BLOCK_SIZE <= hole->second ){
                    blake3_hasher_update_ex(&hasher, zeroes.data(), CACHE_BLOCK_SIZE, 1);
                    pos += CACHE_BLOCK_SIZE;
                }
                if( pos >= fsize ){
                    break;
                } else {
                    file.seekg(pos);
                }
            }
        }
        file.read(buf.data(), buf.size());
        size_t nread = file.gcount();
        if( nread == 0 ){
            printf("[?] nread == 0\n");
            break;
        }
        blake3_hasher_update(&hasher, buf.data(), nread);
        pos += nread;
    }

    // Finalize the hash
    blake3_hasher_finalize(&hasher, hash_output, BLAKE3_OUT_LEN);
}

std::string blake3z_calc_file_str(const std::filesystem::path &file_path){
    uint8_t hash_output[BLAKE3_OUT_LEN];
    blake3z_calc_file(file_path, hash_output);

    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    for (size_t i = 0; i < BLAKE3_OUT_LEN; i++) {
        hex_stream << std::setw(2) << static_cast<int>(hash_output[i]);
    }

    return hex_stream.str();
}
