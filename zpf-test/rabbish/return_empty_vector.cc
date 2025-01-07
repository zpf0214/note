#include <iostream>
#include <vector>

// 函数返回一个空的 std::vector<int>
std::vector<int**> getEmptyVector() {
    std::vector<int**> value;
    //return {}; // C++11 及以后可以直接使用 {} 初始化空 vector
    return value;
}

int main() {
    // 调用函数并接收返回的空 vector
    std::vector<int**> emptyVec;
    emptyVec = getEmptyVector();

    // 检查 vector 是否为空
    if (emptyVec.empty()) {
        std::cout << "The vector is empty.\n";
    } else {
        std::cout << "The vector is not empty.\n";
    }

    return 0;
}
