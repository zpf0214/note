#include<random>
#include<iostream>

using namespace std;

typedef int32_t Torus32; //avant uint32_t
default_random_engine generator;
uniform_int_distribution<Torus32> uniformTorus32_distrib(INT32_MIN, INT32_MAX);

void randomGenrator(Torus32* result, const int32_t n){
    for(auto i=0;i<n;i++){
        result[i] = uniformTorus32_distrib(generator);
    }
}

void printArray(const Torus32* result, const int32_t n){
    for(auto i=0; i<n; i++){
        cout<<result[i]<<", ";
    }
    cout<<endl;
}

int main(){
    const int32_t n = 4;
    Torus32 result[n];
    randomGenrator(result, n);
    printArray(result, n);

    const int32_t m = 5;
    Torus32 ret[m];
    randomGenrator(ret, m);
    printArray(ret, m);
    return 0;

}
