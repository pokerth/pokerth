# Settings shared by the two per-architecture PokerTH triplets.
# Not a triplet itself — included by x64-osx-pokerth.cmake and
# arm64-osx-pokerth.cmake.

set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Darwin)

# Only the release halves are ever linked into the shipped binaries; building
# the debug ones as well would double the time for both architectures.
set(VCPKG_BUILD_TYPE release)

# Same floor as the application (MACOSX_DEPLOYMENT_TARGET in
# build_macos_common.sh); ports built against a newer SDK default would drag the
# minimum system up without anyone noticing.
if(DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
    set(VCPKG_OSX_DEPLOYMENT_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})
else()
    set(VCPKG_OSX_DEPLOYMENT_TARGET 12.0)
endif()
