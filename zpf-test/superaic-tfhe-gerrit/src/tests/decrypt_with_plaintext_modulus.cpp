#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include "tfhe.h"
#include "fakes/tlwe.h"
#include "fakes/tgsw.h"
#include "fakes/lwe-keyswitch.h"
#include "fakes/lwe-bootstrapping.h"
#include "classic_PBS_parameters.h"

#define TFHE_TEST_ENVIRONMENT 1
#define MAX_NOISE 10000.0
#define NUM_BLOCK 16
using namespace std;

using namespace ::testing;

namespace {

    const ClassicPBSParameters M8_PARAM = TEST_PARAM_MESSAGE_8_KS_PBS_GAUSSAIN;

    const int32_t N = M8_PARAM.tlwe_polynomials_numbers;
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




    //int32_t f(int32_t n, int32_t plaintext_modulus){
    //    return n / message_modulus;
    //    //return n*n % plaintext_modulus;
    //}



    class TfheCreateBootstrapKeyTest : public ::testing::Test {
    public:


        LweKeySwitchKey *captured_result;
        vector<int32_t> captured_in_key_copy;
        const LweKey *captured_out_key;

#define INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

#include "../libtfhe-superaic-core/lwe-bootstrapping-functions.cpp"

#undef INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

    };

    //EXPORT void tfhe_createLweBootstrappingKey(
    //	LweBootstrappingKey* bk, 
    //	const LweKey* key_in, 
    //	const TGswKey* rgsw_key) 
    TEST_F(TfheCreateBootstrapKeyTest, PLAINTEXT_MODULUS_VALUES_WITH_PBS) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);

        for(int i=0; i<plaintext_modulus; i++)
        { //zpf test all value in plaintext_modulus = 16

            int32_t messageInM = 1; 
            //zpf plaintext_modulus = 16
            LweSample *insample = real_new_LweSample(extract_params);
            // 1 把明文转换为 Torus32
            // message space -> plaintext space
            Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus);
            // 2 对Toruse32 上的message进行加密
            // 误差要选在正确范围内
            lweSymEncrypt(insample, message, alpha, key);


            /* ----------- pbs --------------*/
            // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上

            LweSample *result_message = new_LweSample(in_params);
            // zpf 这个似乎无法正确解密
            // 是noise的问题吗？我们需要单独测试bootstrap过程是否正确
            apply_lookup_table(result_message, bk, insample, plaintext_modulus, 
                    [](int32_t x){return x ;});
            // 得到 plaintext_modulus 上的明文
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus);
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);

            ASSERT_EQ(messageInM , m_decrypt);

        } //zpf test all value in plaintext_modulus = 16




        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }

    TEST_F(TfheCreateBootstrapKeyTest, PLAINTEXT_MODULUS_VALUES) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);

        for(int i=0; i<plaintext_modulus; i++)
        { //zpf test all value in plaintext_modulus = 16

            int32_t messageInM = i; 
            //zpf plaintext_modulus = 16
            LweSample *insample = real_new_LweSample(extract_params);
            // 1 把明文转换为 Torus32
            // message space -> plaintext space
            Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus);
            // 2 对Toruse32 上的message进行加密
            // 误差要选在正确范围内
            lweSymEncrypt(insample, message, alpha, key);


            //Todo : check resulte
            // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上
            Torus32 decrypt_message = lweSymDecrypt(insample, key, plaintext_modulus);

            // 得到 plaintext_modulus 上的明文
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);

            ASSERT_EQ(messageInM , m_decrypt);

        } //zpf test all value in plaintext_modulus = 16




        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
}
