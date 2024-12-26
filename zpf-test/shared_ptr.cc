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
        std::cout << "deleter start ";
        for (int i = 0; i < VAL; ++i) {
            delete p[i]; // 删除每个 LweSample 对象
        }
        delete[] p; // 删除指针数组
    };
    
    // 将原始指针转换为智能指针，并使用上述删除器
    std::shared_ptr<LweSample*[]> smartA(a, deleter);
    a = nullptr;
    
    a = smartA.get();
    smartA.reset();
    std::swap(a, b);
    //
    for(int i=0; i<VAL; ++i){
    //    std::swap(a[i], b[i]);
    }

    deleter(b);
    deleter(a);
    //{
    //    for(int i=0; i<VAL; i++){
    //        delete b[i];
    //    }
    //    delete[] b;
    //}
    //smartA.reset(a);

}
