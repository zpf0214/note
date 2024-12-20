#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include "tfhe.h"

#include "test_internal.h"

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
    TEST_F(TfheCreateBootstrapKeyTest, bootstrappingTest) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);

        // 生成真值表
        //for(int32_t i=0;i<plaintext_modulus;i++){
        //    truth_table[i] = f(i, plaintext_modulus);
        //}

        int32_t messageInM = 6; 
        //zpf plaintext_modulus = 16
        //值1-8都可以正确的求解出来，但是9解出来的值是15，这是什么原因？
        //9 --> 15
        //10 --> 14
        //11 --> 13
        //12 --> 12
        //13 --> 11
        //14 --> 10
        //15 --> 9
        //这应该与int 还是 uint相关，即我们实际的取值范围应当是[-8，7]
        //去负值仍然有同样的问题
        //这里需要好好看看究竟是什么的问题？
        //是bootstrap还是test polynomial的问题，还是符号相关的问题
        //最有可能是符号问题
        LweSample *insample = real_new_LweSample(extract_params);
        // 1 把明文转换为 Torus32
        // message space -> plaintext space
        Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus);
        // 2 对Toruse32 上的message进行加密
        // 误差要选在正确范围内
        //lweSymEncrypt(insample, message, 0.0, key);
        lweSymEncrypt(insample, message, alpha, key);

        LweSample *result_message = new_LweSample(in_params);
        LweSample *result_carry = new_LweSample(in_params);
        // 会crash
        //tfhe_programmable_bootstrap(result,bk,truth_table,plaintext_modulus,insample);

        apply_lookup_table(result_message, bk, insample, plaintext_modulus, 
                [](int32_t x){return x % message_modulus;});

        apply_lookup_table(result_carry, bk, insample, plaintext_modulus, 
                [](int32_t x){return x / message_modulus;});

        //Todo : check resulte
        // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上
        Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus);
        Torus32 decrypt_carry = lweSymDecrypt(result_carry, key, plaintext_modulus);

        // 得到 plaintext_modulus 上的明文
        int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);
        int32_t c_decrypt = modSwitchFromTorus32(decrypt_carry,plaintext_modulus);

        ASSERT_EQ(messageInM % message_modulus, m_decrypt);
        ASSERT_EQ(messageInM / message_modulus, c_decrypt);
        //ASSERT_EQ(f(messageInM, plaintext_modulus), m_decrypt);
        //
        //zpf use lweAddTo 
        //lweAddTo 做完之后并没有立刻进行bootstrap，可能会导致noise值过高
        lweAddTo(result_message, result_carry, in_params);
        Torus32 decrypt_message_after_add = lweSymDecrypt(result_message, key, plaintext_modulus);
        int32_t m_decrypt_after_ops = modSwitchFromTorus32(decrypt_message_after_add, plaintext_modulus);
        auto carry_message = messageInM % message_modulus + messageInM / message_modulus;
        ASSERT_EQ(carry_message, m_decrypt_after_ops);
        // 基本上该有的都有了，现在的问题是decrypt的范围还是有限制，超过8(plaintext_modulus = 16)就无法正常解码，现在还不确定是哪里出了问题


        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
    
    /* ----ENCRYPT_WITH_PRIVATE_KEY-- */

    TEST_F(TfheCreateBootstrapKeyTest, ENCRYPT_WITH_PRIVATE_KEY) {
        bool _use_fix_random_bak = _use_fix_random;

        const int32_t num_block = NUM_BLOCK; //zpf 16 * 2bits
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);


        int32_t messageInM = 23; 
        //zpf after decompose: [3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        messageInM = -23; 

        LweSample* lwe_cipher_blocks[num_block];
        for(int i=0; i<num_block; i++){
            lwe_cipher_blocks[i] = real_new_LweSample(extract_params);
        }
        encrypt_with_private_key(lwe_cipher_blocks, messageInM, message_modulus, plaintext_modulus, num_block, key, alpha);

        /* -----decrypt----- */
        //zpf decrypt
        int32_t message_after_decrypt = decrypt_with_private_key(lwe_cipher_blocks, message_modulus, plaintext_modulus, num_block, key);

        ASSERT_EQ(messageInM, message_after_decrypt);
        //ASSERT_EQ(9, message_after_decrypt);



        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
}
