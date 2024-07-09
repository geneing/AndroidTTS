cmake_minimum_required(VERSION 3.22)

project(OpenFST C CXX)


SET(OPENFST_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/openfst-1.8.3)
set(fst_source_dir ${OPENFST_ROOT_DIR}/src/lib)
set(fst_include_dir ${OPENFST_ROOT_DIR}/src/include)

include_directories(${fst_include_dir})
file(GLOB fst_sources "${fst_source_dir}/*.cc")
message(STATUS "fst_source_dir... ${fst_source_dir}")
message(STATUS "fst_sources... ${fst_sources}")

add_library(fst ${fst_sources})
target_include_directories(fst PUBLIC
        $<BUILD_INTERFACE:${fst_include_dir}>
        $<INSTALL_INTERFACE:include/openfst>
)

install(TARGETS fst
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