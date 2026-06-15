cmake_minimum_required(VERSION 3.22)
project(jysdl LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# vcpkg 预编译库根路径
set(VCPKG_ARM64_ROOT "D:/project/vcpkg/installed/arm64-android-dynamic")
set(VCPKG_ARM64_LIB "${VCPKG_ARM64_ROOT}/lib")
set(VCPKG_ARM64_INC "${VCPKG_ARM64_ROOT}/include")

# 源文件
set(SOURCES
    Audio.cpp
    charset.cpp
    jymain.cpp
    luafun.cpp
    mainmap.cpp
    ParticleExample.cpp
    ParticleSystem.cpp
    piccache.cpp
    PotConv.cpp
    sdlfun.cpp
    SimpleCC.cpp
    ZipFile.cpp
)

add_library(jysdl SHARED ${SOURCES})

target_include_directories(jysdl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VCPKG_ARM64_INC}
    ${VCPKG_ARM64_INC}/SDL3
)

# 预定义
target_compile_definitions(jysdl PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

# 链接预编译 vcpkg .so (直接指定完整路径)
target_link_libraries(jysdl
    ${VCPKG_ARM64_LIB}/libSDL3.so
    ${VCPKG_ARM64_LIB}/libSDL3_image.so
    ${VCPKG_ARM64_LIB}/libSDL3_ttf.so
    ${VCPKG_ARM64_LIB}/libSDL3_mixer.so
    ${VCPKG_ARM64_LIB}/liblua.so
    ${VCPKG_ARM64_LIB}/libiconv.a
    ${VCPKG_ARM64_LIB}/libcharset.a
    ${VCPKG_ARM64_LIB}/libzip.so
)
