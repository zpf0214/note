#include <memory>
#include <iostream>

// 假设 LweSample 类已经定义
class LweSample {
public:
    // LweSample 的定义
    LweSample(int value):value(value) { std::cout << "LweSample constructed : " << value <<"\n"; }
    ~LweSample() { std::cout << "LweSample destructed : " << value << "\n"; }
    int value;
};

// 定义一个常量
const int VAL = 10;

int main()
{

    // 创建和初始化原始指针数组
    LweSample** a = new LweSample*[VAL];
    for (int i = 0; i < VAL; ++i) {
        a[i] = new LweSample(0); // 假设有一个默认构造函数
    }
    
    LweSample** b = new LweSample*[VAL];
    for (int i = 0; i < VAL; ++i) {
        b[i] = new LweSample(1); // 假设有一个默认构造函数
    }

    // 定义一个lambda表达式作为删除器，并捕获VAL
    auto deleter = [=](LweSample** p) {
        for (int i = 0; i < VAL; ++i) {
            delete p[i]; // 删除每个 LweSample 对象
        }
        delete[] p; // 删除指针数组
    };
    
    // 将原始指针转换为智能指针，并使用上述删除器
    std::shared_ptr<LweSample*[]> smartA(a, deleter);
    a = nullptr;
    
    // 使用智能指针管理每个 LweSample 对象
    for (int i = 0; i < VAL; ++i) {
        // 如果 LweSample 支持拷贝构造，则可以创建副本
//        smartA.get()[i] = new LweSample(*a[i]);
        // 或者如果你想要转移所有权而不是复制对象
        // std::shared_ptr<LweSample> sp(a[i]);
        // smartA.get()[i] = sp.get();
        // 这样做之后，你应该不再使用原始指针 a[i]
    }
    
    // 现在你可以安全地释放原始指针数组 'a'，因为所有权已经被转移给 smartA
    // 注意：这里我们不需要再手动 delete[] a，因为 smartA 会负责清理
    
    // 智能指针会在其生命周期结束时自动调用删除器

    a = smartA.get();
    std::swap(a, b);

    //deleter(b);
    {
        for(int i=0; i<VAL; i++){
            delete b[i];
        }
        delete[] b;
    }

}
