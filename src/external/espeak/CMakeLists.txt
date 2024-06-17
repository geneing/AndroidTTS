cmake_minimum_required(VERSION 3.22)

project(espeak_lib C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JNI_LIB_PATH}/app/src/main/jniLibs/${ANDROID_ABI}" )

add_library(
        espeak_lib
        SHARED
        src/espeak_lib.cpp
        src/espeak_jni.cpp
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
    set(ESPEAK_NG_DIR "${CMAKE_CURRENT_BINARY_DIR}/")

    include(ExternalProject)
    ExternalProject_Add(
            espeak_ng_external
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}/"
            GIT_REPOSITORY "https://github.com/geneing/espeak-ng.git"
#            URL "https://github.com/rhasspy/espeak-ng/archive/0f65aa301e0d6bae5e172cc74197d32a6182200f.zip"
            INSTALL_DIR ${ESPEAK_NG_DIR}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}
            CMAKE_ARGS -DUSE_ASYNC:BOOL=TRUE
            CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=TRUE
            CMAKE_ARGS -DBUILD_ESPEAK_NG_TESTS:BOOL=FALSE
            CMAKE_ARGS -DBUILD_ESPEAK_NG_EXE:BOOL=FALSE
            CMAKE_ARGS -DUSE_MBROLA:BOOL=FALSE
            CMAKE_ARGS -DUSE_LIBSONIC:BOOL=FALSE
            CMAKE_ARGS -DUSE_LIBPCAUDIO:BOOL=FALSE
            CMAKE_ARGS -DUSE_KLATT:BOOL=FALSE
            CMAKE_ARGS -DUSE_SPEECHPLAYER:BOOL=FALSE
            CMAKE_ARGS -DEXTRA_cmn:BOOL=TRUE
            CMAKE_ARGS -DEXTRA_ru:BOOL=TRUE
            CMAKE_ARGS -DCMAKE_C_FLAGS="-D_FILE_OFFSET_BITS=64"

            CMAKE_ARGS -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
            CMAKE_ARGS -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
            CMAKE_ARGS -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}
            CMAKE_ARGS -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            CMAKE_ARGS -DANDROID_ABI=${ANDROID_ABI}
            CMAKE_ARGS -DCMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI}
            CMAKE_ARGS -DANDROID_NDK=${ANDROID_NDK}
            CMAKE_ARGS -DCMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK}
            CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            CMAKE_ARGS -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
            CMAKE_ARGS -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

            CMAKE_ARGS -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
    add_dependencies(espeak_lib espeak_ng_external)
endif()

target_include_directories(
        espeak_lib PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
        ${ESPEAK_NG_DIR}/src/espeak_ng_external/src/include
)

target_link_directories(
        espeak_lib PUBLIC
        ${ESPEAK_NG_DIR}/lib
)

target_link_libraries(
        espeak_lib
        espeak-ng
        log
)

target_compile_features(espeak_lib PUBLIC cxx_std_17)

