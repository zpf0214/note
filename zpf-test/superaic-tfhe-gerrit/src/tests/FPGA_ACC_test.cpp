#include <future>
#include <thread>
#include <string>
#include <gtest/gtest.h>
#include <tfhe.h>
#include "tfhe_package.h"

#include "tfhe_superaic_server.h"

using namespace std;
using namespace tfhe_superaic;
namespace {
    const int32_t plaintext_modulus = 8;

    const LweParams* lweparams500 = new_LweParams(500, plaintext_modulus,0.1,0.3);
    const LweParams* lweparams120 = new_LweParams(120, plaintext_modulus,0.1,0.3);
    const set<const LweParams*> allparams = { lweparams120, lweparams500 };

    const TLweParams* tlweparams1024_1 = new_TLweParams(1024,1, plaintext_modulus,0.1,0.3);
    const TLweParams* tlweparams128_2 = new_TLweParams(128,2, plaintext_modulus,0.1,0.3);
    const set<const TLweParams*> allparams_tlwe = { tlweparams128_2, tlweparams1024_1 };

    const TGswParams* tgswparams1024_1 = new_TGswParams(3,15,tlweparams1024_1);
    const TGswParams* tgswparams128_2 = new_TGswParams(7,4,tlweparams128_2);
    const set<const TGswParams*> allparams_tgsw = { tgswparams1024_1, tgswparams128_2};

    const TFheGateBootstrappingParameterSet* gbp1 = new TFheGateBootstrappingParameterSet(6,2,lweparams120,tgswparams128_2);
    const set<const TFheGateBootstrappingParameterSet*> allgbp = { gbp1 };

    //generate a random lwekey
    LweKey* new_random_lwe_key(const LweParams* params) {
    const int32_t n = params->n;
    LweKey* key = new_LweKey(params);
    for (int32_t i=0; i<n; i++) key->key[i]=rand()%2;
    return key;
    }

    //generate a random tlwekey
    TLweKey* new_random_tlwe_key(const TLweParams* params) {
    const int32_t N = params->N;
    const int32_t k = params->k;
    TLweKey* key = new_TLweKey(params);
    for (int32_t i=0; i<k; i++)
        for (int32_t j=0; j<N; j++)
        key->key[i].coefs[j]=rand()%2;
    return key;
    }

    //generate a random tgswkey
    TGswKey* new_random_tgsw_key(const TGswParams* params) {
    const int32_t N = params->tlwe_params->N;
    const int32_t k = params->tlwe_params->k;
    TGswKey* key = new_TGswKey(params);
    for (int32_t i=0; i<k; i++)
        for (int32_t j=0; j<N; j++)
        key->key[i].coefs[j]=rand()%2;
    return key;
    }

    //generate a random ks
    void random_ks_key(LweKeySwitchKey* key) {
        const int32_t N = key->n;
        const int32_t t = key->t;
        const int32_t base = key->base;
    const int32_t n = key->out_params->n;
    const int32_t length = N*t*base;
    double variance = rand()/double(RAND_MAX);
        LweSample* begin = key->ks0_raw;
    LweSample* end = begin+length;
    for (LweSample* it=begin; it!=end; ++it) {
        for (int32_t j=0; j<n; j++) it->a[j]=rand();
            it->b=rand();
            it->current_variance=variance;
        }
    }

    //generate a random ks
    LweBootstrappingKey* new_random_bk_key(int32_t ks_t, int32_t ks_basebit, const LweParams* in_out_params, const TGswParams* bk_params) {
    const int32_t n = in_out_params->n;
        const int32_t kpl = bk_params->kpl;
        const int32_t k = bk_params->tlwe_params->k;
        const int32_t N = bk_params->tlwe_params->N;
        LweBootstrappingKey* bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_out_params, bk_params);
        random_ks_key(bk->ks);
        double variance = rand()/double(RAND_MAX);
        for (int32_t i=0; i<n; i++) {
            for (int32_t p=0; p<kpl; p++) {
                TLweSample& sample = bk->bk[i].all_sample[p];
                for (int32_t j=0; j<=k; j++) {
                    for (int32_t x=0; x<N; x++)
                        sample.a[j].coefsT[x]=rand();
                }
                sample.current_variance=variance;
            }
        }
        return bk;
    }

    class ACCTest : public ::testing::Test {
public:        
        /** multiplication via direct FFT (it must know the implem of LagrangeHalfCPolynomial because of the tmp+1 notation */
        void torusPolynomialMultFFT(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2) {
        const int32_t N = poly1->N;
        LagrangeHalfCPolynomial* tmp = new_LagrangeHalfCPolynomial_array(3,N);
        struct timespec ts_start, ts_end, ts_dur;
        IntPolynomial_ifft(tmp+0,poly1);
        struct timespec ts_start_total, ts_end_total, ts_dur_total;
        clock_gettime(CLOCK_MONOTONIC, &ts_start_total);

        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        TorusPolynomial_ifft(tmp+1,poly2);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
        printf("\tTorusPolynomial_ifft done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        LagrangeHalfCPolynomialMul(tmp+2,tmp+0,tmp+1);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
        printf("\tLagrangeHalfCPolynomialMul done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        TorusPolynomial_fft(result, tmp+2);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
        printf("\tTorusPolynomial_fft done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);


        clock_gettime(CLOCK_MONOTONIC, &ts_end_total);
        TFHE_ACC::timespec_sub(ts_end_total,ts_start_total,ts_dur_total);
        printf("torusPolynomialMultFFT done! dur: %ld.%09ld\n\n",ts_dur_total.tv_sec,ts_dur_total.tv_nsec);

        delete_LagrangeHalfCPolynomial_array(3,tmp);
    };

    void torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
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


    };



    Torus32 read_v_non_interrupt[1024] ;

#if 1
    TEST_F (ACCTest, tSessionID) {
        LweKey* lwe_key1 = new_random_lwe_key(lweparams500);
        LweKey* lwe_key2 = new_random_lwe_key(lweparams500);

        Session_ID_t ID1 = Session::cal_session_ID(lwe_key1);
        Session_ID_t ID2 = Session::cal_session_ID(lwe_key2);
        Session_ID_t ID1_1 = Session::cal_session_ID(lwe_key1);

        ASSERT_EQ(ID1,ID1_1);
        ASSERT_NE(ID1,ID2);

        delete_LweKey(lwe_key1);
        delete_LweKey(lwe_key2);
    }


    TEST_F (ACCTest, tSwap) {
        TFHE_ACC ACC(2);
        LweBootstrappingKey* bk1 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss;
        export_lweBootstrappingKey_toStream(oss, bk1);
        string result = oss.str();
        istringstream iss(result);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap1 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));
        shared_ptr<Session> session1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap1));


        LweBootstrappingKey* bk2 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss2;
        export_lweBootstrappingKey_toStream(oss2, bk2);
        string result2 = oss.str();
        istringstream iss2(result2);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap2 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss2));
        shared_ptr<Session> session2 = shared_ptr<Session>(new Session(string("2"),bootstrap_key_wrap2));


        LweBootstrappingKey* bk3 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss3;
        export_lweBootstrappingKey_toStream(oss3, bk3);
        string result3 = oss3.str();
        istringstream iss3(result3);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap3 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss3));
        shared_ptr<Session> session3 = shared_ptr<Session>(new Session(string("3"),bootstrap_key_wrap3));


        auto tmp_time1 = session1->get_last_use_time();
        std::chrono::duration<double> seconds1 = tmp_time1.time_since_epoch();
        cout  << "session 1:" << seconds1.count() << endl;

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - tmp_time1);
        cout  << "session 1:" << duration.count() << endl;

        auto tmp_time2 = session2->get_last_use_time();

        auto tmp_time3 = session3->get_last_use_time();

        ASSERT_LE(tmp_time1,tmp_time2); // 后创建的时间大
        ASSERT_LE(tmp_time2,tmp_time3); // 后创建的时间大

        ACC.add_session(session1);
        ACC.add_session(session2);

        ASSERT_EQ(ACC.get_session(string("1")),session1);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),nullptr);

        ACC.add_session(session3);     // 把最老的换出去
        ASSERT_EQ(ACC.get_session(string("1")),nullptr);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),session3);

        session2->update_last_use_time(); // 现在2更新,要把3换出去
        ACC.add_session(session1);
        ASSERT_EQ(ACC.get_session(string("1")),session1);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),nullptr);

        delete_LweBootstrappingKey(bk1);
        delete_LweBootstrappingKey(bk2);
        delete_LweBootstrappingKey(bk3);

    }


    int32_t f(int32_t n, int32_t plaintext_modulus){
        return n*n % plaintext_modulus;
    }

    LweSample *real_new_LweSample(const LweParams *params) {
        return new_LweSample(params);
    }
#if 1
    TEST_F(ACCTest, FPGA_ACC_bootstrappingTest) {
        bool _use_fix_random_bak = _use_fix_random;
        const int32_t N = 1024;
        const int32_t plaintext_modulus = 8;
        const int32_t k = 1;
        const int32_t n = 10;
        const int32_t l_bk = 3; //ell
        const int32_t Bgbit_bk = 10;
        const int32_t ks_t = 15;
        const int32_t ks_basebit = 1;
        const double alpha_in = 5e-4;
        const double alpha_bk = 9e-9;
        const double alpha = 1e-10;

        LweParams *in_params = new_LweParams(n, plaintext_modulus, alpha_in, 1. / 16.);
        TLweParams *accum_params = new_TLweParams(N, k, plaintext_modulus, alpha_bk, 1. / 16.);
        TGswParams *bk_params = new_TGswParams(l_bk, Bgbit_bk, accum_params);
        const LweParams *extract_params = &accum_params->extracted_lweparams;

        int32_t truth_table[plaintext_modulus];
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);

        // 生成真值表
        for(int32_t i=0;i<plaintext_modulus;i++){
            truth_table[i] = f(i, plaintext_modulus);
        }

        int32_t messageInM = 3; // 消息空间8，范围[-4，4)
        shared_ptr<LweSample> insample = shared_ptr<LweSample>(new LweSample(extract_params));
        // 1 把明文转换为 Torus32
        // message space -> plaintext space
        Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus);
        // 2 对Toruse32 上的message进行加密
        // 误差要选在正确范围内
        lweSymEncrypt(insample.get(), message, alpha, key);

        shared_ptr<LweSample> result = shared_ptr<LweSample>(new LweSample(in_params));

        {
            FPGA_ACC_V0 ACC;
            ACC.init();

            // 借用原有的bootstrap key生成session
            ostringstream oss;
            export_lweBootstrappingKey_toStream(oss, bk);
            string oss_str = oss.str();
            istringstream iss(oss_str);
            std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap1 = std::shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));
            shared_ptr<Session> session1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap1));

            // 把session放到ACC里面去
            ACC.add_session(session1);

            //Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result
            ACC.programmable_bootstrap(string("1"),insample,truth_table,plaintext_modulus,result);

        }
        //tfhe_programmable_bootstrap(result.get(),bk,truth_table,plaintext_modulus,insample.get());



        //Todo : check resulte
        // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上
        Torus32 decrypt = lweSymDecrypt(result.get(), key, plaintext_modulus);
        // 4 得到浮点数明文值,应该和 messageInM/M相同
        double ddecrypt = t32tod(decrypt);

        // 得到 plaintext_modulus 上的明文
        int32_t m_decrypt = modSwitchFromTorus32(decrypt,plaintext_modulus);

        ASSERT_EQ(f(messageInM, plaintext_modulus), m_decrypt);


        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        delete_TGswParams(bk_params);
        delete_TLweParams(accum_params);
        delete_LweParams(in_params);
        _use_fix_random = _use_fix_random_bak;
    }

    TEST_F(ACCTest, get_FPGA_version) {
        uint32_t version = 0;
        if(!get_FPGA_version(TFHE_ACC::XDMA_USER_DEV,version)){
            GTEST_FAIL();
        }

        printf("FPGA Version is 0x%x\n",version);
        ASSERT_TRUE(FPGA_ACC_V0::match(version));
    }
#endif

    TEST_F(ACCTest, FPGA_init) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        uint32_t read_back;

        acc.write_word_lite(0x00, 0xffffffff);
        acc.write_word_mask_lite(0x00,0x000000007, 0xfffffff7);
        acc.read_word_lite(0x00,read_back);
        ASSERT_EQ(read_back,0x0000000f);

        acc.write_word_lite(0x00, 0xffffffff);
        acc.write_word_mask_lite(0x00,0x000000007, 0x00000008);
        acc.read_word_lite(0x00,read_back);
        ASSERT_EQ(read_back,0xfffffff7);


        acc.write_word_lite(0x00, 0x00000000);
        acc.write_word_mask_lite(0x00,0x000000008, 0x00000008);
        acc.read_word_lite(0x00,read_back);
        ASSERT_EQ(read_back,0x000000008);


        acc.deinit();
    }


    TEST_F(ACCTest, FPGA_full_read_write_coef) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        Torus32 a[1024] ;
        Torus32 b[1024] ;


        for( int i=0;i<1024;i++){
            a[i] = 2000 + i;
        }

        acc.set_full_coef(a,FPGA_ACC_V0::FULL_P_START_ADDR_0);
        acc.get_full_coef(b,FPGA_ACC_V0::FULL_P_START_ADDR_0);
        for (int i = 0;i < 1024;i++) {
            ASSERT_EQ(a[i],b[i]);
        }
    }


    TEST_F(ACCTest, FPGA_exec_none_interrupt) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        IntPolynomial p(1024);
        TorusPolynomial q(1024);
        TorusPolynomial v2(1024);
        TorusPolynomial v2_fft(1024);

        for( int i=0;i<1024;i++){
            p.coefs[i] = i;
            q.coefsT[i] = 2000 + i;
        }

        acc.set_full_coef(p.coefs,FPGA_ACC_V0::FULL_P_START_ADDR_0);
        acc.set_full_coef(q.coefsT,FPGA_ACC_V0::FULL_Q_START_ADDR_0);
        struct timespec ts_start, ts_end, ts_dur;
        ASSERT_EQ(acc.exec_non_interrupt(&ts_dur),ACC_OK);
        printf("exec_non_interrupt done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);
        acc.get_full_coef(read_v_non_interrupt,FPGA_ACC_V0::FULL_V_START_ADDR_0);

        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        torusPolynomialMultNaive(&v2,&p,&q);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
        printf("torusPolynomialMultNaive_aux done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);
        for (int i = 0;i < 1024;i++) {
            ASSERT_EQ(v2.coefsT[i],read_v_non_interrupt[i]);
        }

        torusPolynomialMultFFT(&v2_fft,&p,&q);
        // FFT计算有误差，在误差范围内就可以
        for (int i = 0;i < 1024;i++) {
            ASSERT_LE(abs(v2_fft.coefsT[i] - read_v_non_interrupt[i]) ,1000);
        }
    }


    TEST_F(ACCTest, FPGA_exec_interrupt) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        Torus32 p[1024] ;
        Torus32 q[1024] ;

        Torus32 read_v[1024] ;
        Torus32 v2[1024] ;

        for( int i=0;i<1024;i++){
            p[i] = 100 + i;
            q[i] = 2000 + i;
        }
        struct timespec ts_start, ts_end, ts_dur;

        acc.clearInterrupt();
        acc.set_full_coef(p,FPGA_ACC_V0::FULL_P_START_ADDR_0);
//        printf("wait_event_timeout return %d\n",acc.wait_event_timeout());
//        return;
        acc.set_full_coef(q,FPGA_ACC_V0::FULL_Q_START_ADDR_0);
        ASSERT_EQ(acc.exec_interrupt(&ts_dur),ACC_OK);
        printf("exec_interrupt done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);

        acc.get_full_coef(read_v,FPGA_ACC_V0::FULL_V_START_ADDR_0);

        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        torusPolynomialMultNaive_aux(v2,p,q,1024);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
        printf("torusPolynomialMultNaive_aux done! dur: %ld.%09ld\n",ts_dur.tv_sec,ts_dur.tv_nsec);
        for (int i = 0;i < 1024;i++) {
            //printf("i=%d\n",i);
            if(v2[i] != read_v[i]) {
                printf("v2[%d]: %d, read_v[%d]:%d, read_v_non_interrupt[%d]:%d\n", i,v2[i],i,read_v[i],i,read_v_non_interrupt[i]);
            }
            ASSERT_EQ(v2[i],read_v[i]);
        }
    }


    TEST_F(ACCTest, FPGA_get_exector_number) {
        printf("CPU exector number is %ld\n", TFHE_ACC::get_CPU_exector_num());
    }

    int add_and_sleep(Executor_List &list,int a, int b, int * result) {
        auto exec = list.aquire_executor();
        printf("Get exec:%s for %d + %d\n",exec->name.c_str(),a,b);
        *result = a + b;
        sleep(1);
        printf("Release exec:%s for %d + %d\n",exec->name.c_str(),a,b);
        list.release_executor(exec);
        return 0;
    }


    
    TEST_F (ACCTest, exector_async) {
        Executor_List list;
        list.append_exector(shared_ptr<Executor>(new Executor(Executor::Executor_CPU,string("cpu1"))));
        list.append_exector(shared_ptr<Executor>(new Executor(Executor::Executor_CPU,string("cpu2"))));
        list.append_exector(shared_ptr<Executor>(new Executor(Executor::Executor_CPU,string("cpu3"))));

        int a[10],b[10],result[10];
        std::future<int> rt[10];
        for(int i=0;i<10;i++){
            a[i] = i;
            b[i] = 100+i;
            result[i] = 0;
        }

        for(int i=0;i<10;i++){
            rt[i] = async(launch::async,add_and_sleep,std::ref(list),a[i],b[i],&result[i]);
        }

        for(int i= 0;i<10;i++) {
            rt[i].get();
            ASSERT_EQ(result[i],a[i]+b[i]);
        }
    }

#endif
#if 1

    TEST_F(ACCTest, FPGA_exec_poll_executors) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        Torus32 p[1024] ;
        Torus32 q[1024] ;

        Torus32 read_v[1024] ;
        Torus32 v2[1024] ;

        for (int exec = 0 ; exec < acc.TOTAL_MUL_EXECULTORS ; exec++) 
        //int exec = 7;
        {
            for( int i=0;i<1024;i++){
                p[i] = 100 + i + exec *13;
                q[i] = 2000 + i;
            }
            struct timespec ts_start, ts_end, ts_dur;

            printf("exec = %d\n",exec);
            ASSERT_EQ(acc.torusPolynomialMultFPGA(read_v,p,q,exec,true),ACC_OK);

            clock_gettime(CLOCK_MONOTONIC, &ts_start);
            torusPolynomialMultNaive_aux(v2,p,q,1024);
            clock_gettime(CLOCK_MONOTONIC, &ts_end);
            TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
            for (int i = 0;i < 1024;i++) {
                //printf("i=%d\n",i);
                if((v2[i] != read_v[i] && i == 0) || i == 0) {
                    printf("v2[%d]: 0x08%x, read_v[%d]:0x08%x \n", i,v2[i],i,read_v[i]);
                }
                ASSERT_EQ(v2[i],read_v[i]);
            }
        }
    }
#endif

#if 1
    TEST_F(ACCTest, FPGA_exec_interrupt_executors) {
        FPGA_ACC_V0 acc;
        ASSERT_EQ(acc.init(),ACC_OK);

        Torus32 p[1024] ;
        Torus32 q[1024] ;

        Torus32 read_v[1024] ;
        Torus32 v2[1024] ;

        for (int exec = 0 ; exec < acc.TOTAL_MUL_EXECULTORS ; exec++) 
        //int exec = 1;
        {
            for( int i=0;i<1024;i++){
                p[i] = 100 + i + exec *19;
                q[i] = 2000 + i;
            }
            struct timespec ts_start, ts_end, ts_dur;

            printf("exec = %d\n",exec);
            ASSERT_EQ(acc.torusPolynomialMultFPGA(read_v,p,q,exec,false),ACC_OK);

            clock_gettime(CLOCK_MONOTONIC, &ts_start);
            torusPolynomialMultNaive_aux(v2,p,q,1024);
            clock_gettime(CLOCK_MONOTONIC, &ts_end);
            TFHE_ACC::timespec_sub(ts_end,ts_start,ts_dur);
            for (int i = 0;i < 1024;i++) {
                //printf("i=%d\n",i);
                if(v2[i] != read_v[i]) {
                    printf("v2[%d]: %d, read_v[%d]:%d, read_v_non_interrupt[%d]:%d\n", i,v2[i],i,read_v[i],i,read_v_non_interrupt[i]);
                }
                ASSERT_EQ(v2[i],read_v[i]);
            }
        }
    }

#endif

}
