#include "Message.hpp"
#include<cstdlib>
#include<iostream>
#include<stdio.h>
using namespace std;

int main(){
    Message say_hello("hello, CMake World!");
    cout << say_hello << endl;

    Message say_goodbye("Goodbye, CMake World!");
    cout << say_goodbye << endl;

    f(1, 2);
    f(1, 2, 3);

    return EXIT_SUCCESS;

}
