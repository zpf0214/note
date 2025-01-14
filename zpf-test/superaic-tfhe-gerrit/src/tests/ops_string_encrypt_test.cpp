#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <vector>
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

    LweSample* real_new_LweSample(const LweParams *params) {
        return new_LweSample(params);
    }

    void real_delete_LweSample(LweSample *sample) {
        delete_LweSample(sample);
    }


#if 0
    string decrypt_str(vector<LweSample**> &str_cipher, const LweKey* lwe_key)
    {
       const int32_t message_modulus = 4; //zpf TODO should be included in LweParams
       const int32_t num_block = 16; // get by sizeof(T)

       const int32_t plaintext_modulus = lwe_key -> params -> plaintext_modulus;

       string s_after_decrypt = decrypt_str_with_private_key(str_cipher, message_modulus, plaintext_modulus, num_block, lwe_key);

       return s_after_decrypt;
    }
#endif

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





    vector<bool> random_binary_key(const int32_t n) {
        vector<bool> rand_vect(n);
        for (int32_t i = 0; i < n; ++i) {
            rand_vect[i] = rand() % 2;
        }
        return rand_vect;
    }




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


    TEST_F(TfheCreateBootstrapKeyTest, ENCRYPT_STR) {
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



        { //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
          //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
            string str = "ab";
            vector<int32_t> str_list;
            {
                for(char ch: str){
                    str_list.push_back(static_cast<int32_t>(ch)); //zpf or emplace_back
                                                                  //其实这一步也并不需要，或者可以推迟到encrypt的时候
                }
            }

            vector<LweSample**> lwe_cipher_list; //TODO in heap
            { //zpf 对整个数组进行加密，之前只是对单个数字加密
              //zpf 对整个数组进行加密，之前只是对单个数字加密
                {
                    for(int32_t num: str_list){
                        //LweSample* lwe_cipher_blocks[num_block]; //zpf 放在栈上的指针在退出作用域的时候会被自动销毁，所以不能分配在栈上
                        LweSample** lwe_cipher_blocks = new LweSample*[num_block]; //zpf 释放交给 lwe_cipher_list 来做
                        { //zpf lwe_cipher_blocks 初始化
                            for(int i=0; i<num_block; i++){
                                lwe_cipher_blocks[i] = real_new_LweSample(extract_params);
                            }
                        }
                        { //zpf lwe_cipher_blocks encrypt
                            encrypt_with_private_key(lwe_cipher_blocks, num, message_modulus, plaintext_modulus, num_block, key, alpha);
                        }
                        lwe_cipher_list.push_back(lwe_cipher_blocks);
                    }
                }
            } //zpf 对整个数组进行加密，之前只是对单个数字加密

            string s_after_decrypt = "";
            { //zpf 对lwe_cipher_list 进行解密
              //zpf 对lwe_cipher_list 进行解密
                for(auto lwe_cipher_blocks: lwe_cipher_list){
                    int32_t message_after_decrypt = decrypt_with_private_key(lwe_cipher_blocks, message_modulus, plaintext_modulus, num_block, key);
                    s_after_decrypt.push_back(static_cast<char>(message_after_decrypt));
                }
                
            } //zpf 对lwe_cipher_list 进行解密
              //
            cout << "s_after_decrypt: " << s_after_decrypt 
                << endl;

            ASSERT_EQ(s_after_decrypt, str);

            { //zpf clean up
              //zpf clean up
                for (auto lwe_cipher_blocks: lwe_cipher_list){
                    for(int i=0; i<num_block; i++){
                        delete lwe_cipher_blocks[i];
                    }
                    delete[] lwe_cipher_blocks;
                }
            } //zpf clean up

        } //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串






        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }

    TEST_F(TfheCreateBootstrapKeyTest, ENCRYPT_STR_WITH_PRIVATE_KEY) {
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



        { //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
          //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
            string str = "abc";
            vector<LweSample**> lwe_cipher_list; //TODO in heap
            {//zpf lwe_cipher_list 初始化
             //zpf lwe_cipher_list 初始化
                for(auto _: str){
                    LweSample** lwe_cipher_blocks = new LweSample*[num_block];
                    for(int i=0; i<num_block; i++){
                        lwe_cipher_blocks[i] = real_new_LweSample(extract_params);
                    }
                    lwe_cipher_list.push_back(lwe_cipher_blocks);
                }
            }//zpf lwe_cipher_list 初始化

            encrypt_str_with_private_key(lwe_cipher_list, str, message_modulus,
                    plaintext_modulus, num_block, extract_params, key, alpha);


            string s_after_decrypt = decrypt_str_with_private_key(lwe_cipher_list, message_modulus, plaintext_modulus, num_block, key);
              
            cout << "s_after_decrypt: " << s_after_decrypt << endl;

            ASSERT_EQ(s_after_decrypt, str);

            clean_up(lwe_cipher_list, num_block);

        } //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串






        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }

    TEST_F(TfheCreateBootstrapKeyTest, ENCRYPT_STR_TEST) {
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



        { //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
          //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
            string str = "abc";
            vector<LweSample**> lwe_cipher_list = encrypt_str(str, key); //TODO in heap



            string s_after_decrypt = decrypt_str_with_private_key(lwe_cipher_list, message_modulus, plaintext_modulus, num_block, key);
              
            cout << "s_after_decrypt: " << s_after_decrypt << endl;

            ASSERT_EQ(s_after_decrypt, str);

            clean_up(lwe_cipher_list, num_block);

        } //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串






        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }

    TEST_F(TfheCreateBootstrapKeyTest, DECRYPT_STR_TEST) {
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



        { //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
          //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串
            string str = "abc";
            vector<LweSample**> lwe_cipher_list = encrypt_str(str, key); //TODO in heap


            string s_after_decrypt = decrypt_str(lwe_cipher_list, key);
              
            cout << "s_after_decrypt: " << s_after_decrypt << endl;

            ASSERT_EQ(s_after_decrypt, str);

            clean_up(lwe_cipher_list, num_block);

        } //zpf 现在我们把message替换成string，为了体现string，先用简单的字符串






        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }

}
