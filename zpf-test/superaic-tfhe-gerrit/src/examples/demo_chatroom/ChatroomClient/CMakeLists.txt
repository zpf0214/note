cmake_minimum_required(VERSION 3.10)

project(ChatroomClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(TFHE_CORE_INCLUDE "${PROJECT_SOURCE_DIR}/../../../tfhe-superaic-core/include")
set(TFHE_ACC_INCLUDE "${PROJECT_SOURCE_DIR}/../../../tfhe-superaic-acc/include")
set(MUDUO_INCLUDE "${PROJECT_SOURCE_DIR}/../../../muduo")

# 查找库
#REQUIRED 找不到不会静默失败，返回错误，cmake配置中止
find_package(Boost REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)      
# find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets REQUIRED)      
# find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)


# 定义包含头文件目录
include_directories(${TFHE_CORE_INCLUDE})
include_directories(${TFHE_ACC_INCLUDE})
include_directories(${MUDUO_INCLUDE})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/forms)


#定义源文件列表变量
set(PROJECT_SOURCES
    src/main.cpp 
    src/ChatroomClient.cpp
    src/clienteventthread.cpp 
    resources/img.qrc 
    src/ChatroomClient.ui       #dzb TODO 解决 .ui不在src目录会找不到 问题
    include/ChatroomClient.h
    include/clienteventthread.h 
)
add_executable(ChatroomClient ${PROJECT_SOURCES})

#关键字，它用于在 Qt 6 的 qt_add_executable 或 qt_add_library 命令中延迟目标的最终化处理
#使用这个关键字时，你需要在之后手动调用 qt_finalize_target() 或 qt_finalize_executable() 来完成目标的最终化步骤。
# if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#     add_executable(ChatroomClient
#         MANUAL_FINALIZATION
#         ${PROJECT_SOURCES}
#     )
# else()  
#     add_executable(ChatroomClient
#         ${PROJECT_SOURCES}
#     )
# endif()

# 链接 Muduo 库
target_link_libraries(ChatroomClient 
PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    tfhe_superaic_acc
    tfhe_superaic_core
    muduo_net
    muduo_base
    pthread
    rt)