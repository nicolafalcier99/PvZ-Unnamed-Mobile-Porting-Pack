include("${VCPKG_ROOT_DIR}/triplets/arm64-android.cmake")
set(VCPKG_CMAKE_SYSTEM_VERSION 24)
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DSDL_SYSTEM_ICONV=OFF)
# PVZU: keep the builder's folder names (e.g. a home folder holding vcpkg) out of the libraries' __FILE__ strings
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} -ffile-prefix-map=${VCPKG_ROOT_DIR}=vcpkg")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -ffile-prefix-map=${VCPKG_ROOT_DIR}=vcpkg")
