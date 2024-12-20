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

    #define GENERATE_EQUALITY_TEST(value) \
    TEST_F(TfheCreateBootstrapKeyTest, EQUALITYTEST_##value) { \
        bool _use_fix_random_bak = _use_fix_random; \
        LweKey *key = new_LweKey(in_params); \
        lweKeyGen(key); \
        TGswKey *key_bk = new_TGswKey(bk_params); \
        tGswKeyGen(key_bk); \
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params); \
        tfhe_createLweBootstrappingKey(bk, key, key_bk); \
        int32_t messageInM = value;  \
        LweSample *insample = real_new_LweSample(extract_params); \
        Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus); \
        lweSymEncrypt(insample, message, alpha, key); \
        LweSample *result_message = new_LweSample(in_params); \
        apply_lookup_table(result_message, bk, insample, plaintext_modulus,  \
                [messageInM](int32_t x)->int32_t{return x == messageInM;}); \
        Torus32 decrypt_message = lweSymDecrypt(result_message, key, plaintext_modulus); \
        int32_t m_decrypt = modSwitchFromTorus32(decrypt_message,plaintext_modulus); \
        ASSERT_EQ(1, m_decrypt); \
        delete_LweBootstrappingKey(bk); \
        delete_TGswKey(key_bk); \
        delete_LweKey(key); \
        _use_fix_random = _use_fix_random_bak; \
    }
    
    GENERATE_EQUALITY_TEST(0)
    GENERATE_EQUALITY_TEST(1)
    GENERATE_EQUALITY_TEST(2)
    GENERATE_EQUALITY_TEST(3)
    GENERATE_EQUALITY_TEST(4)
    GENERATE_EQUALITY_TEST(5)
    GENERATE_EQUALITY_TEST(6)
    GENERATE_EQUALITY_TEST(7)

    //zpf 现在我们还无法保证8-15的正确性

}
