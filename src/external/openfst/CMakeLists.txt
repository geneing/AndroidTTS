cmake_minimum_required(VERSION 3.22)

project(OpenFST C CXX)

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JNI_LIB_PATH}/app/src/main/jniLibs/${ANDROID_ABI}" )

SET(OPENFST_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/openfst-1.8.3)
set(fst_source_dir ${OPENFST_ROOT_DIR}/src/lib)
set(fst_include_dir ${OPENFST_ROOT_DIR}/src/include)

include_directories(${fst_include_dir})
file(GLOB fst_sources "${fst_source_dir}/*.cc")
message(STATUS "fst_source_dir... ${fst_source_dir}")
message(STATUS "fst_sources... ${fst_sources}")

add_library(
        openfst_lib
        SHARED
        openfst_api.cpp
        openfst_jni.cpp
        ${fst_sources}
        ${OPENFST_ROOT_DIR}/src/extensions/far/stlist.cc
        ${OPENFST_ROOT_DIR}/src/extensions/far/sttable.cc
)

target_include_directories(openfst_lib PUBLIC
        $<BUILD_INTERFACE:${fst_include_dir}>
        $<INSTALL_INTERFACE:include/openfst>
)


install(TARGETS openfst_lib
        EXPORT fst-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${fst_include_dir}/fst
        DESTINATION include/openfst
        PATTERN "test/*.h" EXCLUDE
)

unset(fst_source_dir)
unset(fst_include_dir)
unset(fst_sources)