#ifndef torus_polynomail_gen_h
#define torus_polynomail_gen_h

#include<random>
#include<iostream>

using namespace std;

typedef int32_t Torus32; //avant uint32_t
extern default_random_engine generator;
extern uniform_int_distribution<Torus32> uniformTorus32_distrib;

extern void randomGenrator(Torus32* result, const int32_t n);
extern void printArray(const Torus32* result, const int32_t n);

#endif
