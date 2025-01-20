#include<iostream>
using namespace std;

struct MyStruct {
    int value = 3;
    int x;
    int *y = nullptr;
    MyStruct(int x);
    MyStruct(int x, int value, int* y);
    MyStruct(MyStruct&& other)
        noexcept: value(other.value),
                  x(other.x),
                  y((other.y)){}
    ~MyStruct(){
        cout << "call deconstructor" << endl;
        delete y;
    };
};

MyStruct::MyStruct(int x):x(x){}
MyStruct::MyStruct(int x, int value, int* y):x(x), value(value), y((y)){}

MyStruct f(int value, int x){
    int* y = new int(9);
    MyStruct ms(x, value, (y));
    return ms;
}
int main(){
    MyStruct ms = f(4, 7);
    cout << ms.x << "--" << ms.value << "--" << *(ms.y) << endl;

    return 0;
}
