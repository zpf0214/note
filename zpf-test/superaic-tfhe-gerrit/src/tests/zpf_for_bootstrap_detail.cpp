/* ---------------
 *                  我们仔细的一步一步的将bootstrap的过程给拆解出来
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


#define TFHE_TEST_ENVIRONMENT 1
//zpf 没有上述值include 就会有问题
#define MAX_NOISE 10000.0
#define NUM_BLOCK 16

using namespace std;

using namespace ::testing;

namespace {
    /* 参数和key预先定义好 */
    const ClassicPBSParameters M8_PARAM = TEST_PARAM_MESSAGE_8_KS_PBS_GAUSSAIN;

    const int32_t N = M8_PARAM.tlwe_polynomials_numbers;
    const int32_t plaintext_modulus = M8_PARAM.plaintext_modulus;
    const int32_t k = M8_PARAM.tlwe_dimension;
    const int32_t n = M8_PARAM.lwe_dimension;
    const int32_t l_bk = M8_PARAM.tgsw_decompose_length; //ell
    const int32_t Bgbit_bk = M8_PARAM.tgsw_Bgbit;
    const int32_t ks_t = M8_PARAM.pbs_decompose_length;
    const int32_t ks_basebit = M8_PARAM.pbs_Bgbit;
    const double alpha_in = M8_PARAM.lwe_alpha_min;
    const double alpha_bk = M8_PARAM.tlwe_alpha_min;
    const double alpha = alpha_in;

    const LweParams *in_lweparams = new_LweParams(n, plaintext_modulus, alpha_in, 1. / MAX_NOISE);
    const TLweParams *accum_params = new_TLweParams(N, k, plaintext_modulus, alpha_bk, 1. / MAX_NOISE);
    const TGswParams *bk_params = new_TGswParams(l_bk, Bgbit_bk, accum_params);
    const LweParams *extract_params = &accum_params->extracted_lweparams;

    /* 定义需要用到的函数 */
    LweSample *real_new_LweSample(const LweParams *params) {
        return new_LweSample(params);
    }

    void real_delete_LweSample(LweSample *sample) {
        delete_LweSample(sample);
    }

    int32_t modSwitchFromTorus32_63bit(Torus32 phase, int32_t plaintext_modulus){
        uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // zpf 2^63 / plaintext_modulus
        uint64_t half_interval = interv/2; // begin of the first intervall
        //uint64_t phase64 = (uint64_t(phase)<<32) ;
        uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
        return phase64/interv; //zpf phase * 2^32 / 2^63 * plaintext_modulus
    }

    Torus32 modSwitchToTorus32_63bit(int32_t mu, int32_t plaintext_modulus){
        uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // width of each intervall
        uint64_t phase64 = mu*interv; //zpf mu * 2^63 / plaintext_modulus
        return phase64>>32; //zpf mu * 2^63 / 2^32 / plaintext_modulus
    }





    class TfheCreateBootstrapKeyTest : public ::testing::Test {
    public:


        /* key 是共用的，4 个key应该都在这里生成 */
        LweKeySwitchKey *captured_result;
        vector<int32_t> captured_in_key_copy;
        const LweKey *captured_out_key;

#define INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

#include "../libtfhe-superaic-core/lwe-bootstrapping-functions.cpp"

#undef INCLUDE_TFHE_CREATEBOOTSTRAPPINGKEY

    };

    TEST_F(TfheCreateBootstrapKeyTest, BOOTSTRAP_DETAILS) {
        bool _use_fix_random_bak = _use_fix_random;
        LweKey *key = new_LweKey(in_lweparams);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bsk = new_LweBootstrappingKey(ks_t, ks_basebit, in_lweparams, bk_params);
        tfhe_createLweBootstrappingKey(bsk, key, key_bk);
        //zpf 以上缺少了 LweKeySwitchKey 这是为什么？没什么没有这个值，后面看是否用得到
        //

        int32_t messageInM = 14; //zpf 目前情况下8-15 均无法正确decrypt
                                 //
        LweSample *inlwesample_withencrypt = real_new_LweSample(in_lweparams);
        Torus32 message_modswitch = modSwitchToTorus32_63bit(messageInM, plaintext_modulus);
        lweSymEncrypt(inlwesample_withencrypt, message_modswitch, alpha, key);

        { //zpf 保证decrypt 过程是否正确
          //zpf 保证decrypt 过程是否正确
            Torus32 decrypt_message = lweSymDecrypt(inlwesample_withencrypt, key, plaintext_modulus);
            int32_t m_decrypt_message = modSwitchFromTorus32_63bit(decrypt_message, plaintext_modulus);
            ASSERT_EQ(messageInM, m_decrypt_message);
        } //zpf 保证decrypt 过程是否正确

        /* ------------- 拆分bootstrap过程 ------------- */
        LweSample *lwesample_withbootstrap = new_LweSample(in_lweparams);

        //zpf 生成 test polynomial
        int32_t test_polynomial[plaintext_modulus];
        for(int32_t i=0; i<plaintext_modulus;i++){
            test_polynomial[i] = i;
        }

        { //zpf tfhe_programmable_bootstrap 拆开
          //zpf tfhe_programmable_bootstrap 拆开
            LweSample *u_lwesample_woKs = new_LweSample(&bsk->accum_params->extracted_lweparams);

            { //zpf tfhe_programmable_bootstrap_woKS 拆开
              //感觉这样效率不太够啊，本来也没有怀疑到这里
              //还是要先把逻辑理清楚之后才能开始写代码，我们需要仔细的跟踪代码，但是不应像这样子把它拆开，太浪费时间了而且也并没有办法定位到问题究竟在哪里
              //这里还是先停一停，看具体要怎么做吧
              //可以在已经有的函数内部去做一些打印
              //zpf tfhe_programmable_bootstrap_woKS 拆开
            } //zpf tfhe_programmable_bootstrap_woKS 拆开

            delete_LweSample(u_lwesample_woKs);
        } //zpf tfhe_programmable_bootstrap 拆开


    }
}

/* 或许我们不应该着急写代码，而是看rust是如何实现的？ 
 * 问题在于即使我们看了，大概率也不会按照rust来改，现在没有这个时间
 * */
