#include<iostream>
#include<functional>
using namespace std;

template<typename T>
class ScopeGuard {
    public:
        ScopeGuard(T iFunc) {
            mFunc = iFunc;
        }
        ~ScopeGuard() {
            cout << "Exit the scope, so run the scope guard.\n";
            (*mFunc)();
        }
    private:
        T mFunc;
};

int main() {
    int i = 3;
    auto lGuard = ScopeGuard(&[i=i](){
            cout << "access value" << i << endl;
            }
            );
    return 0;
}
