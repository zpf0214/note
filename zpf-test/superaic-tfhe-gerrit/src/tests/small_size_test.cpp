#include <chrono>
#include <gtest/gtest.h>
#include "tfhe.h"
#include <tlwe.h>
#include <tlwe_functions.h>
#include <tgsw_functions.h>
#include "test_internal.h"

#define TFHE_TEST_ENVIRONMENT 1
using namespace std;

using namespace ::testing;

namespace { // namespace



    const int32_t plaintext_modulus = 8;
    const int32_t n = 4;
    const int32_t l_bk = 3; //ell
    const int32_t Bgbit_bk = 10;
    const int32_t ks_t = 15;
    const int32_t ks_basebit = 1;
    const double alpha_in = 5e-4;
    const double alpha_bk = 9e-9;
    //const int32_t alpha_ks = 1e-6;

    LweKey *new_random_LweKey(const LweParams *params) {
        LweKey *key = new_LweKey(params);
        const int32_t n = params->n;
        for (int32_t i = 0; i < n; i++)
            key->key[i] = rand() % 2;
        return key;
    }

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


    class LweTest : public ::testing::Test {
    public:
        //TODO: parallelization
        static const LweParams *params4;
        static const LweKey *key4;
        static const vector<const LweParams *> all_params;
        static const vector<const LweKey *> all_keys;
    };

    const LweParams *LweTest::params4 = new_LweParams(4, plaintext_modulus, 0., 1.);
    const LweKey *LweTest::key4 = new_random_LweKey(params4);

    const vector<const LweParams *> LweTest::all_params = {params4};
    const vector<const LweKey *> LweTest::all_keys = {key4};

        // | frac(x) |
    double absfrac(double d) { return abs(d - rint(d)); }

    // fills a LweSample with random Torus32
    void fillRandom(LweSample *result, const LweParams *params) {
        const int32_t n = params->n;
        for (int32_t i = 0; i < n; i++) result->a[i] = uniformTorus32_distrib(generator);
        result->b = uniformTorus32_distrib(generator);
        result->current_variance = 0.2;
    }

    // copy a LweSample
    void copySample(LweSample *result, const LweSample *sample, const LweParams *params) {
        const int32_t n = params->n;
        for (int32_t i = 0; i < n; i++) result->a[i] = sample->a[i];
        result->b = sample->b;
        result->current_variance = sample->current_variance;
    }


    // This function encrypts message by using key, with stdev alpha
    // The Lwe sample for the result must be allocated and initialized
    // (this means that the parameters are already in the result)
    TEST_F (LweTest, lweSymEncryptDecrypt) {
        //TODO: parallelization
        static const int32_t NB_SAMPLES = 10;
        static const int32_t M = 8; // 消息空间8，范围[-4，4)
        static const double alpha = 1. / (10. * M);

        //随便拿一个key
        const LweKey *key =  all_keys[0];
        const LweParams *params = key->params;
        LweSample *sample = new_LweSample(params);

        int32_t messageInM = 1;
        // 1 把明文转换为 Torus32
        Torus32 message = modSwitchToTorus32(messageInM, M);
        // 2 对Torus32 上的message进行加密
        lweSymEncrypt(sample, message, alpha, key);

        // 3 解密，得到在Torus32上的密文，值已经被约束到 n/M 上
        Torus32 decrypt = lweSymDecrypt(sample, key, M);

        // 4 得到浮点数明文值,应该和 messageInM/M相同
        double ddecrypt = t32tod(decrypt);
        ASSERT_LE(absfrac(ddecrypt - messageInM/((double)M)), 10. * alpha);

        {
            // 5 modSwitchFromTorus32 得到M空间上的明文
            int32_t idecrypt = modSwitchFromTorus32(decrypt,M);
            ASSERT_EQ(idecrypt, messageInM);
        }
        {
            // 5 移位得到M空间上的明文
            int32_t idecrypt = decrypt >> (32-3);
            ASSERT_EQ((idecrypt - messageInM) % M, 0);

        }

        delete_LweSample(sample);

    }


    class PolynomialTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
        }

        virtual void TearDown() {
        }

        const set<int32_t> dimensions = {8};
        const set<int32_t> powers_of_two_dimensions = {8};
    };

    //  TorusPolynomial + TorusPolynomial 
    //EXPORT void torusPolynomialAdd(TorusPolynomial* result, const TorusPolynomial* poly1, const TorusPolynomial* poly2);
    TEST_F(PolynomialTest, torusPolynomialAdd) {
            int N = 8;
            // N = 8

            // pola = 1 + 2x + 3x^2 + 4x^3 +  5x^4 + 6x^5 + 7x^6 + 8x^7
            // polb = 1 + 2x + 3x^2 + 4x^3 +  5x^4 + 6x^5 + 7x^6 + 8x^7
            // polc = 2 + 4x + 6x^2 + 8x^3 +  10x^4 + 12x^5 + 14x^6 + 16x^7
            // 生成多项式数组，用来存放运算过程中用到的临时变量
            TorusPolynomial *pols = new_TorusPolynomial_array(5, N);
            TorusPolynomial *pola = pols + 0;
            TorusPolynomial *polacopy = pols + 1;
            TorusPolynomial *polb = pols + 2;
            TorusPolynomial *polbcopy = pols + 3;
            TorusPolynomial *polc = pols + 4;
            
            for( int i = 0 ; i < N ;i ++){
                pola->coefsT[i] = i+1;
                polb->coefsT[i] = i+1;
            }
            //torusPolynomialUniform(pola);
            //torusPolynomialUniform(polb);
            torusPolynomialCopy(polacopy, pola);
            torusPolynomialCopy(polbcopy, polb);
            torusPolynomialAdd(polc, pola, polb);
            //check equality
            for (int32_t j = 0; j < N; j++) {
                ASSERT_EQ(polacopy->coefsT[j], pola->coefsT[j]);
                ASSERT_EQ(polbcopy->coefsT[j], polb->coefsT[j]);
                ASSERT_EQ(polc->coefsT[j], pola->coefsT[j] + polb->coefsT[j]);
            }
            delete_TorusPolynomial_array(5, pols);
    }

    // This is the naive external multiplication of an integer polynomial
    // with a torus polynomial. (this function should yield exactly the same
    // result as the karatsuba or fft version, but should be slower) 
    //EXPORT void torusPolynomialMultNaive(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2);
    TEST_F(PolynomialTest, torusPolynomialMultNaive) {
        //TODO: parallelization
        static const int32_t NB_TRIALS = 5;
            int N = 4;
            IntPolynomial *ipols = new_IntPolynomial_array(2, N);
            TorusPolynomial *tpols = new_TorusPolynomial_array(3, N);
            IntPolynomial *pola = ipols + 0;
            IntPolynomial *acopy = ipols + 1;
            TorusPolynomial *polb = tpols + 0;
            TorusPolynomial *bcopy = tpols + 1;
            TorusPolynomial *polc = tpols + 2;

            // pola = 1 + 2x + 3x^2 + 4x^3 
            // polb = 1 + 2x + 3x^2 + 4x^3 
            // polc = -24 - 20x - 6x^2 + 20x^3 

            for( int i = 0 ; i < N ;i ++){
                pola->coefs[i] = i+1;
                polb->coefsT[i] = i+1;
            }

            torusPolynomialMultNaive(polc, pola, polb);

            ASSERT_EQ(polc->coefsT[0], -24);
            ASSERT_EQ(polc->coefsT[1], -20);
            ASSERT_EQ(polc->coefsT[2], -6);
            ASSERT_EQ(polc->coefsT[3], 20);

            delete_IntPolynomial_array(2, ipols);
            delete_TorusPolynomial_array(3, tpols);
    }


    TEST_F(PolynomialTest, torusPolynomialMultNaive1024) {
        static const int32_t NB_TRIALS = 5;
            int N = 1024;
            IntPolynomial *ipols = new_IntPolynomial_array(2, N);
            TorusPolynomial *tpols = new_TorusPolynomial_array(3, N);
            IntPolynomial *pola = ipols + 0;
            IntPolynomial *acopy = ipols + 1;
            TorusPolynomial *polb = tpols + 0;
            TorusPolynomial *bcopy = tpols + 1;
            TorusPolynomial *polc = tpols + 2;


            for( int i = 0 ; i < N ;i ++){
                pola->coefs[i] = i+1;
                polb->coefsT[i] = i+1;
            }
            {
                using clock = std::chrono::high_resolution_clock;
                auto start = clock::now();
                torusPolynomialMultNaive(polc, pola, polb);
                auto finish = clock::now();
                std::chrono::duration<double> elapsed = finish - start;
                std::cout << "torusPolynomialMultNaive execution 1 time: " << elapsed.count() << "s\n";
            }

            {
                using clock = std::chrono::high_resolution_clock;
                auto start = clock::now();
                for(int i = 0;i<100;++i)
                    torusPolynomialMultNaive(polc, pola, polb);
                auto finish = clock::now();
                std::chrono::duration<double> elapsed = finish - start;
                std::cout << "torusPolynomialMultNaive execution 100 time: " << elapsed.count() << "s\n";
            }



            delete_IntPolynomial_array(2, ipols);
            delete_TorusPolynomial_array(3, tpols);
    }

    // we use the function rand because in the "const static" context the uniformly random generator doesn't work!
    const TLweKey *new_random_lwe_key(const TLweParams *params) {
        TLweKey *key = new_TLweKey(params);
        const int32_t N = params->N;
        const int32_t k = params->k;

        for (int32_t i = 0; i < k; ++i)
            for (int32_t j = 0; j < N; ++j)
                key->key[i].coefs[j] = rand() % 2;
        return key;
    }

    TLweKey *new_zero_tlwe_key(const TLweParams *params) {
        TLweKey *key = new_TLweKey(params);
        const int32_t N = params->N;
        const int32_t k = params->k;

        for (int32_t i = 0; i < k; ++i)
            for (int32_t j = 0; j < N; ++j)
                key->key[i].coefs[j] = 0;
        return key;
    }


    class TLweTest : public ::testing::Test {
    };


    /*
       Testing the functions tLweSymEncryptT, tLwePhase, tLweSymDecryptT
     * EXPORT void tLweSymEncryptT(TLweSample* result, Torus32 message, double alpha, const TLweKey* key);
     * EXPORT void tLwePhase(TorusPolynomial* phase, const TLweSample* sample, const TLweKey* key);
     * EXPORT Torus32 tLweSymDecryptT(const TLweSample* sample, const TLweKey* key, int32_t plaintext_modulus);
     *
     * This functions encrypt and decrypt a random Torus32 message by using the given key
     */
    TEST_F (TLweTest, tLweSymEncryptPhaseDecryptT) {
        //TODO: parallelization
        const int32_t N = 8;
        const int32_t k = 2;
        static const int32_t NB_SAMPLES = 10;
        static const int32_t M = 8;
        static const double alpha = 1. / (10. * M);

        /* Tolerance factor for the equality between two TorusPolynomial */
        const double toler = 1e-8;

        TLweParams *tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TLweKey *tlwe_key = new_zero_tlwe_key(tlwe_params);

        {
            const TLweParams *params = tlwe_key->params;
            const int32_t N = params->N;
            const int32_t k = params->k;

            {
                bool __use_fix_random_org =  _use_fix_random;
                _use_fix_random = true;
                TLweSample *sample = new_TLweSample(params);    
                TorusPolynomial *phase = new_TorusPolynomial(N);
                int32_t clear_text = 2; // 2 in [-4,4)
                Torus32 message = modSwitchToTorus32(clear_text, M);
                Torus32 decrypt;
                ASSERT_EQ(message,0x40000000);

                // Encrypt and decrypt
                tLweSymEncryptT(sample, message, alpha, tlwe_key);
                //随机数都是0的情况下，mask多项式都是0
                for(int l = 0;l<k;l++){
                    for( int i = 0; i < N ;i ++){
                        ASSERT_EQ(sample->a[l].coefsT[i],0);
                    }
                }

                // 明文在常数项，其他项系数都是0
                ASSERT_EQ(sample->b->coefsT[0],message);
                for( int i = 1; i < N ;i ++){
                    ASSERT_EQ(sample->b->coefsT[i],0);
                }
                
                // 解密以后已经去掉了noise，这时还是在 Torus32上
                decrypt = tLweSymDecryptT(sample, tlwe_key, M);
                ASSERT_EQ(decrypt, message);

                // 得到原来的明文消息
                int32_t decrypt_in_M = modSwitchFromTorus32(decrypt,M);
                ASSERT_EQ(decrypt_in_M, clear_text);
                _use_fix_random = __use_fix_random_org;
                delete_TorusPolynomial(phase);
                delete_TLweSample(sample);
            }

        }
        delete_TLweKey(tlwe_key);
        delete_TLweParams(tlwe_params);
    }


    /*
       Testing the functions tLweSymEncrypt, tLwePhase, tLweApproxPhase, tLweSymDecrypt
     * EXPORT void tLweSymEncrypt(TLweSample* result, TorusPolynomial* message, double alpha, const TLweKey* key);
     * EXPORT void tLwePhase(TorusPolynomial* phase, const TLweSample* sample, const TLweKey* key);
     * EXPORT void tLweApproxPhase(TorusPolynomial* message, const TorusPolynomial* phase, int32_t plaintext_modulus, int32_t N);
     * EXPORT void tLweSymDecrypt(TorusPolynomial* result, const TLweSample* sample, const TLweKey* key, int32_t plaintext_modulus);
     *
     * This functions encrypt and decrypt a random TorusPolynomial message by using the given key
     */
    TEST_F (TLweTest, tLweSymEncryptPhaseDecrypt) {
        //TODO: parallelization
        const int32_t N = 8;
        const int32_t k = 2;

        static const int32_t M = 8;
        static const double alpha = 1. / (10. * M);

        /* Tolerance factor for the equality between two TorusPolynomial */
        const double toler = 1e-8;

        TLweParams *tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TLweKey *tlwe_key = new_zero_tlwe_key(tlwe_params);

        {
            const TLweParams *params = tlwe_key->params;
            const int32_t N = params->N;
            const int32_t k = params->k;

            {
                bool __use_fix_random_org =  _use_fix_random;
                _use_fix_random = true;
                TLweSample *sample = new_TLweSample(params);    
                TorusPolynomial *phase = new_TorusPolynomial(N);
                int32_t clear_text = 2; // 2 in [-4,4)
                Torus32 message_T = modSwitchToTorus32(clear_text, M);
                ASSERT_EQ(message_T,0x40000000);

                TorusPolynomial *message = new_TorusPolynomial(N);
                for (int32_t j = 0; j < N; ++j)
                    message->coefsT[j] = message_T;

                TorusPolynomial *decrypt = new_TorusPolynomial(N);

                // Encrypt and decrypt
                tLweSymEncrypt(sample, message, alpha, tlwe_key);
                //随机数都是0的情况下，mask多项式都是0
                for(int l = 0;l<k;l++){
                    for( int i = 0; i < N ;i ++){
                        ASSERT_EQ(sample->a[l].coefsT[i],0);
                    }
                }

                // 所有项系数都是0
                for( int i = 0; i < N ;i ++){
                    ASSERT_EQ(sample->b->coefsT[i],message_T);
                }
                
                // 解密以后已经去掉了noise，这时还是在 Torus32上
                tLweSymDecrypt(decrypt,sample, tlwe_key, M);
                for (int32_t j = 0; j < N; ++j){
                    ASSERT_EQ(decrypt->coefsT[j],message_T);

                    // 得到原来的明文消息
                    int32_t decrypt_in_M = modSwitchFromTorus32(decrypt->coefsT[j],M);
                    ASSERT_EQ(decrypt_in_M, clear_text);
                }
                

                _use_fix_random = __use_fix_random_org;
                delete_TorusPolynomial(phase);
                delete_TLweSample(sample);
                delete_TorusPolynomial(decrypt);
                delete_TorusPolynomial(message);
            }

        }
        delete_TLweKey(tlwe_key);
        delete_TLweParams(tlwe_params);
    }


    class TGswTest : public ::testing::Test {
    public:
        static TGswKey *new_tgsw_zero_key(const TGswParams *params) {
            TGswKey *key = new_TGswKey(params);
            const int32_t N = params->tlwe_params->N;
            const int32_t k = params->tlwe_params->k;

            for (int32_t i = 0; i < k; ++i)
                for (int32_t j = 0; j < N; ++j)
                    key->key[i].coefs[j] = 0;
            return key;
        }

        static TGswKey *new_tgsw_one_key(const TGswParams *params) {
            TGswKey *key = new_TGswKey(params);
            const int32_t N = params->tlwe_params->N;
            const int32_t k = params->tlwe_params->k;

            for (int32_t i = 0; i < k; ++i)
                for (int32_t j = 0; j < N; ++j)
                    key->key[i].coefs[j] = 1;
            return key;
        }


    };

    TGswKey *new_tgsw_zero_key(const TGswParams *params) {
        TGswKey *key = new_TGswKey(params);
        const int32_t N = params->tlwe_params->N;
        const int32_t k = params->tlwe_params->k;

        for (int32_t i = 0; i < k; ++i)
            for (int32_t j = 0; j < N; ++j)
                key->key[i].coefs[j] = 0;
        return key;
    }

    TEST_F(TGswTest, tGswSymEncrypt) {
        bool __use_fix_random_org =  _use_fix_random;
        const int32_t N = 8;
        const int32_t k = 2;
        const int32_t l = 3;
        const int32_t Bgbit = 8;

        static const int32_t M = 8;  // 因为明文是 1, 所以 M大于2都可以
        static const double alpha = 1. / (10. * M);
        TLweParams * tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TGswParams * tgsw_params = new_TGswParams(l, Bgbit, tlwe_params);
        TGswKey * tgsw_key = new_tgsw_zero_key(tgsw_params);
        TGswSample *tgsw_sample = new_TGswSample(tgsw_params);
        tGswKeyGen(tgsw_key); // KEY GENERATION
        bool need_print = false;
        if ( need_print ){
            tgsw_params->print();
        }
        _use_fix_random = true;
        tGswSymEncryptInt(tgsw_sample, 1, alpha, tgsw_key);
        if ( need_print ){
            tgsw_sample->print();
        }
 
        IntPolynomial *dechifMess = new_IntPolynomial(N);

        tGswSymDecrypt(dechifMess, tgsw_sample, tgsw_key, M);

        ASSERT_EQ(dechifMess->coefs[0] , 1);

        _use_fix_random = __use_fix_random_org;
        delete_IntPolynomial(dechifMess);
        delete_TGswSample(tgsw_sample);
        delete_TGswKey(tgsw_key); // 必须首先删除key,再删除TLweParams，否则会崩溃，bug
        delete_TLweParams(tlwe_params);
        delete_TGswParams(tgsw_params);

    }


    ////TODO: Ilaria.Theoreme3.5
    //EXPORT void tGswExternProduct(TLweSample* result, const TGswSample* a, const TLweSample* b, const TGswParams* params);
    TEST_F(TGswTest, tGswExternProduct) {
        bool __use_fix_random_org =  _use_fix_random;
        // _use_fix_random = true;
        const int32_t N = 8;
        const int32_t k = 2;
        const int32_t l = 3;
        const int32_t Bgbit = 8;
        bool need_print = false;
        static const int32_t M = N*2;  // 因为明文是 0--N, 所以为了避免产生负数，所以M要比2N大
        static const double alpha = 1e-10;
        TLweParams * tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TGswParams * tgsw_params = new_TGswParams(l, Bgbit, tlwe_params);
        const int32_t kpl = tgsw_params->kpl;
        TGswKey * tgsw_key = new_tgsw_zero_key(tgsw_params);
        TGswSample *tgsw_sample = new_TGswSample(tgsw_params);
        tGswKeyGen(tgsw_key); // KEY GENERATION

        TLweSample *result = new_TLweSample(tgsw_params->tlwe_params);
        TLweSample *b = new_TLweSample(tgsw_params->tlwe_params);

        // 生成明文多项式，系数的范围是0-N-1,为了避免产生负数，所以把明文空间设置成 2N
        TorusPolynomial *message = new_TorusPolynomial(N);
        for (int32_t j = 0; j < N; ++j)
            message->coefsT[j] = modSwitchToTorus32(j,M);

        TorusPolynomial *decrypt = new_TorusPolynomial(N);

        // Encrypt TGLWE
        tLweSymEncrypt(b, message, alpha, &(tgsw_key->tlwe_key));
        if(need_print) {
            printf("tGswExternProduct:dumping b +++++++++++++++++++\n");
            b->print();
            printf("tGswExternProduct:dumping b -------------------\n");
        }

        // 生成1的TGSW密文
        tGswSymEncryptInt(tgsw_sample, 1, alpha, tgsw_key);
        if ( need_print ) {
            printf("tGswExternProduct:dumping tgsw_sample +++++++++++++++++++\n");
            b->print();
            printf("tGswExternProduct:dumping tgsw_sample -------------------\n");
        }

        tGswExternProduct(result, tgsw_sample, b, tgsw_params);
        if ( need_print ) { 
            printf("tGswExternProduct:dumping result +++++++++++++++++++\n");
            result->print();
            printf("tGswExternProduct:dumping result -------------------\n");
        }

        // 解密以后已经去掉了noise，这时还是在 Torus32上
        tLweSymDecrypt(decrypt,result, &(tgsw_key->tlwe_key), M);
        for (int32_t j = 0; j < N; ++j){
            // 得到原来的明文消息
            int32_t decrypt_in_M = modSwitchFromTorus32(decrypt->coefsT[j],M);
            ASSERT_EQ(decrypt_in_M, j);
        }


        _use_fix_random = __use_fix_random_org;
        delete_TorusPolynomial(decrypt);
        delete_TorusPolynomial(message);
        delete_TLweSample(b);
        delete_TLweSample(result);
        delete_TGswSample(tgsw_sample);
        delete_TGswKey(tgsw_key);
        delete_TGswParams(tgsw_params);
        delete_TLweParams(tlwe_params);

    }


//  void tfhe_MuxRotate(TLweSample *result, const TLweSample *accum, const TGswSample *bki, const int32_t barai,
//                     const TGswParams *bk_params) {
//     // ACC = BKi*[(X^barai-1)*ACC]+ACC
//     // temp = (X^barai-1)*ACC
//     tLweMulByXaiMinusOne(result, barai, accum, bk_params->tlwe_params);
//     // temp *= BKi
//     tGswExternMulToTLwe(result, bki, bk_params);
//     // ACC += temp
//     tLweAddTo(result, accum, bk_params->tlwe_params);
// }
    EXPORT void tfhe_MuxRotate(TLweSample *result, const TLweSample *accum, const TGswSample *bki, const int32_t barai, const TGswParams *bk_params);
    TEST_F(TGswTest, tCMux) {
        bool __use_fix_random_org =  _use_fix_random;
        _use_fix_random = true;
        const int32_t N = 8;
        const int32_t k = 2;
        const int32_t l = 3;
        const int32_t Bgbit = 8;
        bool need_print = false;

        static const int32_t M = N*2;  // 因为明文是 0--N, 所以为了避免产生负数，所以M要比2N大
        static const double alpha = 1e-10;
        TLweParams * tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TGswParams * tgsw_params = new_TGswParams(l, Bgbit, tlwe_params);
        const int32_t kpl = tgsw_params->kpl;
        TGswKey * tgsw_key = new_tgsw_zero_key(tgsw_params);
        TGswSample *tgsw_sample = new_TGswSample(tgsw_params);
        tGswKeyGen(tgsw_key); // KEY GENERATION

        TLweSample *result = new_TLweSample(tgsw_params->tlwe_params);
        TLweSample *b = new_TLweSample(tgsw_params->tlwe_params);

        TorusPolynomial *message_InM = new_TorusPolynomial(N);
        TorusPolynomial *message = new_TorusPolynomial(N);
        TorusPolynomial *decrypt = new_TorusPolynomial(N);
        TorusPolynomial *decrypt_InM = new_TorusPolynomial(N);

        // Encrypt TGLWE b
        // 生成明文多项式，系数的范围是0-N-1,为了避免产生负数，所以把明文空间设置成 2N
        for (int32_t j = 0; j < N; ++j) {
            message_InM->coefsT[j] = j;
            message->coefsT[j] = modSwitchToTorus32(j,M);
        }

        tLweSymEncrypt(b, message, alpha, &(tgsw_key->tlwe_key));
        if( need_print ) {
            printf("tGswExternProduct:dumping b +++++++++++++++++++\n");
            b->print();
            printf("tGswExternProduct:dumping b -------------------\n");

        }


        {
            // 生成0的TGSW密文
            tGswSymEncryptInt(tgsw_sample, 0, alpha, tgsw_key);
            if ( need_print ) { 
                printf("tGswExternProduct:dumping tgsw_sample +++++++++++++++++++\n");
                tgsw_sample->print();
                printf("tGswExternProduct:dumping tgsw_sample -------------------\n");
            }

            // 选择结果是 b
            tfhe_MuxRotate(result, b , tgsw_sample,  5, tgsw_params);

            if ( need_print ) {
                printf("tGswExternProduct:dumping result +++++++++++++++++++\n");
                result->print();
                printf("tGswExternProduct:dumping result -------------------\n");
            }            

            // 解密以后已经去掉了noise，这时还是在 Torus32上
            tLweSymDecrypt(decrypt,result, &(tgsw_key->tlwe_key), M);
            for (int32_t j = 0; j < N; ++j){
                // 得到原来的明文消息
                decrypt_InM->coefsT[j] = modSwitchFromTorus32(decrypt->coefsT[j],M);
            }

            // mux选择bit为0,结果和原来的多项式一样
            for (int32_t j = 0; j < N; ++j){
                ASSERT_EQ(decrypt_InM->coefsT[j],message_InM->coefsT[j]);
            }
        }


        {
            // 生成1的TGSW密文
            tGswSymEncryptInt(tgsw_sample, 1, alpha, tgsw_key);
            if ( need_print ) {
                printf("tGswExternProduct:dumping tgsw_sample +++++++++++++++++++\n");
                tgsw_sample->print();
                printf("tGswExternProduct:dumping tgsw_sample -------------------\n");
            }

            // 选择结果是 x^5 x b,常数项是-3, 
            tfhe_MuxRotate(result, b , tgsw_sample,  5, tgsw_params);

            if ( need_print ) {
                printf("tGswExternProduct:dumping result +++++++++++++++++++\n");
                result->print();
                printf("tGswExternProduct:dumping result -------------------\n");

            }            

            // 解密以后已经去掉了noise，这时还是在 Torus32上
            tLweSymDecrypt(decrypt,result, &(tgsw_key->tlwe_key), M);
            for (int32_t j = 0; j < N; ++j){
                // 得到原来的明文消息
                decrypt_InM->coefsT[j] = modSwitchFromTorus32(decrypt->coefsT[j],M);
            }
            if ( need_print ) {
                printf("tGswExternProduct:dumping decrypt_InM +++++++++++++++++++\n");
                decrypt_InM->print(true);
                printf("tGswExternProduct:dumping decrypt_InM -------------------\n");
            }
            // 原来的常数项移动到x^5上
            ASSERT_EQ(decrypt_InM->coefsT[5],0);
            // 原来的x^3 转到常数项上，并且变为负数
            ASSERT_EQ(decrypt_InM->coefsT[0],-3 & 0xf);
        }


        _use_fix_random = __use_fix_random_org;
        delete_TorusPolynomial(decrypt_InM);
        delete_TorusPolynomial(decrypt);
        delete_TorusPolynomial(message);
        delete_TorusPolynomial(message_InM);
        delete_TLweSample(b);
        delete_TLweSample(result);
        delete_TGswSample(tgsw_sample);
        delete_TGswKey(tgsw_key);
        delete_TGswParams(tgsw_params);
        delete_TLweParams(tlwe_params);

    }

    TEST_F(TGswTest, tblindRotate) {
        bool __use_fix_random_org =  _use_fix_random;
        _use_fix_random = true;
        const int32_t N = 8;
        const int32_t k = 2;
        const int32_t l = 3;
        const int32_t Bgbit = 8;
        const int32_t n = 4;
        const int32_t M = 8;
        const int32_t Nx2 = N * 2;
        const double alpha = 1e-10;
        bool need_print = false;
        // 生成参数
        TLweParams * tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TGswParams * tgsw_params = new_TGswParams(l, Bgbit, tlwe_params);

        // 生成 LWE key
        LweParams * lwe_params = new_LweParams(n, plaintext_modulus, 0., 1.);
        LweKey *lwe_key =(LweKey *) new_random_LweKey((const LweParams *)lwe_params);

        // 生成  TLWE key
        TGswKey * tgsw_key = new_tgsw_zero_key(tgsw_params);
        tGswKeyGen(tgsw_key); // KEY GENERATION

        // 用TLWE key对 LWE key的每个bit进行加密
        TGswSample *tgsw_samples = new_TGswSample_array(n,tgsw_params);
        for(int32_t i = 0; i < n ; ++i){
            tGswSymEncryptInt(&tgsw_samples[i], lwe_key->key[i], alpha, tgsw_key);
        }


        LweSample *lwe_sample = new_LweSample(lwe_params);
        // 误差与N大小有关，目前4，5，7（or -4，-1，-3）无法正确解码(N=16，32，64，128)
        int32_t messageInM = -2;
        // 1 把明文转换为 Torus32 , plaintext_modulus 不是2N的时候要怎么办？？？？
        Torus32 message = modSwitchToTorus32(messageInM, M); // zpf: 2/M * 2^32 = 2^30 
        //Torus32 message = modSwitchToTorus32(messageInM, Nx2); // zpf: mu * 2^32 / plaintext_modulus = mu/plaintext_modulus * 2^32
        //zpf: 2 = modSwitchFromTorus32(modSwitchToTorus32(2, plaintext_modulus), plaintext_modulus)

        // 2 对Torus32 上的message进行加密
        lweSymEncrypt(lwe_sample, message, alpha, lwe_key);
        if ( need_print ) {
            printf("tblindRotate:dumping lwe_sample +++++++++++++++++++\n");
            lwe_sample->print();
            printf("tblindRotate:dumping lwe_sample -------------------\n");
        }


        // 3 生成测试多项式 //zpf: 从这里的生成方式来看并不是论文中的test polynomial
        // cpp源代码中可以让所有系数全部是message的原因是只是为了搭建gate，也就是说值只有两个，所以可以
        // 但是这里就不可以，因为值不止两个，那么test polynomial的值就需要重新设计，按照论文的实现来设计
        TorusPolynomial *test_vec = new_TorusPolynomial(N);
        testPolynomialGen(test_vec, N, M);

        // 4 把 lwe密文转到 模2N
        // zpf: 没有问题，确实转到了 模2N上
        int32_t *bara = new int32_t[n];
        int32_t barb = modSwitchFromTorus32(lwe_sample->b, Nx2);
        for (int32_t i = 0; i < n; i++) {
            bara[i] = modSwitchFromTorus32(lwe_sample->a[i], Nx2);
        }
        if ( need_print ) {
            printf("tblindRotate: barb 0x%08x\n",barb);
            for (int32_t i = 0; i < n; i++) {
                printf("tblindRotate: bara[%d] 0x%08x\n",i,bara[i]);
            }
        }
        TorusPolynomial *testvectbis = new_TorusPolynomial(N);
        TLweSample *acc = new_TLweSample(tlwe_params);

        // 5 把原来的多项式乘x^{-barb}
        if (barb != 0) torusPolynomialMulByXai(testvectbis, Nx2 - barb, test_vec);
        else torusPolynomialCopy(testvectbis, test_vec);
        if ( need_print ) {
            printf("tblindRotate:dumping testvectbis +++++++++++++++++++\n");
            testvectbis->print(true);
            printf("tblindRotate:dumping testvectbis -------------------\n");
        }

        // 6 给多项式加密，生成TLWE密文
        // !!!! 在加密之前，要把系数从原来的NX2 空间转到Torus32上，否则解密的时候M没法填写
        // zpf: 这里不需要转到Torus32上，这是加密过程需要做的事
        //for ( int i = 0 ; i < N ;i ++) {
        //    testvectbis->coefsT[i] = modSwitchToTorus32(testvectbis->coefsT[i],Nx2);
        //}

        tLweNoiselessTrivial(acc, testvectbis, tlwe_params);
        if ( need_print ) {
            printf("tblindRotate:dumping acc before blindrotate +++++++++++++++++++\n");
            acc->print();
            printf("tblindRotate:dumping acc before blindrotate-------------------\n");
        }

        // 7 blind rotate, 得到新的TLWE key加密的 TLWE密文
        tfhe_blindRotate(acc, tgsw_samples, bara, n, tgsw_params);

        // 8 把acc解密，得到旋转后的多项式，常数项应该是2
        TorusPolynomial *result = new_TorusPolynomial(N);
        tLweSymDecrypt(result,acc, &(tgsw_key->tlwe_key), M);
        if ( need_print ) {
            printf("tblindRotate:dumping acc after blindrotate +++++++++++++++++++\n");
            acc->print();
            printf("tblindRotate:dumping acc after blindrotate-------------------\n");
        }

        if ( need_print ) {
            printf("tblindRotate: dumping result start ++++++++++++++\n");
            result->print(true);
            printf("tblindRotate: dumping result end ++++++++++++++\n");
        }
        ASSERT_EQ((modSwitchFromTorus32(result->coefsT[0],M) - messageInM) % M, 0);
        //ASSERT_EQ(modSwitchFromTorus32(result->coefsT[0],M) , messageInM );

        delete_TorusPolynomial(result);
        delete_TorusPolynomial(testvectbis);
        delete_TLweSample(acc);
        delete []bara;
        delete_TorusPolynomial(test_vec);
        delete_LweSample(lwe_sample);
        delete_LweKey(lwe_key);
        delete_TGswKey(tgsw_key);
        delete_TGswSample_array(n,tgsw_samples);
        delete_TGswParams(tgsw_params);
        delete_TLweParams(tlwe_params);
        delete_LweParams(lwe_params);
        _use_fix_random = __use_fix_random_org;
    }


    TEST_F (TLweTest, tSampleExtract) {
        //TODO: parallelization
        const int32_t N = 8;
        const int32_t k = 2;

        static const int32_t M = 8;
        static const double alpha = 1. / (10. * M);

        /* Tolerance factor for the equality between two TorusPolynomial */
        const double toler = 1e-8;
        bool need_print = false;

        TLweParams *tlwe_params = new_TLweParams(N, k, plaintext_modulus, 0., 1.);
        TLweKey *tlwe_key = new_zero_tlwe_key(tlwe_params);

        LweParams * extract_params = new_LweParams(N*k, plaintext_modulus,0., 1.);
        LweKey *extracted_key = new_LweKey(extract_params);
        LweSample *extracted_sample = new_LweSample(extract_params);
        tLweExtractKey(extracted_key, tlwe_key);

        {
            const TLweParams *params = tlwe_key->params;
            const int32_t N = params->N;
            const int32_t k = params->k;

            {
                bool __use_fix_random_org =  _use_fix_random;
                _use_fix_random = false;
                TLweSample *sample = new_TLweSample(params);    
                TorusPolynomial *phase = new_TorusPolynomial(N);
                int32_t clear_text = 2; // 2 in [-4,4)
                Torus32 message_T = modSwitchToTorus32(clear_text, M);
                ASSERT_EQ(message_T,0x40000000);

                TorusPolynomial *message = new_TorusPolynomial(N);


                // 常数项是2，其他项是0的多项式
                for (int32_t j = 0; j < N; ++j) {
                    if (j == 0) 
                        message->coefsT[j] = message_T;
                    else 
                        message->coefsT[j] = 0;
                }

                TorusPolynomial *decrypt = new_TorusPolynomial(N);

                // Encrypt TLWE
                tLweSymEncrypt(sample, message, alpha, tlwe_key);

                // extract 常数项
                tLweExtractLweSample(extracted_sample, sample, extract_params,  tlwe_params);
                if ( need_print ) {
                    printf("tSampleExtract: dumping extracted_sample start ++++++++++++++\n");
                    extracted_sample->print();
                    printf("tSampleExtract: dumping extracted_sample end ++++++++++++++\n");
                }

                // 得到在Torus32上的密文，值已经被约束到 n/M 上
                Torus32 decrypt32 = lweSymDecrypt(extracted_sample, extracted_key, M);
                int32_t idecrypt = modSwitchFromTorus32(decrypt32,M);
                ASSERT_EQ(idecrypt,clear_text);


                _use_fix_random = __use_fix_random_org;
                delete_TorusPolynomial(phase);
                delete_TLweSample(sample);
                delete_TorusPolynomial(decrypt);
                delete_TorusPolynomial(message);
            }

        }
        delete_TLweKey(tlwe_key);
        delete_TLweParams(tlwe_params);
        delete_LweSample(extracted_sample);
        delete_LweKey(extracted_key);
        delete_LweParams(extract_params);
    }


    TEST_F (TLweTest, tKeySwitch) {
        //TODO: parallelization
        const int32_t N = 8;
        const int32_t k = 2;
        const int32_t n1 = 8;
        const int32_t n2 = 16;

        static const int32_t M = 8;
        static const double alpha = 1. / (10. * M);

        /* Tolerance factor for the equality between two TorusPolynomial */
        const double toler = 1e-8;
        bool need_print = false;

        LweParams *params_s1 = new_LweParams(n1, plaintext_modulus, 0., 1.);
        LweKey * key_s1 = new_random_LweKey(params_s1);
        LweSample * lwe_sample_s1 = new_LweSample(params_s1);

        LweParams *params_s2 = new_LweParams(n2, plaintext_modulus, 0., 1.);
        LweKey * key_s2 = new_random_LweKey(params_s2);
        LweSample * lwe_sample_s2 = new_LweSample(params_s2);

        int32_t clear_text = 2; // 2 in [-4,4)
        Torus32 message = modSwitchToTorus32(clear_text, M);

        // key1 加密
        lweSymEncrypt(lwe_sample_s1, message, alpha, key_s1);

        LweKeySwitchKey *ksk = new_LweKeySwitchKey(n1, 3, 2, params_s2);
        lweCreateKeySwitchKey(ksk,key_s1,key_s2);

        // key1->key2
        lweKeySwitch(lwe_sample_s2,ksk,lwe_sample_s1);

        // 用key2解密
        // 解密，得到在Torus32上的密文，值已经被约束到 n/M 上
        Torus32 decrypt = lweSymDecrypt(lwe_sample_s2, key_s2, M);

        // 5 modSwitchFromTorus32 得到M空间上的明文
        int32_t idecrypt = modSwitchFromTorus32(decrypt,M);
        ASSERT_EQ(idecrypt, clear_text);
        

        delete_LweKeySwitchKey(ksk);

        delete_LweSample(lwe_sample_s2);
        delete_LweKey(key_s2);
        delete_LweParams(params_s2);

        delete_LweSample(lwe_sample_s1);
        delete_LweKey(key_s1);
        delete_LweParams(params_s1);

    }

} // namespace
