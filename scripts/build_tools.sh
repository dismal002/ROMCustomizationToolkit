#!/usr/bin/env bash
# Build the open-source firmware tools in New_Tools/ and install them into bin/,
# where Unpacker looks them up (Platform::findTool searches bin/ first, adding
# ".exe" on Windows). Linux binaries and Windows ".exe" files coexist in bin/.
#
#   bin/afptool-rs(.exe)     Rockchip RKFW / RKAF / SSFW      Apache-2.0
#   bin/ampack(.exe)         Amlogic burning images           AGPL-3.0
#   bin/imagewty-tool(.exe)  Allwinner IMAGEWTY images        GPL-3.0
#
# Requirements: cargo (Rust) for the two Rust tools, gcc/make for imagewty-tool.
# Windows builds additionally need `rustup target add x86_64-pc-windows-gnu`
# and mingw-w64; both are skipped with a warning when unavailable.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools="$root/New_Tools"
out="$root/bin"
mkdir -p "$out"

# Replace instead of writing through a copy: the Rust targets hardlink their binary.
install_tool() { cp -f "$1" "$2"; chmod +x "$2"; }

echo "== Linux build =="
cargo build --release --manifest-path "$tools/afptool-rs/Cargo.toml"
install_tool "$tools/afptool-rs/target/release/afptool-rs" "$out/afptool-rs"

cargo build --release --manifest-path "$tools/ampack/Cargo.toml"
install_tool "$tools/ampack/target/release/ampack" "$out/ampack"

make -C "$tools/imagewty-tool"
install_tool "$tools/imagewty-tool/imagewty-tool" "$out/imagewty-tool"

echo "== Windows build =="
win_target="x86_64-pc-windows-gnu"

if rustup target list --installed 2>/dev/null | grep -qx "$win_target"; then
    cargo build --release --target "$win_target" --manifest-path "$tools/afptool-rs/Cargo.toml"
    install_tool "$tools/afptool-rs/target/$win_target/release/afptool-rs.exe" "$out/afptool-rs.exe"

    cargo build --release --target "$win_target" --manifest-path "$tools/ampack/Cargo.toml"
    install_tool "$tools/ampack/target/$win_target/release/ampack.exe" "$out/ampack.exe"
else
    echo "warning: rust target $win_target is missing – skipping Windows Rust tools" >&2
    echo "         run: rustup target add $win_target" >&2
fi

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    mkdir -p "$tools/imagewty-tool/build/win"
    # mingw_compat.h supplies the POSIX calls (lstat/pathconf/mkdir) mingw lacks.
    x86_64-w64-mingw32-gcc -D_FILE_OFFSET_BITS=64 -I"$tools/imagewty-tool/include" \
        -include "$root/scripts/mingw_compat.h" \
        -Wall -Wextra -std=c11 -O2 \
        "$tools/imagewty-tool"/src/{main,img_header,img_extract,img_repack,checksum,config_file,print_info}.c \
        -o "$out/imagewty-tool.exe"
else
    echo "warning: x86_64-w64-mingw32-gcc not found – skipping Windows imagewty-tool" >&2
    echo "         install mingw-w64 to build it" >&2
fi

echo "== bin/ =="
ls -l "$out"
