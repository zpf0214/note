cmake_minimum_required(VERSION 3.10)
project(tfhe_superaic_client)

add_library(tfhe_superaic_client SHARED)

target_sources(tfhe_superaic_client
PRIVATE
    tfhe_superaic_client.c
)

target_include_directories(tfhe_superaic_client
PUBLIC
    include
PRIVATE
    $(CMAKE_CURRENT_LIST_DIR)
)


target_link_libraries(tfhe_superaic_client 
PUBLIC
    tfhe_superaic_core
)