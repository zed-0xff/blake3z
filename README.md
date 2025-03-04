# 🚀 Zero Block Caching for Faster BLAKE3 Hashing  

This update adds **zero block caching** to [BLAKE3](https://github.com/BLAKE3-team/BLAKE3), improving performance when hashing large zero-filled regions like sparse files or disk images. Instead of repeatedly processing zero chunks, the hasher now **reuses precomputed values**, reducing CPU load while keeping the hash results unchanged. A **32MB block size** was chosen as a balance between **cache size and memory usage**, ensuring efficient hashing without excessive resource consumption.  

The resulting library remains **fully backwards compatible** with the original BLAKE3, producing identical hashes for all inputs.  

## New function: `blake3_hasher_update_ex`  

```c
void blake3_hasher_update_ex(blake3_hasher *self, const void *input, size_t input_len, int is_zeroes);
```

This function extends `blake3_hasher_update()` by allowing explicit marking of zero-filled data.

When `is_zeroes` is set to `1`, the function skips hashing and instead uses precomputed zero block values, improving performance.
When `is_zeroes` is `0`, it behaves exactly like `blake3_hasher_update()`, ensuring seamless integration with existing code.

## Benchmark

```
# du -sh data
 80M	data

# du -shA data
932G	data

# time b3sum data
b1c2261fe9e9b7c16bd68dc02bc50aeb5988fccbc13d6a0fb1be06ade28b1071  data
b3sum   843,07s user 857,59s system 342% cpu 8:16,35 total

# time b3sum_sp data
b1c2261fe9e9b7c16bd68dc02bc50aeb5988fccbc13d6a0fb1be06ade28b1071  data
b3zsum  0,16s user 0,04s system 78% cpu 0,261 total
```

## Code example

(see [b3zsum/](b3zsum/) or [gen_cache/](gen_cache/) for full code)

```c
blake3_open_cache(cache_fname); // look for pregenerated "blake3z.cache"
for(const auto& fname : fnames){
    uint8_t hash_output[BLAKE3_OUT_LEN];
    blake3z_calc_file(fname, hash_output);
    // do something with output
}
blake3_close_cache();
```

```c
blake3_open_cache(cache_fname); // look for pregenerated "blake3z.cache"
for(const auto& fname : fnames){
    std::string hash = blake3z_calc_file_str(fname);
    // do something with hash
}
blake3_close_cache();
```

## TODO

 - keep only one CV per cached block (instead of current 2 for minimal blake3 patching)
 - port to Rust
 - add multithread processing of non-sparse areas
