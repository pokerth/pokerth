# vcpkg overlay triplet: Apple Silicon half of the universal macOS build.
#
# PokerTH ships one universal DMG, but the dependencies cannot be built fat in a
# single pass: boost-context selects its hand-written assembly once, from the
# host's CMAKE_SYSTEM_PROCESSOR, and then compiles it for every slice — the
# foreign slice fails to assemble. Each architecture is therefore built on its
# own and the static libraries are merged with lipo afterwards
# (merge_universal_libs in build_macos_common.sh).

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_OSX_ARCHITECTURES arm64)

include("${CMAKE_CURRENT_LIST_DIR}/_pokerth-osx-shared.cmake")
