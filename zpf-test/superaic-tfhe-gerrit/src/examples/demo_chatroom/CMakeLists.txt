cmake_minimum_required(VERSION 3.10)

project(ChatroomClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# 添加子目录 client 和 server
add_subdirectory(ChatroomClient)
add_subdirectory(ChatroomServer)