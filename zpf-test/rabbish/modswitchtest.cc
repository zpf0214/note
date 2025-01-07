#include<iostream>
#include<cassert>

using namespace std;

typedef int32_t Torus32;
const int32_t plaintext_modulus = 16;


Torus32 modSwitchToTorus32(int32_t mu, int32_t plaintext_modulus){
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t phase64 = mu*interv; //zpf 注意到这里用的是 uint64_t， 那么phase64 在移位处理的时候是否能够保证移位结果正确？
    Torus32 phase32 = phase64 >> 32;
    return phase32;
    //return phase64>>32;
}

int32_t modSwitchFromTorus32(Torus32 phase, int32_t plaintext_modulus){
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    //uint64_t phase64 = (uint64_t(static_cast<uint32_t>(phase))<<32) ;
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    int32_t x = static_cast<int32_t>(phase64 / interv);

    {
        if (x >= (plaintext_modulus / 2)){
            x -= plaintext_modulus;
        }
    }
    return x;
    //return phase64/interv;
}

int32_t approx(Torus32 phase, int32_t plaintext_modulus){
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2;
    uint64_t half_interval = interv / 2;
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    phase64 -= phase64%interv;
    return int32_t(phase64 >> 32);
}

int main(){
    for(int32_t value=-8; value < plaintext_modulus; value++){
        Torus32 phase = modSwitchToTorus32(value, plaintext_modulus);
        int32_t appr = approx(phase, plaintext_modulus);
        int32_t phase_mod = modSwitchFromTorus32(appr, plaintext_modulus);

        cout << "value: " << value;
        cout << "  ---  appr: " << appr ;
        cout << "  ---  phase_mod: " << phase_mod;
        cout << endl;
    }
}
