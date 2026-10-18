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
    echo "PASS first-party CAD C ABI iPhoneOS arm64 static archive"
    ;;
  *)
    echo "usage: bash tests/run_native_crossbuild.sh android|ios-archive" >&2
    exit 2
    ;;
esac
echo "Cross-compile/archive only; NO APK, no emulator or physical-device approval."
