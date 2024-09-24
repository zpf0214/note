/* 多项式乘法，将与多项式乘法相关的代码copy到这里
 * 我们最终只想要p q v
 * 那么是否用python生成会比较快？因为我们也不需要
 * 验证p*q == v,这样看的话，用python会比较快
 * 因为我们不关心中间过程是什么
 */
// 文章变得很乱了，因为我们想要的太多了，不过
// 现在我们可以先写这些注释，Git之后再删除，保持代码的整洁
typedef Torus32 int32_t; //avant uint32_t
#define DEGREE 1024

#include <random>
extern std::default_random_engine generator;
extern std::uniform_int_distribution<Torus32> uniformTorus32_distrib;
default_random_engine generator;
uniform_int_distribution<Torus32> uniformTorus32_distrib(INT32_MIN, INT32_MAX);
uniform_int_distribution<int32_t> uniformInt_distrib(INT_MIN, INT_MAX);

// 计算mod(N,q)上的多项式乘法
// 这里没有考虑结果溢出，这是非常自然的
// 因为有符号整数就是定义在环上的(mod 2^32)
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


// 这一代码对应论文的BlindRotate部分，这里需要将u的取值范围rescale to 2N，because X is order of 2N
// 需要注意一下在代码中是如何实现的
// 目前关于多项式乘法暂时还不涉及这里
void torusPolynomialMultNaive_plain_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
    const int32_t _2Nm1 = 2*N-1;
    Torus32 ri;
    for (int32_t i=0; i<N; i++) {
	ri=0;
	for (int32_t j=0; j<=i; j++) {
	    ri += poly1[j]*poly2[i-j];
	}
	result[i]=ri;
    }
    for (int32_t i=N; i<_2Nm1; i++) {
	ri=0;
	for (int32_t j=i-N+1; j<N; j++) {
	    ri += poly1[j]*poly2[i-j];
	}
	result[i]=ri;
    }
}
