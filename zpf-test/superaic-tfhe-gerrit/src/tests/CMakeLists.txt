cmake_minimum_required(VERSION 3.10)


add_subdirectory(googletest EXCLUDE_FROM_ALL)


#if (ENABLE_MEMTESTS  STREQUAL "ON" )
if ( ENABLE_MEMTESTS )
    message("ENABLE_MEMTESTS")
    set(CORE_CPP_ITESTS
        small_size_test
        package_test
        session_test
        ACC_test
        # QEMU_test    # QEMU_test 依赖于设备，所以从内存测试中移除
        # moduo_test   # moduo_test 在服务器上algrind会报非法指令的错误，可能是服务器的处理器比较新，valgrind没有支持最新的指令
    )

    find_program(VALGRIND_EXECUTABLE valgrind)
    
    foreach (CORE_CPP_ITEST ${CORE_CPP_ITESTS})
        add_executable(${CORE_CPP_ITEST} ${CORE_CPP_ITEST}.cpp )
        target_link_libraries(${CORE_CPP_ITEST}
    PRIVATE
        tfhe_superaic_core 
        tfhe_superaic_server
        gtest gtest_main -lpthread
        )
        add_test(
        NAME ${CORE_CPP_ITEST} 
        COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --track-origins=yes --show-reachable=yes --error-exitcode=5 ./${CORE_CPP_ITEST} 
    )

    endforeach (CORE_CPP_ITEST)

elseif( ENABLE_QEMUTESTS )

    message("ENABLE_QEMUTESTS")
    set(CORE_CPP_ITESTS
        QEMU_test
    )

    find_program(VALGRIND_EXECUTABLE valgrind)

    foreach (CORE_CPP_ITEST ${CORE_CPP_ITESTS})
        add_executable(${CORE_CPP_ITEST} ${CORE_CPP_ITEST}.cpp )
        target_link_libraries(${CORE_CPP_ITEST}
    PRIVATE
        tfhe_superaic_core 
        tfhe_superaic_server
        gtest gtest_main -lpthread
        )
        add_test(
        NAME ${CORE_CPP_ITEST} 
        COMMAND ${CORE_CPP_ITEST} 
    )

    endforeach (CORE_CPP_ITEST)



elseif( ENABLE_FPGATESTS )

    message("ENABLE_FPGATESTS")
    set(CORE_CPP_ITESTS
        FPGA_ACC_test
    )


    foreach (CORE_CPP_ITEST ${CORE_CPP_ITESTS})
        add_executable(${CORE_CPP_ITEST} ${CORE_CPP_ITEST}.cpp )
        target_link_libraries(${CORE_CPP_ITEST}
    PRIVATE
        tfhe_superaic_core 
        tfhe_superaic_server
        gtest gtest_main -lpthread
        )
        add_test(
        NAME ${CORE_CPP_ITEST} 
        COMMAND ${CORE_CPP_ITEST} 
    )

    endforeach (CORE_CPP_ITEST)



else()
    set(CORE_CPP_ITESTS
        #        io_test
        #        tgsw_test
        #        tgsw_direct_test
        #        tlwe_test
        #        polynomial_test
        #        lagrangehalfc_test
        #        lwe_test
        #        arithmetic_test
        #        test-lwe
        #        unit_test_torus
        #        lwekeyswitch_test
        #        test-decomp-tgsw
        #        bootstrapping_test
        #        bootstrapping_direct_test
        #        small_size_test
        #        package_test
        #        session_test
        #        ACC_test
        #        moduo_test
        #        bootstrapping_classic_PBS_parameters_test
        #        message_encode
        #        ops_add
        #        ops_add_with_carry
        decrypt_with_plaintext_modulus
    )

    foreach (CORE_CPP_ITEST ${CORE_CPP_ITESTS})
        add_executable(${CORE_CPP_ITEST} ${CORE_CPP_ITEST}.cpp )
        target_link_libraries(${CORE_CPP_ITEST}
    PRIVATE
        tfhe_superaic_core 
        tfhe_superaic_server
        muduo_net
        gtest gtest_main -lpthread
        )
        add_test(
        NAME ${CORE_CPP_ITEST} 
        COMMAND ${CORE_CPP_ITEST} 
    )

    endforeach (CORE_CPP_ITEST)

endif ()


