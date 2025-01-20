#include "Message.hpp"

#include<iostream>
#include<string>

using namespace std;

ostream &Message::printObject(ostream &os) {
    os << "This is my very nice message: " << endl;
    os << message_;
    return os;
}

void f(int x, int y){
    cout << x << "-" << y << endl;
}

void f(int x, int y, int z){
    cout << x << "-" << y << endl;
    cout << z << endl;
}
