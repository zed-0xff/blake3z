#include <blake3z_file.hpp>

#define VERSION "0.5"

#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <libloaderapi.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#include <libgen.h>
#endif

std::filesystem::path get_exe_path() {
#if defined(_WIN32) || defined(_WIN64)
    // Windows
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path();  // Return the parent directory
#elif defined(__APPLE__)
    // macOS
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        std::filesystem::path exePath(buffer);
        return exePath.parent_path();  // Return the parent directory
    }
    return ".";  // Return "." if empty or error
#else
    // Linux/Unix
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::filesystem::path exePath(buffer);
        return exePath.parent_path();  // Return the parent directory
    }
    return ".";  // Return "." if empty or error
#endif
}

void usage(char* argv[]){
    printf("b3zsum v%s\n", VERSION);
    printf("Usage: %s [-c cache_fname] <fname1> [fname2 ...]\n", argv[0]);
    exit(1);
}

int main(int argc, char *argv[]) {
    std::filesystem::path cache_fname = DEFAULT_CACHE_PATHNAME;
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

    const int exe_pos = cache_fname.string().find("{exe_path}");
    if( exe_pos != std::string::npos ){
        std::string exe_path = get_exe_path().string();
        cache_fname = cache_fname.string().replace(exe_pos, 10, exe_path);
    }

    if( !blake3_open_cache(cache_fname.string().c_str()) ){
        fprintf(stderr, "[?] Failed to open %s: %s\n", cache_fname.string().c_str(), strerror(errno));
    }

    int result = 0;
    for(const auto& fname : fnames){
        if( std::filesystem::is_directory(fname) ){
            // mimic b3sum behavior
            fprintf(stderr, "b3zsum: %s: Is a directory\n", fname.c_str());
            result = 1;
            continue;
        }

        uint8_t hash_output[BLAKE3_OUT_LEN];
        blake3z_calc_file(fname, hash_output);

        // Print the hash
        for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i) {
            printf("%02x", hash_output[i]);
        }
        printf("  %s\n", fname.c_str());
    }
    blake3_close_cache();
    return result;
}
