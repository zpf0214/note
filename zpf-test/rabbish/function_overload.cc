#include<iostream>

using namespace std;

void f(int i){
    cout << i << endl;
}

void f(string s){
    cout << s << endl;
}

int main(){
    f(1);
    f("ab");

    return 0;
}
