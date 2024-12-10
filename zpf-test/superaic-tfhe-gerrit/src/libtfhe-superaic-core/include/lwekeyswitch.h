#ifndef LWEKEYSWITCH_H
#define LWEKEYSWITCH_H

///@file
///@brief This file contains the declaration of LWE keyswitch structures

#include "tfhe_superaic_torus.h"
#include "lweparams.h"
#include "lwesamples.h"

struct LweKeySwitchKey {
    int32_t n; ///< length of the input key: s'
    int32_t t; ///< decomposition length
    int32_t basebit; ///< log_2(base)
    int32_t base; ///< decomposition base: a power of 2 
    const LweParams* out_params; ///< params of the output key s 
    LweSample* ks0_raw; //tableau qui contient tout les Lwe samples de taille nlbase  百度翻译：包含NLBase大小的所有LWE示例的表
    LweSample** ks1_raw;// de taille nl  pointe vers un tableau ks0_raw dont les cases sont espaceés de base positions  百度翻译：大小为NL的表指向KS0-RAW，其框与基本位置间隔开
    LweSample*** ks; ///< the keyswitch elements: a n.l.base matrix
    // de taille n pointe vers ks1 un tableau dont les cases sont espaceés de ell positions  百度翻译：大小为N的表指向KS1，其框间隔ELL位置

#ifdef __cplusplus
    LweKeySwitchKey(int32_t n, int32_t t, int32_t basebit, const LweParams* out_params, LweSample* ks0_raw);
    ~LweKeySwitchKey();
    LweKeySwitchKey(const LweKeySwitchKey&) = delete;
    void operator=(const LweKeySwitchKey&) = delete;
#endif
};

//allocate memory space for a LweKeySwitchKey
EXPORT LweKeySwitchKey* alloc_LweKeySwitchKey();
EXPORT LweKeySwitchKey* alloc_LweKeySwitchKey_array(int32_t nbelts);

//free memory space for a LweKeySwitchKey
EXPORT void free_LweKeySwitchKey(LweKeySwitchKey* ptr);
EXPORT void free_LweKeySwitchKey_array(int32_t nbelts, LweKeySwitchKey* ptr);

//initialize the LweKeySwitchKey structure
//(equivalent of the C++ constructor)
EXPORT void init_LweKeySwitchKey(LweKeySwitchKey* obj, int32_t n, int32_t t, int32_t basebit, const LweParams* out_params);
EXPORT void init_LweKeySwitchKey_array(int32_t nbelts, LweKeySwitchKey* obj, int32_t n, int32_t t, int32_t basebit, const LweParams* out_params);

//destroys the LweKeySwitchKey structure
//(equivalent of the C++ destructor)
EXPORT void destroy_LweKeySwitchKey(LweKeySwitchKey* obj);
EXPORT void destroy_LweKeySwitchKey_array(int32_t nbelts, LweKeySwitchKey* obj);

//allocates and initialize the LweKeySwitchKey structure
//(equivalent of the C++ new)
EXPORT LweKeySwitchKey* new_LweKeySwitchKey(int32_t n, int32_t t, int32_t basebit, const LweParams* out_params);
EXPORT LweKeySwitchKey* new_LweKeySwitchKey_array(int32_t nbelts, int32_t n, int32_t t, int32_t basebit, const LweParams* out_params);

//destroys and frees the LweKeySwitchKey structure
//(equivalent of the C++ delete)
EXPORT void delete_LweKeySwitchKey(LweKeySwitchKey* obj);
EXPORT void delete_LweKeySwitchKey_array(int32_t nbelts, LweKeySwitchKey* obj);

#endif // LWEKEYSWITCH_H
