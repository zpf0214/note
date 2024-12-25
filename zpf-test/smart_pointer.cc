#include <memory>
#include <cstddef>
#include <iostream>

class LweSample {
public:
    LweSample() { std::cout << "LweSample constructed\n"; }
    ~LweSample() { std::cout << "LweSample destructed\n"; }
    // 假设这里有一些成员函数和数据成员
};

// 自定义删除器，用于正确释放 LweSample* 数组及其元素
struct LweSampleArrayDeleter {
    // 删除单个 LweSample 对象
    void operator()(LweSample* ptr) const {
        delete ptr;
    }

    // 删除 LweSample** 数组及其包含的 LweSample* 指针
    void operator()(LweSample** array, size_t size) const {
        if (array != nullptr) {
            for (size_t i = 0; i < size; ++i) {
                if (array[i] != nullptr) {
                    this->operator()(array[i]); // 删除 LweSample 对象
                }
            }
            delete[] array; // 删除 LweSample* 数组
        }
    }
};

// 使用 std::unique_ptr 和自定义删除器管理 LweSample* 数组
template<size_t N>
struct ArrayDeleter {
    void operator()(LweSample** array) const {
        LweSampleArrayDeleter deleter;
        deleter(array, N);
    }
};

int main() {
    // 创建一个包含 10 个 LweSample* 的数组
    std::unique_ptr<LweSample*[], ArrayDeleter<10>> b(new LweSample*[10]);

    // 分配 LweSample 对象
    for (size_t i = 0; i < 10; ++i) {
        b[i] = new LweSample();
    }

    // 当 b 超出作用域或被重置时，LweSampleArrayDeleter 会自动调用以清理资源
    return 0;
}
