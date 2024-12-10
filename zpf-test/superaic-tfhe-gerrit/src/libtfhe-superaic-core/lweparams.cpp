#include <cmath>
#include  <stdio.h>
#include "lweparams.h"

using namespace std;

LweParams::LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max):
		n(n),
        plaintext_modulus(plaintext_modulus),
		alpha_min(alpha_min),
		alpha_max(alpha_max) {}

LweParams::~LweParams() {}

void LweParams::print(void) const {
	printf("LweParams\n");
	printf("n:%d\n",n);
    printf("plaintext_modulus:%d\n", plaintext_modulus);
	printf("alpha_max:%f\n",alpha_max);
	printf("alpha_min:%f\n",alpha_min);
}
