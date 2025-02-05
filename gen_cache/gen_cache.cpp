#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include <blake3.h>
#include <blake3_impl.h> // for IV

#define CACHE_BLOCK_SIZE (BLAKE3_CACHE_BLOCK_CHUNKS * BLAKE3_CHUNK_LEN)
#define THR_PAYLOAD_LEN 32

typedef std::array<uint8_t, 2*BLAKE3_OUT_LEN> record_t;

void gen_cache(const char* fname, size_t num_blocks, size_t nthreads){
    const std::vector<uint8_t> buf(CACHE_BLOCK_SIZE, 0);
    const uint8_t flags = 0;
    std::vector<record_t> out(THR_PAYLOAD_LEN*nthreads);

    std::ofstream of(fname, std::ios::binary);
    if( !of ){
        std::cerr << "Failed to open file: " << fname << ": " << strerror(errno) << std::endl;
        return;
    }

    std::vector<std::thread> threads;
    for(size_t pos=0; pos<num_blocks; pos += THR_PAYLOAD_LEN*nthreads){
        printf("\r[%%] Generating block %zu/%zu ..", pos, num_blocks);
        fflush(stdout);

        for(size_t i=0; i<nthreads; i++){
            threads.push_back(std::thread([pos, i, &buf, flags, &out](){
                for(size_t j=0; j<THR_PAYLOAD_LEN; j++){
                    uint64_t chunk_counter = (pos + i*THR_PAYLOAD_LEN + j)*CACHE_BLOCK_SIZE/BLAKE3_CHUNK_LEN;
                    compress_subtree_to_parent_node_ex(buf.data(), CACHE_BLOCK_SIZE, IV, chunk_counter, flags, out[i*THR_PAYLOAD_LEN + j].data());
                }
            }));
        }
        for(auto& t: threads){
            t.join();
        }
        threads.clear();
        of.write((const char*)out.data(), out.size()*2*BLAKE3_OUT_LEN);
    }

//    for(size_t i = 0; i<num_blocks; i++){
//        printf("\r[%%] Generating block %zu/%zu ..", i+1, num_blocks);
//        fflush(stdout);
//        uint64_t chunk_counter = i*CACHE_BLOCK_SIZE/BLAKE3_CHUNK_LEN;
//        compress_subtree_to_parent_node_ex(buf.data(), CACHE_BLOCK_SIZE, IV, chunk_counter, flags, out);
//        of.write((const char*)out, 2*BLAKE3_OUT_LEN);
//    }
//    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <cache_fname> <num_blocks> [nthreads]\n", argv[0]);
        return 0;
    }
    size_t nthreads;
    if( argc > 3 ){
        nthreads = std::stoul(argv[3]);
    } else {
        nthreads = std::thread::hardware_concurrency();
    }
    printf("Using %zu threads\n", nthreads);

    size_t num_blocks = std::stoul(argv[2]);
    if( num_blocks % (THR_PAYLOAD_LEN*nthreads) != 0 ){
        printf("num_blocks must be divisible by (nthreads*%d)\n", THR_PAYLOAD_LEN);
        return 1;
    }
    gen_cache(argv[1], num_blocks, nthreads);
    return 0;
}

