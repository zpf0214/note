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

EXPORT LweParams* new_LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_LweParams(LweParams* obj);


void _delete_LweParams(LweParams * p) {
    delete_LweParams(p);
}

std::shared_ptr<LweParams> new_LweParams_shared(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max) {
	std::shared_ptr<LweParams>  p (new_LweParams(n,plaintext_modulus,alpha_min,alpha_max),_delete_LweParams);
	return p;
}
