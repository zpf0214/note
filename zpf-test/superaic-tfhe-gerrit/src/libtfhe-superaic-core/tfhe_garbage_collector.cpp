#include "tfhe_garbage_collector.h"
#include "lwe-functions.h"
#include "tlwe_functions.h"
#include "tgsw_functions.h"
#include "tfhe_gate_bootstrapping_functions.h"

using namespace std;

// TODO 以后要换成智能指针
LweParams* new_LweParams(int32_t n, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_LweParams(LweParams* obj);

TLweParams *new_TLweParams(int32_t N, int32_t k, int32_t plaintext_modulus, double alpha_min, double alpha_max);
EXPORT void delete_TLweParams(TLweParams *obj);

TGswParams *new_TGswParams(int32_t l, int32_t Bgbit, const TLweParams *tlwe_params);
EXPORT void delete_TGswParams(TGswParams *obj);


/**
 * this class implements a simple garbage collector for simple parameter pointers
 */
void TfheGarbageCollector::m_register_param(LweParams* param) { lwe_params.push_back(param); }
void TfheGarbageCollector::m_register_param(TLweParams* param) { tlwe_params.push_back(param); }
void TfheGarbageCollector::m_register_param(TGswParams* param) { tgsw_params.push_back(param); }
void TfheGarbageCollector::m_register_param(TFheGateBootstrappingParameterSet* param) { gb_params.push_back(param); }

void TfheGarbageCollector::register_param(LweParams* param) { init(); global_tfheGarbageCollector->m_register_param(param); }

void TfheGarbageCollector::register_param(TLweParams* param) { init(); global_tfheGarbageCollector->m_register_param(param); }
void TfheGarbageCollector::register_param(TGswParams* param) { init(); global_tfheGarbageCollector->m_register_param(param); }
void TfheGarbageCollector::register_param(TFheGateBootstrappingParameterSet* param) { init(); global_tfheGarbageCollector->m_register_param(param); }

void TfheGarbageCollector::init() { if (!global_tfheGarbageCollector) global_tfheGarbageCollector = new TfheGarbageCollector(); }
void TfheGarbageCollector::finalize() { if (global_tfheGarbageCollector) delete global_tfheGarbageCollector; global_tfheGarbageCollector=0; }

TfheGarbageCollector::TfheGarbageCollector() {}
TfheGarbageCollector::~TfheGarbageCollector() {
    for (LweParams* param: lwe_params) delete_LweParams(param);
    for (TLweParams* param: tlwe_params) delete_TLweParams(param);
    for (TGswParams* param: tgsw_params) delete_TGswParams(param);
    for (TFheGateBootstrappingParameterSet* param: gb_params) delete_gate_bootstrapping_parameters(param);
    lwe_params.clear();
    tlwe_params.clear();
    tgsw_params.clear();
    gb_params.clear();
}

TfheGarbageCollector* TfheGarbageCollector::global_tfheGarbageCollector(0);
