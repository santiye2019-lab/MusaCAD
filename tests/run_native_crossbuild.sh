#!/usr/bin/env bash
# Test-only compilation of the exact same first-party C engine for mobile targets.
set -euo pipefail
require_symbols() {
    local archive="$1" symbols="$2" needle="$3"
    test -s "$archive"
    grep -F "$needle" "$symbols" >/dev/null
}
case "${1:-}" in
  android)
    if [[ -n "${ANDROID_NDK_HOME:-}" && -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then
      ndk="$ANDROID_NDK_HOME"
    else
      sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-/usr/local/lib/android/sdk}}"
      test -d "$sdk/ndk"
      ndk="$(find "$sdk/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
    fi
    test -n "$ndk"
    test -f "$ndk/build/cmake/android.toolchain.cmake"
    llvm_nm="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm"
    test -x "$llvm_nm"
    for abi in arm64-v8a x86_64; do
      target="build/shared-android-$abi"
      cmake -S core/native -B "$target" -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" -DANDROID_ABI="$abi" -DANDROID_PLATFORM=android-24 -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
      cmake --build "$target" --config Release --parallel 2
      archive="$target/libmusacad_core.a"
      "$llvm_nm" --defined-only "$archive" > "$target/symbols.txt"
      require_symbols "$archive" "$target/symbols.txt" musa_bits_read
      require_symbols "$archive" "$target/symbols.txt" musa_core_mesh_mass_verified_f32
      echo "PASS first-party CAD C ABI Android $abi"
    done
    ;;
  ios-archive)
    test -n "${RUNNER_TEMP:-}"
    test -n "$(find "$RUNNER_TEMP" -maxdepth 1 -name 'musacad-ios-*.o' -print -quit)"
    archive="$RUNNER_TEMP/libmusacad_core-iphoneos-arm64.a"
    xcrun --sdk iphoneos ar -rcs "$archive" "$RUNNER_TEMP"/musacad-ios-*.o
    xcrun --sdk iphoneos nm -g "$archive" > "$RUNNER_TEMP/musacad-ios-symbols.txt"
    require_symbols "$archive" "$RUNNER_TEMP/musacad-ios-symbols.txt" _musa_bits_read
    require_symbols "$archive" "$RUNNER_TEMP/musacad-ios-symbols.txt" _musa_core_mesh_mass_verified_f32
    require_symbols "$archive" "$RUNNER_TEMP/musacad-ios-symbols.txt" _musa_dwg_ac1024_line_fields_probe
    echo "PASS first-party CAD C ABI iPhoneOS arm64 static archive"
    ;;
  ios-simulator)
    # A separate macOS runner cross-compiles the entire shared C core and
    # type-checks the real Swift C ABI import. No iPhone simulator is run.
    test "$(uname -s)" = "Darwin"
    test -n "${RUNNER_TEMP:-}"
    sdk="$(xcrun --sdk iphonesimulator --show-sdk-path)"
    test -d "$sdk"
    source_list="$RUNNER_TEMP/musacad-ios-simulator-sources.txt"
    python3 - <<'PY' > "$source_list"
from pathlib import Path
import re
root = Path("core/native")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
match = re.search(r"add_library\(musacad_core STATIC\s+(.*?)\)", cmake, re.S)
if not match:
    raise SystemExit("Missing the canonical shared C target")
sources = re.findall(r"src/[A-Za-z0-9_]+\.c", match.group(1))
tracked = {str(p.relative_to(root)) for p in (root / "src").glob("*.c")}
if not sources or len(sources) != len(set(sources)) or set(sources) != tracked:
    raise SystemExit("iOS simulator C sources do not match the shared core")
print("\n".join(str(root / p) for p in sources))
PY
    test -s "$source_list"
    target="arm64-apple-ios15.0-simulator"
    out_dir="$RUNNER_TEMP/musacad-core-iossim-arm64"
    mkdir -p "$out_dir"
    compiled=0
    while IFS= read -r source; do
      test -f "$source"
      out="$out_dir/$(basename "${source%.c}").o"
      xcrun --sdk iphonesimulator clang -std=c11 -target "$target" -isysroot "$sdk" \
        -I core/native/include -c "$source" -o "$out"
      test -s "$out"
      compiled=$((compiled+1))
    done < "$source_list"
    test "$compiled" -gt 0
    archive="$out_dir/libmusacad_core.a"
    xcrun --sdk iphonesimulator ar -rcs "$archive" "$out_dir"/*.o
    xcrun --sdk iphonesimulator nm -g "$archive" > "$out_dir/symbols.txt"
    require_symbols "$archive" "$out_dir/symbols.txt" _musa_bits_read
    require_symbols "$archive" "$out_dir/symbols.txt" _musa_core_mesh_mass_verified_f32
    require_symbols "$archive" "$out_dir/symbols.txt" _musa_dwg_ac1024_line_fields_probe
    xcrun --sdk iphonesimulator clang -std=c11 -target "$target" -isysroot "$sdk" \
      -I core/native/include tests/apple_core_abi_link_smoke.c "$archive" -lm \
      -o "$out_dir/apple-abi-link-smoke"
    test -s "$out_dir/apple-abi-link-smoke"
    # The thin Swift layer must import C, with ZERO copied CAD math/parser.
    xcrun --sdk iphonesimulator swiftc -target "$target" -sdk "$sdk" \
      -I platforms/ios/include \
      -Xcc -I -Xcc "$PWD/core/native/include" \
      -parse-as-library -typecheck platforms/ios/MusaCADCoreAdapter.swift
    echo "PASS iOS/iPadOS simulator ARM64 C archive, link and Swift module typecheck ($compiled C units)"
    ;;
  *)
    echo "usage: bash tests/run_native_crossbuild.sh android|ios-archive|ios-simulator" >&2
    exit 2
    ;;
esac
echo "Cross-compile/archive only; NO APK, no emulator or physical-device approval."
