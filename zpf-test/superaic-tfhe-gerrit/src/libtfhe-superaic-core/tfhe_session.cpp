#include <string>
#include <map>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <string.h>

#include <openssl/md5.h>
#include <openssl/evp.h>
#include "tfhe_session.h"
#include "tfhe_io.h"

namespace tfhe_superaic {
static map<Session_ID_t,shared_ptr<Session>> g_session_map;
static mutex map_mtx;

// 垃圾收集： 定期检查session的use_count，把长时间没有人用的session删掉
// Todo: 检查
// void clear_sessions();

int md5(const char *in_data , size_t in_len, unsigned char * out_data, unsigned int *max_out_len){
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        printf("Failed to create EVP_MD_CTX\n");
        return -1;
    }

    const EVP_MD *md = EVP_md5();
    if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1) {
        printf("Failed to initialize digest\n");
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    // 输入数据

    if (EVP_DigestUpdate(md_ctx, in_data, in_len) != 1) {
        printf("Failed to update digest\n");
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    if (EVP_DigestFinal_ex(md_ctx, out_data, max_out_len) != 1) {
        printf("Failed to finalize digest\n");
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }


    EVP_MD_CTX_free(md_ctx);
    return 0;
}

Session::Session(Session_ID_t sessionID , shared_ptr<LweBootstrappingKey_Wrap> bootstrap_key_wrap):
   sessionID(sessionID),bootstrap_key_wrap(bootstrap_key_wrap){
    update_last_use_time();
}

Session_ID_t Session::cal_session_ID(const LweKey *lwe_key)
{
    ostringstream oss;
    export_lweKey_toStream(oss, lwe_key);
    string result = oss.str();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md5_size = EVP_MAX_MD_SIZE-1;
    md5(result.c_str(),result.length(),md_value,&md5_size);
    std::stringstream hexstream;
    hexstream << std::hex << std::setfill('0');
    for (int i = 0; i < md5_size; ++i) {
        hexstream << std::setw(2) << static_cast<unsigned>(md_value[i]);
    }
    return hexstream.str();
}

Session_ID_t Session::get_session_ID()
{
    return sessionID;
}

void Session::update_last_use_time()
{
    last_use_time = std::chrono::steady_clock::now();
}

chrono::steady_clock::time_point Session::get_last_use_time()
{
    return last_use_time;
}

const LweBootstrappingKey *Session::get_bootstrap_key()
{
    return bootstrap_key_wrap->getkey();
}



static shared_ptr<Session> _get_session(Session_ID_t sessionID){
    auto iter = g_session_map.find(sessionID);
    if (iter != g_session_map.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

shared_ptr<Session> get_session(Session_ID_t sessionID){
    lock_guard<mutex> lock(map_mtx);
    return _get_session(sessionID);
}



static void _remove_session(Session_ID_t sessionID){

    auto iter = g_session_map.find(sessionID);
    if (iter != g_session_map.end()) {
        g_session_map.erase(iter);
        return;
    } else {
        return ;
    }

}


void remove_session(Session_ID_t sessionID){
    lock_guard<mutex> lock(map_mtx);
    _remove_session(sessionID);

}


static void _add_session(shared_ptr<Session> session){
    _remove_session(session->get_session_ID());
    g_session_map.insert(pair<Session_ID_t, shared_ptr<Session>>(session->get_session_ID(), session));
}

void add_session(shared_ptr<Session> session){
    lock_guard<mutex> lock(map_mtx);
    _add_session(session);
}
}
