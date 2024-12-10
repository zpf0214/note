#include <stdio.h>

#include "tfhe_superaic_driver.h"
#include "tfhe_superaic_torus.h"


int main(){
    printf("driver name is %s\n",DRIVER_NAME);
    printf("cipher size %ld byts\n",superaic_get_cipher_word_size());
}
