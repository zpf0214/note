#include<iostream>

using namespace std;

int f(int x, int a, int b, int c){
    return x;
}

int f(int x, int s, int a, int b, int c){
    cout << s << endl;
    return x;
}

int main() {
    int x = 1;
    int a = 1;
    int b = 1;
    int c = 1;
    int s = 1;
    auto d = f(x, a, b, c);

    auto y = f(x, s, a, b, c);

    return 0;
}
