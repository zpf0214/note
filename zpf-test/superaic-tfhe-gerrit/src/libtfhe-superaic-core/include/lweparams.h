#ifndef LWEPARAMS_H
#define LWEPARAMS_H

///@file
///@brief This file contains the declaration of lwe parameters structures

#include "tfhe_superaic_torus.h"

//this structure contains Lwe parameters
//this structure is constant (cannot be modified once initialized): 
//the pointer to the param can be passed directly
//to all the Lwe keys that use these params.
//TODO: 当我们有了config，是否还需要这个参数？
//TODO: 是否需要添加message space的信息？
struct LweParams {
	const int32_t n;
    const int32_t plaintext_modulus;
	const double alpha_min;//le plus petit bruit tq sur
	const double alpha_max;//le plus gd bruit qui permet le déchiffrement



//since all members are declared constant, a constructor is 
//required in the structure.
//TODO: 如果想要直接从config中构造LweParams，可以在此基础上添加
//委托构造函数
#ifdef __cplusplus
	void print(void) const;
	LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
	~LweParams();
	LweParams(const LweParams&) = delete; //forbidden
	LweParams& operator=(const LweParams& ) = delete; //forbidden
#endif
};

//allocate memory space for a LweParams
EXPORT LweParams* alloc_LweParams();
EXPORT LweParams* alloc_LweParams_array(int32_t nbelts);

//free memory space for a LweParams
EXPORT void free_LweParams(LweParams* ptr);
EXPORT void free_LweParams_array(int32_t nbelts, LweParams* ptr);

//initialize the LweParams structure
//(equivalent of the C++ constructor)
EXPORT void init_LweParams(LweParams* obj, int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void init_LweParams_array(int32_t nbelts, LweParams* obj, int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);

//destroys the LweParams structure
//(equivalent of the C++ destructor)
EXPORT void destroy_LweParams(LweParams* obj);
EXPORT void destroy_LweParams_array(int32_t nbelts, LweParams* obj);
 
//allocates and initialize the LweParams structure
//(equivalent of the C++ new)
//EXPORT LweParams* new_LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT LweParams* new_LweParams_array(int32_t nbelts, int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);

//destroys and frees the LweParams structure
//(equivalent of the C++ delete)
//EXPORT void delete_LweParams(LweParams* obj);
EXPORT void delete_LweParams_array(int32_t nbelts, LweParams* obj);

std::shared_ptr<LweParams> new_LweParams_shared(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
#endif //LWEPARAMS_H
