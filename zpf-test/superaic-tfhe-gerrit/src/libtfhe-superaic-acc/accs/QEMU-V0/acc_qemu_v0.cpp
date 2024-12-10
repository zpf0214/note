#include <iostream>
#include <cassert>
#include <functional>
#include "libacc.h"
#include "tfhe.h"
#include "tfhe_acc.h"
namespace tfhe_superaic {


void QEMU_ACC_V0::torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
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

void QEMU_ACC_V0::torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2) {
    const int32_t N = poly1->N;
    assert(result!=poly2);
    assert(poly2->N==N && result->N==N);
    TorusPolynomial temp(N);
    torusPolynomialMultNaive_aux(temp.coefsT, poly1->coefs, poly2->coefsT, N);
    torusPolynomialAddTo(result,&temp);

}

QEMU_ACC_V0::QEMU_ACC_V0():
    TFHE_ACC(3,ACC_QEMU,CAPABILITY_PBS){
}


ACC_RESULT QEMU_ACC_V0::programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result){
    // 要加锁，防止函数重入的时候把 torusPolynomialAddMulR 指针给改写了。 但是这样会导致无法并行执行。目前是测试阶段，只使用一个加速器，暂时不考虑这个问题。后续要把整个计算放到类中成为成员函数才能避免这个问题。
    //lock_guard<mutex> lock(torusPolynomialAddMulR_mtx);

    shared_ptr<Session> session = get_session(sessionID);
    if( session == nullptr ) {
        cerr << "Can not find session " << sessionID << endl;
        return ACC_NO_SESSION;
    }

    //auto torusPolynomialAddMulR_bak = torusPolynomialAddMulR;
    //torusPolynomialAddMulR = bind(&QEMU_ACC_V0::torusPolynomialAddMulRQemu, this,placeholders::_1,placeholders::_2,placeholders::_3);

    auto bk = session->get_bootstrap_key();

    shared_ptr<LweSample> u = shared_ptr<LweSample>(new LweSample(&bk->accum_params->extracted_lweparams));



    tfhe_programmable_bootstrap_woKS(u.get(), bk, function_table, table_size, in_sample.get());

    lweKeySwitch(result.get(), bk->ks, u.get());

    //torusPolynomialAddMulR = torusPolynomialAddMulR_bak;
    return ACC_OK;
}


}
