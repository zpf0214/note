# include<iostream>
using namespace std;

class A
{
public:
    virtual ~A() { cout << "A" << endl; }
    virtual void foo(){
        cout << "foo in A" << endl;
    }
};

class B : public A
{
public:
    ~B() { cout << "B" << endl; }
    void foo(){
        cout << "foo in B" << endl;
    }
};

int main()
{
    A* ptr = new B;
    ptr -> foo();
    delete ptr;
//    A ptr = B();
//    ptr .foo();
    return 0;
}
