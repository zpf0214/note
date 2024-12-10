cmake_minimum_required(VERSION 3.10)
project(tfhe_superaic)

set(CMAKE_CXX_STANDARD 17)

set(ENABLE_TESTS OFF CACHE BOOL "Build the tests (requires googletest)")


if(DEFINED MUL_TYPE)
else()
    set(MUL_TYPE "NAIVE")
endif()

# 0: NAIVE; 1: FFT; 2: Karatsuba
add_definitions(-DMUL_TYPE_NAIVE=0)
add_definitions(-DMUL_TYPE_FFT=1)
add_definitions(-DMUL_TYPE_KARATSUB=2)

if ( MUL_TYPE STREQUAL "NAIVE" )
    add_definitions(-DMUL_TYPE=MUL_TYPE_NAIVE)
    message("using polynomial mul: NAIVE")
elseif( MUL_TYPE STREQUAL "FFT" )
    add_definitions(-DMUL_TYPE=MUL_TYPE_FFT)
    message("using polynomial mul: FFT")
elseif( MUL_TYPE STREQUAL "KARATSUB" )
    add_definitions(-DMUL_TYPE=MUL_TYPE_KARATSUB)
    message("using polynomial mul: KARATSUB")
endif()

add_subdirectory(driver)
add_subdirectory(muduo)
add_subdirectory(examples)
add_subdirectory(libtfhe-superaic-core)
add_subdirectory(libtfhe-superaic-server)
add_subdirectory(libtfhe-superaic-client)
add_subdirectory(libtfhe-superaic-acc)


if (ENABLE_TESTS)
enable_testing()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DartConfiguration.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/DartConfiguration.tcl COPYONLY)

add_subdirectory(tests)
endif (ENABLE_TESTS)
