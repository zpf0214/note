#include<iostream>
#include<vector>
#include<cmath>
#define PLAINTEXT_MODULUS 16
#define TORUS_N 1024
using namespace std;

typedef int32_t Torus32;
const int32_t plaintext_modulus = PLAINTEXT_MODULUS;

struct TorusPolynomial {
    const int32_t N;
    Torus32* coefsT = nullptr;

    TorusPolynomial(const int32_t N):N(N) {
        this->coefsT = new Torus32[N];
    }
    ~TorusPolynomial() {
        delete[] coefsT;
    }
};

int32_t modSwitchFromTorus32(Torus32 phase, int32_t plaintext_modulus){
    //uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // zpf 尝试按照论文进行修改
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    return phase64/interv;
}

Torus32 modSwitchToTorus32(int32_t mu, int32_t plaintext_modulus){
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t phase64 = mu*interv;
    //floor to the nearest multiples of interv
    return phase64>>32;
}

void testPolynomialGenWithPBSTable(TorusPolynomial *testvect, const int32_t N, const int32_t plaintext_modulus, const int32_t *truth_table){
    for(int i=0; i<N; i++){
        int32_t polynomial_elem = static_cast<int32_t>(double(i) * plaintext_modulus / (N*2) );
        {
            const int32_t half = plaintext_modulus/2;
            double value = double(i) * plaintext_modulus / (N*2) ;
            double intPart, fracPart;
            fracPart = modf(value, &intPart);
            polynomial_elem = (fracPart <= 0.5) ? intPart : intPart + 1;

            polynomial_elem = polynomial_elem % half;
        }
        int32_t pbs_poly_elem = truth_table[polynomial_elem];
        testvect->coefsT[i] = modSwitchToTorus32(pbs_poly_elem, plaintext_modulus);
    }
}

void printTestvect(TorusPolynomial *testvect) {
    const int32_t N = testvect->N;
    for(int i=0; i<N; i++) {
        cout << "index: " << i << " ";
        cout << "value: " << testvect->coefsT[i]<<", ";
        cout << endl;
    }
    cout << endl;
}


int main_1() {
    const int32_t N = TORUS_N;
    int32_t truth_table[plaintext_modulus];
    {
        for(int i=0; i<plaintext_modulus; i++) {
            truth_table[i] = i;
        }
    }
    TorusPolynomial *testvect = new TorusPolynomial(N);

    testPolynomialGenWithPBSTable(testvect, N, plaintext_modulus, truth_table);

    printTestvect(testvect);

    auto x = modSwitchToTorus32(8, plaintext_modulus);
    auto y = modSwitchToTorus32(0, plaintext_modulus);

    {
        cout << "x : " << x << endl;
        cout << "y : " << y << endl;
    }

    return 0;
}

int main(){
//    main_1();
    vector<int32_t> a = {0, 8, 9, -8, -7};
    for(auto val: a){
        auto x = modSwitchToTorus32(val, plaintext_modulus);
        cout << "val: " << val 
            << ", modSwitchToTorus32: " << x << endl;


    }
    return 0;
}
