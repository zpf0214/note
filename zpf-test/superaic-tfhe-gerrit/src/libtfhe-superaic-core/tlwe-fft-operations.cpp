#ifndef TFHE_TEST_ENVIRONMENT
/* ***************************************************
   TLWE fft operations
 *************************************************** */

#include <random>
#include <cassert>
#include "tfhe_superaic_torus.h"
// #include "numeric_functions.h"
#include "lweparams.h"
#include "lwekey.h"
#include "lwesamples.h"
#include "lwe-functions.h"
#include "tlwe_functions.h"
#include "tgsw_functions.h"
#include "polynomials_arithmetic.h"
#include "lagrangehalfc_arithmetic.h"

using namespace std;
#define INCLUDE_ALL

#else
#undef EXPORT
#define EXPORT
#endif


#if defined INCLUDE_ALL || defined INCLUDE_INIT_TLWESAMPLE_FFT
#undef INCLUDE_INIT_TLWESAMPLE_FFT
EXPORT void init_TLweSampleFFT(TLweSampleFFT *obj, const TLweParams *params) {
    //a is a table of k+1 polynomials, b is an alias for &a[k]
    const int32_t k = params->k;
    LagrangeHalfCPolynomial *a = new_LagrangeHalfCPolynomial_array(k + 1, params->N);
    double current_variance = 0;
    new(obj) TLweSampleFFT(params, a, current_variance);
}
#endif

#if defined INCLUDE_ALL || defined INCLUDE_DESTROY_TLWESAMPLE_FFT
#undef INCLUDE_DESTROY_TLWESAMPLE_FFT
EXPORT void destroy_TLweSampleFFT(TLweSampleFFT *obj) {
    const int32_t k = obj->k;
    delete_LagrangeHalfCPolynomial_array(k + 1, obj->a);
    obj->~TLweSampleFFT();
}
#endif


#if defined INCLUDE_ALL || defined INCLUDE_TLWE_TO_FFT_CONVERT
#undef INCLUDE_TLWE_TO_FFT_CONVERT
// Computes the inverse FFT of the coefficients of the TLWE sample
EXPORT void tLweToFFTConvert(TLweSampleFFT *result, const TLweSample *source, const TLweParams *params) {
    const int32_t k = params->k;

    for (int32_t i = 0; i <= k; ++i)
        TorusPolynomial_ifft(result->a + i, source->a + i);
    result->current_variance = source->current_variance;
}
#endif


#if defined INCLUDE_ALL || defined INCLUDE_TLWE_FROM_FFT_CONVERT
#undef INCLUDE_TLWE_FROM_FFT_CONVERT
// Computes the FFT of the coefficients of the TLWEfft sample
EXPORT void tLweFromFFTConvert(TLweSample *result, const TLweSampleFFT *source, const TLweParams *params) {
    const int32_t k = params->k;

    for (int32_t i = 0; i <= k; ++i)
        TorusPolynomial_fft(result->a + i, source->a + i);
    result->current_variance = source->current_variance;
}
#endif


#if defined INCLUDE_ALL || defined INCLUDE_TLWE_FFT_CLEAR
#undef INCLUDE_TLWE_FFT_CLEAR
//Arithmetic operations on TLwe samples
/** result = (0,0) */
EXPORT void tLweFFTClear(TLweSampleFFT *result, const TLweParams *params) {
    int32_t k = params->k;

    for (int32_t i = 0; i <= k; ++i)
        LagrangeHalfCPolynomialClear(&result->a[i]);
    result->current_variance = 0.;
}
#endif


#if defined INCLUDE_ALL || defined INCLUDE_TLWE_FFT_ADDMULRTO
#undef INCLUDE_TLWE_FFT_ADDMULRTO
// result = result + p*sample
EXPORT void tLweFFTAddMulRTo(TLweSampleFFT *result, const LagrangeHalfCPolynomial *p, const TLweSampleFFT *sample,
                             const TLweParams *params) {
    const int32_t k = params->k;

    for (int32_t i = 0; i <= k; i++)
        LagrangeHalfCPolynomialAddMul(result->a + i, p, sample->a + i);
    //result->current_variance += sample->current_variance; 
    //TODO: how to compute the variance correctly?
}
#endif


//autogenerated memory functions (they will always be included, even in
//tests)

USE_DEFAULT_CONSTRUCTOR_DESTRUCTOR_IMPLEMENTATIONS1(TLweSampleFFT, TLweParams);

#undef INCLUDE_ALL
