#!/usr/bin/env bash
# Collect libraries, gallery binary and pkg-config file for CI artifact upload.
set -euo pipefail

platform="${1:?usage: stage-artifacts.sh linux|macos}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
dest="$root/ci-artifacts/$platform"

mkdir -p "$dest/lib" "$dest/bin" "$dest/include"

copy_if_exists()
{
    local file="$1"
    local target_dir="$2"
    if [[ -f "$file" ]]; then
        cp "$file" "$target_dir/"
        return 0
    fi
    echo "Missing expected file: $file" >&2
    return 1
}

case "$platform" in
    linux)
        copy_if_exists "$root/build/libCompages.a" "$dest/lib"
        copy_if_exists "$root/build/libCompages.so" "$dest/lib"
        copy_if_exists "$root/build/Compages-examples" "$dest/bin"
        chmod +x "$dest/bin/Compages-examples" "$dest/lib/"*.so* 2>/dev/null || true
        ;;
    macos)
        copy_if_exists "$root/build/libCompages.a" "$dest/lib"
        copy_if_exists "$root/build/libCompages.dylib" "$dest/lib"
        copy_if_exists "$root/build/Compages-examples" "$dest/bin"
        chmod +x "$dest/bin/Compages-examples" 2>/dev/null || true
        ;;
    *)
        echo "Unknown platform: $platform" >&2
        exit 1
        ;;
esac

copy_if_exists "$root/build/Compages.pc" "$dest/lib"
cp -R "$root/include/Compages" "$dest/include/"
[[ -f "$root/LICENSE" ]] && cp "$root/LICENSE" "$dest/"
grep '^PROJECT_VERSION' "$root/Makefile.common" | awk '{print $3}' > "$dest/VERSION"

echo "Staged $platform artifacts:"
find "$dest" -type f | sort
