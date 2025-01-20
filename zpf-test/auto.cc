#include<iostream>
using namespace std;

int main() {
    int i = 5;
    int& j = i;
    auto m = j;

    cout << m << endl;
    cout << &m << endl;

    return 0;
}
