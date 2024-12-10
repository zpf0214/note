#include <gtest/gtest.h>
#include <tfhe.h>
#include "tfhe_package.h"

#include "tfhe_superaic_server.h"

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

    class ACCTest : public ::testing::Test {
    };


    TEST_F (ACCTest, tSessionID) {
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


    TEST_F (ACCTest, tSwap) {
        TFHE_ACC ACC(2);
        LweBootstrappingKey* bk1 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss;
        export_lweBootstrappingKey_toStream(oss, bk1);
        string result = oss.str();
        istringstream iss(result);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap1 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));
        shared_ptr<Session> session1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap1));


        LweBootstrappingKey* bk2 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss2;
        export_lweBootstrappingKey_toStream(oss2, bk2);
        string result2 = oss.str();
        istringstream iss2(result2);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap2 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss2));
        shared_ptr<Session> session2 = shared_ptr<Session>(new Session(string("2"),bootstrap_key_wrap2));


        LweBootstrappingKey* bk3 = new_random_bk_key(11,1,lweparams120, tgswparams128_2);
        ostringstream oss3;
        export_lweBootstrappingKey_toStream(oss3, bk3);
        string result3 = oss3.str();
        istringstream iss3(result3);
        std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap3 = shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss3));
        shared_ptr<Session> session3 = shared_ptr<Session>(new Session(string("3"),bootstrap_key_wrap3));


        auto tmp_time1 = session1->get_last_use_time();
        std::chrono::duration<double> seconds1 = tmp_time1.time_since_epoch();
        cout  << "session 1:" << seconds1.count() << endl;

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - tmp_time1);
        cout  << "session 1:" << duration.count() << endl;

        auto tmp_time2 = session2->get_last_use_time();

        auto tmp_time3 = session3->get_last_use_time();

        ASSERT_LE(tmp_time1,tmp_time2); // 后创建的时间大
        ASSERT_LE(tmp_time2,tmp_time3); // 后创建的时间大

        ACC.add_session(session1);
        ACC.add_session(session2);

        ASSERT_EQ(ACC.get_session(string("1")),session1);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),nullptr);

        ACC.add_session(session3);     // 把最老的换出去
        ASSERT_EQ(ACC.get_session(string("1")),nullptr);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),session3);

        session2->update_last_use_time(); // 现在2更新,要把3换出去
        ACC.add_session(session1);
        ASSERT_EQ(ACC.get_session(string("1")),session1);
        ASSERT_EQ(ACC.get_session(string("2")),session2);
        ASSERT_EQ(ACC.get_session(string("3")),nullptr);

        delete_LweBootstrappingKey(bk1);
        delete_LweBootstrappingKey(bk2);
        delete_LweBootstrappingKey(bk3);

    }


    int32_t f(int32_t n, int32_t plaintext_modulus){
        return n*n % plaintext_modulus;
    }

    LweSample *real_new_LweSample(const LweParams *params) {
        return new_LweSample(params);
    }

    TEST_F(ACCTest, CPU_ACC_bootstrappingTest) {
        bool _use_fix_random_bak = _use_fix_random;
        const int32_t N = 32;
        const int32_t plaintext_modulus = 8;
        const int32_t k = 1;
        const int32_t n = 10;
        const int32_t l_bk = 3; //ell
        const int32_t Bgbit_bk = 10;
        const int32_t ks_t = 15;
        const int32_t ks_basebit = 1;
        const double alpha_in = 5e-4;
        const double alpha_bk = 9e-9;
        const double alpha = 1e-10;

        LweParams *in_params = new_LweParams(n, plaintext_modulus, alpha_in, 1. / 16.);
        TLweParams *accum_params = new_TLweParams(N, k, plaintext_modulus, alpha_bk, 1. / 16.);
        TGswParams *bk_params = new_TGswParams(l_bk, Bgbit_bk, accum_params);
        const LweParams *extract_params = &accum_params->extracted_lweparams;

        int32_t truth_table[plaintext_modulus];
        LweKey *key = new_LweKey(in_params);
        lweKeyGen(key);
        TGswKey *key_bk = new_TGswKey(bk_params);
        tGswKeyGen(key_bk);
        LweBootstrappingKey *bk = new_LweBootstrappingKey(ks_t, ks_basebit, in_params, bk_params);

        //call the function
        tfhe_createLweBootstrappingKey(bk, key, key_bk);

        // 生成真值表
        for(int32_t i=0;i<plaintext_modulus;i++){
            truth_table[i] = f(i, plaintext_modulus);
        }

        int32_t messageInM = 3; // 消息空间8，范围[-4，4)
        shared_ptr<LweSample> insample = shared_ptr<LweSample>(new LweSample(extract_params));
        // 1 把明文转换为 Torus32
        // message space -> plaintext space
        Torus32 message = modSwitchToTorus32(messageInM, plaintext_modulus);
        // 2 对Toruse32 上的message进行加密
        // 误差要选在正确范围内
        lweSymEncrypt(insample.get(), message, alpha, key);

        shared_ptr<LweSample> result = shared_ptr<LweSample>(new LweSample(in_params));

        {
            CPU_ACC ACC;

            // 借用原有的bootstrap key生成session
            ostringstream oss;
            export_lweBootstrappingKey_toStream(oss, bk);
            string oss_str = oss.str();
            istringstream iss(oss_str);
            std::shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap1 = std::shared_ptr<LweBootstrappingKey_Wrap>(new LweBootstrappingKey_Wrap(iss));
            shared_ptr<Session> session1 = shared_ptr<Session>(new Session(string("1"),bootstrap_key_wrap1));

            // 把session放到ACC里面去
            ACC.add_session(session1);

            //Session_ID_t sessionID,const shared_ptr<LweSample> in_sample, int32_t * function_table, const int32_t table_size, shared_ptr<LweSample> result
            ACC.programmable_bootstrap(string("1"),insample,truth_table,plaintext_modulus,result);

        }
        //tfhe_programmable_bootstrap(result.get(),bk,truth_table,plaintext_modulus,insample.get());



        //Todo : check resulte
        // 3 解密，得到在Torus32上的密文，值已经被约束到 n/plaintext_modulus 上
        Torus32 decrypt = lweSymDecrypt(result.get(), key, plaintext_modulus);
        // 4 得到浮点数明文值,应该和 messageInM/M相同
        double ddecrypt = t32tod(decrypt);

        // 得到 plaintext_modulus 上的明文
        int32_t m_decrypt = modSwitchFromTorus32(decrypt,plaintext_modulus);

        ASSERT_EQ(f(messageInM, plaintext_modulus), m_decrypt);


        //cleanup
        delete_LweBootstrappingKey(bk);
        delete_TGswKey(key_bk);
        delete_LweKey(key);
        delete_TGswParams(bk_params);
        delete_TLweParams(accum_params);
        delete_LweParams(in_params);
        _use_fix_random = _use_fix_random_bak;
    }



}
