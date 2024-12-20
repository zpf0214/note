#ifndef TEST_INTERNAL_H
#define TEST_INTERNAL_H
EXPORT LweParams* new_LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_LweParams(LweParams* obj);

LweKey* new_LweKey(const LweParams* params);
EXPORT void delete_LweKey(LweKey* obj);

LweSample* new_LweSample(const LweParams* params);
void delete_LweSample(LweSample* obj);

TLweParams *new_TLweParams(int32_t N, int32_t k, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_TLweParams(TLweParams *obj);

TLweKey *new_TLweKey(const TLweParams *params);
EXPORT void delete_TLweKey(TLweKey *obj);

TLweSample *new_TLweSample(const TLweParams *params);
EXPORT void delete_TLweSample(TLweSample *obj);


TGswParams *new_TGswParams(int32_t l, int32_t Bgbit, const TLweParams *tlwe_params);
EXPORT void delete_TGswParams(TGswParams *obj);

TGswKey* new_TGswKey(const TGswParams* params);
EXPORT void delete_TGswKey(TGswKey* obj);


TGswSample *new_TGswSample(const TGswParams *params);
EXPORT void delete_TGswSample(TGswSample *obj);
#endif
