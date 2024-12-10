#ifndef classic_PBS_parameters_H
#define classic_PBS_parameters_H

///@file
///@brief contains the declaration of classic_PBS_parameters
///


struct ClassicPBSParameters{
    //TODO: 在rust中message space 和 plaintext space是严格区分的
    //c中我们一直使用的plaintext_modulus本质上是plaintext space size
    const int32_t message_modulus; ///< size of message space
    const int32_t plaintext_modulus; ///< size of plaintext space
    
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

    // programmable bootstrap parameters
    const int32_t pbs_decompose_length;
    const int32_t pbs_Bgbit;

    // TODO: add cipher space if we extend cipher sapce from Torus32 to Torus 64
};

// 更多参数可以参考rust版本中的实现
// 用来做测试的话数据过大，test用时过长，需要写一版test用
// TODO: 暂时还不清楚noise大小与其它参数之间的关系，所以如果
// 随意改动noise可能就会造成noise值溢出无法解密
EXPORT const ClassicPBSParameters PARAM_MESSAGE_8_KS_PBS_GAUSSAIN = {
    message_modulus: 4,
    plaintext_modulus: 16,
    lwe_dimension: 857,
    lwe_alpha_min: 1e-6,
    lwe_alpha_max: 1.0/(10*8), //lwe_alpha_max 作为标准差传入
    tlwe_dimension: 2,
    tlwe_polynomials_numbers: 2048,
    tlwe_alpha_min: 1e-15,
    tlwe_alpha_max: 1.0/(10*8),
    tgsw_decompose_length: 3,
    tgsw_Bgbit: 10,
    pbs_decompose_length: 15,
    pbs_Bgbit: 1,
};

// for test
EXPORT const ClassicPBSParameters TEST_PARAM_MESSAGE_8_KS_PBS_GAUSSAIN = {
    message_modulus: 4,
    plaintext_modulus: 16,
    lwe_dimension: 500,
    lwe_alpha_min: 1e-6,
    lwe_alpha_max: 1.0/(10*8), //lwe_alpha_max 作为标准差传入
    tlwe_dimension: 1,
    tlwe_polynomials_numbers: 1024,
    tlwe_alpha_min: 9e-9,
    tlwe_alpha_max: 1.0/(10*8),
    tgsw_decompose_length: 3,
    tgsw_Bgbit: 10,
    pbs_decompose_length: 15,
    pbs_Bgbit: 1,
};

#endif //classic_PBS_parameters_H
