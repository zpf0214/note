#include <stdio.h>
#include "tfhe_superaic_torus.h"
#include "tlwe.h"
#include "polynomials.h"

//struct TLweParams {
//    const int32_t k; //number of polynomials in the mask
//    const int32_t N; //a power of 2: degree of the polynomials
//    const double alpha_min;
//    const double alpha_max;
//};

TLweParams::TLweParams(int32_t N, int32_t k, int32_t plaintext_modulus, double alpha_min, double alpha_max) :
        N(N),
        k(k),
        plaintext_modulus(plaintext_modulus),
        alpha_min(alpha_min),
        alpha_max(alpha_max),
        extracted_lweparams(N * k, plaintext_modulus, alpha_min, alpha_max) {}

TLweParams::~TLweParams() {}

void TLweParams::print(void) const {
    printf("TLweParams\n");
    printf("k:%d\n",k);
    printf("k:%d\n",k);
    printf("alpha_min:%f\n",alpha_min);
    printf("alpha_max:%f\n",alpha_max);
    printf("extracted_lweparams:\n");
    extracted_lweparams.print();
}
//struct TLweKey {
//    const TLweParams* params;
//    IntPolynomial* key;
//};

TLweKey::TLweKey(const TLweParams *params) :
    params_shared(nullptr),  params(params) {
    key = new_IntPolynomial_array(params->k, params->N);
}

TLweKey::TLweKey(std::shared_ptr<TLweParams> params) : 
    params_shared(params), params(params_shared.get()){
    key = new_IntPolynomial_array(params->k, params->N);
}



TLweKey::~TLweKey() {
    delete_IntPolynomial_array(params->k, key);
}

void TLweKey::print(void) const {
    printf("TLweKey:\n");
    printf("params:\n");
    params->print();
    printf("key:\n");
    key->print();

}

void TLweSample::print(void) const {
    for( int i = 0 ; i < k ; ++i) {
        printf("a[%d]:\t", i);
        a[i].print(true);
        printf("\n");
    }
    printf("b:\t");
    a[k].print(true);
    printf("\n");
}

//struct TLweSample {
//    TorusPolynomial* a;
//    TorusPolynomial* b;
//    double current_variance;
//};

TLweSample::TLweSample(const TLweParams *params) : k(params->k) {
    //Small change here: 
    //a is a table of k+1 polynomials, b is an alias for &a[k]
    //like that, we can access all the coefficients as before:
    //  &sample->a[0],...,&sample->a[k-1]  and &sample->b
    //or we can also do it in a single for loop
    //  &sample->a[0],...,&sample->a[k]
    a = new_TorusPolynomial_array(k + 1, params->N);
    b = a + k;
    current_variance = 0;
}

TLweSample::TLweSample(std::shared_ptr<TLweParams> params) : k(params->k)  {
    //Small change here: 
    //a is a table of k+1 polynomials, b is an alias for &a[k]
    //like that, we can access all the coefficients as before:
    //  &sample->a[0],...,&sample->a[k-1]  and &sample->b
    //or we can also do it in a single for loop
    //  &sample->a[0],...,&sample->a[k]
    a = new_TorusPolynomial_array(k + 1, params->N);
    b = a + k;
    current_variance = 0;
}

TLweSample::~TLweSample() {
    delete_TorusPolynomial_array(k + 1, a);
}

TLweSampleFFT::TLweSampleFFT(const TLweParams *params, LagrangeHalfCPolynomial *arr, double current_variance) : k(
        params->k) {
    //a is a table of k+1 polynomials, b is an alias for &a[k]
    a = arr;
    b = a + k;
    current_variance = 0;
}



TLweSampleFFT::~TLweSampleFFT() {
}

TLweParams *new_TLweParams(int32_t N, int32_t k, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_TLweParams(TLweParams *obj);


void _delete_TLweParams(TLweParams * p) {
    delete_TLweParams(p);
}


std::shared_ptr<TLweParams> new_TLweParams_shared(int32_t N, int32_t k, int32_t plaintext_modulus, double alpha_min, double alpha_max){
	std::shared_ptr<TLweParams>  p (new_TLweParams(N,k,plaintext_modulus,alpha_min,alpha_max),_delete_TLweParams);
	return p;

}

//////////////////////////////////////////////////////////////

EXPORT TLweKey* alloc_TLweKey() { 
    return (TLweKey*) malloc(sizeof(TLweKey)); 
} 

EXPORT TLweKey* alloc_TLweKey_array(int32_t nbelts) { 
    return (TLweKey*) malloc(nbelts*sizeof(TLweKey)); 
} 
    /*free memory */ \
EXPORT void free_TLweKey(TLweKey* ptr) { 
    free(ptr); 
} 

EXPORT void free_TLweKey_array(int32_t nbelts, TLweKey* ptr) { 
    free(ptr); 
} 
    /* init array */ \
EXPORT void init_TLweKey_array(int32_t nbelts, TLweKey* obj, const TLweParams* params) { 
    for (int32_t ii=0; ii<nbelts; ++ii) init_TLweKey(obj+ii,params); 
} 

    /* destroy array */ \
EXPORT void destroy_TLweKey_array(int32_t nbelts, TLweKey* obj) { 
    for (int32_t ii=0; ii<nbelts; ++ii) destroy_TLweKey(obj+ii); 
} 
    /* alloc+init */ \
TLweKey* new_TLweKey(const TLweParams* params) { 
    TLweKey* reps = alloc_TLweKey(); 
    init_TLweKey(reps, params); 
    return reps; 
} 

TLweKey* new_TLweKey(const std::shared_ptr<TLweParams> params) { 
    TLweKey* reps = alloc_TLweKey(); 
    init_TLweKey(reps, params); 
    return reps; 
} 


EXPORT TLweKey* new_TLweKey_array(int32_t nbelts, const TLweParams* params) { 
    TLweKey* reps = alloc_TLweKey_array(nbelts); 
    init_TLweKey_array(nbelts, reps, params); 
    return reps; 
} 

    /* destroy+free */ \
EXPORT void delete_TLweKey(TLweKey* obj) { 
    destroy_TLweKey(obj); 
    free_TLweKey(obj); 
} 

EXPORT void delete_TLweKey_array(int32_t nbelts, TLweKey* obj) { 
    destroy_TLweKey_array(nbelts, obj); 
    free_TLweKey_array(nbelts, obj); 
}

void _delete_tlwekey(TLweKey * p) {
    delete_TLweKey(p);
}

std::shared_ptr<TLweKey> new_TLweKey_shared(const TLweParams * params) {
    std::shared_ptr<TLweKey>  p (new_TLweKey(params),_delete_tlwekey);
    return p;
}


std::shared_ptr<TLweKey> new_TLweKey_shared(std::shared_ptr<TLweParams>  params) {
    std::shared_ptr<TLweKey>  p (new_TLweKey(params),_delete_tlwekey);
    return p;
}


//////////////////////////////////////////////////////////////


EXPORT TLweSample* alloc_TLweSample() { 
    return (TLweSample*) malloc(sizeof(TLweSample)); 
} 
    
EXPORT TLweSample* alloc_TLweSample_array(int32_t nbelts) { 
    return (TLweSample*) malloc(nbelts*sizeof(TLweSample)); 
} 
    /*free memory */ \
EXPORT void free_TLweSample(TLweSample* ptr) { 
    free(ptr); 
} 

EXPORT void free_TLweSample_array(int32_t nbelts, TLweSample* ptr) { 
    free(ptr); 
} 

EXPORT void init_TLweSample_array(int32_t nbelts, TLweSample* obj, const TLweParams* params) { 
    for (int32_t ii=0; ii<nbelts; ++ii) init_TLweSample(obj+ii,params); 
} 

EXPORT void destroy_TLweSample_array(int32_t nbelts, TLweSample* obj) { 
    for (int32_t ii=0; ii<nbelts; ++ii) destroy_TLweSample(obj+ii); 
} 
    /* alloc+init */ 
TLweSample* new_TLweSample(const TLweParams* params) { 
    TLweSample* reps = alloc_TLweSample(); 
    init_TLweSample(reps, params); 
    return reps; 
} 

TLweSample* new_TLweSample(std::shared_ptr<TLweParams> params) { 
    TLweSample* reps = alloc_TLweSample(); 
    init_TLweSample(reps, params); 
    return reps; 
} 

EXPORT TLweSample* new_TLweSample_array(int32_t nbelts, const TLweParams* params) { 
    TLweSample* reps = alloc_TLweSample_array(nbelts); 
    init_TLweSample_array(nbelts, reps, params); 
    return reps; 
} 
    /* destroy+free */ \
EXPORT void delete_TLweSample(TLweSample* obj) { 
    destroy_TLweSample(obj); 
    free_TLweSample(obj); 
} 

EXPORT void delete_TLweSample_array(int32_t nbelts, TLweSample* obj) { \
    destroy_TLweSample_array(nbelts, obj); \
    free_TLweSample_array(nbelts, obj); \
}

void _delete_TLweSample(TLweSample * p) {
    delete_TLweSample(p);
}

std::shared_ptr<TLweSample> new_TLweSample_shared(const TLweParams* params) {
	std::shared_ptr<TLweSample>  p (new_TLweSample(params),_delete_TLweSample);
	return p;

}

std::shared_ptr<TLweSample> new_TLweSample_shared(std::shared_ptr<TLweParams> params) {
	std::shared_ptr<TLweSample>  p (new_TLweSample(params),_delete_TLweSample);
	return p;

}