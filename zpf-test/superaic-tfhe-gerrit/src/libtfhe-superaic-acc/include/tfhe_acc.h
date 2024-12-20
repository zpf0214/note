#ifndef _TFHE_ACC_H_
#define _TFHE_ACC_H_

#include <map>
#include <mutex>
#include <condition_variable>
#include <endian.h>
#include <atomic>

#include "tfhe_session.h"
using namespace std;
namespace tfhe_superaic {

enum ACC_RESULT{
    ACC_OK = 0,
    ACC_NO_SESSION,         
    ACC_TIMEOUT,
    ACC_INVALID_PARAM,      // 非法参数
    ACC_INVALID_RES,        // 资源没有准备好，可能的原因是：没有分配内存，没有打开文件
    ACC_INVALID_FILE,       // 无法打开文件，可能的原因是：没有加载驱动，没有检测到FPGA，没有权限。
};



/* ltoh: little to host */
/* htol: host to little */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#  define ltohl(x)       (x)
#  define ltohs(x)       (x)
#  define htoll(x)       (x)
#  define htols(x)       (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#  define ltohl(x)     __bswap_32(x)
#  define ltohs(x)     __bswap_16(x)
#  define htoll(x)     __bswap_32(x)
#  define htols(x)     __bswap_16(x)
#endif


/**
 * Executor 用于提供执行设备是否空闲的管理，管理设备的文件信息，但是不提供具体的执行函数
 */
class Executor {
public:
    enum Executor_Type {
        Executor_CPU,
        Executor_FPGA,
    };

    virtual ~Executor(){}
    bool isBusy();
    const Executor_Type executor_type;
    const string name;
    Executor(Executor_Type type,string name);
    friend class Executor_List;
protected:
    volatile bool busy = false;
private:
    Executor(const Executor&) = delete;
    void operator =(const Executor&) = delete;
};


class Executor_FPGA :public Executor {
public:
    Executor_FPGA(string name, uint8_t executor);
    const uint8_t channel;
private:
    Executor_FPGA(const Executor_FPGA&) = delete;
    void operator =(const Executor_FPGA&) = delete;

};

class Executor_List {
public:
    Executor_List(void);
    ~Executor_List();

    /**
     * 把Executor加入到队列中。
     * 注意，先加入的Executor的优先级较高，
     * 因此应该首先加入FPGA Executor，再加入CPU Executor
     */
    void append_exector(shared_ptr<Executor> executor);

    shared_ptr<Executor>  aquire_executor(void) __attribute__((optimize("O0")));
    void release_executor(shared_ptr<Executor> executor) __attribute__((optimize("O0")));
private:    
    vector<shared_ptr<Executor>> executors;
    mutex _executors_mtx;
    condition_variable _notFree;
    Executor_List(const Executor_List&) = delete;
    void operator =(const Executor_List&) = delete;
};

class TFHE_ACC{
public:
    enum ELIM_ALGO {//elimination algorithm
        LRU = 0,
        LFU,
    };
    enum ACC_TYPE{
    ACC_FPGA = 0,
    ACC_QEMU,
    ACC_CPU,
    ACC_UNKONW
    };

    enum CAPABILITY{
        CAPABILITY_NONE,
        CAPABILITY_FIX_POLYNOMIAL_MUL , // 固定长度多项式乘法
        CAPABILITY_PBS,                     // 全功能bootstap
    };

    /* Subtract timespec t2 from t1
    t3 = t1 - t2
    *
    * Both t1 and t2 must already be normalized
    * i.e. 0 <= nsec < 1000000000 */
    static void timespec_sub(struct timespec &t1, struct timespec &t2, struct timespec &t3);


    shared_ptr<Session> get_session(Session_ID_t sessionID);
    void remove_session(Session_ID_t sessionID);
    void add_session(shared_ptr<Session> session);

    virtual ~TFHE_ACC(){};
    TFHE_ACC(int max_sessions);

    ACC_TYPE get_type(){return type; }

    virtual float get_loading(void);
    virtual ACC_RESULT programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result);


    static constexpr const char * XDMA_CONTROL_DEV = "//dev//xdma0_control";
    static constexpr const char * XDMA_USER_DEV = "//dev//xdma0_user";
    static constexpr const char * XDMA_H2C_DEV = "//dev//xdma0_h2c_0";
    static constexpr const char * XDMA_C2H_DEV = "//dev//xdma0_c2h_0";

    static constexpr uint32_t REG_VERSION_ID_Addr = 0x00003300; // 所有的硬件版本都有这个寄存器
    static size_t get_CPU_exector_num(void);
    static std::shared_ptr<TFHE_ACC> get_valid_acc(void);

protected:
    bool paral_mul = false;
    ACC_TYPE type = ACC_UNKONW;
    CAPABILITY capability = CAPABILITY_NONE;
    TFHE_ACC(int max_sessions,ACC_TYPE type, CAPABILITY capability);
    virtual void del_session_on_device(Session_ID_t sessionID);
    float get_CPU_loading(void);
    virtual void tfhe_programmable_bootstrap_woKS(LweSample *result,
                                    const LweBootstrappingKey *bk,
                                    Torus32 * const truth_table, const int32_t truth_table_size, const LweSample *x);
    virtual void tfhe_blindRotateAndExtract(LweSample *result,
                                           const TorusPolynomial *v,
                                           const TGswSample *bk,
                                           const int32_t barb,
                                           const int32_t *bara,
                                           const int32_t n,
                                           const TGswParams *bk_params);
    virtual void tfhe_blindRotate(TLweSample *accum, const TGswSample *bk, const int32_t *bara, const int32_t n, const TGswParams *bk_params);
    virtual void tfhe_MuxRotate(TLweSample *result, const TLweSample *accum, const TGswSample *bki, const int32_t barai,
                        const TGswParams *bk_params);
    virtual void tGswExternMulToTLwe(TLweSample *accum, const TGswSample *sample, const TGswParams *params);
    virtual void tGswExternMulToTLwe_paral(TLweSample *accum, const TGswSample *sample, const TGswParams *params);
    virtual void tLweAddMulRTo(TLweSample *result, const IntPolynomial *p, const TLweSample *sample, const TLweParams *params);
    virtual void tLweAddMulRTo_paral(TLweSample *result, const IntPolynomial *p, const TLweSample *sample, const TLweParams *params);
    virtual void torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2);
    virtual void torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N);

private:

    TFHE_ACC(const TFHE_ACC&) = delete;
    void operator =(const TFHE_ACC&) = delete;

    int max_sessions;
    map<Session_ID_t,shared_ptr<Session>> sessions;
    mutex session_mtx;

    shared_ptr<Session> _get_session(Session_ID_t sessionID);
    void _remove_session(Session_ID_t sessionID);
    void _add_session(shared_ptr<Session> session);
    ELIM_ALGO elim_algo = LRU;   // 加速器上Sesstion的淘汰算法 最近最少使用算法淘汰老的
    void swap_out_LRU(void);

};

class CPU_ACC: public TFHE_ACC{
public:
    CPU_ACC();
    virtual ACC_RESULT programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result);

private:
    CPU_ACC(const TFHE_ACC&) = delete;
    void operator =(const CPU_ACC&) = delete;

};


class QEMU_ACC_V0: public TFHE_ACC{
public:
    QEMU_ACC_V0();
    virtual ACC_RESULT programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result);
    virtual void torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N);
    virtual void torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2);

private:
    QEMU_ACC_V0(const QEMU_ACC_V0&) = delete;
    void operator =(const QEMU_ACC_V0&) = delete;

};





/**
 * 获取FPGA版本信息.返回false则说明FPGA设备不存在。
 */
bool get_FPGA_version(const char * dev,uint32_t &version);


class FPGA_ACC_V0: public TFHE_ACC{
public:
    FPGA_ACC_V0();
    ~FPGA_ACC_V0();

    static bool match(uint32_t version);
    static size_t get_HW_exector_num(void);

    virtual ACC_RESULT programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result);
    virtual void torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N);
    virtual void torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2);
    void torusPolynomialAddMulR_async(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2);
    virtual void tLweAddMulRTo_paral(TLweSample *result, const IntPolynomial *p, const TLweSample *sample, const TLweParams *params);
    virtual void tGswExternMulToTLwe_paral(TLweSample *accum, const TGswSample *sample, const TGswParams *params);

    ACC_RESULT torusPolynomialMultFPGA(Torus32* __restrict result, const Torus32* __restrict poly1, const Torus32* __restrict poly2,uint8_t channel, bool poll=false);

    ACC_RESULT init(const char * config_dev = XDMA_CONTROL_DEV, const char * user_dev =  XDMA_USER_DEV, const char * h2c_dev = XDMA_H2C_DEV, const char * c2h_dev = XDMA_C2H_DEV);
    void deinit(void);
    ACC_RESULT start(uint8_t channel = 0);
    ACC_RESULT clear_start(uint8_t channel = 0);
    ACC_RESULT enableInterrupt(bool en);
    ACC_RESULT clearInterrupt(uint8_t channel = 0);
    ACC_RESULT exec_non_interrupt(struct timespec *ts_dur = nullptr,uint8_t channel = 0);
    ACC_RESULT exec_interrupt(struct timespec *ts_dur = nullptr,uint8_t channel = 0);
    static const uint32_t N = 1024;

    shared_ptr<Executor>  aquire_executor(void);
    void release_executor(shared_ptr<Executor> exec);
    bool wait_event_timeout(void);

    int get_full_coef(Torus32 * out_buffer,uint64_t address );
    int set_full_coef(const Torus32 * in_buffer,uint64_t address );

    ACC_RESULT write_word_mask_lite(uint32_t address, uint32_t value, uint32_t mask);
    ACC_RESULT read_word_lite(uint32_t address, uint32_t & value);
    ACC_RESULT write_word_lite(uint32_t address, uint32_t value);


    // 下面的寄存器都定义在lite通道上，使用 xdma0_user 设备
    // V = P * Q
    static const uint32_t REG_PMUL_ST_Addr = 0x00003000;              // 0->1时启动计算  
    static const uint8_t    Polynomial_Mul_Start_Offset_0 = 0;        
    static const uint8_t    Polynomial_Mul_Start_Offset_1 = 1;        
    static const uint8_t    Polynomial_Mul_Start_Offset_2 = 2;        
    static const uint8_t    Polynomial_Mul_Start_Offset_3 = 3;        
    static const uint8_t    Polynomial_Mul_Start_Offset_4 = 4;        
    static const uint8_t    Polynomial_Mul_Start_Offset_5 = 5;        
    static const uint8_t    Polynomial_Mul_Start_Offset_6 = 6;        
    static const uint8_t    Polynomial_Mul_Start_Offset_7 = 7;        

    static const uint32_t REG_XDMA_Interrupt_Clear_Addr = 0x00003000; // 清除中断状态寄存器
    static const uint8_t    XDMA_Interrupt_Clear_Offset_0 = 8;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_1 = 9;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_2 = 10;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_3 = 11;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_4 = 12;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_5 = 13;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_6 = 14;
    static const uint8_t    XDMA_Interrupt_Clear_Offset_7 = 15;

    static const uint32_t REG_XDMA_Interrupt_Enable_Addr = 0x00003000; // 使能中断寄存器
    static const uint8_t    XDMA_Interrupt_Enable_Offset = 16;

    static const uint32_t REG_PMUL_END_Addr = 0x00003300;     // 为1时表示运算结束。当Polynomial_Mul_Start设置为1时会清0
    static const uint8_t    PMUL_END_Offset_0 = 8;            
    static const uint8_t    PMUL_END_Offset_1 = 9;            
    static const uint8_t    PMUL_END_Offset_2 = 10;           
    static const uint8_t    PMUL_END_Offset_3 = 11;           
    static const uint8_t    PMUL_END_Offset_4 = 12;           
    static const uint8_t    PMUL_END_Offset_5 = 13;           
    static const uint8_t    PMUL_END_Offset_6 = 14;           
    static const uint8_t    PMUL_END_Offset_7 = 15;           


    // 下面的寄存器都定义在full通道上，使用 xdma0_h2c_0 /  xdma0_c2h_0 设备
    // P * Q = V
    static const uint64_t FULL_P_START_ADDR_0 = 0x00000000;
    static const uint64_t FULL_Q_START_ADDR_0 = 0x00001000;
    static const uint64_t FULL_V_START_ADDR_0 = 0x00002000;

    static const uint64_t FULL_P_START_ADDR_1 = 0x00010000;
    static const uint64_t FULL_Q_START_ADDR_1 = 0x00011000;
    static const uint64_t FULL_V_START_ADDR_1 = 0x00012000;

    static const uint64_t FULL_P_START_ADDR_2 = 0x00020000;
    static const uint64_t FULL_Q_START_ADDR_2 = 0x00021000;
    static const uint64_t FULL_V_START_ADDR_2 = 0x00022000;

    static const uint64_t FULL_P_START_ADDR_3 = 0x00030000;
    static const uint64_t FULL_Q_START_ADDR_3 = 0x00031000;
    static const uint64_t FULL_V_START_ADDR_3 = 0x00032000;

    static const uint64_t FULL_P_START_ADDR_4 = 0x00040000;
    static const uint64_t FULL_Q_START_ADDR_4 = 0x00041000;
    static const uint64_t FULL_V_START_ADDR_4 = 0x00042000;

    static const uint64_t FULL_P_START_ADDR_5 = 0x00050000;
    static const uint64_t FULL_Q_START_ADDR_5 = 0x00051000;
    static const uint64_t FULL_V_START_ADDR_5 = 0x00052000;

    static const uint64_t FULL_P_START_ADDR_6 = 0x00060000;
    static const uint64_t FULL_Q_START_ADDR_6 = 0x00061000;
    static const uint64_t FULL_V_START_ADDR_6 = 0x00062000;

    static const uint64_t FULL_P_START_ADDR_7 = 0x00070000;
    static const uint64_t FULL_Q_START_ADDR_7 = 0x00071000;
    static const uint64_t FULL_V_START_ADDR_7 = 0x00072000;


    static const uint8_t TOTAL_MUL_CHANNELS = 8;
    static constexpr const uint8_t Polynomial_Mul_Start_Offsets[] = {
        Polynomial_Mul_Start_Offset_0,
        Polynomial_Mul_Start_Offset_1,
        Polynomial_Mul_Start_Offset_2,
        Polynomial_Mul_Start_Offset_3,
        Polynomial_Mul_Start_Offset_4,
        Polynomial_Mul_Start_Offset_5,
        Polynomial_Mul_Start_Offset_6,
        Polynomial_Mul_Start_Offset_7
    };

    static constexpr const uint8_t XDMA_Interrupt_Clear_Offsets[] = {
        XDMA_Interrupt_Clear_Offset_0,
        XDMA_Interrupt_Clear_Offset_1,
        XDMA_Interrupt_Clear_Offset_2,
        XDMA_Interrupt_Clear_Offset_3,
        XDMA_Interrupt_Clear_Offset_4,
        XDMA_Interrupt_Clear_Offset_5,
        XDMA_Interrupt_Clear_Offset_6,
        XDMA_Interrupt_Clear_Offset_7
    };

    static constexpr const uint8_t PMUL_END_Offsets[] = {
        PMUL_END_Offset_0,
        PMUL_END_Offset_1,
        PMUL_END_Offset_2,
        PMUL_END_Offset_3,
        PMUL_END_Offset_4,
        PMUL_END_Offset_5,
        PMUL_END_Offset_6,
        PMUL_END_Offset_7
    };

    static constexpr const uint64_t FULL_P_START_ADDRs[] = {
        FULL_P_START_ADDR_0,
        FULL_P_START_ADDR_1,
        FULL_P_START_ADDR_2,
        FULL_P_START_ADDR_3,
        FULL_P_START_ADDR_4,
        FULL_P_START_ADDR_5,
        FULL_P_START_ADDR_6,
        FULL_P_START_ADDR_7
    };

    static constexpr const uint64_t FULL_V_START_ADDRs[] = {
        FULL_V_START_ADDR_0,
        FULL_V_START_ADDR_1,
        FULL_V_START_ADDR_2,
        FULL_V_START_ADDR_3,
        FULL_V_START_ADDR_4,
        FULL_V_START_ADDR_5,
        FULL_V_START_ADDR_6,
        FULL_V_START_ADDR_7
    };


    static constexpr const uint64_t FULL_Q_START_ADDRs[] = {
        FULL_Q_START_ADDR_0,
        FULL_Q_START_ADDR_1,
        FULL_Q_START_ADDR_2,
        FULL_Q_START_ADDR_3,
        FULL_Q_START_ADDR_4,
        FULL_Q_START_ADDR_5,
        FULL_Q_START_ADDR_6,
        FULL_Q_START_ADDR_7
    };


protected:
    inline void __write_word_mask_lite_list(uint32_t *address, uint32_t *value, uint32_t *mask,size_t count);
    inline void __write_word_mask_lite(uint32_t address, uint32_t value, uint32_t mask);
    inline void __read_word_lite(uint32_t address, volatile uint32_t & value);
    inline void __write_word_lite(uint32_t address, uint32_t value);
    inline uint32_t __save_read_word_lite(uint32_t address);
    inline void __save_write_word_lite(uint32_t address, uint32_t value);



private:
    FPGA_ACC_V0(const FPGA_ACC_V0&) = delete;
    void operator =(const FPGA_ACC_V0&) = delete;
    int control_fp = 0;
    void * control_map_base = nullptr;

    int user_fp = 0;
    void * user_map_base = nullptr;
    atomic_flag user_automic_flag;  // 访问寄存器的时间短，而且没有休眠，所以用atomic_flag，效率比较高

    int h2c_fp = 0;
    int c2h_fp = 0;
    mutex full_dma_mtx;

    Executor_List executors;

};



}

#endif
