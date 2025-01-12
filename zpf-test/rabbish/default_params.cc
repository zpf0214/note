#include<iostream>
using namespace std;

class Base {
public:
    Base() { std::cout << "Base::Base()" << std::endl;}
    virtual ~Base() { std::cout << "Base::~Base()" << std::endl;}
    virtual void func(int a=10) { std::cout << "Base::func(int) a = " << a << std::endl;}
};

class D1 : public Base {
public:
    D1() {std::cout << "D1::D1()" << std::endl;}
    ~D1() {std::cout << "D1::~D1()" << std::endl;}
    void func(int a=100) override {std::cout << "D1::func(int) a = " << a << std::endl; }
};

int main(int argc, char* argv[])
{
    D1 * object = new D1();
    object->func();
    delete object;
    return 0;
}
