#include<iostream>
using namespace std;

template<typename T>
class Base {
    public:
        void bar(){
            cout << "in base" << endl;
        }
};

template<typename T>
class Derived: Base<T> {
    public:
        void f(){
           this -> bar();
        }
};

int main() {
    Derived<int> d;
    d.f();

    return 0;
}
