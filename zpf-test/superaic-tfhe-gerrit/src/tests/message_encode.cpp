#include<iostream>
#include<cmath>
#include<cassert>
#include "tfhe.h"
#include "message_functions.h"
#include "test_internal.h"

using namespace std;

int main(){
    const int32_t num_block = 16;
    const int32_t message_modulu = 4;
    const int32_t plaintext_modulus = 16;
    int32_t result[num_block];
    int32_t message = -23;

    block_decomposer(result, message, message_modulu, num_block);
    //23 -> [3, 1, 1, 0, 0]

    { //zpf test block funtion
        int32_t after_mess = block_recomposer(result, message_modulu, num_block);

        assert(message == after_mess);
    } //zpf test block funtion


    return 0;

}
