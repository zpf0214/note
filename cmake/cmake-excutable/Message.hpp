#pragma once

#include<iosfwd>
#include<ostream>
#include<string>

using namespace std;

class Message {
    public:
        Message(const string &m): message_(m){}
        friend std::ostream &operator<<(std::ostream &os, Message &obj) {
            return obj.printObject(os);
        }
    private:
        std::string message_;
        std::ostream &printObject(std::ostream &os);
};

void f(int x, int y);
void f(int x, int y, int z);
