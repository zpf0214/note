#include <cassert>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <thread>
#include <future>

#include "tfhe_acc.h"

namespace tfhe_superaic {

// TODO 目前只支持一个加速器.要支持多个加速器.
static std::shared_ptr<TFHE_ACC> g_ACC = nullptr;
mutex acc_mtx;


static std::shared_ptr<TFHE_ACC> ACC_factory(){
    // 如果有xdma设备，那么就用FPGA加速器，否则就用CPU加速器
    uint32_t version;
    bool has_dev = get_FPGA_version(FPGA_ACC_V0::XDMA_USER_DEV,version);
    if(has_dev) {
        if(FPGA_ACC_V0::match(version)){
            printf("FIND FPGA Device!\n");
            std::shared_ptr<FPGA_ACC_V0> fpga = std::shared_ptr<FPGA_ACC_V0>(new FPGA_ACC_V0());
            fpga->init();
            return fpga;

        }else{
            printf("FIND FPGA Device, but version unknow! 0x%x\n",version);
            return nullptr;
        }
    }else{
        printf("No FPGA Device, use CPU!\n");
        std::shared_ptr<CPU_ACC> cpu = std::shared_ptr<CPU_ACC>(new CPU_ACC());
        return cpu;

    }

}

std::shared_ptr<TFHE_ACC> TFHE_ACC::get_valid_acc(void){
    lock_guard<mutex> lock(acc_mtx);
    if(!g_ACC) {
        g_ACC = ACC_factory();
    }

    return g_ACC;
}

shared_ptr<Session> TFHE_ACC::get_session(Session_ID_t sessionID)
{
    lock_guard<mutex> lock(session_mtx);
    return _get_session(sessionID);
}

void TFHE_ACC::remove_session(Session_ID_t sessionID)
{
    lock_guard<mutex> lock(session_mtx);
    _remove_session(sessionID);
}

void TFHE_ACC::add_session(shared_ptr<Session> session)
{
    lock_guard<mutex> lock(session_mtx);
    _remove_session(session->get_session_ID());

    if(sessions.size() >= max_sessions){
        assert((elim_algo == LRU));
        if (elim_algo == LRU)
            swap_out_LRU();
        else
            swap_out_LRU();
    }
    _add_session(session);

}

TFHE_ACC::TFHE_ACC(int max_sessions):
    max_sessions(max_sessions),type(ACC_UNKONW)
{
}

float TFHE_ACC::get_loading()
{
    return get_CPU_loading();
}

ACC_RESULT TFHE_ACC::programmable_bootstrap(Session_ID_t sessionID, shared_ptr<LweSample> in_sample, int32_t *function_table, int32_t table_size, shared_ptr<LweSample> out_sample)
{
    return ACC_OK;
}

TFHE_ACC::TFHE_ACC(int max_sessions, ACC_TYPE type, CAPABILITY capability):
    max_sessions(max_sessions),type(type),capability(capability)
{

}

size_t TFHE_ACC::get_CPU_exector_num(void) {
    size_t cpu_concurrency =  std::thread::hardware_concurrency();

    // 不要把CPU占满，保留2核心作其他任务,以免系统卡死
    if( cpu_concurrency > 2) {
        cpu_concurrency -= 2;
    }

    return cpu_concurrency;

}


shared_ptr<Session> TFHE_ACC::_get_session(Session_ID_t sessionID)
{
    auto iter = sessions.find(sessionID);
    if (iter != sessions.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

void TFHE_ACC::_remove_session(Session_ID_t sessionID)
{
    auto iter = sessions.find(sessionID);
    del_session_on_device(sessionID);
    if (iter != sessions.end()) {
        sessions.erase(iter);
        return;
    } else {
        return ;
    }
}

void TFHE_ACC::_add_session(shared_ptr<Session> session)
{
    sessions.insert(pair<Session_ID_t, shared_ptr<Session>>(session->get_session_ID(), session));

}

void TFHE_ACC::swap_out_LRU()
{
    chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
    bool bfirst = true;
    shared_ptr<Session> earlest_session;

    // 我不知道怎么定义duration，用 auto 预先定义一个
    auto duration_max = now_time - now_time;

    for(auto iter = sessions.begin(); iter != sessions.end() ; ++iter) {
        shared_ptr<Session> current_session = iter->second;
        auto duration = now_time - current_session->get_last_use_time();
        if( bfirst) {
            earlest_session = current_session;
            duration_max = duration;
            bfirst = false;
        }else{
            if( duration > duration_max){
                duration_max = duration;
                earlest_session = current_session;
            }
        }
    }

    _remove_session(earlest_session->get_session_ID());

}

void TFHE_ACC::del_session_on_device(Session_ID_t sessionID)
{

}

float TFHE_ACC::get_CPU_loading()
{
    return 0.5f;
}



void TFHE_ACC::tfhe_programmable_bootstrap_woKS(LweSample *result,
                                const LweBootstrappingKey *bk,
                                Torus32 * const truth_table, const int32_t truth_table_size, const LweSample *x) {

    const TGswParams *bk_params = bk->bk_params;
    const TLweParams *accum_params = bk->accum_params;
    const LweParams *in_params = bk->in_out_params;
    const int32_t N = accum_params->N;
    const int32_t Nx2 = 2 * N;
    const int32_t n = in_params->n;
    TorusPolynomial *testvect = new_TorusPolynomial(N);
    int32_t *bara = new int32_t[N];

    int32_t barb = modSwitchFromTorus32(x->b, Nx2);
    for (int32_t i = 0; i < n; i++) {
        bara[i] = modSwitchFromTorus32(x->a[i], Nx2);
    }

    testPolynomialGenWithPBSTable(testvect, N, truth_table_size, truth_table);

    tfhe_blindRotateAndExtract(result, testvect, bk->bk, barb, bara, n, bk_params);

    delete[] bara;
    delete_TorusPolynomial(testvect);
}

void TFHE_ACC::tfhe_blindRotateAndExtract(LweSample *result,
                                       const TorusPolynomial *v,
                                       const TGswSample *bk,
                                       const int32_t barb,
                                       const int32_t *bara,
                                       const int32_t n,
                                       const TGswParams *bk_params) {

    const TLweParams *accum_params = bk_params->tlwe_params;
    const LweParams *extract_params = &accum_params->extracted_lweparams;
    const int32_t N = accum_params->N;
    const int32_t _2N = 2 * N;

    TorusPolynomial *testvectbis = new_TorusPolynomial(N);
    std::shared_ptr<TLweSample> acc = new_TLweSample_shared(accum_params);

    if (barb != 0) torusPolynomialMulByXai(testvectbis, _2N - barb, v);
    else torusPolynomialCopy(testvectbis, v);
    tLweNoiselessTrivial(acc.get(), testvectbis, accum_params);
    tfhe_blindRotate(acc.get(), bk, bara, n, bk_params);
    tLweExtractLweSample(result, acc.get(), extract_params, accum_params);

    delete_TorusPolynomial(testvectbis);
}


void TFHE_ACC::tfhe_blindRotate(TLweSample *accum, const TGswSample *bk, const int32_t *bara, const int32_t n, const TGswParams *bk_params) {

    //TGswSample* temp = new_TGswSample(bk_params);
    std::shared_ptr<TLweSample> temp = new_TLweSample_shared(bk_params->tlwe_params);
    TLweSample *temp2 = temp.get();
    TLweSample *temp3 = accum;

    for (int32_t i = 0; i < n; i++) {
        const int32_t barai = bara[i];
        if (barai == 0) continue; //indeed, this is an easy case!

        tfhe_MuxRotate(temp2, temp3, bk + i, barai, bk_params);
        swap(temp2, temp3);

    }
    if (temp3 != accum) {
        tLweCopy(accum, temp3, bk_params->tlwe_params);
    }

}

void TFHE_ACC::tfhe_MuxRotate(TLweSample *result, const TLweSample *accum, const TGswSample *bki, const int32_t barai,
                    const TGswParams *bk_params) {
    // ACC = BKi*[(X^barai-1)*ACC]+ACC
    // temp = (X^barai-1)*ACC
    tLweMulByXaiMinusOne(result, barai, accum, bk_params->tlwe_params);
    // temp *= BKi
    if( paral_mul )
        tGswExternMulToTLwe_paral(result, bki, bk_params);
    else
        tGswExternMulToTLwe(result, bki, bk_params);
    // ACC += temp
    tLweAddTo(result, accum, bk_params->tlwe_params);
}

void TFHE_ACC::tGswExternMulToTLwe(TLweSample *accum, const TGswSample *sample, const TGswParams *params) {
    const TLweParams *par = params->tlwe_params;
    const int32_t N = par->N;
    const int32_t kpl = params->kpl;
    //TODO: improve this new/delete
    IntPolynomial *dec = new_IntPolynomial_array(kpl, N);

    tGswTLweDecompH(dec, accum, params);
    tLweClear(accum, par);
    for (int32_t i = 0; i < kpl; i++) {
        tLweAddMulRTo(accum, &dec[i], &sample->all_sample[i], par);
    }

    delete_IntPolynomial_array(kpl, dec);
}


void TFHE_ACC::tGswExternMulToTLwe_paral(TLweSample *accum, const TGswSample *sample, const TGswParams *params) {
    const TLweParams *par = params->tlwe_params;
    const int32_t N = par->N;
    const int32_t kpl = params->kpl;
    //TODO: improve this new/delete
    IntPolynomial *dec = new_IntPolynomial_array(kpl, N);

    tGswTLweDecompH(dec, accum, params);
    tLweClear(accum, par);
    TLweSample * tmp_lwe = new_TLweSample_array(kpl,par);
    std::future<void> ret[kpl];
    for (int32_t i = 0; i < kpl; i++) {
        tLweClear(tmp_lwe + i, par);
        ret[i] = async(launch::async,&TFHE_ACC::tLweAddMulRTo_paral,this,tmp_lwe + i,&dec[i],&sample->all_sample[i],par);
        //tLweAddMulRTo(accum, &dec[i], &sample->all_sample[i], par);
    }

    for( int32_t i= 0;i < kpl; i++) {
        ret[i].get();
        tLweAddTo(accum,tmp_lwe + i, par);
    }
    delete_TLweSample_array(kpl,tmp_lwe);
    delete_IntPolynomial_array(kpl, dec);
}


/** result = result + p.sample */
void
TFHE_ACC::tLweAddMulRTo(TLweSample *result, const IntPolynomial *p, const TLweSample *sample, const TLweParams *params) {
    const int32_t k = params->k;

    // 这里i <= k，不仅仅计算了a，也计算了b
    for (int32_t i = 0; i <= k; ++i)
        torusPolynomialAddMulR(result->a + i, p, sample->a + i);
    result->current_variance += intPolynomialNormSq2(p) * sample->current_variance;
}

/** result = result + p.sample */
void
TFHE_ACC::tLweAddMulRTo_paral(TLweSample *result, const IntPolynomial *p, const TLweSample *sample, const TLweParams *params) {
    const int32_t k = params->k;

    std::future<void> ret[k];
    // 这里i <= k，不仅仅计算了a，也计算了b
    for (int32_t i = 0; i <= k; ++i)
        ret[i] = async(launch::async,&TFHE_ACC::torusPolynomialAddMulR, this,result->a + i, p, sample->a + i);

    for (int32_t i = 0; i <= k; ++i)
        ret[i].get();

    result->current_variance += intPolynomialNormSq2(p) * sample->current_variance;
}


void TFHE_ACC::torusPolynomialAddMulR(TorusPolynomial* result, const IntPolynomial* poly1, const TorusPolynomial* poly2) {
    const int32_t N = poly1->N;
    assert(result!=poly2);
    assert(poly2->N==N && result->N==N);
    TorusPolynomial temp(N);
    torusPolynomialMultNaive_aux(temp.coefsT, poly1->coefs, poly2->coefsT, N);
    torusPolynomialAddTo(result,&temp);

}


void TFHE_ACC::torusPolynomialMultNaive_aux(Torus32* __restrict result, const int32_t* __restrict poly1, const Torus32* __restrict poly2, const int32_t N) {
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


void TFHE_ACC::timespec_sub(struct timespec &t1, struct timespec &t2, struct timespec &t3) {
  assert(t1.tv_nsec >= 0);
  assert(t1.tv_nsec < 1000000000);
  assert(t2.tv_nsec >= 0);
  assert(t2.tv_nsec < 1000000000);
  t3.tv_sec = t1.tv_sec - t2.tv_sec;
  t3.tv_nsec = t1.tv_nsec - t2.tv_nsec;
  if (t3.tv_nsec >= 1000000000)
  {
    t3.tv_sec++;
    t3.tv_nsec -= 1000000000;
  }
  else if (t3.tv_nsec < 0)
  {
    t3.tv_sec--;
    t3.tv_nsec += 1000000000;
  }

}


}
