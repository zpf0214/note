# `parameters config`

```cpp
// 需要LweParams 来初始化LweKey
struct LweKey {
   const LweParams* params;
   int32_t* key;

#ifdef __cplusplus   
   LweKey(const LweParams* params);
   ~LweKey();
   LweKey(const LweKey&) = delete; //forbidden 
   LweKey* operator=(const LweKey&) = delete; //forbidden
#endif
};

//this structure contains Lwe parameters
//this structure is constant (cannot be modified once initialized): 
//the pointer to the param can be passed directly
//to all the Lwe keys that use these params.
struct LweParams {
	const int32_t n;
	const double alpha_min;//le plus petit bruit tq sur
	const double alpha_max;//le plus gd bruit qui permet le déchiffrement



//since all members are declared constant, a constructor is 
//required in the structure.
#ifdef __cplusplus
    // LweParams的初始化非常零碎，需要的值并没有关联
    // 但是这里是(至少是)LweKey的起源的地方
	LweParams(int32_t n, double alpha_min, double alpha_max);
	~LweParams();
	LweParams(const LweParams&) = delete; //forbidden
	LweParams& operator=(const LweParams& ) = delete; //forbidden
#endif
};

// TGswParams的初始化需要TLweParams
struct TGswParams {
    const int32_t l; ///< decomp length //zpf 3
    const int32_t Bgbit;///< log_2(Bg) // zpf 15
    const int32_t Bg;///< decomposition base (must be a power of 2)
    const int32_t halfBg; ///< Bg/2
    const uint32_t maskMod; ///< Bg-1
    const TLweParams *tlwe_params; ///< Params of each row
    const int32_t kpl; ///< number of rows = (k+1)*l
    Torus32 *h; ///< powers of Bgbit
    uint32_t offset; ///< offset = Bg/2 * (2^(32-Bgbit) + 2^(32-2*Bgbit) + ... + 2^(32-l*Bgbit))

#ifdef __cplusplus

    TGswParams(int32_t l, int32_t Bgbit, const TLweParams *tlwe_params);

    ~TGswParams();

    TGswParams(const TGswParams &) = delete;

    void operator=(const TGswParams &) = delete;

#endif
};

struct TLweParams {
    const int32_t N; ///< a power of 2: degree of the polynomials
    const int32_t k; ///< number of polynomials in the mask
    const double alpha_min; ///< minimal noise s.t. the sample is secure
    const double alpha_max; ///< maximal noise s.t. we can decrypt
    const LweParams extracted_lweparams; ///< lwe params if one extracts

#ifdef __cplusplus

    TLweParams(int32_t N, int32_t k, double alpha_min, double alpha_max);

    ~TLweParams();

    TLweParams(const TLweParams &) = delete;

    void operator=(const TLweParams &) = delete;

#endif
};

/** This structure represents an integer polynomial modulo X^N+1 */
struct IntPolynomial {
   const int32_t N;
   int32_t* coefs;

#ifdef __cplusplus   
   IntPolynomial(const int32_t N);
   ~IntPolynomial();
   IntPolynomial(const IntPolynomial&) = delete; //forbidden 
   IntPolynomial* operator=(const IntPolynomial&) = delete; //forbidden
#endif
};

/** This structure represents an torus polynomial modulo X^N+1 */
struct TorusPolynomial {
   const int32_t N;
   Torus32* coefsT;

#ifdef __cplusplus   
   TorusPolynomial(const int32_t N);
   ~TorusPolynomial();
   TorusPolynomial(const TorusPolynomial&) = delete; //forbidden 
   TorusPolynomial* operator=(const TorusPolynomial&) = delete; //forbidden
#endif
};

//need TLweParams
struct TLweKey {
    const TLweParams *params; ///< the parameters of the key
    IntPolynomial *key; ///< the key (i.e k binary polynomials)
#ifdef __cplusplus

    TLweKey(const TLweParams *params);

    ~TLweKey();

    TLweKey(const TLweKey &) = delete;

    void operator=(const TLweKey &) = delete;

#endif
};
```

总的看下来，关键就在于`LweParams, TLweParams, TGswParams, IntPolynomial, TorusPolynomial` 我们需要将他们需要的参数给写到`config`中

- `config`是否需要单独放到一个文件中
- 相应的`hardcore`是放在头文件中还是放到源文件中
- 或者在头文件中使用`inline`关键字将其定义在这里

