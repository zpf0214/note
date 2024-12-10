#include<cstdint>
//奉行一个原则，哪个范围new的就在那个范围delete，这种方式并不好
struct ServerKey{
    LweKeySwitchKey *lwe_key_switching_key;
    LweBootstrappingKey *lwe_bootstrapping_key;
    int32_t message_modulus;
    //TODO: add cipher space size if we want Tours64 not Torus32
#ifdef __cplusplus
    ServerKey(const ServerKey&) = delete;
    ServerKey* operator=(const ServerKey&) = delete;
    ~ServerKey();
#endif
};
// must be kept secret
// 这里需要注意指针的二次释放问题
struct ClientKey{
    LweKey *lwe_secret_key; // plaintext <--encrypt/decrypt--> ciphertext
    TLweKey *tlwe_secret_key; // used to generate the bootstrapping keys and key switching keys

#ifdef __cplusplus
    //是否应该一步到位直接生成key？
    //不应该直接生成，我们还需要生成相应的params，
    //然后传递给server，我们要建立一个generate()
    //生成key和params
    //ClientKey(const ClassicPBSParameters *params);
    ClientKey(const ClientKey&) = delete;
    ClientKey* operator=(const ClientKey&) = delete;
    ~ClientKey();
#endif
};

ClientKey::~ClientKey(){
    // 如何避免二次释放？
}

struct ClassicPBSParameters{
    const int32_t message_modulus; ///< size of message sapce
    
    // lwe parameters
    const int32_t lwe_dimension; ///< size of the mask
    const double lwe_alpha_min; ///< minimal noise s.t. the lwe sample is secure
                                ///这里并不是指noise低于lwe_alpha_min 就不安全，
                                ///而是指低于这个做运算是安全的
    const double lwe_alpha_max; ///< maximal noise s.t. we can decrypt

    // tlwe parameters
    const int32_t tlwe_dimension; ///< a power of 2: degree of the polynomials
    const int32_t tlwe_polynomials_numbers; ///< number of polynomials in the mask
    const double tlwe_alpha_min; ///< minimal noise s.t. the tlwe sample is secure
    const double tlwe_alpha_max; ///< maximal noise s.t. we can decrypt

    // tgsw parameters
    const int32_t tgsw_decompose_length; ///< B^{decompose_length} | q
    const int32_t tgsw_Bgbit; ///< log of decomposition base (base must be a power of 2)

    // TODO: add cipher space if we extend cipher sapce from Torus32 to Torus 64
};

// 更多参数可以参考rust版本中的实现
const ClassicPBSParameters TEST_PARAM_MESSAGE_8_KS_PBS_GAUSSAIN = {
    message_modulus: 8,
    lwe_dimension: 834,
    lwe_alpha_min: 1e-06,
    lwe_alpha_max: 1.0/(10*8), //lwe_alpha_max 作为标准差传入
    tlwe_dimension: 1,
    tlwe_polynomials_numbers: 2048,
    tlwe_alpha_min: 9e-15,
    tlwe_alpha_max: 1.0/(10*8),
    tgsw_decompose_length: 10,
    tgsw_Bgbit: 1,
};

int main(){
    return 0;
}
