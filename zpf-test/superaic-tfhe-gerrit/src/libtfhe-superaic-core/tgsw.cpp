#include <stdio.h>
#include <cassert>
#include "tfhe_superaic_torus.h"
#include "tlwe.h"
#include "tgsw.h"
#include "polynomials_arithmetic.h"


TGswParams::TGswParams(int32_t l, int32_t Bgbit, const TLweParams *tlwe_params) :
        l(l),
        Bgbit(Bgbit),
        Bg(1 << Bgbit),
        halfBg(Bg / 2),
        maskMod(Bg - 1),
        tlwe_params(tlwe_params),
        kpl(int32_t((tlwe_params->k + 1) * l)) {
    h = new Torus32[l];
    for (int32_t i = 0; i < l; ++i) {
        int32_t kk = (32 - (i + 1) * Bgbit);
        h[i] = 1 << kk; // q = 2^32, q/(Bg^(i+1)) as a Torus32
    }

    // offset = Bg/2 * (2^(32-Bgbit) + 2^(32-2*Bgbit) + ... + 2^(32-l*Bgbit))
    uint32_t temp1 = 0;
    for (int32_t i = 0; i < l; ++i) {
        uint32_t temp0 = 1 << (32 - (i + 1) * Bgbit);
        temp1 += temp0;
    }
    offset = temp1 * halfBg;

}

void TGswParams::print(void) const{
        printf("TGswParams:\n");
        printf("l:%d\n",l);
        printf("Bgbit:%d\n",Bgbit);
        printf("Bg:%d\n",Bg);
        printf("halfBg:%d\n",halfBg);
        printf("maskMod:%d(0x%x)\n",maskMod,maskMod);
        printf("kpl:%d\n",kpl);
        printf("offset:%u(0x%x)\n",offset,offset);
        for (int32_t i = 0; i < l; ++i) {
            printf("h[%d] = 0x%8x\n",i,h[i]);
        }

        printf("\n");
}

TGswParams::~TGswParams() {
    delete[] h;
}


// same key as in TLwe
TGswKey::TGswKey(const TGswParams *params) :
        params(params), tlwe_params(params->tlwe_params), tlwe_key(tlwe_params) {
    key = tlwe_key.key;
}

void TGswKey::print(void) const {
    printf("TGswKey:\n");
    printf("params:\n");
    params->print();
    printf("tlwe_params:\n");
    tlwe_params->print();
    printf("tlwe_key:\n");
    tlwe_key.print();
    key->print();
}

TGswKey::~TGswKey() {
}


void TGswSample::print(void) const {
    printf("TGswSample:\n");
    printf("l: %d\n",l);
    printf("k: %d\n",k);
    for(int bloc = 0; bloc < k + 1 ; ++bloc ) {
        printf("Glwe block %d:\n", bloc);
        for ( int i = 0 ; i < l ; ++i) {
            printf("l: %d\n",i);
            bloc_sample[bloc][i].print();
        }
    }
    printf("\n");
}

TGswSampleFFT::TGswSampleFFT(const TGswParams *params, TLweSampleFFT *all_samples_raw) : k(params->tlwe_params->k),
                                                                                         l(params->l) {
    all_samples = all_samples_raw;
    sample = new TLweSampleFFT *[(k + 1) * l];

    for (int32_t p = 0; p < (k + 1); ++p)
        sample[p] = all_samples + p * l;
}

TGswSampleFFT::~TGswSampleFFT() {
    delete[] sample;
}


