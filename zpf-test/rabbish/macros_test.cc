#include<iostream>

using namespace std;

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define GENERATE_NAME(xx) CONCAT(xx, __LINE__)

int main() {
    // 假设这一行是第206行，那么TEST_LINE会被替换为xx_206
    #define TEST_LINE GENERATE_NAME(xx)

    std::cout << TEST_LINE << " = " TEST_LINE << std::endl;
    return 0;
}
