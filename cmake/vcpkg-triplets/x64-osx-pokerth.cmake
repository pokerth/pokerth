# vcpkg overlay triplet: Intel half of the universal macOS build.
# See arm64-osx-pokerth.cmake for why the two architectures are built
# separately and merged afterwards.

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_OSX_ARCHITECTURES x86_64)

include("${CMAKE_CURRENT_LIST_DIR}/_pokerth-osx-shared.cmake")
