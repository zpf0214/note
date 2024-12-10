/* 
 * tgsw_test.cpp
 * Tests the functions defined in /src/include/tgsw_functions.h
 * A set of parameters ad keys is defined in the beginning (N=512,1024,2048 and k=1,2)
 */


#include <gtest/gtest.h>
#include <tlwe.h>
#include <tlwe_functions.h>
#include <tgsw_functions.h>
// #include <numeric_functions.h>
#include <polynomials_arithmetic.h>
#include <lagrangehalfc_arithmetic.h>

#define TFHE_TEST_ENVIRONMENT 1
#define plaintext_modulus 2

using namespace std;
using namespace ::testing;

namespace {

    // we use the function rand because in the "const static" context the uniformly random generator doesn't work!
    const TGswKey *new_random_key(const TGswParams *params) {
        TGswKey *key = new_TGswKey(params);
        const int32_t N = params->tlwe_params->N;
        const int32_t k = params->tlwe_params->k;

        for (int32_t i = 0; i < k; ++i)
            for (int32_t j = 0; j < N; ++j)
                key->key[i].coefs[j] = rand() % 2;
        return key;
    }


    // we use the function rand because in the "const static" context the uniformly random generator doesn't work!
    IntPolynomial *new_random_IntPolynomial(const int32_t N) {
        IntPolynomial *poly = new_IntPolynomial(N);

        for (int32_t i = 0; i < N; ++i)
            poly->coefs[i] = rand() % 10 - 5;
        return poly;
    }


    /*
     * Parameters and keys (for N=512,1024,2048 and k=1,2)
     */
    const TGswParams *params512_1 = new_TGswParams(4, 8, new_TLweParams(512, 1, plaintext_modulus, 0., 1.));
    const TGswParams *params512_2 = new_TGswParams(3, 10, new_TLweParams(512, 2, plaintext_modulus, 0., 1.));
    const TGswParams *params1024_1 = new_TGswParams(3, 10, new_TLweParams(1024, 1, plaintext_modulus, 0., 1.));
    const TGswParams *params1024_2 = new_TGswParams(4, 8, new_TLweParams(1024, 2, plaintext_modulus, 0., 1.));
    const TGswParams *params2048_1 = new_TGswParams(4, 8, new_TLweParams(2048, 1, plaintext_modulus, 0., 1.));
    const TGswParams *params2048_2 = new_TGswParams(3, 10, new_TLweParams(2048, 2, plaintext_modulus, 0., 1.));
    vector<const TGswParams *> all_params = {params512_1, params512_2, params1024_1, params1024_2, params2048_1,
                                             params2048_2};
    vector<const TGswParams *> all_params1024 = {params1024_1, params1024_2};

    const TGswKey *key512_1 = new_random_key(params512_1);
    const TGswKey *key512_2 = new_random_key(params512_2);
    const TGswKey *key1024_1 = new_random_key(params1024_1);
    const TGswKey *key1024_2 = new_random_key(params1024_2);
    const TGswKey *key2048_1 = new_random_key(params2048_1);
    const TGswKey *key2048_2 = new_random_key(params2048_2);
    vector<const TGswKey *> all_keys = {key512_1, key512_2, key1024_1, key1024_2, key2048_1, key2048_2};
    vector<const TGswKey *> all_keys1024 = {key1024_1, key1024_2};

    /* Tolerance factor for the equality between two TorusPolynomial */
    //const double toler = 1e-8;



    /* This class fixture is for testing tgsw functions that entirely
     * do direct access to raw coefficients: no fake is used */
    //EXPORT void tGswAddMuH(TGswSample* result, const IntPolynomial* message, const TGswParams* params);
    //EXPORT void tGswAddMuIntH(TGswSample* result, const int32_t message, const TGswParams* params);
    //EXPORT void tGswTorus32PolynomialDecompH(IntPolynomial* result, const TorusPolynomial* sample, const TGswParams* params);
    //EXPORT void tGswTLweDecompH(IntPolynomial* result, const TLweSample* sample,const TGswParams* params);	
    class TGswDirectTest : public ::testing::Test {
    public:

        //this function will create a fixed (random-looking) TGsw sample
        inline void fullyRandomTGsw(TGswSample *result, double alpha, const TGswParams *params) {
            int32_t l = params->l;
            int32_t k = params->tlwe_params->k;

            for (int32_t i = 0; i <= k; i++) {
                for (int32_t j = 0; j < l; j++) {
                    TLweSample *row = &result->bloc_sample[i][j];
                    for (int32_t u = 0; u <= k; u++) {
                        torusPolynomialUniform(row->a + u);
                    }
                    row->current_variance = alpha * alpha;
                }
            }
        }
    };




#if 0
    /*
     * Definition of the function absfrac: | frac(d) |
     * Computes the absolute value of the fractional part of a double d 
     */
    double absfrac(double d) {return abs(d-rint(d));}


    /*
     * Definition of the function fillRandom
     * Fills a TLweSample with random Torus32 values (uniform distribution) 
     */
    void fillRandom(TLweSample* result, const TLweParams* params) {
    const int32_t k = params->k;
    const int32_t N = params->N;

    for (int32_t i = 0; i <= k; ++i)
        for (int32_t j = 0; j < N; ++j)
        result->a[i].coefsT[j] = uniformTorus32_distrib(generator);
    result->current_variance=0.2;
    }


    /*
     * Definition of the function copySample
     * Copies a TLweSample
     */
    void copySample(TLweSample* result, const TLweSample* sample, const TLweParams* params) {
    const int32_t k = params->k;
    const int32_t N = params->N;

    for (int32_t i = 0; i <= k; ++i)
        for (int32_t j = 0; j < N; ++j)
        result->a[i].coefsT[j] = sample->a[i].coefsT[j];
    result->current_variance=sample->current_variance;
    }
#endif




    /* ***************************************************************
     *************************** TESTS ********************************
     *************************************************************** */




    // ***************************************************************************************************************
    // TGswDirectTest will be used to do direct tests while the function to be tested is not compatible with the fakes
    // ***************************************************************************************************************

    //// Result += H
    //EXPORT void tGswAddH(TGswSample* result, const TGswParams* params);
    TEST_F(TGswDirectTest, tGswAddH) {
        for (const TGswParams *params: all_params) {
            TGswSample *s = new_TGswSample(params);
            TGswSample *stemp = new_TGswSample(params);
            int32_t kpl = params->kpl;
            int32_t l = params->l;
            int32_t k = params->tlwe_params->k;
            int32_t N = params->tlwe_params->N;
            Torus32 *h = params->h;
            double alpha = 4.2; // valeur pseudo aleatoire fixé

            // make a full random TGSW
            fullyRandomTGsw(s, alpha, params);

            // copy s to stemp
            for (int32_t i = 0; i < kpl; ++i) {
                tLweCopy(&stemp->all_sample[i], &s->all_sample[i], params->tlwe_params);
            }

            tGswAddH(s, params);

            //verify all coefficients
            for (int32_t bloc = 0; bloc <= k; bloc++) {
                for (int32_t i = 0; i < l; ++i) {
                    ASSERT_EQ(s->bloc_sample[bloc][i].current_variance, stemp->bloc_sample[bloc][i].current_variance);
                    for (int32_t u = 0; u <= k; u++) {
                        //verify that pol[bloc][i][u]=initial[bloc][i][u]+(bloc==u?hi:0)
                        TorusPolynomial *newpol = &s->bloc_sample[bloc][i].a[u];
                        TorusPolynomial *oldpol = &stemp->bloc_sample[bloc][i].a[u];
                        ASSERT_EQ(newpol->coefsT[0], oldpol->coefsT[0] + (bloc == u ? h[i] : 0));
                        for (int32_t j = 1; j < N; j++)
                            ASSERT_EQ(newpol->coefsT[j], oldpol->coefsT[j]);
                    }
                }
            }

            delete_TGswSample(stemp);
            delete_TGswSample(s);
        }
    }


    //// Result += mu*H
    //EXPORT void tGswAddMuH(TGswSample* result, const IntPolynomial* message, const TGswParams* params);
    TEST_F(TGswDirectTest, tGswAddMuH) {
        for (const TGswParams *params: all_params) {
            TGswSample *s = new_TGswSample(params);
            TGswSample *stemp = new_TGswSample(params);
            int32_t kpl = params->kpl;
            int32_t l = params->l;
            int32_t k = params->tlwe_params->k;
            int32_t N = params->tlwe_params->N;
            Torus32 *h = params->h;
            double alpha = 4.2; // valeur pseudo aleatoire fixé
            IntPolynomial *mess = new_random_IntPolynomial(N);

            // make a full random TGSW
            fullyRandomTGsw(s, alpha, params);


            // copy s to stemp
            for (int32_t i = 0; i < kpl; ++i) {
                tLweCopy(&stemp->all_sample[i], &s->all_sample[i], params->tlwe_params);
            }

            tGswAddMuH(s, mess, params);

            //verify all coefficients
            for (int32_t bloc = 0; bloc <= k; bloc++) {
                for (int32_t i = 0; i < l; ++i) {
                    ASSERT_EQ(s->bloc_sample[bloc][i].current_variance, stemp->bloc_sample[bloc][i].current_variance);
                    for (int32_t u = 0; u <= k; u++) {
                        //verify that pol[bloc][i][u]=initial[bloc][i][u]+(bloc==u?hi*mess:0)
                        TorusPolynomial *newpol = &s->bloc_sample[bloc][i].a[u];
                        TorusPolynomial *oldpol = &stemp->bloc_sample[bloc][i].a[u];
                        if (bloc == u) {
                            for (int32_t j = 0; j < N; j++)
                                ASSERT_EQ(newpol->coefsT[j], oldpol->coefsT[j] + h[i] * mess->coefs[j]);
                        } else {
                            for (int32_t j = 0; j < N; j++)
                                ASSERT_EQ(newpol->coefsT[j], oldpol->coefsT[j]);
                        }
                    }
                }
            }

            delete_IntPolynomial(mess);
            delete_TGswSample(stemp);
            delete_TGswSample(s);
        }
    }




    //// Result += mu*H, mu integer
    //EXPORT void tGswAddMuIntH(TGswSample* result, const int32_t message, const TGswParams* params);
    TEST_F(TGswDirectTest, tGswAddMuIntH) {
        for (const TGswParams *params: all_params) {
            TGswSample *s = new_TGswSample(params);
            TGswSample *stemp = new_TGswSample(params);
            int32_t kpl = params->kpl;
            int32_t l = params->l;
            int32_t k = params->tlwe_params->k;
            int32_t N = params->tlwe_params->N;
            Torus32 *h = params->h;
            double alpha = 4.2; // valeur pseudo aleatoire fixé
            int32_t mess = rand() * 2345 - 1234;

            // make a full random TGSW
            fullyRandomTGsw(s, alpha, params);


            // copy s to stemp
            for (int32_t i = 0; i < kpl; ++i) {
                tLweCopy(&stemp->all_sample[i], &s->all_sample[i], params->tlwe_params);
            }

            tGswAddMuIntH(s, mess, params);

            //verify all coefficients
            for (int32_t bloc = 0; bloc <= k; bloc++) {
                for (int32_t i = 0; i < l; ++i) {
                    ASSERT_EQ(s->bloc_sample[bloc][i].current_variance, stemp->bloc_sample[bloc][i].current_variance);
                    for (int32_t u = 0; u <= k; u++) {
                        //verify that pol[bloc][i][u]=initial[bloc][i][u]+(bloc==u?hi*mess:0)
                        TorusPolynomial *newpol = &s->bloc_sample[bloc][i].a[u];
                        TorusPolynomial *oldpol = &stemp->bloc_sample[bloc][i].a[u];
                        ASSERT_EQ(newpol->coefsT[0], oldpol->coefsT[0] + (bloc == u ? h[i] * mess : 0));
                        for (int32_t j = 1; j < N; j++)
                            ASSERT_EQ(newpol->coefsT[j], oldpol->coefsT[j]);
                    }
                }
            }

            delete_TGswSample(stemp);
            delete_TGswSample(s);
        }
    }










    //EXPORT void tGswTorus32PolynomialDecompH(IntPolynomial* result, const TorusPolynomial* sample, const TGswParams* params);
    // sample: torus polynomial with N coefficients
    // result: int32_t polynomial with Nl coefficients
    TEST_F(TGswDirectTest, tGswTorus32PolynomialDecompH) {
        for (const TGswParams *param: all_params) {
            int32_t N = param->tlwe_params->N;
            int32_t l = param->l;
            int32_t Bgbit = param->Bgbit;
            Torus32 *h = param->h;

            //compute the tolerance
            int32_t toler = 0;
            if (Bgbit * l < 32) toler = 1 << (32 - Bgbit * l);
            //printf("%d,%d,%d\n",Bgbit,l,toler);

            IntPolynomial *result = new_IntPolynomial_array(l, N);
            TorusPolynomial *sample = new_TorusPolynomial(N);
            torusPolynomialUniform(sample);

            tGswTorus32PolynomialDecompH(result, sample, param);


            for (int32_t i = 0; i < N; ++i) {
                // recomposition
                Torus32 test = 0;
                for (int32_t j = 0; j < l; ++j) {
                    test += result[j].coefs[i] * h[j];
                }
                ASSERT_LE(abs(test - sample->coefsT[i]), toler); //exact or approx decomposition
            }

            delete_TorusPolynomial(sample);
            delete_IntPolynomial_array(l, result);
        }
    }



    //EXPORT void tGswTLweDecompH(IntPolynomial* result, const TLweSample* sample,const TGswParams* params);	
    // Test direct Result*H donne le bon resultat
    // sample: TLweSample composed by k+1 torus polynomials, each with N coefficients
    // result: int32_t polynomial with Nl(k+1) coefficients
    TEST_F(TGswDirectTest, tGswTLweDecompH) {
        for (const TGswParams *param: all_params) {
            int32_t N = param->tlwe_params->N;
            int32_t k = param->tlwe_params->k;
            int32_t Bgbit = param->Bgbit;
            int32_t l = param->l;
            int32_t kpl = param->kpl;
            Torus32 *h = param->h;

            //compute the tolerance
            int32_t toler = 0;
            if (Bgbit * l < 32) toler = 1 << (32 - Bgbit * l);
            //printf("%d,%d,%d\n",Bgbit,l,toler);

            IntPolynomial *result = new_IntPolynomial_array(kpl, N);
            TLweSample *sample = new_TLweSample(param->tlwe_params);

            // sample randomly generated
            for (int32_t bloc = 0; bloc <= k; ++bloc) {
                torusPolynomialUniform(&sample->a[bloc]);
            }

            tGswTLweDecompH(result, sample, param);

            for (int32_t bloc = 0; bloc <= k; ++bloc) {
                for (int32_t i = 0; i < N; i++) {
                    Torus32 test = 0;
                    for (int32_t j = 0; j < l; ++j) {
                        test += result[bloc * l + j].coefs[i] * h[j];
                    }
                    ASSERT_LE(abs(test - sample->a[bloc].coefsT[i]), toler); //exact or approx decomposition
                }
            }

            delete_TLweSample(sample);
            delete_IntPolynomial_array(kpl, result);
        }
    }



    //EXPORT void tGswSymEncrypt(TGswSample* result, const IntPolynomial* message, double alpha, const TGswKey* key);
    TEST_F(TGswDirectTest, tGswSymEncrypt) {
#if MUL_TYPE == MUL_TYPE_FFT // FFT ONLY SUPPORT N=1024
        for (const TGswKey *key: all_keys1024) {
#else
        for (const TGswKey *key: all_keys) {
#endif
            int32_t N = key->params->tlwe_params->N;
            TGswSample *s = new_TGswSample(key->params);
            IntPolynomial *mess = new_random_IntPolynomial(N);
            IntPolynomial *dechifMess = new_IntPolynomial(N);
            double alpha = 1e-6; // valeur pseudo aleatoire fixé

            tGswSymEncrypt(s, mess, alpha, key);
#if 1
            tGswSymDecrypt(dechifMess, s, key, plaintext_modulus);

            for (int32_t j = 0; j < N; j++)
                ASSERT_EQ((mess->coefs[j] - dechifMess->coefs[j]) % plaintext_modulus, 0);
#endif

            delete_IntPolynomial(mess);
            delete_IntPolynomial(dechifMess);
            delete_TGswSample(s);
        }
    }



    // ILA: Not used for now
    /** result = result + sample */
    //EXPORT void tGswAddTo(TGswSample* result, const TGswSample* sample, const TGswParams* params);
    /** result = result - sample */
    //EXPORT void tGswSubTo(TLweSample* result, const TLweSample* sample, const TLweParams* params);
    /** result = result + p.sample */
    //EXPORT void tGswAddMulTo(TLweSample* result, int32_t p, const TLweSample* sample, const TLweParams* params);
    /** result = result - p.sample */
    //EXPORT void tGswSubMulTo(TLweSample* result, int32_t p, const TLweSample* sample, const TLweParams* params);

}//namespace

