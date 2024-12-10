#include <cstdlib>
#include <stdio.h>
#include "lwekey.h"
#include "lweparams.h"

using namespace std;

LweKey::LweKey(const LweParams* params) {
    this->params = params;
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

