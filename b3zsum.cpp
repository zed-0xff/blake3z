#include <fstream>
#include <iostream>
#include <utility> // for std::pair
#include <vector>

#include <fcntl.h>   // for open
#include <unistd.h>  // for lseek

#include <blake3.h>
#include <blake3_impl.h>

typedef std::vector<std::pair<int64_t, int64_t>> SparseMap;

#ifdef O_BINARY
#define OPEN_MODE O_RDONLY|O_BINARY
#else
#define OPEN_MODE O_RDONLY
#endif

SparseMap buildSparseMap(const std::string& file_path, int64_t fileSize) {
    int fd = open(file_path.c_str(), OPEN_MODE);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file: " + file_path + " (" + std::to_string(errno) + ")");
    }
    SparseMap result;
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
    return result;
}

#define CACHE_BLOCK_SIZE (BLAKE3_CACHE_BLOCK_CHUNKS * BLAKE3_CHUNK_LEN)

void bl3_calc_file(const std::string &file_path){
    const std::vector<char> zeroes(CACHE_BLOCK_SIZE, 0);

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }

    size_t fsize = 0;
    file.seekg(0, std::ios::end);
    fsize = file.tellg();
    SparseMap sparseMap = buildSparseMap(file_path, fsize);
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
    uint8_t hash_output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, hash_output, sizeof(hash_output));

    // Print the hash
    for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i) {
        printf("%02x", hash_output[i]);
    }
    printf("  %s\n", file_path.c_str());
}

void usage(char* argv[]){
    printf("Usage: %s [-c cache_fname] <fname1> [fname2 ...]\n", argv[0]);
    exit(1);
}

int main(int argc, char *argv[]) {
    const char* cache_fname = DEFAULT_CACHE_PATHNAME;
    if (argc < 2) {
        usage(argv);
    }

    std::vector<std::string> fnames;
    for(int i=1; i<argc; i++){
        if( strcmp(argv[i], "-c") == 0 ){
            if( i+1 >= argc ){
                usage(argv);
            }
            cache_fname = argv[++i];
        } else {
            fnames.push_back(argv[i]);
        }
    }

    if( fnames.empty() ){
        usage(argv);
    }

    if( !blake3_open_cache(cache_fname) ){
        fprintf(stderr, "[?] Failed to open %s: %s\n", cache_fname, strerror(errno));
    }

    for(const auto& fname : fnames){
        bl3_calc_file(fname);
    }
    blake3_close_cache();
    return 0;
}
