#include<cassert>
#include<random>
#include<iostream>

using namespace std;

typedef int32_t Torus32; //avant uint32_t
#define DEGREE 1024

default_random_engine generator;
uniform_int_distribution<Torus32> uniformTorus32_distrib(INT32_MIN, INT32_MAX);

void torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
    Torus32 ri;
    for (int32_t i=0; i<N; i++) {
		ri=0;
			for (int32_t j=0; j<=i; j++) {
		    	ri += poly1[j]*poly2[i-j];
			}
			for (int32_t j=i+1; j<N; j++) {
		    	ri -= poly1[j]*poly2[N+i-j];
			}
		result[i]=ri;
    }
}

void randomGenrator(Torus32* result, const int32_t n){
    for(auto i=0;i<n;i++){
        result[i] = uniformTorus32_distrib(generator);
    }
}


void printArray(const Torus32* result, const int32_t n){
    for(auto i=0; i<n-1; i++){
        cout<<result[i]<<", ";
    }
    cout<<result[n-1];
    cout<<endl;
}

int main(){
    Torus32 p[DEGREE];
    Torus32 q[DEGREE];
    Torus32 v[DEGREE];
    randomGenrator(p, DEGREE);
    randomGenrator(q, DEGREE);
    torusPolynomialMultNaive_aux(v, p, q, DEGREE);

    printArray(p, DEGREE);
    printArray(q, DEGREE);
    printArray(v, DEGREE);

    //Torus32 pp[]={3,5,0,2};
    //Torus32 qq[]={1,0,0,2};
    //Torus32 r[4];
    //Torus32 rr[]={1,5,4,8};
    //torusPolynomialMultNaive_aux(r,pp,qq,4);
    //printArray(pp,4);
    //printArray(qq,4);
    //printArray(r,4);
    //assert(r==rr);//failed, r need to mod 8 then to cmp with rr
    
    return 0;

}
