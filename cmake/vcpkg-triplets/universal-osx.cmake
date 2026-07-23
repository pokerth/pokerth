# vcpkg overlay triplet: universal macOS libraries (Intel + Apple Silicon).
#
# PokerTH ships a single DMG for both architectures, so the static libraries it
# links (boost, protobuf) must carry both slices — a per-host triplet like
# arm64-osx would silently limit the release to the machine that built it.
#
# Used via build_macos_common.sh:
#   vcpkg install --triplet=universal-osx --overlay-triplets=cmake/vcpkg-triplets
#   cmake -DVCPKG_TARGET_TRIPLET=universal-osx -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"

# The host architecture, so vcpkg does not treat this as a cross build: the fat
# slices come from VCPKG_OSX_ARCHITECTURES below, not from this setting. Asking
# uname rather than CMAKE_HOST_SYSTEM_PROCESSOR, which is empty when cmake
# evaluates a triplet in script mode and would pin every host to x64.
execute_process(COMMAND uname -m
                OUTPUT_VARIABLE _POKERTH_HOST_ARCH
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(_POKERTH_HOST_ARCH STREQUAL "arm64")
    set(VCPKG_TARGET_ARCHITECTURE arm64)
else()
    set(VCPKG_TARGET_ARCHITECTURE x64)
endif()

set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Darwin)

set(VCPKG_OSX_ARCHITECTURES "x86_64;arm64")

# Same floor as the application (see MACOSX_DEPLOYMENT_TARGET in
# build_macos_common.sh); ports built against a newer SDK default would drag the
# minimum system up without anyone noticing.
if(DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
    set(VCPKG_OSX_DEPLOYMENT_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})
else()
    set(VCPKG_OSX_DEPLOYMENT_TARGET 12.0)
endif()
