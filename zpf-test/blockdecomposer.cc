#include <iostream>
#include <array>
#include <cstdint>
#include <bitset>

int main() {
    int32_t value = -23;
    std::array<int32_t, 16> bits;

    for (int i = 0; i < 16; ++i) {
        // Extract the 2-bit chunk from the current position
        bits[i] = (value >> (i * 2)) & 0x3;
    }

    // Print the result in reverse order to show the most significant chunks first
    for (int i = 15; i >= 0; --i) {
        std::cout << "Chunk " << 15 - i << ": " 
                  << std::bitset<2>(bits[i]) << " (decimal: " 
                  << static_cast<int>(bits[i]) << ")\n";
    }

    int32_t result = 0;
    for(int i=0; i<16; i++){
        result |= static_cast<int32_t>(bits[i]) << (i * 2);
    }

    std::cout << result << std::endl;

    return 0;
}
