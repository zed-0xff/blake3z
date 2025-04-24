#!/bin/sh -e

PROJECT=b3zsum

build_image() {
    docker build -t ${PROJECT}-build .
}

build_windows() {
    docker run --rm -v `pwd`:/src -w /src ${PROJECT}-build sh -c "cmake -B build-windows -DCMAKE_TOOLCHAIN_FILE=cmake/mingw_toolchain.cmake && cmake --build build-windows -j4"
}

build_linux() {
    docker run --rm -v `pwd`:/src -w /src ${PROJECT}-build sh -c "cmake -B build-linux -DCMAKE_TOOLCHAIN_FILE=cmake/x64_toolchain.cmake && cmake --build build-linux -j4"
}

if [ $# -eq 0 ]; then
    set -- all
fi

while [ -n "$1" ]; do
    case $1 in
        a|all)
            build_image
            build_linux
            build_windows
            ;;
        c|clean)
            for f in build-windows build-linux; do
                echo "[.] removing $f"
                rm -rf $f
            done
            ;;
        l|linux)
            build_image
            build_linux
            ;;
        w|windows)
            build_image
            build_windows
            ;;
        *)
            echo "Usage: $0 [all|clean|linux|windows].."
            ;;
    esac
    shift
done
