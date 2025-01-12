#include<iostream>
#include<cmath>
using namespace std;

int main() {
    int a;
    double value = 123.5000001;
    double intPart, fracPart;

    fracPart = modf(value, &intPart);

    a = (fracPart<=0.5)?intPart:intPart+1;

    cout << "intPart: " << intPart << endl;
    cout << "fracPart: " << fracPart << endl;
    cout << "a: " << a << endl;

    return 0;
}
