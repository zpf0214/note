#include<cmath>
#include<functional>
#include<iostream>
#include<cassert>
#include "tfhe.h"
#include "message_functions.h"

using namespace std;

// TODO 以后要换成智能指针版本
LweSample* new_LweSample(const LweParams* params);
void delete_LweSample(LweSample* obj);

TLweSample *new_TLweSample(const TLweParams *params);
EXPORT void delete_TLweSample(TLweSample *obj);

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
        //int32_t (*fp)(int32_t) //zpf 用std::function可以更有扩展性，
                               //比如可以将函数curry化
        std::function<int32_t(int32_t)> fp
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
EXPORT void encrypt_with_private_key(
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
EXPORT int32_t decrypt_with_private_key(
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

        {//zpf print for debug TODO 以后可以添加条件编译

        //    for(int i=0; i<num_block; i++){
        //        cout << message_blocks[i] << ", ";
        //    }
        //    cout << "message_after_decrypt in function decrypt_with_private_key" << endl;
        }//zpf print for debug
        
        int32_t message_after_decrypt = block_recomposer(message_blocks, message_modulus, num_block);

        return message_after_decrypt;
}


//zpf 最终的encrypt函数应该只需要message和key
//裸指针内存风险比较大，后续需要考虑使用智能指针
EXPORT LweSample** encrypt_lwe(
        int32_t message,
        const int32_t message_modulus,
        const int32_t plaintext_modulus,
        const int32_t num_block,
        const LweKey *key, //zpf in heap
        const LweParams *lwe_params, // LweKey 中包含该参数，所以该函数事实上可以省略
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

EXPORT void cleanup_LweSample_blocks(LweSample** blocks, int32_t num_block){
    for (int i=0; i<num_block; i++){
        delete_LweSample(blocks[i]);
    }
    delete[] blocks;
}



    /*zpf
     * 这里似乎还是没有考虑清楚，如果我们不知道前一个的匹配结果
     * 如何知道后面应该如何匹配？
     * 例如用 ab 去匹配 abcde，因为我们只有在解密之后才知道
     * 匹配结果如何，所以
     * 我们就需要把abcde 按照 ab bc cd de 全部都匹配一遍
     * 然后解密的时候判断是否有成功匹配到的值
     * 即，即使我们一开始就已经匹配到了结果，整个过程
     * 可以结束了，但是由于我们不知道已经匹配到了，所以
     * 我们需要穷尽所有可能
     */
EXPORT void Lwecipher_message_eq(
        LweSample** lwe_mess_match,
        LweSample** lwe_cipher_blocks,
        LweBootstrappingKey *bk,
        int32_t message,
        int32_t message_modulus,
        int32_t plaintext_modulus,
        int32_t num_block)
{
    int32_t message_decomposer_result[num_block];
    block_decomposer(message_decomposer_result, message, message_modulus, num_block);
    for(int i=0; i<num_block; i++){
        auto message_decomposer_bits = message_decomposer_result[i];
        apply_lookup_table(lwe_mess_match[i], bk, lwe_cipher_blocks[i], plaintext_modulus, 
                [message_decomposer_bits](int32_t x)->int32_t{
                    return x == message_decomposer_bits;
                });
    }
}


void encrypt_str_with_private_key(vector<LweSample**> &lwe_cipher_list, //zpf vector的话可以使用 & 语义避免copy
        const string str, const int32_t message_modulus, const int32_t plaintext_modulus, 
        const int32_t num_block, const LweParams* extract_params, const LweKey *key, double alpha)
{
    vector<int32_t> str_list;
    {
        for(char ch: str){
            str_list.push_back(static_cast<int32_t>(ch)); //zpf or emplace_back
                                                          //其实这一步也并不需要，或者可以推迟到encrypt的时候
        }
    }

    {
        auto str_list_len = str_list.size();
        for(int i=0; i<str_list_len; i++){
            encrypt_with_private_key(lwe_cipher_list[i], str_list[i], message_modulus, plaintext_modulus, num_block, key, alpha);
        }
    }

}


string decrypt_str_with_private_key(vector<LweSample**> &lwe_cipher_list,
        const int32_t message_modulus, const int32_t plaintext_modulus,
        const int32_t num_block, const LweKey* key) //zpf key has included plaintext_modulus
{
    string s_after_decrypt = "";
    for(auto lwe_cipher_blocks: lwe_cipher_list){
        int32_t message_after_decrypt = decrypt_with_private_key(lwe_cipher_blocks, message_modulus, plaintext_modulus, num_block, key);
        //cout << "message_after_decrypt in func decrypt_str_with_private_key: " << message_after_decrypt << endl;
        s_after_decrypt.push_back(static_cast<char>(message_after_decrypt));
    }
        
    //cout << "s_after_decrypt: " << s_after_decrypt << " in func decrypt_str_with_private_key" << endl;
    return s_after_decrypt;
}



vector<LweSample**> encrypt_str(const string &s, const LweKey* lwe_key)
{
    const int32_t message_modulus = 4; //TODO zpf 这个应该放到LweParams中
    const int32_t num_block = 16; //TODO zpf 这里以后可以通过sizeof(T)获得

    const LweParams* extract_params = lwe_key -> params;
    const int32_t plaintext_modulus = extract_params -> plaintext_modulus;
    double alpha = extract_params -> alpha_min;

    vector<LweSample**> lwe_cipher_list;
    {   //zpf lwe_cipher_list 初始化
        //zpf lwe_cipher_list 初始化
        for(auto _: s){
            LweSample** lwe_cipher_blocks = new LweSample*[num_block];
            for(int i=0; i<num_block; i++){
                lwe_cipher_blocks[i] = new_LweSample(extract_params);
            }
            lwe_cipher_list.push_back(lwe_cipher_blocks);
        }

    }   //zpf lwe_cipher_list 初始化

    encrypt_str_with_private_key(lwe_cipher_list, s, message_modulus, plaintext_modulus, num_block, extract_params, lwe_key, alpha);

    return lwe_cipher_list; //TODO in heap
}


// zpf  encrypt_str 返回的值目前(2024年12月20日)是分配在堆上的，这个值的生命
// 周期一直跟随到程序结束(因为我们还要decrypt) 所以这里可以使用std::atexit 注册该函数
// 后续提供智能指针版本则不需要再考虑这个问题
void clean_up(vector<LweSample**> lwe_cipher_list, const int32_t num_block)
{ //zpf clean up
    for (auto lwe_cipher_blocks: lwe_cipher_list){
        for(int i=0; i<num_block; i++){
            delete lwe_cipher_blocks[i];
        }
        delete[] lwe_cipher_blocks;
    }
}



string decrypt_str(vector<LweSample**> &str_cipher, const LweKey* lwe_key)
{
   const int32_t message_modulus = 4; //zpf TODO should be included in LweParams
   const int32_t num_block = 16; // get by sizeof(T)

   const int32_t plaintext_modulus = lwe_key -> params -> plaintext_modulus;

   string s_after_decrypt = decrypt_str_with_private_key(str_cipher, message_modulus, plaintext_modulus, num_block, lwe_key);

   return s_after_decrypt;
}

