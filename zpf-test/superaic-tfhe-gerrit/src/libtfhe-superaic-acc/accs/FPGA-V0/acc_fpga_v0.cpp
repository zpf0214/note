#include <iostream>
#include <cassert>
#include <functional>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

#include "libacc.h"
#include "tfhe.h"
#include "tfhe_acc.h"

namespace tfhe_superaic {

#define MAP_SIZE (32*1024UL)
#define MAP_MASK (MAP_SIZE - 1)

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));  } while(0)

bool get_FPGA_version(const char * dev,uint32_t &version) {
    int fd;
    void *map_base, *virt_addr; 
    if ((fd = open(dev, O_RDWR | O_SYNC)) == -1) {
        FATAL;
        return(false);
    }
    printf("character device %s opened.\n", dev); 
    fflush(stdout);

    /* map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_base == (void *) -1) {
        FATAL;
        close(fd);
        return(false);
    }
    printf("Memory mapped at address %p.\n", map_base); 
    fflush(stdout);

    virt_addr = (char *)map_base + TFHE_ACC::REG_VERSION_ID_Addr;
    version = *((uint32_t *) virt_addr);
    version =  ltohl(version);
    if (munmap(map_base, MAP_SIZE) == -1) {
        FATAL;
        close(fd);
        return false;
    }
    close(fd);
    return true;

}


void FPGA_ACC_V0::torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
    Torus32 ri;
    for (int32_t i=0; i<N; i++) {
        ri=0;
            for (int32_t j=0; j<=i; j++) {
                ri += poly1[j]*poly2[i-j];
            }
            for (int32_t j=i+1; j<N; j++) {
                ri -= poly1[j]*poly2[N+i-j];
            }
        result[i]=ri;
    }
}

void FPGA_ACC_V0::torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2) {
    static int run_index = 0;
    const int32_t N = poly1->N;
    assert(N==1024);
    assert(result!=poly2);
    assert(poly2->N==N && result->N==N);
    TorusPolynomial temp(N);
#if 0     
    torusPolynomialMultNaive_aux(temp.coefsT, poly1->coefs, poly2->coefsT, N);
#else
//#define CHECK_RESULT
#ifdef CHECK_RESULT // check result
    TorusPolynomial temp1(N);
    torusPolynomialMultNaive_aux(temp1.coefsT, poly1->coefs, poly2->coefsT, N);
#endif

    set_full_coef(poly1->coefs,FPGA_ACC_V0::FULL_P_START_ADDR_0);
    set_full_coef(poly2->coefsT,FPGA_ACC_V0::FULL_Q_START_ADDR_0);
    assert(exec_interrupt() == ACC_OK);
    //assert(exec_non_interrupt() == ACC_OK);
    get_full_coef(temp.coefsT,FPGA_ACC_V0::FULL_V_START_ADDR_0);

#ifdef CHECK_RESULT // check result
    bool bError;
    for( int i = 0 ;i< N;++i) {
        if (temp1.coefsT[i] != temp.coefsT[i]) {
            bError = true;
            //break;
            fprintf(stderr, "[%d:%i]find error: %d(FPGA) vs %d(CPU) \n", run_index,i,temp.coefsT[i],temp1.coefsT[i] );
        }
    }
    if(bError) {
        char poly1_filename[255];
        char poly2_filename[255];
        char result_FPGA_filename[255];
        char reuslt_CPU_filename[255];

        char poly1_hex_filename[255];
        char poly2_hex_filename[255];
        char result_hex_FPGA_filename[255];
        char reuslt_hex_CPU_filename[255];


        sprintf(poly1_filename,"poly1_%03d.txt",run_index);
        sprintf(poly2_filename,"poly2_%03d.txt",run_index);
        sprintf(result_FPGA_filename,"result_FPGA_%03d.txt",run_index);
        sprintf(reuslt_CPU_filename,"result_CPU_%03d.txt",run_index);

        sprintf(poly1_hex_filename,"poly1_hex_%03d.txt",run_index);
        sprintf(poly2_hex_filename,"poly2_hex_%03d.txt",run_index);
        sprintf(result_hex_FPGA_filename,"result_hex_FPGA_%03d.txt",run_index);
        sprintf(reuslt_hex_CPU_filename,"result_hex_CPU_%03d.txt",run_index);


        FILE * fp_poly1 = fopen(poly1_filename,"w");
        FILE * fp_poly2 = fopen(poly2_filename,"w");
        FILE * fp_result_FPGA = fopen(result_FPGA_filename,"w");
        FILE * fp_result_CPU = fopen(reuslt_CPU_filename,"w");

        FILE * fp_poly1_hex = fopen(poly1_hex_filename,"w");
        FILE * fp_poly2_hex = fopen(poly2_hex_filename,"w");
        FILE * fp_result_FPGA_hex = fopen(result_hex_FPGA_filename,"w");
        FILE * fp_result_CPU_hex = fopen(reuslt_hex_CPU_filename,"w");


        for ( int i = 0;i < N-1;i++){
            fprintf(fp_poly1,"%d\n",poly1->coefs[i]);
            fprintf(fp_poly1_hex,"0x%08x\n",poly1->coefs[i]);

            fprintf(fp_poly2,"%d\n",poly2->coefsT[i]);
            fprintf(fp_poly2_hex,"0x%08x\n",poly2->coefsT[i]);

            fprintf(fp_result_FPGA,"%d\n",temp.coefsT[i]);
            fprintf(fp_result_FPGA_hex,"0x%08x\n",temp.coefsT[i]);

            fprintf(fp_result_CPU,"%d\n",temp1.coefsT[i]);
            fprintf(fp_result_CPU_hex,"0x%08x\n",temp1.coefsT[i]);


        }

        fclose(fp_result_CPU_hex);
        fclose(fp_result_FPGA_hex);
        fclose(fp_poly2_hex);
        fclose(fp_poly1_hex);

        fclose(fp_result_CPU);
        fclose(fp_result_FPGA);
        fclose(fp_poly2);
        fclose(fp_poly1);

    }

#endif // check result
#endif
    torusPolynomialAddTo(result,&temp);

    run_index ++;
}

FPGA_ACC_V0::FPGA_ACC_V0():
    TFHE_ACC(3,ACC_FPGA,CAPABILITY_PBS){
        paral_mul = false;
}

FPGA_ACC_V0::~FPGA_ACC_V0() {
    if( control_fp ||  user_fp || h2c_fp || c2h_fp || control_map_base || user_map_base ) {
        deinit();
    }
}

bool FPGA_ACC_V0::match(uint32_t version) {
    if( (version & 0xff) == 0xa1 ) {
        return true;
    }

    return false;

}


ACC_RESULT FPGA_ACC_V0::init(const char * config_dev, const char * user_dev, const char * h2c_dev, const char * c2h_dev) {
    if( control_fp ||  user_fp || h2c_fp || c2h_fp || control_map_base || user_map_base ) {
        deinit();
    }

    if ((control_fp = open(config_dev, O_RDWR | O_SYNC)) == -1) { FATAL; control_fp = 0;return ACC_INVALID_FILE;}

    control_map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, control_fp, 0);
    if (control_map_base == (void *) -1) {FATAL; control_map_base = nullptr;return ACC_INVALID_RES;}


    if ((user_fp = open(user_dev, O_RDWR | O_SYNC)) == -1) { FATAL; user_fp = 0;return ACC_INVALID_FILE;}

    user_map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, user_fp, 0);
    if (user_map_base == (void *) -1) {FATAL; user_map_base = nullptr;return ACC_INVALID_RES;}


    if ((h2c_fp = open(h2c_dev, O_RDWR | O_SYNC)) == -1) { FATAL; h2c_fp = 0;return ACC_INVALID_FILE;}


    if ((c2h_fp = open(c2h_dev, O_RDWR | O_SYNC)) == -1) { FATAL; c2h_fp = 0;return ACC_INVALID_FILE;}

    return ACC_OK;

}

void FPGA_ACC_V0::deinit(void) {
    if(control_map_base){
        if (munmap(control_map_base, MAP_SIZE) == -1) FATAL;
        control_map_base = nullptr;
    }


    if(control_fp){
        if( close(control_fp) != 0 ) FATAL;
        control_fp = 0;
    }


    if(user_map_base){
        if (munmap(user_map_base, MAP_SIZE) == -1) FATAL;
        user_map_base = nullptr;
    }


    if(user_fp){
        if(close(user_fp) != 0) FATAL;
        user_fp = 0;
    }


    if(h2c_fp){
        if(close(h2c_fp) != 0) FATAL;
        h2c_fp = 0;
    }


    if(c2h_fp){
        if(close(c2h_fp) != 0) FATAL;
        c2h_fp = 0;
    }

}


ACC_RESULT FPGA_ACC_V0::torusPolynomialMultFPGA(Torus32* __restrict result, const Torus32* __restrict poly1, const Torus32* __restrict poly2,uint8_t executor, bool poll) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }

    if (!c2h_fp ) {
        fprintf(stderr, "c2h_fp is not init\n");
        return ACC_INVALID_RES;
    }

    if (!h2c_fp ) {
        fprintf(stderr, "c2h_fp is not init\n");
        return ACC_INVALID_RES;
    }

    if (executor >= TOTAL_MUL_EXECULTORS) {
        fprintf(stderr, "executor(%d) larger than TOTAL_MUL_EXECULTORS(%d)\n", executor, TOTAL_MUL_EXECULTORS);
        return ACC_INVALID_PARAM;
    }

    set_full_coef(poly1,FULL_P_START_ADDRs[executor]);
    set_full_coef(poly2,FULL_Q_START_ADDRs[executor]);
    if( !poll ) {
        exec_interrupt(nullptr,executor);
    }else{
        exec_non_interrupt(nullptr,executor);
    }

    get_full_coef(result,FULL_V_START_ADDRs[executor]);


    return ACC_OK;
}



ACC_RESULT FPGA_ACC_V0::enableInterrupt(bool en) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }
    __write_word_mask_lite(REG_XDMA_Interrupt_Enable_Addr,(en ? 1 : 0) << XDMA_Interrupt_Enable_Offset,1<<XDMA_Interrupt_Enable_Offset);
    return ACC_OK;
}

ACC_RESULT FPGA_ACC_V0::clearInterrupt(uint8_t executor) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }
    assert(executor < TOTAL_MUL_EXECULTORS);
    __write_word_mask_lite(REG_XDMA_Interrupt_Clear_Addr,0 << XDMA_Interrupt_Clear_Offsets[executor],1 << XDMA_Interrupt_Clear_Offsets[executor]);
    __write_word_mask_lite(REG_XDMA_Interrupt_Clear_Addr,1 << XDMA_Interrupt_Clear_Offsets[executor],1 << XDMA_Interrupt_Clear_Offsets[executor]);
    return ACC_OK;

}


ACC_RESULT FPGA_ACC_V0::start(uint8_t executor) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }

    assert(executor < TOTAL_MUL_EXECULTORS);

    __write_word_mask_lite(REG_PMUL_ST_Addr,0 << Polynomial_Mul_Start_Offsets[executor],1 << Polynomial_Mul_Start_Offsets[executor]);
    __write_word_mask_lite(REG_PMUL_ST_Addr,1 << Polynomial_Mul_Start_Offsets[executor],1 << Polynomial_Mul_Start_Offsets[executor]);
    return ACC_OK;
}

ACC_RESULT FPGA_ACC_V0::clear_start(uint8_t executor) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }

    assert(executor < TOTAL_MUL_EXECULTORS);

    __write_word_mask_lite(REG_PMUL_ST_Addr,0 << Polynomial_Mul_Start_Offsets[executor],1 << Polynomial_Mul_Start_Offsets[executor]);
    return ACC_OK;
}


ACC_RESULT FPGA_ACC_V0::exec_non_interrupt(struct timespec *ts_dur,uint8_t executor) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }
    assert(executor < TOTAL_MUL_EXECULTORS);

    uint64_t try_cnt =  10000;
    enableInterrupt(false);
    
    struct timespec ts_start, ts_end;
    if(ts_dur){
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
    }

    start(executor);
    while(true){
        uint32_t read_val;
        __read_word_lite(REG_PMUL_END_Addr,read_val);
        if(read_val & 1 << PMUL_END_Offsets[executor]){
            break;
        }
        if(try_cnt <= 0){
            fprintf(stderr, "try too many times, exit\n");
            return ACC_TIMEOUT;
        }
    }
    if(ts_dur){
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        timespec_sub(ts_end,ts_start,*ts_dur);
    }

    clear_start(executor);
    return ACC_OK;
}


ACC_RESULT FPGA_ACC_V0::exec_interrupt(struct timespec *ts_dur,uint8_t executor) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }
    assert(executor < TOTAL_MUL_EXECULTORS);
    char event_file[128];
    snprintf(event_file,sizeof(event_file),"//dev//xdma0_events_%d",executor);
    uint32_t events = 0;
    int event_fp = 0;
    //printf("event file is %s\n",event_file);
    if ((event_fp = open(event_file, O_RDWR | O_SYNC)) == -1) { FATAL; event_fp = 0;return ACC_INVALID_FILE;}

    uint64_t try_cnt =  10000;
    enableInterrupt(true);
    struct timespec ts_start, ts_end;
    if(ts_dur){
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
    }
    start(executor);

    // 有可能是假的中断，所以要检查 PMUL_END 过滤假中断
    bool bEnd = false;
    while ( !bEnd ){
        // pending for read
        int rc = read(event_fp,&events,4);
        uint32_t read_val;
        __read_word_lite(REG_PMUL_END_Addr,read_val);
        if(rc != 4) {FATAL; close(event_fp);return ACC_INVALID_RES;}
        if(read_val & 1 << PMUL_END_Offsets[executor]){
            bEnd = true;
        }else{
            // 当中断信号到达CPU时，LITE 总线比较慢，可能还读不到，所以要循环读几次
            int loop_count = 0;
            while (loop_count < 2000){
                if( loop_count % 100 == 0)
                    printf("REG_PMUL_END_Addr[%d] = 0x%x\n",loop_count, read_val);
                if(read_val & 1 << PMUL_END_Offsets[executor]){
                    bEnd = true;
                    break;
                }
                loop_count ++;
                __read_word_lite(REG_PMUL_END_Addr,read_val);

            }

        }
    }

    if(ts_dur){
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        timespec_sub(ts_end,ts_start,*ts_dur);
    }

    //clear interrupt status in FPGA user interrupt
    clearInterrupt(executor);

    return ACC_OK;

}

bool FPGA_ACC_V0::wait_event_timeout(void) {
    int ret = 0;
    int wait_seconds = 1;
    if ( wait_seconds) {
        int event_fp = 0;
        if ((event_fp = open("//dev//xdma0_events_0", O_RDWR | O_SYNC)) == -1) { FATAL; event_fp = 0;return false;}
        
        fd_set read_fdset;
        struct  timeval timeout; 
        FD_ZERO(&read_fdset);
        FD_SET(event_fp, &read_fdset); 
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec =  0 ; 
        do {
            ret = select(event_fp + 1 , &read_fdset,  NULL ,  NULL , &timeout);  //select会阻塞直到检测到事件或者超时
            if( ret >= 0 ) {
                uint32_t events = 0;
                int rc = read(event_fp,&events,4);
                printf("Data ready, events is %d\n",events);
                close(event_fp);
                break;
            }

            printf("ret %d \n",ret);

        }while (ret <  0  && errno == EINTR);


    }
    return true;
}


ACC_RESULT FPGA_ACC_V0::write_word_mask_lite(uint32_t address, uint32_t value, uint32_t mask) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }

    __write_word_mask_lite(address,value,mask);

    return ACC_OK;
}


void FPGA_ACC_V0::__write_word_mask_lite(uint32_t address, uint32_t value, uint32_t mask) {

    volatile uint32_t *vir_addr;
    vir_addr = (uint32_t * )((char *)user_map_base + address);

    uint32_t read_val = ltohl((*vir_addr));

    uint32_t write_val = (read_val & ~mask) | (value & mask);

    *vir_addr = htoll(write_val);

}

ACC_RESULT FPGA_ACC_V0::read_word_lite(uint32_t address, uint32_t & value) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }
    __read_word_lite(address,value);
    return ACC_OK;
}

void FPGA_ACC_V0::__read_word_lite(uint32_t address, uint32_t & value) {


    volatile uint32_t *vir_addr;
    vir_addr = (uint32_t * )((char *)user_map_base + address);

    value = ltohl((*vir_addr));


}

ACC_RESULT FPGA_ACC_V0::write_word_lite(uint32_t address, uint32_t value) {
    if (!user_map_base ) {
        fprintf(stderr, "user_map_base is not init\n");
        return ACC_INVALID_RES;
    }

    __write_word_lite(address, value);
    return ACC_OK;
}

void FPGA_ACC_V0::__write_word_lite(uint32_t address, uint32_t value) {


    volatile uint32_t *vir_addr;
    vir_addr = (uint32_t * )((char *)user_map_base + address);

    *vir_addr = htoll(value);
}


int FPGA_ACC_V0::get_full_coef(Torus32 * out_buffer,uint64_t address){
    if (!c2h_fp ) {
        fprintf(stderr, "c2h_fp is not init\n");
        return 0;
    }

    char xdam_buffer[4*N];
    uint32_t size = sizeof(xdam_buffer);
    memset(xdam_buffer, 0x00, size);
    /* select AXI MM address */
    off_t off = lseek(c2h_fp, address, SEEK_SET);

    /* read data from AXI MM into buffer using SGDMA */
    int rc = read(c2h_fp, xdam_buffer, size);
    if ((rc > 0) && (rc < size)) {
      printf("Short read of %d bytes into a %d bytes buffer, could be a packet read?\n", rc, size);
      return rc;
    }

    uint32_t *p = (uint32_t *)xdam_buffer;
    for(int i = 0;i<N;i++) {
        out_buffer[i] = ltohl(p[i]);
    }

    return rc;
}

int FPGA_ACC_V0::set_full_coef(const Torus32 * in_buffer,uint64_t address){

    if (!h2c_fp ) {
        fprintf(stderr, "h2c_fp is not init\n");
        return 0;
    }
    char xdam_buffer[4*N];
    uint32_t size = sizeof(xdam_buffer);
    uint32_t *p = (uint32_t *)xdam_buffer;
    for(int i = 0;i < N ;i++) {
        p[i] = htoll(in_buffer[i]);
    }

    off_t off = lseek(h2c_fp, address, SEEK_SET);
    int wc = write(h2c_fp, xdam_buffer, size);
    if ((wc > 0) && (wc < size)) {
      printf("Short write of %d bytes into a %d bytes buffer, could be a packet write?\n", wc, size);
      return wc;
    }

    return wc;

}




ACC_RESULT FPGA_ACC_V0::programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result){
    // 要加锁，防止函数重入的时候把 torusPolynomialAddMulR 指针给改写了。 但是这样会导致无法并行执行。目前是测试阶段，只使用一个加速器，暂时不考虑这个问题。后续要把整个计算放到类中成为成员函数才能避免这个问题。
    //lock_guard<mutex> lock(torusPolynomialAddMulR_mtx);

    shared_ptr<Session> session = get_session(sessionID);
    if( session == nullptr ) {
        cerr << "Can not find session " << sessionID << endl;
        return ACC_NO_SESSION;
    }

    //auto torusPolynomialAddMulR_bak = torusPolynomialAddMulR;
    //torusPolynomialAddMulR = bind(&QEMU_ACC_V0::torusPolynomialAddMulRQemu, this,placeholders::_1,placeholders::_2,placeholders::_3);

    auto bk = session->get_bootstrap_key();

    shared_ptr<LweSample> u = shared_ptr<LweSample>(new LweSample(&bk->accum_params->extracted_lweparams));



    tfhe_programmable_bootstrap_woKS(u.get(), bk, function_table, table_size, in_sample.get());

    lweKeySwitch(result.get(), bk->ks, u.get());

    //torusPolynomialAddMulR = torusPolynomialAddMulR_bak;
    return ACC_OK;
}



}