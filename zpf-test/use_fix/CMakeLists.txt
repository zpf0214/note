cmake_minimum_required(VERSION 3.10)
project(MyProject)

# 设置 C++ 标准（可选）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加源文件并创建静态库（如果需要）
add_library(mylib a.cc b.cc)

# 添加可执行文件
add_executable(myexe main.cpp)

# 链接库到可执行文件
target_link_libraries(myexe PRIVATE mylib)

# 如果需要，定义 USE_FIX 宏（可选）
#option(USE_FIX "Enable fixed behavior in function f" OFF)
if(USE_FIX)
    target_compile_definitions(mylib PRIVATE USE_FIX)
endif()

# 设置头文件搜索路径（如果有头文件）
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
