#include<iostream>
using namespace std;

void f(int a) {
    a += 1;
    cout << a << endl;
}
int main() {
    const int a = 4;
    f(a);
    cout << "a: " << a << endl;

    return 0;
}
