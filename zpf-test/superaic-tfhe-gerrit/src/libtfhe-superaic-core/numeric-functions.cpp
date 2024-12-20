#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <iostream>
#include <random>
#include <cassert>
#include <limits.h>


#include "tfhe_superaic_torus.h"
using namespace std;
#if 0
static const int64_t _two31 = INT64_C(1) << 31; // 2^31
static const int64_t _two32 = INT64_C(1) << 32; // 2^32
static const double _two32_double = _two32;
static const double _two31_double = _two31;

#endif

default_random_engine generator;
uniform_int_distribution<Torus32> uniformTorus32_distrib(INT32_MIN, INT32_MAX);
//uniform_int_distribution<int32_t> uniformInt_distrib(INT_MIN, INT_MAX);
bool _use_fix_random = true;
//bool _use_fix_random = false;
struct Exception42 {
};

EXPORT void die_dramatically(const char *message) {
    cerr << message << endl;
    abort();
    throw Exception42();
}




/** sets the seed of the random number generator to the given values */
EXPORT void tfhe_random_generator_setSeed(uint32_t* values, int32_t size) {
    seed_seq seeds(values, values+size);
    generator.seed(seeds);
}

// Gaussian sample centered in message, with standard deviation sigma
EXPORT Torus32 gaussian32(Torus32 message, double sigma){
    //Attention: all the implementation will use the stdev instead of the gaussian fourier param
    normal_distribution<double> distribution(0.,sigma); //TODO: can we create a global distrib of param 1 and multiply by sigma?
    double err = distribution(generator);
    return _use_fix_random ? message: message + dtot32(err);
}



// from double to Torus32
EXPORT Torus32 dtot32(double d) {
    return int32_t(int64_t((d - int64_t(d))*_two32));
}
// from Torus32 to double
EXPORT double t32tod(Torus32 x) {
    return double(x)/_two32_double;
}

// Used to approximate the phase to the nearest message possible in the message space
// The constant plaintext_modulus will indicate on which message space we are working (how many messages possible)
//
// "travailler sur 63 bits au lieu de 64, car dans nos cas pratiques, c'est plus précis"
EXPORT Torus32 approxPhase(Torus32 phase, int32_t plaintext_modulus){
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    phase64 -= phase64%interv;
    //rescale to torus32
    return int32_t(phase64>>32);
}

// Used to approximate the phase to the nearest message possible in the message space
// The constant plaintext_modulus will indicate on which message space we are working (how many messages possible)
//
// "travailler sur 63 bits au lieu de 64, car dans nos cas pratiques, c'est plus précis"
EXPORT int32_t modSwitchFromTorus32(Torus32 phase, int32_t plaintext_modulus){
    //uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // zpf 尝试按照论文进行修改
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    return phase64/interv;
}

EXPORT int32_t modSwitchFromTorus32_old(Torus32 phase, int32_t plaintext_modulus){ //zpf pbs错误变得更奇怪了
    //uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // zpf 尝试按照论文进行修改
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    int32_t x = phase64/interv;
    if(x >= (plaintext_modulus/2)){
        x -= plaintext_modulus;
    }
    return x;
}

// Used to approximate the phase to the nearest message possible in the message space
// The constant plaintext_modulus will indicate on which message space we are working (how many messages possible)
//
// "travailler sur 63 bits au lieu de 64, car dans nos cas pratiques, c'est plus précis"
EXPORT Torus32 modSwitchToTorus32(int32_t mu, int32_t plaintext_modulus){
    //uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus); // zpf 尝试按照论文修改
    uint64_t interv = ((UINT64_C(1)<<63)/plaintext_modulus)*2; // width of each intervall
    uint64_t phase64 = mu*interv;
    //floor to the nearest multiples of interv
    return phase64>>32;
}

