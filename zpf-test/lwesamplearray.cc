#include <iostream>
#include <memory>

// 假设 LweSample 类已经定义
class LweSample {
public:
    // LweSample 的定义
    LweSample(int value) : value(value) { std::cout << "LweSample constructed : " << value << "\n"; }
    ~LweSample() { std::cout << "LweSample destructed : " << value << "\n"; }
    int value;
};

// LweSampleArray 类
class LweSampleArray {
private:
    LweSample** a;  // 指向 LweSample* 数组的指针
    int val;        // 数组的大小

    // 辅助函数：释放所有资源
    void freeResources() {
        if (a != nullptr) {
            for (int i = 0; i < val; ++i) {
                delete a[i]; // 删除每个 LweSample 对象
            }
            delete[] a;      // 删除指针数组
            a = nullptr;
        }
    }

public:
    // 构造函数：动态分配并初始化 a 数组及其元素
    LweSampleArray(int size) : val(size), a(new LweSample*[size]) {
        for (int i = 0; i < val; ++i) {
            a[i] = new LweSample(0); // 使用构造函数初始化
        }
    }

    // 新增构造函数：接受外部传入的 LweSample** a 和 int size
    LweSampleArray(int size, LweSample** externalA) : val(size), a(externalA) {
        // 将外部传入的 a 置为 nullptr
        externalA = nullptr;
    }

    // 析构函数：释放所有资源
    ~LweSampleArray() {
        freeResources();
    }

    // 拷贝构造函数：实现深拷贝
    LweSampleArray(const LweSampleArray& other) : val(other.val), a(new LweSample*[other.val]) {
        for (int i = 0; i < val; ++i) {
            a[i] = new LweSample(*other.a[i]); // 深拷贝每个 LweSample 对象
        }
    }

    // 拷贝赋值操作符：实现深拷贝
    LweSampleArray& operator=(const LweSampleArray& other) {
        if (this == &other) {
            return *this; // 自赋值检查
        }

        // 释放当前资源
        freeResources();

        // 分配新资源并深拷贝
        val = other.val;
        a = new LweSample*[val];
        for (int i = 0; i < val; ++i) {
            a[i] = new LweSample(*other.a[i]); // 深拷贝每个 LweSample 对象
        }

        return *this;
    }

    // 移动构造函数：实现资源转移
    LweSampleArray(LweSampleArray&& other) noexcept : val(other.val), a(other.a) {
        other.val = 0;
        other.a = nullptr;
    }

    // 移动赋值操作符：实现资源转移
    LweSampleArray& operator=(LweSampleArray&& other) noexcept {
        if (this == &other) {
            return *this; // 自赋值检查
        }

        // 释放当前资源
        freeResources();

        // 转移资源
        val = other.val;
        a = other.a;

        // 置空其他对象的资源
        other.val = 0;
        other.a = nullptr;

        return *this;
    }

    // 获取数组大小
    int size() const {
        return val;
    }

    // 访问数组元素
    LweSample* operator[](int index) const {
        if (index >= 0 && index < val) {
            return a[index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }

    // 修改数组元素
    LweSample*& operator[](int index) {
        if (index >= 0 && index < val) {
            return a[index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }
};

// 测试代码
int main() {
    const int VAL = 10;

    // 创建和初始化原始指针数组
    LweSample** a = new LweSample*[VAL];
    for (int i = 0; i < VAL; ++i) {
        a[i] = new LweSample(i + 1); // 使用构造函数初始化
    }

    // 创建 LweSampleArray 对象，传入外部指针 a 并将其置为 nullptr
    LweSampleArray array1(VAL, a);
    a = nullptr;

    // 打印初始值
    std::cout << "Initial values:\n";
    for (int i = 0; i < array1.size(); ++i) {
        std::cout << "array1[" << i << "] = " << array1[i]->value << "\n";
    }

    // 确认 a 已经被置为 nullptr
    if (a == nullptr) {
        std::cout << "a has been set to nullptr\n";
    } else {
        std::cout << "a is not nullptr, something went wrong\n";
    }

    // 拷贝构造函数测试
    LweSampleArray array2 = array1;

    // 打印拷贝后的值
    std::cout << "Copied values:\n";
    for (int i = 0; i < array2.size(); ++i) {
        std::cout << "array2[" << i << "] = " << array2[i]->value << "\n";
    }

    // 移动构造函数测试
    LweSampleArray array3(std::move(array1));

    // 打印移动后的值
    std::cout << "Moved values:\n";
    for (int i = 0; i < array3.size(); ++i) {
        std::cout << "array3[" << i << "] = " << array3[i]->value << "\n";
    }

    // array1 应该为空
    std::cout << "array1 size after move: " << array1.size() << "\n";

    return 0;
}
