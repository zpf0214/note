// b.cc

#include <iostream>
#include "b.h"

void f(int b) {
    if (b == 1) {
        std::cout << "f is using the fixed behavior." << std::endl;
    } else {
        std::cout << "f is using the default behavior." << std::endl;
    }
}
