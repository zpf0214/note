#include <iostream>

int main() {
    const int value = 2;

    switch (value) {
        case 1:
            std::cout << "Value is 1" << std::endl;
            break;
        case 2:   // 这个 case 是合法的
            std::cout << "Value is 2" << std::endl;
            break;
        case value: // 这是非法的，因为 value 不是一个编译时常量
            std::cout << "This is illegal" << std::endl;
            break;
        default:
            std::cout << "Default case" << std::endl;
    }

    return 0;
}
