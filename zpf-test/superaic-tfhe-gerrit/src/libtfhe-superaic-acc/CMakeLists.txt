cmake_minimum_required(VERSION 3.10)
project(tfhe_superaic_acc)

add_library(tfhe_superaic_acc SHARED)

target_sources(tfhe_superaic_acc
PRIVATE
    tfhe_acc.cpp include/tfhe_acc.h
    accs/executor.cpp
    accs/CPU-ser/acc_cpu_ser.cpp
    accs/FPGA-V0/acc_fpga_v0.cpp
    accs/QEMU-V0/acc_qemu_v0.cpp
)

target_include_directories(tfhe_superaic_acc
PUBLIC
    include
PRIVATE
    accs
    $(CMAKE_CURRENT_LIST_DIR)
)

target_link_libraries(tfhe_superaic_acc
PUBLIC
    tfhe_superaic_core
)
