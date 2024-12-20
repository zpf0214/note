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
    //const int32_t plaintext_modulus = 8;
    const int32_t plaintext_modulus = M8_PARAM.plaintext_modulus;
    const int32_t message_modulus = M8_PARAM.message_modulus;
    const int32_t k = M8_PARAM.tlwe_dimension;
    const int32_t n = M8_PARAM.lwe_dimension;
    const int32_t l_bsk = M8_PARAM.tgsw_decompose_length; //ell
    const int32_t Bgbit_bsk = M8_PARAM.tgsw_Bgbit;
    const int32_t ks_t = M8_PARAM.pbs_decompose_length;
    const int32_t ks_basebit = M8_PARAM.pbs_Bgbit;
    const double alpha_in = M8_PARAM.lwe_alpha_min;
    const double alpha_bsk = M8_PARAM.tlwe_alpha_min;
    const double alpha = alpha_in;

    const LweParams *in_params = new_LweParams(n, plaintext_modulus, alpha_in, 1. / MAX_NOISE);
    const TLweParams *accum_params = new_TLweParams(N, k, plaintext_modulus, alpha_bsk, 1. / MAX_NOISE);
    const TGswParams *bsk_params = new_TGswParams(l_bsk, Bgbit_bsk, accum_params);
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






    class TfheCreateBootstrapKeyTest : public ::testing::Test {
    public:


        LweKeySwitchKey *captured_result;
        vector<int32_t> captured_in_key_copy;
        const LweKey *captured_out_key;

#define INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

#include "../libtfhe-superaic-core/lwe-bootstrapping-functions.cpp"

#undef INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

    };

    TEST_F(TfheCreateBootstrapKeyTest, PLAINTEXT_MODULUS_VALUES) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bsk = new_TGswKey(bsk_params);
        tGswKeyGen(key_bsk);
        LweBootstrappingKey *bsk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bsk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bsk, key, key_bsk);

        //for(int i=-7; i<plaintext_modulus; i++) //zpf 我们从这里知道求解的范围不包含负值？TODO 如何从理论上解释为什么不包含负值？
        //for(int i=0; i<plaintext_modulus; i++)
        { //zpf test all value in plaintext_modulus = 16

            int32_t messageInM = 7; 
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
        delete_LweBootstrappingKey(bsk);
        delete_TGswKey(key_bsk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }


#if 1

    //EXPORT void tfhe_createLweBootstrappingKey(
    //	LweBootstrappingKey* bsk, 
    //	const LweKey* key_in, 
    //	const TGswKey* rgsw_key) 
    TEST_F(TfheCreateBootstrapKeyTest, PLAINTEXT_MODULUS_VALUES_WITH_PBS) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bsk = new_TGswKey(bsk_params);
        tGswKeyGen(key_bsk);
        LweBootstrappingKey *bsk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bsk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bsk, key, key_bsk);

        //for(int i=0; i<plaintext_modulus; i++)
        { //zpf test all value in plaintext_modulus = 16

            int32_t messageInM = 7; 
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
            /*
             * 首先，没有bootstrap过程的加密解密是完全正确的
             * 包含了bootstrap过程会有一些无法解密，我们需要回到最核心的地方 -- bootstrap
             *      bootstrap这个过程本身设计的对不对
             */
            apply_lookup_table(result_message, bsk, insample, plaintext_modulus, 
                    [](int32_t x){return x ;});
            // 得到 plaintext_modulus 上的明文
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus);
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);

            ASSERT_EQ(messageInM , m_decrypt);

        } //zpf test all value in plaintext_modulus = 16




        //cleanup
        delete_LweBootstrappingKey(bsk);
        delete_TGswKey(key_bsk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
#endif

    TEST_F(TfheCreateBootstrapKeyTest, TFHE_PROGRAMMABLE_BOOTSTRAP_TEST) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bsk = new_TGswKey(bsk_params);
        tGswKeyGen(key_bsk);
        LweBootstrappingKey *bsk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bsk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bsk, key, key_bsk);

        // 生成真值表
        int32_t truth_table[plaintext_modulus];
        for(int32_t i=0;i<plaintext_modulus;i++){
            truth_table[i] = f(i, plaintext_modulus);
        }

        //for(int i=-1; i<plaintext_modulus/2; i++) //zpf 根据上面的测试我们知道取值范围0-15，不包含负值
        //for(int i=0; i<plaintext_modulus; i++)
        { //zpf test all value in plaintext_modulus = 16

            int32_t messageInM = 5; 
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
            tfhe_programmable_bootstrap(result_message, bsk, truth_table, plaintext_modulus, insample);// zpf 我们想要知道这个函数是否能够正常工作
            // zpf 这个似乎无法正确解密
            // 是noise的问题吗？我们需要单独测试bootstrap过程是否正确

            /*
             * 首先，没有bootstrap过程的加密解密是完全正确的
             * 包含了bootstrap过程会有一些无法解密，我们需要回到最核心的地方 -- bootstrap
             *      bootstrap这个过程本身设计的对不对
             *      我们发现似乎是bootstrap的问题
             *      0-8能够正确的rotate，但是9-15就无法正确的rotate
             *      非常奇怪
             *
             *      现在我们知道，plaintext 0-15，不包含负值
             *      那么可能会是哪一步错了？
             *      我们需要把bootstrap整个过程拆开仔细看看
             *
             *      目前的问题是固定的将一个值错误的解密成另一个值
             *      9  --> 15
             *      10 --> 14
             *      11 --> 13
             *      12 --> 12
             *      13 --> 11
             *      14 --> 10
             *      15 --> 9
             *
             *      这个问题非常的固定，与noise值大小无关，而且非常有规律
             *      可以猜测是代码逻辑有问题，
             *
             *      首先这个问题毫无疑问在bootstrap过程中，我们的bootstrap过程是沿用了之前为逻辑门设计的代码，逻辑门的rotate部分是非常简单的
             *      我们有理由怀疑是这里出了问题，
             *
             *      其次，只有rotate部分才会涉及到原有值的问题，其它部分主要功能是降低noise值，如果这里发生问题，那么解密的值不应当如此规律
             *          这里也将noise值设置为零求解过了，得到的结论是一致的
             *      然后，为什么是9-15解密出了问题？因为原先的代码根本就没有处理相关问题
             *
             *      综上，我们需要去验证一下rotate代码是否有问题
             *      rotate部分十分复杂，我们要如何验证？
             *
             *  当plaintext_modulus = 4
             *  0 --> 2
             *  1 --> 1
             *  2 --> 0
             *  3 --> 3
             *  没有什么非常明显的规律
             *  TODO 有没有好用的宏来根据值的不同生成不同的测试，并行起来测试而不是像现在这样放到一个循环里测试
             *  guideToFullyHomomorphicEncryption 31page 提到 
             *  the most significant bit is set 0 
             *  从这里理解似乎是我们的message --> plaintext 
             *  过程中的处理有些问题
             *
             *  至少从目前来看blindRotate部分没有什么问题，
             *  严格按照论文实现的
             */


            // 得到 plaintext_modulus 上的明文
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus);
            int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus);

            ASSERT_EQ(f(messageInM, plaintext_modulus) , m_decrypt);
            //ASSERT_EQ(messageInM , m_decrypt);

        } //zpf test all value in plaintext_modulus = 16




        //cleanup
        delete_LweBootstrappingKey(bsk);
        delete_TGswKey(key_bsk);
        delete_LweKey(key);
        _use_fix_random = _use_fix_random_bak;
    }
}
