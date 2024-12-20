cmake_minimum_required(VERSION 3.10)
project(tfhe_superaic_examples)

add_executable(server_with_driver server_with_driver.cpp)
target_link_libraries(server_with_driver 
PRIVATE
    tfhe_superaic_driver 
    tfhe_superaic_server )



add_executable(server_without_driver server_without_driver.cpp)
target_link_libraries(server_without_driver 
PRIVATE
    tfhe_superaic_server )

    
add_executable(client_only client_only.cpp)
target_link_libraries(client_only 
PRIVATE
    tfhe_superaic_client )
    

add_executable(comparebits comparebits.cpp)
target_link_libraries(comparebits 
PRIVATE
    tfhe_superaic_core )
    
add_subdirectory(demo_chatroom)