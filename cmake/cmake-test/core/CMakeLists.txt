add_library(core src/a.cpp)

target_include_directories(core PRIVATE
    ${CMAKE_CURRENT_DIR}/../../server/include
)

