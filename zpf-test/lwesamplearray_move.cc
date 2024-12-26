#include <iostream>
#include <vector>
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
public:
    std::vector<LweSample**> a;  // 使用 std::vector 管理 LweSample** 数组
    int val;                     // 数组的大小

    // 辅助函数：释放所有资源
    void freeResources() {
        for (auto& ptr : a) {
            if (ptr != nullptr) {
                for (int i = 0; i < val; ++i) {
                    delete ptr[i]; // 删除每个 LweSample 对象
                }
                delete[] ptr;      // 删除 LweSample* 数组
                ptr = nullptr;
            }
        }
        a.clear(); // 清空 vector
    }

public:
    // 构造函数：动态分配并初始化 a 数组及其元素
    LweSampleArray(int size) : val(size), a(1, new LweSample*[size]) {
        for (int i = 0; i < val; ++i) {
            a[0][i] = new LweSample(0); // 使用构造函数初始化
        }
    }

    // 新增构造函数：接受外部传入的 std::vector<LweSample**> 并将其移动到类内部
    LweSampleArray(std::vector<LweSample**>&& externalA) : a(std::move(externalA)), val(10) {
        //if (!a.empty()) {
        //    val = (*a[0])[0]->value; // 假设第一个元素的 value 表示数组大小，或者你可以传递一个额外的参数
        //}
        // 外部传入的 vector 已经被置为 empty
    }

    // 析构函数：释放所有资源
    ~LweSampleArray() {
        freeResources();
    }

    // 禁用拷贝构造函数和拷贝赋值操作符
    LweSampleArray(const LweSampleArray&) = delete;
    LweSampleArray& operator=(const LweSampleArray&) = delete;

    // 移动构造函数：实现资源转移
    LweSampleArray(LweSampleArray&& other) noexcept : a(std::move(other.a)), val(other.val) {
        other.val = 0;
    }

    // 移动赋值操作符：实现资源转移
    LweSampleArray& operator=(LweSampleArray&& other) noexcept {
        if (this != &other) {
            // 释放当前资源
            freeResources();

            // 转移资源
            a = std::move(other.a);
            val = other.val;

            // 置空其他对象的资源
            other.val = 0;
        }
        return *this;
    }

    // 获取数组大小
    int size() const {
        return val;
    }

    // 访问数组元素
    LweSample* operator[](int index) const {
        if (index >= 0 && index < val) {
            return a[0][index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }

    // 修改数组元素
    LweSample*& operator[](int index) {
        if (index >= 0 && index < val) {
            return a[0][index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }
};

// 测试代码
int main() {
    const int VAL = 10;

    // 创建和初始化原始 vector<LweSample**>
    std::vector<LweSample**> externalA;
    for (int i = 0; i < 1; ++i) { // 假设只有一个 LweSample** 数组
        LweSample** temp = new LweSample*[VAL];
        for (int j = 0; j < VAL; ++j) {
            temp[j] = new LweSample(j + 1); // 使用构造函数初始化
        }
        externalA.push_back(temp);
    }

    // 打印初始值
    //std::cout << "Initial values in externalA:\n";
    //for (int i = 0; i < VAL; ++i) {
    //    std::cout << "externalA[" << i << "] = " << externalA[0][i]->value << "\n";
    //}

    // 创建 LweSampleArray 对象，传入外部 vector 并将其移动到类内部
    LweSampleArray array1(std::move(externalA));

    // 打印初始值
    std::cout << "Initial values in array1:\n";
    for (int i = 0; i < array1.a.size(); ++i) {
        std::cout << "array1[" << i << "] = " << array1[i]->value << "\n";
    }

    // 确认 externalA 已经被清空
    if (externalA.empty()) {
        std::cout << "externalA has been moved and is now empty\n";
    } else {
        std::cout << "externalA is not empty, something went wrong\n";
    }

    return 0;
}
