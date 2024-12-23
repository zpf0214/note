#include <iostream>
#include <array>
#include <cstdint>
#include<cmath>
#include <bitset>

using namespace std;

void block_decomposer(int32_t *result, int32_t message, const int32_t message_modulu, const int32_t num_block){
    int32_t message_bits = log2(message_modulu);
    for(int i=0; i<num_block; i++){
        result[i] = (message >> (i * message_bits)) & (message_modulu - 1); 
    }
}

//TODO zpf 这样恢复对于负值要如何处理？
//位操作决定了log(message_modulu) * num_block == log(int32_t)
int32_t block_recomposer(const int32_t *plaintext_block, const int32_t message_modulu, const int32_t num_block){
    int32_t message = 0;
    int32_t message_bits = log2(message_modulu);
    for(int i=0; i<num_block; i++){
        message |= static_cast<int32_t>(plaintext_block[i]) << (i * message_bits);
    }
    return message;

}


int main() {
    const int32_t num_block = 16;
    const int32_t message_modulu = 4;
    int32_t plaintext_block[num_block];
    for(int i=0; i<num_block; i++){
        plaintext_block[i] = 1;
    }

    auto x = block_recomposer(plaintext_block, message_modulu, num_block);

    cout << x << endl;

    int32_t result[num_block];
    block_decomposer(result, x, message_modulu, num_block);

    {
        for(int i=0; i<num_block; i++){
            cout << "plaintext_block[" << i << "] = " 
                << plaintext_block[i];
            cout << " result[" << i << "] = " 
                << result[i];
            cout << endl;
        }
    }

    return 0;
}
