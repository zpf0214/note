#include <iostream>
#include <array>
#include <cstdint>
#include<cmath>
#include <bitset>

using namespace std;

int main() {
    int32_t value = -23;
    std::array<int32_t, 16> bits;

    for (int i = 0; i < 16; ++i) {
        // Extract the 2-bit chunk from the current position
        bits[i] = (value >> (i * 2)) & 3;
    }

    // Print the result in reverse order to show the most significant chunks first
    for (int i = 0; i < 16; i++) {
        //std::cout << "Chunk " << 15 - i << ": " 
        //          << std::bitset<2>(bits[i]) << " (decimal: " 
        //          << static_cast<int>(bits[i]) << ")\n";
        std::cout << bits[i] << ", ";
    }
    std::cout << std::endl;

    // recompose
    //bits[0] -= 23; // 可以正确的执行加法
                   // 如果是减法还可以正常执行吗？

    for(int i=0; i<16; i++){
        bits[i] += bits[i];
    }
    //处理进位
    int32_t carry = 0;
    for(int i=0; i<16; i++){
        bits[i] += carry;
        carry = bits[i] / 4;
        bits[i] = bits[i] % 4;
    }
    int32_t result = 0;
    for(int i=0; i<16; i++){
        cout << bits[i] << ", ";
        result |= static_cast<int32_t>(bits[i]) << (i * 2);
    }
    std::cout << std::endl;

    std::cout << result << std::endl;
    
    auto ans = static_cast<int64_t>(value); //不是直接填充，而是会根据最高位进行填充
    cout << ans << endl;

    int ilog = 8;
    auto ilogr = log2(ilog);
    std::cout << ilogr << std::endl;

    return 0;
}
