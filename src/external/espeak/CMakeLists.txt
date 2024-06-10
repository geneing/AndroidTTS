cmake_minimum_required(VERSION 3.22)

project(espeak_lib C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
        espeak_lib
        SHARED
        src/espeak_lib.cpp
)

if(MSVC)
    # Force compiler to use UTF-8 for IPA constants
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

elseif(NOT APPLE)
    # Linux flags
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
endif()

if(NOT DEFINED ESPEAK_NG_DIR)
    set(ESPEAK_NG_DIR "${CMAKE_CURRENT_BINARY_DIR}/espeak_ng_external")

    include(ExternalProject)
    ExternalProject_Add(
            espeak_ng_external
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}/espeak_ng_external"
            URL "https://github.com/rhasspy/espeak-ng/archive/0f65aa301e0d6bae5e172cc74197d32a6182200f.zip"
            INSTALL_DIR ${ESPEAK_NG_DIR}/${ANDROID_ABI}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${ESPEAK_NG_DIR}/${ANDROID_ABI}
            CMAKE_ARGS -DUSE_ASYNC:BOOL=OFF
            CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON
            CMAKE_ARGS -BUILD_ESPEAK_NG_TESTS:BOOL=OFF
            CMAKE_ARGS -DUSE_MBROLA:BOOL=OFF
            CMAKE_ARGS -DUSE_LIBSONIC:BOOL=OFF
            CMAKE_ARGS -DUSE_LIBPCAUDIO:BOOL=OFF
            CMAKE_ARGS -DUSE_KLATT:BOOL=OFF
            CMAKE_ARGS -DUSE_SPEECHPLAYER:BOOL=OFF
            CMAKE_ARGS -DEXTRA_cmn:BOOL=ON
            CMAKE_ARGS -DEXTRA_ru:BOOL=ON
            CMAKE_ARGS -DCMAKE_C_FLAGS="-D_FILE_OFFSET_BITS=64"
    )
    add_dependencies(espeak_lib espeak_ng_external)
endif()

target_include_directories(
        espeak_lib PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
        ${ESPEAK_NG_DIR}/include
)

target_link_directories(
        espeak_lib PUBLIC
        ${ESPEAK_NG_DIR}/lib
)

target_link_libraries(
        espeak_lib
        espeak-ng
)

target_compile_features(espeak_lib PUBLIC cxx_std_17)
