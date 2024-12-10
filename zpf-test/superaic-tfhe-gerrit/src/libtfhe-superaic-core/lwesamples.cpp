#include "lwesamples.h"
#include "lweparams.h"


LweSample::LweSample(const LweParams* params)
{
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