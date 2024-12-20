#include<functional>
#include<vector>
#ifndef MESSAGE_FUNCTIONS_H
#define MESSAGE_FUNCTIONS_H
/*
 *上层用户不需要知道我们如何encode，只需要一个encrypt api用以完成encode，encrypt的过程
 */

using namespace std; //zpf 可以避免重复写类似 std::vector<xx> 这个形式看上去噪音比较多
                  

EXPORT void block_decomposer(int32_t *result, int32_t message, const int32_t message_modulu, const int32_t num_block);

EXPORT int32_t block_recomposer(const int32_t *plaintext_block, const int32_t message_modulu, const int32_t num_block);


EXPORT void apply_lookup_table(
        LweSample *result,
        const LweBootstrappingKey *bk,
        const LweSample *x,
        const int32_t plaintext_modulus,
        //int32_t (*fp)(int32_t) //zpf 用std::function可以更有扩展性，
                               //比如可以将函数curry化
        std::function<int32_t(int32_t)> fp
        );

EXPORT void encrypt_with_private_key(
        LweSample** lwe_cipher_result, //zpf [*lwe_ciphe, ...]
        int32_t message,
        const int32_t message_modulus, 
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key, //zpf in heap
        const double alpha
        );

EXPORT int32_t decrypt_with_private_key(
        LweSample** lwe_cipher_blocks,
        const int32_t message_modulus,
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key //zpf in heap
        );

EXPORT LweSample** encrypt_lwe(
        int32_t message,
        const int32_t message_modulus,
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key, //zpf in heap
        const LweParams *lwe_params,
        const double alpha
        );

EXPORT void cleanup_LweSample_blocks(LweSample** blocks, int32_t num_block);


EXPORT void Lwecipher_message_eq(
        LweSample** lwe_mess_match,
        LweSample** lwe_cipher_blocks,
        LweBootstrappingKey *bk, //zpf in heap
        int32_t message,
        int32_t message_modulus,   //zpf be included in bk
        int32_t plaintext_modulus, //zpf be included in bk
        int32_t num_block);

/*
 * message -> [p1, p2, ...] -> [[lwe, lwe, ..], ...]
 */
EXPORT LweSample** encrypt(int32_t value, const LweKey* lwe_key);
EXPORT int32_t decrypt(LweSample** lwe_cipher, const LweKey* lwe_key);

//zpf 加密字符串
EXPORT std::vector<LweSample**> encrypt_str(const std::string &s, const LweKey* lwe_key); //zpf LweSample** in heap
                                                                                 //TODO 上层接口应该尽量提供智能指针，需要与底层交互的时候可以
                                                                                 //短暂的转换为裸指针，注意不要释放裸指针

//zpf 解密字符串
EXPORT std::string decrypt_str(std::vector<LweSample**> &str_cipher, const LweKey* lwe_key);

//zpf 字符串匹配
EXPORT std::vector<LweSample**> has_match(std::vector<LweSample**> str_cipher, const std::string pattern, const LweKey* lwe_key); //TODO LweSample* in heap



void encrypt_str_with_private_key(vector<LweSample**> &lwe_cipher_list, //zpf vector的话可以使用 & 语义避免copy
        const string str, const int32_t message_modulus, const int32_t plaintext_modulus, 
        const int32_t num_block, const LweParams* extract_params, const LweKey *key, double alpha);



string decrypt_str_with_private_key(vector<LweSample**> &lwe_cipher_list,
        const int32_t message_modulus, const int32_t plaintext_modulus,
        const int32_t num_block, const LweKey* key); //zpf key has included plaintext_modulus


void clean_up(vector<LweSample**> lwe_cipher_list, const int32_t num_block);

#endif //MESSAGE_FUNCTIONS_H
