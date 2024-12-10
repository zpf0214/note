#include <iostream>

#include "libacc.h"
#include "tfhe.h"
#include "tfhe_acc.h"
namespace tfhe_superaic {

CPU_ACC::CPU_ACC():
    TFHE_ACC(3,ACC_CPU,CAPABILITY_PBS) {
        paral_mul = false;
}


ACC_RESULT CPU_ACC::programmable_bootstrap(Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result){
    shared_ptr<Session> session = get_session(sessionID);
    if( session == nullptr ) {
        cerr << "Can not find session " << sessionID << endl;
        return ACC_NO_SESSION;
    }
    auto bk = session->get_bootstrap_key();

    shared_ptr<LweSample> u = shared_ptr<LweSample>(new LweSample(&bk->accum_params->extracted_lweparams));



    tfhe_programmable_bootstrap_woKS(u.get(), bk, function_table, table_size, in_sample.get());

    lweKeySwitch(result.get(), bk->ks, u.get());

    return ACC_OK;
}


}
