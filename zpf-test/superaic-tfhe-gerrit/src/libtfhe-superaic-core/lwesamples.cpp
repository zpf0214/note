#include "lwesamples.h"
#include "lweparams.h"


LweSample::LweSample(const LweParams* params)
{
    n = params->n;
	this->a = new Torus32[params->n];
    this->b = 0;
    this->current_variance = 0.;
}

LweSample::LweSample(const std::shared_ptr<LweParams> params) {
    n = params->n;
	this->a = new Torus32[params->n];
    this->b = 0;
    this->current_variance = 0.;
}

LweSample::~LweSample() {
    delete[] a;
}

void LweSample::print(){
    printf("a:\n");
    for(int i = 0;i < n;i ++) {
        printf("0x%08x ",a[i]);
        if(i % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    printf("b : 0x%08x\n",b);
}

EXPORT LweSample* alloc_LweSample() { 
    return (LweSample*) malloc(sizeof(LweSample)); 
}

EXPORT LweSample* alloc_LweSample_array(int32_t nbelts) { 
    return (LweSample*) malloc(nbelts*sizeof(LweSample)); 
}

EXPORT void free_LweSample(LweSample* ptr) { 
    free(ptr); 
}

EXPORT void free_LweSample_array(int32_t nbelts, LweSample* ptr) { 
    free(ptr); 
}

EXPORT void init_LweSample_array(int32_t nbelts, LweSample* obj, const LweParams* params) { 
    for (int32_t ii=0; ii<nbelts; ++ii) init_LweSample(obj+ii,params); 
}

EXPORT void destroy_LweSample_array(int32_t nbelts, LweSample* obj) { 
    for (int32_t ii=0; ii<nbelts; ++ii) destroy_LweSample(obj+ii); 
}

LweSample* new_LweSample(const LweParams* params) { 
    LweSample* reps = alloc_LweSample(); 
    init_LweSample(reps, params); 
    return reps; 
}

LweSample* new_LweSample(const std::shared_ptr<LweParams> params) { 
    LweSample* reps = alloc_LweSample(); 
    init_LweSample(reps, params); 
    return reps; 
}

EXPORT LweSample* new_LweSample_array(int32_t nbelts, const LweParams* params) { 
    LweSample* reps = alloc_LweSample_array(nbelts); 
    init_LweSample_array(nbelts, reps, params); 
    return reps; 
}

void delete_LweSample(LweSample* obj) { 
    destroy_LweSample(obj); 
    free_LweSample(obj); 
}

EXPORT void delete_LweSample_array(int32_t nbelts, LweSample* obj) { 
    destroy_LweSample_array(nbelts, obj); 
    free_LweSample_array(nbelts, obj); 
}


void _delete_LweSample(LweSample * p) {
    delete_LweSample(p);
}

std::shared_ptr<LweSample> new_LweSample_shared(const LweParams* params) {
	std::shared_ptr<LweSample>  p (new_LweSample(params),_delete_LweSample);
	return p;

}

std::shared_ptr<LweSample> new_LweSample_shared(const std::shared_ptr<LweParams> params) {
	std::shared_ptr<LweSample>  p (new_LweSample(params),_delete_LweSample);
	return p;

}