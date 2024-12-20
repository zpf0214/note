#include <gtest/gtest.h>
#include <tfhe.h>
#include "tfhe_package.h"

#include "tfhe_superaic_server.h"
#include "test_internal.h"

using namespace std;
using namespace tfhe_superaic;
namespace {

    const int32_t plaintext_modulus = 8;
    const LweParams* lweparams500 = new_LweParams(500, plaintext_modulus,0.1,0.3);
    const LweParams* lweparams120 = new_LweParams(120, plaintext_modulus,0.1,0.3);
    const set<const LweParams*> allparams = { lweparams120, lweparams500 };

    const TLweParams* tlweparams1024_1 = new_TLweParams(1024,1, plaintext_modulus,0.1,0.3);
    const TLweParams* tlweparams128_2 = new_TLweParams(128,2, plaintext_modulus,0.1,0.3);
    const set<const TLweParams*> allparams_tlwe = { tlweparams128_2, tlweparams1024_1 };

    const TGswParams* tgswparams1024_1 = new_TGswParams(3,15,tlweparams1024_1);
    const TGswParams* tgswparams128_2 = new_TGswParams(7,4,tlweparams128_2);
    const set<const TGswParams*> allparams_tgsw = { tgswparams1024_1, tgswparams128_2};

    const TFheGateBootstrappingParameterSet* gbp1 = new TFheGateBootstrappingParameterSet(6,2,lweparams120,tgswparams128_2);
    const set<const TFheGateBootstrappingParameterSet*> allgbp = { gbp1 };

    //generate a random lwekey
    LweKey* new_random_lwe_key(const LweParams* params) {
    const int32_t n = params->n;
    LweKey* key = new_LweKey(params);
    for (int32_t i=0; i<n; i++) key->key[i]=rand()%2;
    return key;
    }

    //generate a random tlwekey
    TLweKey* new_random_tlwe_key(const TLweParams* params) {
    const int32_t N = params->N;
    const int32_t k = params->k;
    TLweKey* key = new_TLweKey(params);
    for (int32_t i=0; i<k; i++)
        for (int32_t j=0; j<N; j++)
        key->key[i].coefs[j]=rand()%2;
    return key;
    }

    //generate a random tgswkey
    TGswKey* new_random_tgsw_key(const TGswParams* params) {
    const int32_t N = params->tlwe_params->N;
    const int32_t k = params->tlwe_params->k;
    TGswKey* key = new_TGswKey(params);
    for (int32_t i=0; i<k; i++)
        for (int32_t j=0; j<N; j++)
        key->key[i].coefs[j]=rand()%2;
    return key;
    }

    //generate a random ks
    void random_ks_key(LweKeySwitchKey* key) {
        const int32_t N = key->n;
        const int32_t t = key->t;
        const int32_t base = key->base;
    const int32_t n = key->out_params->n;
    const int32_t length = N*t*base;
    double variance = rand()/double(RAND_MAX);
        LweSample* begin = key->ks0_raw;
    LweSample* end = begin+length;
    for (LweSample* it=begin; it!=end; ++it) {
        for (int32_t j=0; j<n; j++) it->a[j]=rand();
            it->b=rand();
            it->current_variance=variance;
        }
    }

    //generate a random ks
    LweBootstrappingKey* new_random_bk_key(int32_t ks_t, int32_t ks_basebit, const LweParams* in_out_params, const TGswParams* bk_params) {
    const int32_t n = in_out_params->n;
        const int32_t kpl = bk_params->kpl;
        const int32_t k = bk_params->tlwe_params->k;
        const int32_t N = bk_params->tlwe_params->N;
        LweBootstrappingKey* bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_out_params, bk_params);
        random_ks_key(bk->ks);
        double variance = rand()/double(RAND_MAX);
        for (int32_t i=0; i<n; i++) {
            for (int32_t p=0; p<kpl; p++) {
                TLweSample& sample = bk->bk[i].all_sample[p];
                for (int32_t j=0; j<=k; j++) {
                    for (int32_t x=0; x<N; x++)
                        sample.a[j].coefsT[x]=rand();
                }
                sample.current_variance=variance;
            }
        }
        return bk;
    }

    class SessionTest : public ::testing::Test {
    };

    TEST_F (SessionTest, tLweBootstrappingKey_Wrap) {
        LweBootstrappingKey* bk1 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss;
        export_lweBootstrappingKey_toStream(oss, bk1);
        string result = oss.str();
        istringstream iss(result);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap = std::shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));
        delete_LweBootstrappingKey(bk1);
    }


    TEST_F (SessionTest, tSessionMap) {
        LweBootstrappingKey* bk1 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss;
        export_lweBootstrappingKey_toStream(oss, bk1);
        string result = oss.str();
        istringstream iss(result);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap1 = std::shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));


        LweBootstrappingKey* bk2 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss2;
        export_lweBootstrappingKey_toStream(oss2, bk2);
        string result2 = oss2.str();
        istringstream iss2(result2);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap2 = std::shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss2));

        shared_ptr<Session> session1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap1));
        session1->ext_tag = 1;
        shared_ptr<Session> session2 = shared_ptr<Session>(new Session(string("2"),bootstrap_key_wrap2));
        session2->ext_tag = 2;
        shared_ptr<Session> session_another1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap2));
        session_another1->ext_tag = 3;
        {
            shared_ptr<Session> session_get = get_session(string("3"));
            ASSERT_EQ(session_get,nullptr);
        }
        {
            add_session(session1);
            shared_ptr<Session> session_get = get_session(string("1"));
            ASSERT_EQ(session_get,session1);

            remove_session(string("1"));
            session_get = get_session(string("1"));
            ASSERT_EQ(session_get,nullptr);

        }
        {
            add_session(session1);
            add_session(session_another1);
            shared_ptr<Session> session_get = get_session(string("1"));
            ASSERT_EQ(session_get->ext_tag,3);
            ASSERT_EQ(session_get,session_another1);

        }

        delete_LweBootstrappingKey(bk1);
        delete_LweBootstrappingKey(bk2);


    }

    TEST_F (SessionTest, tSessionID) {
        LweKey* lwe_key1 = new_random_lwe_key(lweparams500);
        LweKey* lwe_key2 = new_random_lwe_key(lweparams500);

        Session_ID_t ID1 = Session::cal_session_ID(lwe_key1);
        Session_ID_t ID2 = Session::cal_session_ID(lwe_key2);
        Session_ID_t ID1_1 = Session::cal_session_ID(lwe_key1);

        ASSERT_EQ(ID1,ID1_1);
        ASSERT_NE(ID1,ID2);

        delete_LweKey(lwe_key1);
        delete_LweKey(lwe_key2);
    }


}
