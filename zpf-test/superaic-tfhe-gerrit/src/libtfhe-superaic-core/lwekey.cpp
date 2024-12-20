#include <cstdlib>
#include <stdio.h>
#include "lwekey.h"
#include "lweparams.h"

using namespace std;

LweKey::LweKey(const LweParams* params) {
    this->params = params;
    this->key = new int32_t[params->n];
}

LweKey::LweKey(const shared_ptr<LweParams> params) {
    this->params = params.get();
    this->key = new int32_t[params->n];
}

void LweKey::print(void) const {
    printf("LweKey:\n");
    printf("n:%d\n",params->n);
    int val_per_line = 100;
    int line_number = val_per_line;
    for(int i= 0 ;i < params->n; i ++) {
        printf("%d ", key[i]);
        line_number --;
        if( line_number == 0){
            printf("\n");
            line_number = val_per_line;
        }
    }
    printf("\n");
}
LweKey::~LweKey() {
    delete[] key;
}



EXPORT LweKey * alloc_LweKey() { 
    return (LweKey*) malloc(sizeof(LweKey)); 
}

EXPORT LweKey* alloc_LweKey_array(int32_t nbelts) { 
    return (LweKey*) malloc(nbelts*sizeof(LweKey)); 
} 

EXPORT void free_LweKey(LweKey* ptr) { 
    free(ptr); 
} 

EXPORT void free_LweKey_array(int32_t nbelts, LweKey* ptr) { 
    free(ptr); 
}

EXPORT void init_LweKey_array(int32_t nbelts, LweKey* obj, const LweParams* params) { \
    for (int32_t ii=0; ii<nbelts; ++ii) init_LweKey(obj+ii,params); 
} 

EXPORT void destroy_LweKey_array(int32_t nbelts, LweKey* obj) { 
    for (int32_t ii=0; ii<nbelts; ++ii) destroy_LweKey(obj+ii); 
}

LweKey* new_LweKey( const LweParams* params) { 
    LweKey* reps = alloc_LweKey(); 
    init_LweKey(reps, params); 
    return reps; 
}

LweKey* new_LweKey( std::shared_ptr <LweParams> params) { 
    LweKey* reps = alloc_LweKey(); 
    init_LweKey(reps, params); 
    return reps; 
}


EXPORT LweKey* new_LweKey_array(int32_t nbelts, const LweParams* params) { 
    LweKey* reps = alloc_LweKey_array(nbelts); 
    init_LweKey_array(nbelts, reps, params); 
    return reps; 
}

EXPORT void delete_LweKey(LweKey* obj) { 
    destroy_LweKey(obj); 
    free_LweKey(obj); 
}

EXPORT void delete_LweKey_array(int32_t nbelts, LweKey* obj) { 
    destroy_LweKey_array(nbelts, obj); 
    free_LweKey_array(nbelts, obj); 
}


void _delete_lwekey(LweKey * p) {
    delete_LweKey(p);
}

std::shared_ptr<LweKey> new_LweKey_shared(const shared_ptr<LweParams> params) {
    std::shared_ptr<LweKey>  p (new_LweKey(params.get()),_delete_lwekey);
    return p;
}

std::shared_ptr<LweKey> new_LweKey_shared(const LweParams * params) {
    std::shared_ptr<LweKey>  p (new_LweKey(params),_delete_lwekey);
    return p;
}