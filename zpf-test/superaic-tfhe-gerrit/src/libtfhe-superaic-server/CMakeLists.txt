cmake_minimum_required(VERSION 3.10)
project(tfhe_superaic_server)

add_library(tfhe_superaic_server SHARED)

target_sources(tfhe_superaic_server
PRIVATE
    tfhe_superaic_server.cpp include/tfhe_superaic_server.h
)

target_include_directories(tfhe_superaic_server
PUBLIC
    include
PRIVATE
    $(CMAKE_CURRENT_LIST_DIR)
)


target_link_libraries(tfhe_superaic_server 
PUBLIC
    tfhe_superaic_core
    tfhe_superaic_acc
)
