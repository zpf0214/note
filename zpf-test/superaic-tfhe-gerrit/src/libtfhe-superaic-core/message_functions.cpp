#include<cmath>
#include<iostream>
#include<cassert>
#include "tfhe.h"
#include "message_functions.h"

using namespace std;

/*
 * 根据message modulu 大小和plaintext大小决定blocks数量
 * EXAMPLE
 * message_modulu = 4
 * plaintext = int32_t
 *
 * blocks = 32 / message_modulu.ilog() = 32 / 2 = 16
 * 由于这一点的存在，message_modulu 的选择是受到限制的，必须是2^n
 *
 * 我们目前的实现并没有考虑提供更多的类型，即不提供i16，i8等
 * 那么这个blocks就是定死的，后续如果我们提供更多情况，那么就需要
 * 写一个对应的实现用来实现不同的blocks
 *
 * 我们的函数需要的参数
 *  - 数值，如果我们给了一个比如int64，在plaintext = int32_t
 *      的情况下就会有数值损失，我们需要判断这个，但不是在这个
 *      函数里，而是调用前做判断
 *  - message_modulu
 *  - plaintext，不需要这个，我们只需要blocks的数量就行，不需要额外的冗余信息
 *  以上信息可以知道如果以后想要写成泛型函数，需要改变的只有值--传入值和返回值
 *
 *  返回值，这种情况下是否简单的返回一个blocks？是否进行包装则由
 *  调用者自己决定，这里的包装指的是是否需要携带一些额外的信息
 *  比如message_modulu, plaintext
 */

//TODO 函数还是有些问题，分解后的值与我们期待的值不一致，从这里看的话还需要修改
//-23的分解应该填充最高位，而不是[-3, ...]
//需要讨论result中的值的类型是否应该是int32_t，这个无法避免负数的存在
EXPORT void block_decomposer(int32_t *result, int32_t message, const int32_t message_modulu, const int32_t num_block){
    int32_t message_bits = log2(message_modulu);
    for(int i=0; i<num_block; i++){
        result[i] = (message >> (i * message_bits)) & (message_modulu - 1); 
    }
}

//TODO zpf 这样恢复对于负值要如何处理？
//位操作决定了log(message_modulu) * num_block == log(int32_t)
EXPORT int32_t block_recomposer(const int32_t *plaintext_block, const int32_t message_modulu, const int32_t num_block){
    int32_t message = 0;
    int32_t message_bits = log2(message_modulu);
    for(int i=0; i<num_block; i++){
        message |= static_cast<int32_t>(plaintext_block[i]) << (i * message_bits);
    }
    return message;

}

/*
 * i23 = 8 * 2bits = [3int32_t, ...]
 * 我们进行加密的时候仍然是对每一个block单独加密
 * encode，我们需要将信息从message space转到plaintext space
 *      需要知道plaintext modulus大小
 *      Torus32 modSwitchToTorus32(int32_t mu, int32_t plaintext_modulus)
 *
 * 对于加密，我们已经实现了对应的函数 
 *      lweSymEncrypt(LweSample* result, Torus32 message, double alpha, const LweKey* key);
 *      这个已经足以进行加密，并且颗粒度足够小
 *
 * 如何进行加法运算，我们事实上只关心lwe层面的加法
 *      void lweAddTo(LweSample* result, const LweSample* sample, const LweParams* params);
 *          该函数已经测试过，可以直接用
 *          result = result + sample
 *          params 传递结构信息
 *      如何进位?
 *          我们需要利用pbs完成进位运算，具体来说
 *              将blocks拆分为message和carry，然后利用加法完成进位
 *                  这里是否可以做的更细一点？
 *              拆分的时候需要bootstrap
 *              进位加法的时候还是需要bootstrap
 *
 * 我们有pbs了，可以很自然的提取message和carry，然后执行进位加法
 *
 * 通过整理，我们已经获得了所有需要的内容，现在可以将它们组合到一起了
 */


// generate message block
// message_bits = block_value % message_modulus
// carry_bits = block_value / message_modulus
EXPORT void apply_lookup_table(
        LweSample *result,
        const LweBootstrappingKey *bk,
        const LweSample *x,
        const int32_t plaintext_modulus,
        int32_t (*fp)(int32_t) //zpf 用std::function可以更有扩展性，
                               //比如可以将函数curry化
        ){
            // generate lookup table
            int32_t truth_table[plaintext_modulus];

            // 生成真值表
            for(int32_t i=0;i<plaintext_modulus;i++){
                truth_table[i] = fp(i);
            }

            tfhe_programmable_bootstrap(result, bk, truth_table,plaintext_modulus, x);

}

//zpf encrypt with secret key--lwe key
//这不是最终完成版，我们暂时不返回任何东西
/*TODO
 * key分配在堆上，准确释放是个很大的问题
 * 后续
 *      1. 可以使用智能指针避免手动释放
 *      2. 新建一个类，将lwekey bsk放到类里
 *          然后在析构函数中释放
 */
void encrypt_with_private_key(
        LweSample** lwe_cipher_result, //zpf [*lwe_ciphe, ...]
        int32_t message,
        const int32_t message_modulus, 
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key, //zpf in heap
        const double alpha
        ){
            //zpf lwe_cipher 是一个数组，数组元素是指向LweSample的指针
            //LweSample* lwe_cipher[num_block];

            //zpf decompose
            int32_t message_decomposer_result[num_block];
            block_decomposer(message_decomposer_result, message, message_modulus, num_block);

            {
                //zpf test for decompose/recompose
                int32_t after_mess = block_recomposer(message_decomposer_result, message_modulus, num_block);
                assert(message == after_mess);
            }

            Torus32 plaintext_blocks[num_block];
            for(int i=0; i<num_block; i++){
                plaintext_blocks[i] = modSwitchToTorus32(message_decomposer_result[i], plaintext_modulus);
            }

            for (int i=0; i<num_block; i++){
                lweSymEncrypt(lwe_cipher_result[i], plaintext_blocks[i], alpha, key);
            }
}


//zpf decrypt with private key
//TODO key使用智能指针进行管理，避免手动释放
int32_t decrypt_with_private_key(
        LweSample** lwe_cipher_blocks,
        const int32_t message_modulus,
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key //zpf in heap
        ){
            Torus32 decrypt_blocks[num_block];
            for(int i=0; i<num_block; i++){
                decrypt_blocks[i] = lweSymDecrypt(lwe_cipher_blocks[i], key, plaintext_modulus);
            }

        //zpf plaintext -> message
        int32_t message_blocks[num_block];
        for(int i=0; i<num_block; i++){
            message_blocks[i] = modSwitchFromTorus32(decrypt_blocks[i], plaintext_modulus);
        }

        {//zpf print for debug

            for(int i=0; i<num_block; i++){
                cout << message_blocks[i] << ", ";
            }
            cout << "message_after_decrypt in function decrypt_with_private_key" << endl;
        }//zpf print for debug
        
        int32_t message_after_decrypt = block_recomposer(message_blocks, message_modulus, num_block);

        return message_after_decrypt;
}


//zpf 最终的encrypt函数应该只需要message和key
//裸指针内存风险比较大，后续需要考虑使用智能指针
LweSample** encrypt_lwe(
        int32_t message,
        const int32_t message_modulus,
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key, //zpf in heap
        const LweParams *lwe_params,
        const double alpha
        ){
            LweSample** lwe_cipher_blocks = new LweSample*[num_block];

            for(int i=0; i<num_block; i++){
                // zpf 分配到了堆上，那么需要手动释放
                // 或者使用智能指针
                // LweSample占用空间大，应该放到堆上
                lwe_cipher_blocks[i] = new_LweSample(lwe_params);
            }

            encrypt_with_private_key(lwe_cipher_blocks, message, message_modulus, plaintext_modulus, num_block, key, alpha);

            return lwe_cipher_blocks; //局部变量指针不可以作为返回值，除非值是new出来的
}

void cleanup_LweSample_blocks(LweSample** blocks, int32_t num_block){
    for (int i=0; i<num_block; i++){
        delete_LweSample(blocks[i]);
    }
    delete[] blocks;
}

