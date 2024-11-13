#include <cmath>
#include <cstdint>
#include <iostream>
#include <unordered_map>

using namespace std;

template<typename T>
using LookupTable = std::unordered_map<T, T>;

typedef int32_t Torus32;

struct TorusPolynomial {
    const int32_t N;
    Torus32* coefsT = nullptr;
    TorusPolynomial(const int32_t N): N(N){
        this->coefsT = new Torus32[N];
    }
};

int nearestInteger(double value) {
    return static_cast<int>(std::round(value));
}

Torus32 modSwitchToTorus32(int32_t mu, int32_t Msize){
    uint64_t interv = ((UINT64_C(1)<<63)/Msize)*2; // width of each intervall
    uint64_t phase64 = mu*interv;
    //floor to the nearest multiples of interv
    return phase64>>32;
}


int32_t modSwitchFromTorus32(Torus32 phase, int32_t Msize){
    uint64_t interv = ((UINT64_C(1)<<63)/Msize)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    return phase64/interv;
}

void testPolynomialGen(TorusPolynomial *testpoly, int32_t N, int32_t Msize){
    for(auto i=0; i<N; i++){
        int32_t polynomial_elem = static_cast<int32_t>(double(i) * Msize / (N*2) + 0.5);
        testpoly->coefsT[i] = modSwitchToTorus32(polynomial_elem, Msize);
    }
}

int32_t pow(int32_t n, int32_t Msize){
    return n*n % Msize;
}
//fake map，创建正确/错误的unordered_map
//返回指针的话会有生命周期的问题
LookupTable<int32_t> generatePBSTableFake(int32_t Msize, bool flag){
    LookupTable<int32_t> pbs_table;
    if (flag){
        for(auto i=0; i<Msize; i++){
            pbs_table[i] = pow(i, Msize);
        }
    }else{
        // 1.长度不够
        // 2.长度超过
        // 3.Tp不在该范围之内
        // 4.负值输入
        for(auto i=0; i<Msize; i++){
            pbs_table[i] = i*i;
        }
    }
    return pbs_table;
}
//zpf 利用用户传入的f: Tp -> Tp 生成dict，并应用到programmable test polynomial上
//这个暂时不用做，用户传入的只有数据，没有函数
//传入的dict需要预先校验正确性
//1. 所有Tp值都要被包含，key，value有且只能有Tp中的值
//2. key的数量只能是Tp，同时要考虑用户的输入可能是从负值开始
void validatePBSTableInRange(LookupTable<int> *pbs_table, int32_t Msize);

void testPolynomialGenWitPBSTable(TorusPolynomial *testpoly, int32_t N, int32_t Msize, LookupTable<int32_t> pbs_table){
    for(auto i=0; i<N; i++){
        int32_t polynomial_elem = static_cast<int32_t>(double(i) * Msize / (N*2) + 0.5);
        int32_t pbs_poly_elem = pbs_table[polynomial_elem];
        testpoly->coefsT[i] = modSwitchToTorus32(pbs_poly_elem, Msize);
    }
}

int main() {
    int32_t N = 8;
    int32_t Msize = 4;

    TorusPolynomial *testpoly = new TorusPolynomial(N);
    LookupTable<int32_t> pbs_table = generatePBSTableFake(Msize, true);
    testPolynomialGenWitPBSTable(testpoly, N, Msize, pbs_table);
    //testPolynomialGen(testpoly, N, Msize);
    for(auto i=0; i<N; i++){
        cout << testpoly->coefsT[i] << " ";
        cout << " after mod to int32_t ";
        cout << modSwitchFromTorus32(testpoly->coefsT[i], Msize);
        cout << endl;
    }
    cout << endl;

    delete testpoly;

    return 0;
}
