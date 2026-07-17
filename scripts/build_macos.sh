#!/bin/sh
# Usage: scripts/build_macos.sh [debug|release]   (default: debug)
#
# Only src/main.c is compiled. Everything else reaches the compiler through
# src/unity_build.c. Override the compiler with e.g. CC=/opt/homebrew/bin/clang.

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(dirname -- "$script_dir")
build_dir="$root_dir/build"

mode="${1:-debug}"
CC="${CC:-clang}"

# gnu11 rather than c11. -std=c11 defines __STRICT_ANSI__ and hides the POSIX
# declarations base_os.h needs: mmap, clock_gettime, pthreads.
common="-std=gnu11 -Wall -Wextra"

case "$mode" in
    debug)
        flags="$common -g -O0 -DBUILD_DEBUG=1"
        ;;
    release)
        flags="$common -O2 -DBUILD_DEBUG=0 -DNDEBUG"
        ;;
    *)
        echo "usage: $0 [debug|release]" >&2
        exit 1
        ;;
esac

mkdir -p "$build_dir"

set -x
$CC $flags "$root_dir/src/main.c" -o "$build_dir/vsh"
