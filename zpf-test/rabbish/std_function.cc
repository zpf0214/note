#include <iostream>
#include <functional>

// 接受裸函数指针的 print 函数
void print(std::function<int(int)> f, int y) {
    std::cout << "Result: " << f(y) << std::endl;
}

// 多参数函数
int multiply(int a, int b) {
    return a * b;
}

int raw_pointer(int x){
    return x;
}

int main() {
    int b = 3;
    auto bound_multiply = [=](int x)->int{return multiply(x, b);};


    // 将 std::function 转换为裸函数指针并传递给 print
    print(bound_multiply, 5);  // 输出: Result: 15
                               //
    print(raw_pointer, b);

    print([b](int x)->int{
            return multiply(x, b);
            },
            5);

    return 0;
}
