#ifndef MESSAGE_FUNCTIONS_H
#define MESSAGE_FUNCTIONS_H
/*
 *上层用户不需要知道我们如何encode，只需要一个encrypt api用以完成encode，encrypt的过程
 */

EXPORT void block_decomposer(int32_t *result, int32_t message, const int32_t message_modulu, const int32_t num_block);

EXPORT int32_t block_recomposer(const int32_t *plaintext_block, const int32_t message_modulu, const int32_t num_block);


EXPORT void apply_lookup_table(
        LweSample *result,
        const LweBootstrappingKey *bk,
        const LweSample *x,
        const int32_t plaintext_modulus,
        int32_t (*fp)(int32_t) //zpf 用std::function可以更有扩展性，
                               //比如可以将函数curry化
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

#endif //MESSAGE_FUNCTIONS_H
