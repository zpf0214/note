cmake_minimum_required(VERSION 3.20)
project(myProgram)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#add_library(message
#    STATIC
#        Message.hpp
#        Message.cpp
#)

add_library(message-objs
    OBJECT
        Message.hpp
        Message.cpp
)

add_library( message-shared
    SHARED
        $<TARGET_OBJECTS:message-objs>
)

add_library( message-static
    STATIC 
        $<TARGET_OBJECTS:message-objs>
)





add_executable(myProgram hello-world.cpp)
target_link_libraries(myProgram message-static)
