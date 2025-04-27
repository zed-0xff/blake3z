#include "common.hpp"

#include <windows.h>
#include <fcntl.h>
#include <iostream>

std::filesystem::path get_exe_path(wchar_t* argv[]){
    std::filesystem::path exePath(argv[0]);
    return exePath.parent_path(); // Return the parent directory
}

void usage(wchar_t* argv[]){
    std::wcout << "b3zsum v" << VERSION << std::endl;
    std::wcout << "Usage: " << argv[0] << " [-s] [-c cache_fname] <fname1> [fname2 ...]" << std::endl;
    exit(1);
}

int main(int, char*[]){
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::filesystem::path cache_fname = DEFAULT_CACHE_PATHNAME;
    bool silent = false;

    if (argc < 2) {
        usage(argv);
    }

    std::vector<std::filesystem::path> fnames;
    for(int i=1; i<argc; i++){
        if( wcscmp(argv[i], L"-s") == 0 ){
            silent = true;
        } else if( wcscmp(argv[i], L"-c") == 0 ){
            if( i+1 >= argc ){
                usage(argv);
            }
            cache_fname = argv[++i];
        } else if( wcscmp(argv[i], L"-sc") == 0 ){
            silent = true;
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

    const int exe_pos = cache_fname.u8string().find((char8_t*)"{exe_path}");
    if( exe_pos != std::u8string::npos ){
        cache_fname = cache_fname.u8string().replace(exe_pos, 10, get_exe_path(argv).u8string());
    }

    if( !blake3_open_cache(cache_fname.string().c_str()) ){
        if( !silent ){
            std::wcerr << "[?] Failed to open " << cache_fname << ": " << strerror(errno) << std::endl;
        }
    }

    int result = 0;
    for(const auto& fname : fnames){
        if( std::filesystem::is_directory(fname) ){
            if( !silent ){
                // mimic b3sum behavior
                std::wcerr << "b3zsum: " << fname.native() << ": Is a directory" << std::endl;
                result = 1;
            }
            continue;
        }

        {
        std::ifstream f(fname, std::ios::binary);
        if( !f ){
            if( !silent ){
                std::wcerr << "b3zsum: " << fname.native() << ": " << strerror(errno) << " (os error " << errno << ")" << std::endl;
                result = 1;
            }
            continue;
        }
        }

        uint8_t hash_output[BLAKE3_OUT_LEN];
        blake3z_calc_file(fname, hash_output);

        // Print the hash
        for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i) {
            std::wcout << std::setw(2) << std::setfill(L'0') << std::hex << (int)hash_output[i];
        }
        std::wcout << "  " << fname.native() << std::endl;
    }
    blake3_close_cache();
    return result;
}
