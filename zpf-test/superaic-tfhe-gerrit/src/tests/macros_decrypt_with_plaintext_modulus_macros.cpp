/* 
 *  通过宏定义测试所有的值，确保以后不会再出现有些值无法正确处理的情况
 */
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include "tfhe.h"
#include "fakes/tlwe.h"
#include "fakes/tgsw.h"
#include "fakes/lwe-keyswitch.h"
#include "fakes/lwe-bootstrapping.h"
#include "classic_PBS_parameters.h"


#include "test_internal.h"

#define TFHE_TEST_ENVIRONMENT 1
#define MAX_NOISE 10000.0
#define NUM_BLOCK 16
using namespace std;

using namespace ::testing;

namespace {

    int32_t f(int32_t n, int32_t plaintext_modulus){
        return n;
        //return n*n % plaintext_modulus;
    }

    const ClassicPBSParameters M8_PARAM = TEST_PARAM_MESSAGE_8_KS_PBS_GAUSSAIN;

    const int32_t N = M8_PARAM.tlwe_polynomials_numbers;
//    const int32_t plaintext_modulus = 8;
    const int32_t plaintext_modulus = M8_PARAM.plaintext_modulus;
    const int32_t message_modulus = M8_PARAM.message_modulus;
    const int32_t k = M8_PARAM.tlwe_dimension;
    const int32_t n = M8_PARAM.lwe_dimension;
    const int32_t l_bk = M8_PARAM.tgsw_decompose_length; //ell
    const int32_t Bgbit_bk = M8_PARAM.tgsw_Bgbit;
    const int32_t ks_t = M8_PARAM.pbs_decompose_length;
    const int32_t ks_basebit = M8_PARAM.pbs_Bgbit;
    const double alpha_in = M8_PARAM.lwe_alpha_min;
    const double alpha_bk = M8_PARAM.tlwe_alpha_min;
    const double alpha = alpha_in;

    const LweParams *in_params = new_LweParams(n, plaintext_modulus, alpha_in, 1. / MAX_NOISE);
    const TLweParams *accum_params = new_TLweParams(N, k, plaintext_modulus, alpha_bk, 1. / MAX_NOISE);
    const TGswParams *bk_params = new_TGswParams(l_bk, Bgbit_bk, accum_params);
    const LweParams *extract_params = &accum_params->extracted_lweparams;


    LweSample *real_new_LweSample(const LweParams *params) {
        return new_LweSample(params);
    }

    void real_delete_LweSample(LweSample *sample) {
        delete_LweSample(sample);
    }




    vector<bool> random_binary_key(const int32_t n) {
        vector<bool> rand_vect(n);
        for (int32_t i = 0; i < n; ++i) {
            rand_vect[i] = rand() % 2;
        }
        return rand_vect;
    }


    //zpf generate_truth_table
    void generate_truth_table(int32_t* truth_table, int32_t plaintext_modulus){
        for(int32_t i=0;i<plaintext_modulus;i++){
            if (i<plaintext_modulus/2){
                truth_table[i] = f(i, plaintext_modulus);
            }else{
                int32_t a = i;
                a -= plaintext_modulus;
                truth_table[i] = f(a, plaintext_modulus);
            }
        }

    }



    class TfheCreateBootstrapKeyTest : public ::testing::Test {
    public:


        LweKeySwitchKey *captured_result;
        vector<int32_t> captured_in_key_copy;
        const LweKey *captured_out_key;

#define INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

#include "../libtfhe-superaic-core/lwe-bootstrapping-functions.cpp"

#undef INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

    };

    /* --------
     *  宏只是一个简单的替换动作，这里我们用宏生成对应的测试函数
     *  同时我们也要注意宏可能存在的误用
     */


#if 0 //zpf pbs test


    #define GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(value) \
    TEST_F(TfheCreateBootstrapKeyTest, PLAINTEXT_MODULUS_VALUES_##value){\
        bool _use_fix_random_bak = _use_fix_random;      \
        LweKey *key = new_LweKey(in_params);             \
        lweKeyGen(key);                                  \
        TGswKey *key_bk = new_TGswKey(bk_params);        \
        tGswKeyGen(key_bk);                              \
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params); \
        tfhe_createLweBootstrappingKey(bk, key, key_bk); \
        { \
            int32_t messageInM = value % plaintext_modulus; \
            if(messageInM >=(plaintext_modulus/2)){ \
                messageInM -= plaintext_modulus; \
            } \
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            Torus32 decrypt_message = lweSymDecrypt(insample, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus); \
            ASSERT_EQ(messageInM , m_decrypt); \
        } \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }

    //zpf 宏无法处理循环，所以没有办法处理不定长列表
    //#define GENERATE_ALL_TEST_FUNCTIONS(start, end)\
    //{ \
    //    for (int i=start; i<end; i++) { \
    //        GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(i); \
    //    } \
    //} 

    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(0)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(1)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(2)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(3)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(4)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(5)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(6)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(7)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(8) // zpf 转到负值
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(9)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(10)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(11)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(12)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(13)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(14)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(15)

#endif

#if 1

    #define GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(value) \
    TEST_F(TfheCreateBootstrapKeyTest, TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_##value) { \
        bool _use_fix_random_bak = _use_fix_random; \
        LweKey *key = new_LweKey(in_params); \
        lweKeyGen(key); \
        TGswKey *key_bk = new_TGswKey(bk_params); \
        tGswKeyGen(key_bk); \
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params); \
        tfhe_createLweBootstrappingKey(bk, key, key_bk); \
        int32_t truth_table[plaintext_modulus]; \
        generate_truth_table(truth_table, plaintext_modulus); \
        { \
            int32_t messageInM = value % plaintext_modulus; \
            if(messageInM >=(plaintext_modulus/2)){ \
                messageInM -= plaintext_modulus; \
            } \
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            LweSample *result_message = new_LweSample(in_params); \
            tfhe_programmable_bootstrap(result_message, bk, truth_table, plaintext_modulus, insample); \
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus); \
            ASSERT_EQ(f(messageInM, plaintext_modulus) , m_decrypt); \
        } \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }

    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(0)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(1)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(2)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(3)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(4)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(5)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(6)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(7)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(8)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(9)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(10)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(11)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(12)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(13)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(14)
    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(15)

#endif
#if 0

    #define GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(value) \
    TEST_F(TfheCreateBootstrapKeyTest, TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_##value) { \
        bool _use_fix_random_bak = _use_fix_random; \
        LweKey *key = new_LweKey(in_params); \
        lweKeyGen(key); \
        TGswKey *key_bk = new_TGswKey(bk_params); \
        tGswKeyGen(key_bk); \
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params); \
        tfhe_createLweBootstrappingKey(bk, key, key_bk); \
        int32_t truth_table[plaintext_modulus]; \
        generate_truth_table(truth_table, plaintext_modulus); \
        { \
            int32_t messageInM = value % plaintext_modulus; \
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            LweSample *result_message = new_LweSample(in_params); \
            tfhe_programmable_bootstrap(result_message, bk, truth_table, plaintext_modulus, insample); \
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus); \
            Torus32 decrypt_message_without_pbs = lweSymDecrypt(insample, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus); \
            int32_t m_decrypt_without_pbs = modSwitchFromTorus32(decrypt_message_without_pbs, plaintext_modulus); \
            ASSERT_EQ(m_decrypt_without_pbs, m_decrypt); \
        } \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }

    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(0)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(1)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(2)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(3)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(4)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(5)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(6)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(7)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(8)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(9)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(10)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(11)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(12)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(13)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(14)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(15)

#endif //zpf pbs test

    #define GENERATE_MODSWITCH_TEST(value) \
    TEST_F(TfheCreateBootstrapKeyTest, MODSWITCH_TEST_##value){ \
            Torus32 plain = modSwitchToTorus32(value, plaintext_modulus); \
            int32_t value_after_mod = modSwitchFromTorus32(plain, plaintext_modulus); \
            ASSERT_EQ(value, value_after_mod); \
    } 

    GENERATE_MODSWITCH_TEST(0)
    GENERATE_MODSWITCH_TEST(1)
    GENERATE_MODSWITCH_TEST(2)
    GENERATE_MODSWITCH_TEST(3)
    GENERATE_MODSWITCH_TEST(4)
    GENERATE_MODSWITCH_TEST(5)
    GENERATE_MODSWITCH_TEST(6)
    GENERATE_MODSWITCH_TEST(7)
    GENERATE_MODSWITCH_TEST(8)
    GENERATE_MODSWITCH_TEST(9)
    GENERATE_MODSWITCH_TEST(10)
    GENERATE_MODSWITCH_TEST(11)
    GENERATE_MODSWITCH_TEST(12)
    GENERATE_MODSWITCH_TEST(13)
    GENERATE_MODSWITCH_TEST(14)
    GENERATE_MODSWITCH_TEST(15)




    #define GENERATE_MODSWITCH_TEST_MINUS_VALUE(value) \
    TEST_F(TfheCreateBootstrapKeyTest, MODSWITCH_TEST_MINUS##value){ \
            Torus32 plain = modSwitchToTorus32(-value, plaintext_modulus); \
            int32_t value_after_mod = modSwitchFromTorus32(plain, plaintext_modulus); \
            ASSERT_EQ(-value, value_after_mod); \
    } 

#if 0
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(1)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(2)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(3)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(4)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(5)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(6)
    GENERATE_MODSWITCH_TEST_MINUS_VALUE(7)
#endif

    /* ------------ zpf ----------------------*/
    /*
     * 以上的测试证明了modSwitchFromTorus32/modSwitchToTorus32
     * 无法正确的将负值转换为负值
     * 是否是这里导致了我们始终无法正确的求解？
     */
}
