#ifndef _TORUS_H_
#define _TORUS_H_

#include <memory>

#include <stdint.h>
#include <stddef.h>

//not very important, but all the functions exported in the output library
//should use the C naming convention, for inter-compiler compatibility
//reasons, or to bind it with non C++ code.
#ifdef __cplusplus
#define EXPORT extern "C"
//#include "tfhe_generic_templates.h"
#else
#define EXPORT
#endif



// Idea:
// we may want to represent an element x of the real torus by 
// the integer rint(2^32.x) modulo 2^32
//  -- addition, subtraction and integer combinations are native operation
//  -- modulo 1 is mapped to mod 2^32, which is also native!
// This looks much better than using float/doubles, where modulo 1 is not
// natural at all.
typedef int32_t Torus32; //avant uint32_t
//typedef int64_t Torus64; //avant uint64_t
typedef Torus32 cipher_word;


#ifdef __cplusplus
#include <random>
extern std::default_random_engine generator;
extern std::uniform_int_distribution<Torus32> uniformTorus32_distrib;
extern bool _use_fix_random; // 在测试用例中，随机数的值是固定为0，用来验证算法
static const int64_t _two31 = INT64_C(1) << 31; // 2^31
static const int64_t _two32 = INT64_C(1) << 32; // 2^32
static const double _two32_double = _two32;
static const double _two31_double = _two31;
#endif


EXPORT void die_dramatically(const char* message);


/** 
 * modular gaussian distribution of standard deviation sigma centered on
 * the message, on the Torus32
 * 把高斯噪声加到message上。
 * 由于message的模是2^32，因此会造成误差。
 * 为了避免在解密的时候产生误差，要限制message的模，在这个库中，这属于编码范畴，需要用户自己实现。
 * 和rust版本的实现是有区别的。
 */ 
EXPORT Torus32 gaussian32(Torus32 message, double sigma);

/** 
 * conversion from double to Torus32 0.5 is 0x80000000 
 * 把【-0.5～0.5）的映射到int_32上，超过的部分会从0x80000000开始绕回，
 * 所以0.5就是0x80000000,和 -0.5是一样的
 */
EXPORT Torus32 dtot32(double d);

/** conversion from Torus32 to double  【-0.5,0.5)
 * 0x80000000 is -0.5  */
EXPORT double t32tod(Torus32 x);


/**
 *  Used to approximate the phase to the nearest multiple of  1/plaintext_modulus 
 */
EXPORT Torus32 approxPhase(Torus32 phase, int32_t plaintext_modulus);

/**
 *  computes rountToNearestInteger(plaintext_modulus*phase)
 *  plaintext_modulus 是新的模的，fnhu 返回结果的取值范围是从 [-plaintext_modulus/2,plaintext_modulus/2), 
 *  要注意的是如果返回的是负数，要进行符号位扩展。
 *  比如plaintext_modulus = 16， 那么bit3是符号位，当返回-4的时候，实际返回值是0x0000000c
 *  其中c是-4在4bit长度下的补码，bit3是符号位。要把bit3一直扩展到最高位，变成0xfffffffc,才是int32下的-4
 *  
 */
EXPORT int32_t modSwitchFromTorus32(Torus32 phase, int32_t plaintext_modulus);

EXPORT int32_t modSwitchFromTorus32_old(Torus32 phase, int32_t plaintext_modulus);

/**
 *  converts mu/plaintext_modulus to a Torus32 for mu in [0,plaintext_modulus[
 */
EXPORT Torus32 modSwitchToTorus32(int32_t mu, int32_t plaintext_modulus);



EXPORT size_t superaic_get_cipher_word_size(void);




struct LweParams;
struct LweKey;
struct LweSample;
struct LweKeySwitchKey;
struct TLweParams;
struct TLweKey;
struct TLweSample;
struct TLweSampleFFT;
struct TGswParams;
struct TGswKey;
struct TGswSample;
struct TGswSampleFFT;
struct LweBootstrappingKey;
struct LweBootstrappingKeyFFT;
struct IntPolynomial;
struct TorusPolynomial;
struct LagrangeHalfCPolynomial;
struct TFheGateBootstrappingParameterSet;
struct TFheGateBootstrappingCloudKeySet;
struct TFheGateBootstrappingSecretKeySet;
//this is for compatibility with C code, to be able to use
//"LweParams" as a type and not "struct LweParams"
typedef struct LweParams           LweParams;
typedef struct LweKey              LweKey;
typedef struct LweSample           LweSample;
typedef struct LweKeySwitchKey     LweKeySwitchKey;
typedef struct TLweParams       TLweParams;
typedef struct TLweKey          TLweKey;
typedef struct TLweSample       TLweSample;
typedef struct TLweSampleFFT       TLweSampleFFT;
typedef struct TGswParams       TGswParams;
typedef struct TGswKey          TGswKey;
typedef struct TGswSample       TGswSample;
typedef struct TGswSampleFFT       TGswSampleFFT;
typedef struct LweBootstrappingKey LweBootstrappingKey;
typedef struct LweBootstrappingKeyFFT LweBootstrappingKeyFFT;
typedef struct IntPolynomial	   IntPolynomial;
typedef struct TorusPolynomial	   TorusPolynomial;
typedef struct LagrangeHalfCPolynomial	   LagrangeHalfCPolynomial;
typedef struct TFheGateBootstrappingParameterSet TFheGateBootstrappingParameterSet;
typedef struct TFheGateBootstrappingCloudKeySet TFheGateBootstrappingCloudKeySet;
typedef struct TFheGateBootstrappingSecretKeySet TFheGateBootstrappingSecretKeySet;

#endif
