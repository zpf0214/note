#include<iostream>
#include<string>
#include<future>
using namespace std;

class Work {
    private:
        int value;

    public:
        Work():value(42){}
        std::future<int>spawn(){
            return std::async([=, tmp = this]()->int {return tmp->value;});
            //return std::async([=, tmp = *this]()->int {return tmp->value;});
        }
};

std::future<int>foo(){
    Work tmp;
    return tmp.spawn();
}

int main() {
    std::future<int> f = foo();
    f.wait();
    std::cout << "f.get() = " << f.get() << endl;

    return 0;
}
