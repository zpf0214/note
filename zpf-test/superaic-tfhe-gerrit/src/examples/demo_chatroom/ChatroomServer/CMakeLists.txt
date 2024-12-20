cmake_minimum_required(VERSION 3.10)

project(ChatroomServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# 查找 Muduo 库
#set(MUDUO_DIR_LIB "${PROJECT_SOURCE_DIR}/../../../../build/muduo/lib")
set(MUDUO_DIR_SRC "${PROJECT_SOURCE_DIR}/../../../muduo")
#message(${MUDUO_DIR_LIB})
#message(${MUDUO_DIR_SRC})

find_package(Boost REQUIRED)
# find_library(MUDUO_BASE muduo_base ${MUDUO_DIR_LIB})
# find_library(MUDUO_NET muduo_net ${MUDUO_DIR_LIB})
# message(STATUS ${MUDUO_BASE})
# message(STATUS ${MUDUO_NET})

# 定义包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${MUDUO_DIR_SRC})

#定义源文件列表变量
set(SOURCE_FILES 
    ${PROJECT_SOURCE_DIR}/src/ChatroomServer.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)

# 定义可执行文件
add_executable(ChatroomServer ${SOURCE_FILES})

# 链接 Muduo 库
target_link_libraries(ChatroomServer muduo_net)
target_link_libraries(ChatroomServer muduo_base)
target_link_libraries(ChatroomServer pthread rt)
