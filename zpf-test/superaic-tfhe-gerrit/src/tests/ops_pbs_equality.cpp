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



#if 0
    /*zpf
     * 这里似乎还是没有考虑清楚，如果我们不知道前一个的匹配结果
     * 如何知道后面应该如何匹配？
     * 例如用 ab 去匹配 abcde，因为我们只有在解密之后才知道
     * 匹配结果如何，所以
     * 我们就需要把abcde 按照 ab bc cd de 全部都匹配一遍
     * 然后解密的时候判断是否有成功匹配到的值
     * 即，即使我们一开始就已经匹配到了，整个过程
     * 可以结束了，但是由于我们不知道已经匹配到了，所以
     * 我们需要穷尽所有可能
     */
    void Lwecipher_message_eq(
            LweSample** lwe_mess_match,
            LweSample** lwe_cipher_blocks,
            LweBootstrappingKey *bk,
            int32_t message,
            int32_t message_modulus,
            int32_t plaintext_modulus,
            int32_t num_block)
    {
        int32_t message_decomposer_result[num_block];
        block_decomposer(message_decomposer_result, message, message_modulus, num_block);
        for(int i=0; i<num_block; i++){
            auto message_decomposer_bits = message_decomposer_result[i];
            apply_lookup_table(lwe_mess_match[i], bk, lwe_cipher_blocks[i], plaintext_modulus, 
                    [message_decomposer_bits](int32_t x)->int32_t{
                        return x == message_decomposer_bits;
                    });
        }
    }
#endif

    int32_t truth_number(const int32_t message_modulus, const int32_t num_block){
        int32_t truth_block[num_block];
        for(int i=0; i<num_block; i++){
            truth_block[i] = 1;
        }
        auto truth = block_recomposer(truth_block, message_modulus, num_block);
        return truth;
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

        apply_lookup_table(result_message, bk, insample, plaintext_modulus, 
                [messageInM](int32_t x)->int32_t{return x == messageInM;});


        //Todo : check resulte
        // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上
        Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus);

        // 得到 plaintext_modulus 上的明文
        int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);

        ASSERT_EQ(1, m_decrypt);
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

    TEST_F(TfheCreateBootstrapKeyTest, EQUALITYTEST) {
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
        //messageInM = -23; 

        LweSample* lwe_cipher_blocks[num_block];
        for(int i=0; i<num_block; i++){
            lwe_cipher_blocks[i] = real_new_LweSample(extract_params);
        }
        encrypt_with_private_key(lwe_cipher_blocks, messageInM, message_modulus, plaintext_modulus, num_block, key, alpha);

        int32_t message_decomposer_result[num_block]; //zpf 直接分解，然后一个一个对比是否相等
        block_decomposer(message_decomposer_result, messageInM, message_modulus, num_block);

        { //zpf 通过bootstrap 验证两者是否相等
          //zpf 通过bootstrap 验证两者是否相等
            LweSample* lwe_mess_match[num_block];
            for(int i=0; i<num_block; i++){
                lwe_mess_match[i] = real_new_LweSample(extract_params);
            }

            for(int i=0; i<num_block; i++){
                auto message_decomposer_bits = message_decomposer_result[i];
                apply_lookup_table(lwe_mess_match[i], bk, lwe_cipher_blocks[i], plaintext_modulus,
                        [message_decomposer_bits](int32_t x)->int32_t{
                            return x == message_decomposer_bits;
                        }); //我们只有在解密了之后才能知道是否匹配到了
                                     //否则就违反了保密原则
            }
            //zpf 解密 lwe_mess_match
            //这里并不适合用decrypt_with_private_key
            //因为一个字符被分成多段，只有每一段都被匹配上了
            //或者解出来之后和某个固定值进行比较
            int32_t lwe_mess_match_decrypt = decrypt_with_private_key(lwe_mess_match, message_modulus, plaintext_modulus, num_block, key);

            int32_t truth;
            {
                int32_t truth_block[num_block];
                for(int i=0; i<num_block; i++){
                    truth_block[i] = 1;
                }
                truth = block_recomposer(truth_block, message_modulus, num_block);
            }

            ASSERT_EQ(truth, lwe_mess_match_decrypt);

        } //zpf 通过bootstrap 验证两者是否相等

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

    TEST_F(TfheCreateBootstrapKeyTest, EQUALITYTEST_Lwecipher_message_eq) {
        /*  验证了单独一个message是否能够正确判断 */
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
        //messageInM = -23; 

        LweSample* lwe_cipher_blocks[num_block];
        for(int i=0; i<num_block; i++){
            lwe_cipher_blocks[i] = real_new_LweSample(extract_params);
        }
        encrypt_with_private_key(lwe_cipher_blocks, messageInM, message_modulus, plaintext_modulus, num_block, key, alpha);


        { //zpf 通过bootstrap 验证两者是否相等
          //zpf 通过bootstrap 验证两者是否相等
            LweSample* lwe_mess_match[num_block]; //TODO zpf need delete
            for(int i=0; i<num_block; i++){
                lwe_mess_match[i] = real_new_LweSample(extract_params);
            }

            Lwecipher_message_eq(lwe_mess_match, lwe_cipher_blocks, bk, messageInM, message_modulus, plaintext_modulus, num_block);

            int32_t lwe_mess_match_decrypt = decrypt_with_private_key(lwe_mess_match, message_modulus, plaintext_modulus, num_block, key);

            int32_t truth = truth_number(message_modulus, num_block);

            ASSERT_EQ(truth, lwe_mess_match_decrypt);

        } //zpf 通过bootstrap 验证两者是否相等

        /* -----decrypt----- */
        //zpf decrypt
        int32_t message_after_decrypt = decrypt_with_private_key(lwe_cipher_blocks, message_modulus, plaintext_modulus, num_block, key);

        ASSERT_EQ(messageInM, message_after_decrypt);



        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
}
