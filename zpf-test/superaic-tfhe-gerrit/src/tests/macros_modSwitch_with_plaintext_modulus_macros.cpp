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

#define TFHE_TEST_ENVIRONMENT 1
#define MAX_NOISE 10000.0
#define NUM_BLOCK 16
using namespace std;

using namespace ::testing;

namespace {

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
    const double alpha = 0.0;
    //const double alpha = alpha_in;

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
#if 1
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
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32_63bit(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            Torus32 decrypt_message = lweSymDecrypt(insample, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32_63bit(decrypt_message,plaintext_modulus); \
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
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(8)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(9)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(10)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(11)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(12)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(13)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(14)
    GENERATE_TEST_FUNCTION_WITH_DIFFERENT_VALUE(15)


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
        for(int32_t i=0;i<plaintext_modulus;i++){ \
            truth_table[i] = f(i, plaintext_modulus); \
        } \
        { \
            int32_t messageInM = value % plaintext_modulus; \
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32_63bit(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            LweSample *result_message = new_LweSample(in_params); \
            tfhe_programmable_bootstrap(result_message, bk, truth_table, plaintext_modulus, insample); \
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32_63bit(decrypt_message,plaintext_modulus); \
            ASSERT_EQ(f(messageInM, plaintext_modulus) , m_decrypt); \
        } \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }

    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(0)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(1)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(2)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(3)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(4)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(5)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(6)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(7)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(8)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(9)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(10)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(11)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(12)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(13)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(14)
    GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_TEST(15)
#endif

    TEST_F(TfheCreateBootstrapKeyTest, MODSWITCH_TEST_63BIT){
        for(int32_t i=0; i<plaintext_modulus; i++){ //负值无法正确求解
                                                    //TODO 是否会影响加法减法的运算？
                                                    //如果是原来的函数是否可以求解负值？
                                                    //如果上面的测试没有通过，所有的讨论也就没有意义了
            Torus32 plain = modSwitchToTorus32_63bit(i, plaintext_modulus);
            int32_t i_after_mod = modSwitchFromTorus32_63bit(plain, plaintext_modulus);
            ASSERT_EQ(i, i_after_mod);
        }

    }

    TEST_F(TfheCreateBootstrapKeyTest, MODSWITCH_TEST){
        for(int32_t i=0; i<plaintext_modulus; i++){ //负值无法正确求解
                                                    //TODO 是否会影响加法减法的运算？
            Torus32 plain = modSwitchToTorus32(i, plaintext_modulus);
            int32_t i_after_mod = modSwitchFromTorus32(plain, plaintext_modulus);
            ASSERT_EQ(i, i_after_mod);
        }

    }

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
        for(int32_t i=0;i<plaintext_modulus;i++){ \
            truth_table[i] = f(i, plaintext_modulus); \
        } \
        { \
            int32_t messageInM = value % plaintext_modulus; \
            LweSample *insample = real_new_LweSample(extract_params); \
            Torus32 message = modSwitchToTorus32_63bit(messageInM, plaintext_modulus); \
            lweSymEncrypt(insample, message, alpha, key); \
            LweSample *result_message = new_LweSample(in_params); \
            tfhe_programmable_bootstrap(result_message, bk, truth_table, plaintext_modulus, insample); \
            Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus); \
            Torus32 decrypt_message_without_pbs = lweSymDecrypt(insample, key, plaintext_modulus); \
            int32_t m_decrypt = modSwitchFromTorus32_63bit(decrypt_message,plaintext_modulus); \
            int32_t m_decrypt_without_pbs = modSwitchFromTorus32_63bit(decrypt_message_without_pbs, plaintext_modulus); \
            ASSERT_EQ(m_decrypt_without_pbs, m_decrypt); \
        } \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }

    //GENERATE_TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST(0)
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
}
/* ---
 *
 * 仍然无法正确解出来，只不过现在是所有偶数值可以解出来，所有奇数值解不出来，是求解的方式有问题吗？还是rotate有问题？
 * 先看是否是decrypt有问题，因为现在不仅仅是bootstrap过程有问题，连简单的lwe过程有都有了问题:
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_1
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_3
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_5
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_7
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_9
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_11
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_13
[  FAILED  ] TfheCreateBootstrapKeyTest.PLAINTEXT_MODULUS_VALUES_15
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_1
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_3
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_5
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_7
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_9
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_11
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_13
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_15

不过从这里看似乎也有一个好消息，就是经过bootstrap和不经过bootstrap的过程错误的地方是一样的，这是否说明bootstrap过程没有问题，现在是decrypt过程出了问题？
仔细看失败的值差距并不大，是否是noise的问题？

可以在目前的基础上回去看看decrypt 的具体过程，但是decrypt 事实上是一个非常简单的过程，这里怎么可能会出错呢？难道这个过程有什么写死了的值？
无论如何需要去看看

在设置noise值为 0 之后仍然是以上的值无法被正确解出来，需要去看看decrypt是如何实现的

    //uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
这一句导致decrypt阶段总是出错，像这种含有硬编码的过程就应该抽出来写一个 inline 函数

现在lweSymEncrypt 和 lweSymDecrypt 过程已经可以正常工作了（没有noise的情况下，有的情况还没有测试，但是直觉上与noise无关）

现在怀疑是否bootstrap过程也有这种硬编码？

我们grep了一下，有63硬编码的函数只有approxPhase modSwitchFromTorus32 modSwitchToTorus32 ，
 */

/*
 * 
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_1
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_3
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_5
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_7
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_9
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_11
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_13
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_15
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_0
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_1
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_3
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_5
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_7
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_9
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_11
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_13
[  FAILED  ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_WITHOUT_PBS_TEST_15

现在我们可以看到问题再次出现在bootstrap过程中，这个过程到底哪里错了呢？
可能要把整个过程重新自己写一遍？

我们整理 tfhe_programmable_bootstrap_woKS 发现其中也使用到了 modSwitchFromTorus32 函数，或许这里就是我们为什么一直无法正确解出来的原因？

[ RUN      ] TfheCreateBootstrapKeyTest.TFHE_PROGRAMMABLE_BOOTSTRAP_TEST_0
macros_modSwitch_with_plaintext_modulus_macros: /home/superarc_zpf/TFHE/superaic-tfhe-gerrit/src/libtfhe-superaic-core/toruspolynomial-functions.cpp:121: void torusPolynomialMulByXaiMinusOne(TorusPolynomial*, int32_t, const TorusPolynomial*): Assertion `a >= 0 && a < 2 * N' failed.

这里大概与 0 mod 之后的操作有关，我们可以先跳过 0 值再试试

这里需要把整个函数调用逻辑链条弄清楚，看看为什么只是修改了encode方式就会导致 assert 无法通过，这个assert还无法提供更多有效信息，到底发生了什么问题？

    for (int32_t i = 0; i < n; i++) {
        bara[i] = modSwitchFromTorus32(x->a[i], Nx2);
        assert(bara[i] >= 0 && bara[i] < Nx2);
    }
现在是定位到这里传入的值有负值，为什么会有负值这一点需要确认
是不是这种简单的移位方式会有巨大的问题，比如符号上的问题？
之前没有崩为什么在改为63进位之后就崩掉了？所以传入的值大概率也没什么问题
但是需要去看看对应的参数是如何初始化的

即使将public key a 全部值设置为0，仍然无法正确decrypt，但是
首先，说明assert断言出错了，基本和下面代码有关
        result->a[i] = _use_fix_random ? 0 : uniformTorus32_distrib(generator);
这里导致了我们modSwitchFromTorus32 出了错，这里也与我们的进位有着极大的关系，我们究竟应该mod到哪里？

其次，现在又变回8-15 无法被正确解出来，而且值差别巨大
14 --> 26
15 --> 25

所以应该还有什么地方我们没有弄清楚

*/
