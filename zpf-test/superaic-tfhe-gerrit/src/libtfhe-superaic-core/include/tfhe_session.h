#ifndef TFHE_SESSION_H
#define TFHE_SESSION_H
#include <stdint.h>
#include <memory>
#include <chrono>
#include <iomanip>

#include <tfhe.h>
#include <tfhe_io.h>

#include "lwebootstrappingkey.h"

using namespace std;
namespace tfhe_superaic {

class LweBootstrappingKey_Wrap{
    LweBootstrappingKey* key;   // 没有办法阻止其他的程序把key拿到以后在外面释放，因此要在调用 delete_LweBootstrappingKey 的地方格外小心。
public:
    LweBootstrappingKey_Wrap(std::istream &iss) {
        key = new_lweBootstrappingKey_fromStream(iss);
    }

    ~LweBootstrappingKey_Wrap(){
        //new_LweBootstrappingKey
        delete_LweBootstrappingKey(key);
    }
public:
    const LweBootstrappingKey* getkey(){return key;};
};

typedef string Session_ID_t;
class Session{
    Session_ID_t sessionID;
    chrono::steady_clock::time_point last_use_time;

    shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap;
public:
    uint32_t ext_tag;   //for test

    Session(Session_ID_t sessionID , shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap);

    static Session_ID_t cal_session_ID(const LweKey *lwe_key);

    Session_ID_t get_session_ID(void);

    void update_last_use_time(void);
    chrono::steady_clock::time_point get_last_use_time(void);

    const LweBootstrappingKey * get_bootstrap_key(void);

};


shared_ptr<Session> get_session(Session_ID_t sessionID);

void add_session(shared_ptr<Session> session);

void remove_session(Session_ID_t sessionID);

}

#endif // TFHE_SESSION_H
