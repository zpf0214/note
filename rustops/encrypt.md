# `encrypt`

还是要知道`encrypt`之后`ciphertext`的形式是怎么样的，不然其实还是没有办法对整个流程有更为清晰的了解

```rust
    /// let (client_key, server_key) = generate_keys(ConfigBuilder::default());
    /// set_server_key(server_key);
    ///
    /// let a = FheInt16::encrypt(23i16, &client_key);
    /// let b = FheInt16::encrypt(3i16, &client_key);
    ///
    /// let result = &a + &b;
    /// let result: i16 = result.decrypt(&client_key);
```

## `FheInt16::encrypt(23i16, &client_key);`

我们已知：`FheInt16 = FheInt<FheInt16Id>`
`src/high_level_api/integers/signed/base.rs`

```rust
pub struct FheInt<Id: FheIntId> {
    pub(in crate::high_level_api) ciphertext: RadixCiphertext,
    pub(in crate::high_level_api) id: Id,
    pub(crate) tag: Tag,
}

```

这个的返回值是什么情况？我们主要关心这个，

- 如何`encrypt`的`23i16`？
- 得到的`ciphertext`是什么类型，又包含了什么内容？

`src/high_level_api/traits.rs`

```rust
pub trait FheEncrypt<T, Key> {
    fn encrypt(value: T, key: &Key) -> Self;
}

impl<Clear, Key, T> FheEncrypt<Clear, Key> for T
where
    T: FheTryEncrypt<Clear, Key>,
{
    fn encrypt(value: Clear, key: &Key) -> Self {
        T::try_encrypt(value, key).unwrap()
    }
}

pub trait FheTryEncrypt<T, Key>
where
    Self: Sized,
{
    type Error: std::error::Error;

    fn try_encrypt(value: T, key: &Key) -> Result<Self, Self::Error>;
}
```

由`encrypt`的实现我们知道下一步是去看`try_encrypt`是如何实现的

### `try_encrypt(value: T, key: &Key) -> Result<Self, Self::Error>;`

`src/high_level_api/integers/signed/encrypt.rs`

```rust
impl<Id, T> FheTryEncrypt<T, ClientKey> for FheInt<Id>
where
    Id: FheIntId,
    T: DecomposableInto<u64> + SignedNumeric,
{
    type Error = crate::Error;

    fn try_encrypt(value: T, key: &ClientKey) -> Result<Self, Self::Error> {
        let ciphertext = key
            .key
            .key
            .encrypt_signed_radix(value, Id::num_blocks(key.message_modulus()));
        Ok(Self::new(ciphertext, key.tag.clone()))
    }
}
```

#### `Self::new(ciphertext, key.tag.clone())`

`Self::new(ciphertext, key.tag.clone())`等价于`FheInt<Id>::new(ciphertext, key.tag.clone())`

`src/high_level_api/integers/signed/base.rs`

```rust
pub struct FheInt<Id: FheIntId> {
    pub(in crate::high_level_api) ciphertext: RadixCiphertext,
    pub(in crate::high_level_api) id: Id,
    pub(crate) tag: Tag,
}

impl<Id> FheInt<Id>
where
    Id: FheIntId,
{
    pub(in crate::high_level_api) fn new(ciphertext: impl Into<RadixCiphertext>, tag: Tag) -> Self {
        Self {
            ciphertext: ciphertext.into(),
            id: Id::default(),
            tag,
        }
    }
}
```

可以看到`ciphertext`的形式，我们不关心加密的具体细节，我们想要知道加密之后的密文是什么形式

##### `pub(in crate::high_level_api) ciphertext: RadixCiphertext,`

`src/high_level_api/integers/signed/inner.rs`

```rust
pub(crate) enum RadixCiphertext {
    Cpu(crate::integer::SignedRadixCiphertext),
    #[cfg(feature = "gpu")]
    Cuda(CudaSignedRadixCiphertext),
}
```

`src/integer/ciphertext/base.rs`

```rust
pub struct BaseSignedRadixCiphertext<Block> {
    /// The blocks are stored from LSB to MSB
    pub(crate) blocks: Vec<Block>,
}
pub type SignedRadixCiphertext = BaseSignedRadixCiphertext<Ciphertext>;
```

`src/shortint/ciphertext/standard.rs`

```rust
pub struct Ciphertext {
    pub ct: LweCiphertextOwned<u64>,
    pub degree: Degree,
    pub(crate) noise_level: NoiseLevel,
    pub message_modulus: MessageModulus,
    pub carry_modulus: CarryModulus,
    pub pbs_order: PBSOrder,
}
```

`src/core_crypto/entities/lwe_ciphertext.rs`

```rust
pub struct LweCiphertext<C: Container>
where
    C::Element: UnsignedInteger,
{
    data: C,
    ciphertext_modulus: CiphertextModulus<C::Element>,
}
pub type LweCiphertextOwned<Scalar> = LweCiphertext<Vec<Scalar>>;
```

`src/core_crypto/commons/ciphertext_modulus.rs`

```rust
pub struct CiphertextModulus<Scalar: UnsignedInteger> {
    inner: CiphertextModulusInner,
    _scalar: PhantomData<Scalar>,
}
```

##### 总结

我们有

\[\begin{aligned}
ciphertext &= RadixCiphertext \\
&=Cpu(SignedRadixCiphertext) \\
&= Cpu(BaseSignedRadixCiphertext<Ciphertext>) \\
&= Cpu(Vec(Ciphertext))
\end{aligned}\]

同时，

\[\begin{aligned}
Ciphertext &= LweCiphertextOwned<u64> \\
&= LweCiphertext<Vec<u64>> \\
&= Vec<u64>
\end{aligned}\]


简单来说，`ciphertext = Vec<Vec<u64>>`

但是这个结构与论文中是如何匹配起来的？

我们还是要回去看看`encrypt`的实现

### `encrypt_signed_radix(value, Id::num_blocks(key.message_modulus()));`


#### `Id::num_blocks(key.message_modulus())`

`src/high_level_api/integers/mod.rs`

```rust
    fn num_blocks(message_modulus: MessageModulus) -> usize {
        Self::num_bits() / message_modulus.0.ilog2() as usize
        //zpf 16 / MessageModulus(4).0.ilog2() as usize
        //zpf 16 / 2 = 8
    }
```

根据例子，`Id`是`FheInt16Id`，`num_bits`的实现位于`src/high_level_api/integers/signed/static_.rs`

##### `key.message_modulus()`

`src/high_level_api/keys/client.rs`

```rust
    pub(crate) fn message_modulus(&self) -> MessageModulus {
        self.key.block_parameters().message_modulus()
    }
```

`src/high_level_api/keys/inner.rs`

```rust
    pub(crate) fn block_parameters(&self) -> crate::shortint::parameters::PBSParameters {
        self.key.parameters()
    }
```

根据以上代码，我们知道返回值是`PBSParameters`，我们去看这个

`src/shortint/parameters/mod.rs`

```rust
pub enum PBSParameters {
    PBS(ClassicPBSParameters),
    MultiBitPBS(MultiBitPBSParameters),
}

impl PBSParameters {
    pub const fn message_modulus(&self) -> MessageModulus {
        match self {
            Self::PBS(params) => params.message_modulus,
            Self::MultiBitPBS(params) => params.message_modulus,
        }
    }
}
```

`src/shortint/parameters/mod.rs`

```rust
pub struct ClassicPBSParameters {
    pub lwe_dimension: LweDimension,
    pub glwe_dimension: GlweDimension,
    pub polynomial_size: PolynomialSize,
    pub lwe_noise_distribution: DynamicDistribution<u64>,
    pub glwe_noise_distribution: DynamicDistribution<u64>,
    pub pbs_base_log: DecompositionBaseLog,
    pub pbs_level: DecompositionLevelCount,
    pub ks_base_log: DecompositionBaseLog,
    pub ks_level: DecompositionLevelCount,
    pub message_modulus: MessageModulus,
    pub carry_modulus: CarryModulus,
    pub max_noise_level: MaxNoiseLevel,
    pub log2_p_fail: f64,
    pub ciphertext_modulus: CiphertextModulus,
    pub encryption_key_choice: EncryptionKeyChoice,
}
```


这里已经与我们之前探索`client_key`的时候重叠了。

`src/shortint/parameters/mod.rs`

```rust
pub const PARAM_MESSAGE_2_CARRY_2_KS_PBS: ClassicPBSParameters =
    PARAM_MESSAGE_2_CARRY_2_KS_PBS_GAUSSIAN_2M64;
```

`src/shortint/parameters/classic/gaussian/p_fail_2_minus_64/ks_pbs.rs`

```rust
// zpf our client_key
// p-fail = 2^-64.074, algorithmic cost ~ 106, 2-norm = 5
pub const PARAM_MESSAGE_2_CARRY_2_KS_PBS_GAUSSIAN_2M64: ClassicPBSParameters =
    ClassicPBSParameters {
        lwe_dimension: LweDimension(834),
        glwe_dimension: GlweDimension(1),
        polynomial_size: PolynomialSize(2048),
        lwe_noise_distribution: DynamicDistribution::new_gaussian_from_std_dev(StandardDev(
            3.5539902359442825e-06,
        )),
        glwe_noise_distribution: DynamicDistribution::new_gaussian_from_std_dev(StandardDev(
            2.845267479601915e-15,
        )),
        pbs_base_log: DecompositionBaseLog(23),
        pbs_level: DecompositionLevelCount(1),
        ks_base_log: DecompositionBaseLog(3),
        ks_level: DecompositionLevelCount(5),
        message_modulus: MessageModulus(4),
        carry_modulus: CarryModulus(4),
        max_noise_level: MaxNoiseLevel::new(5),
        log2_p_fail: -64.074,
        ciphertext_modulus: CiphertextModulus::new_native(),
        encryption_key_choice: EncryptionKeyChoice::Big,
    };
```

###### 总结

`message_modulus`是`4`，即`key.message_modulus() = 4`

##### 总结

`Id::num_blocks(key.message_modulus()) = 8`

`encrypt_signed_radix(value, Id::num_blocks(key.message_modulus())) = encrypt_signed_radix(value,8)`


#### `encrypt_signed_radix`

`src/integer/client_key/mod.rs`

```rust
impl ClientKey {
    //zpf encrypt_signed_radix(23i16, 8)
    pub fn encrypt_signed_radix<T>(&self, message: T, num_blocks: usize) -> SignedRadixCiphertext
    where
        T: DecomposableInto<u64> + SignedNumeric,
    {
        encrypt_words_radix_impl(
            &self.key,
            message,
            num_blocks,
            crate::shortint::ClientKey::encrypt,
        )
    }
}
```

#### `encrypt_words_radix_impl`

`src/integer/encryption.rs`

```rust
/// Encrypts an arbitrary sized number under radix decomposition
///
/// This function encrypts a number represented as a slice of 64bits words
/// into an integer ciphertext in radix decomposition
///
/// - Each block in encrypted under the same `encrypting_key`.
/// - `message_words` is expected to be in the current machine byte order.
/// - `num_block` is the number of radix block the final ciphertext will have.
pub(crate) fn encrypt_words_radix_impl<BlockKey, Block, RadixCiphertextType, T, F>(
    encrypting_key: &BlockKey,
    message: T, //zpf 23i16
    num_blocks: usize, //zpf 8
    encrypt_block: F, //zpf crate::shortint::ClientKey::encrypt
) -> RadixCiphertextType
where
    T: DecomposableInto<u64>,
    BlockKey: KnowsMessageModulus,
    F: Fn(&BlockKey, u64) -> Block,
    RadixCiphertextType: From<Vec<Block>>,
{
    let message_modulus = encrypting_key.message_modulus(); //zpf 4
    let clear_block_iterator =
        create_clear_radix_block_iterator(message, message_modulus, num_blocks); //zpf (23i16, 4, 8)
    //zpf create_clear_radix_block_iterator : 23i16 decompose by 2bit as [00, 00, 00, 00, 00, 01, 01, 11]
    // 上述结果要倒序来写

    let blocks = clear_block_iterator
        .map(|clear_block| encrypt_block(encrypting_key, clear_block))
        .collect::<Vec<_>>();

    RadixCiphertextType::from(blocks)
}
```

`src/integer/encryption.rs`

```rust
pub(crate) fn create_clear_radix_block_iterator<T>(
    message: T, //zpf 23i16
    message_modulus: MessageModulus, //zpf 4
    num_blocks: usize, //zpf 8
) -> ClearRadixBlockIterator<T>
where
    T: DecomposableInto<u64>,
{
    let bits_in_block = message_modulus.0.ilog2(); //zpf 2
    let decomposer = BlockDecomposer::new(message, bits_in_block); //zpf (23i16, 2)

    decomposer
        .iter_as::<u64>()
        .chain(std::iter::repeat(0u64))
        .take(num_blocks)
    //zpf create_clear_radix_block_iterator : 23i16 decompose by 2bit as [00, 00, 00, 00, 00, 01, 01, 11]
    // 上述结果要倒序来写
}
```

`src/integer/block_decomposition.rs`

```rust
#[derive(Clone)]
pub struct BlockDecomposer<T> {
    data: T, //zpf 23i16
    bit_mask: T, //zpf 3
    num_bits_in_mask: u32, //zpf 2
    num_bits_valid: u32, //zpf 16
    padding_bit: T, //zpf i32::ZERO
    limit: Option<T>, //zpf None
}

impl<T> BlockDecomposer<T>
where
    T: Decomposable,
{
    pub fn new(value: T, bits_per_block: u32) -> Self {
        //zpf (23i16, 2, ..)
        Self::new_(value, bits_per_block, None, T::ZERO)
    }

    fn new_(value: T, bits_per_block: u32, limit: Option<T>, padding_bit: T) -> Self {//zpf T=i16
        assert!(bits_per_block <= T::BITS as u32);
        let num_bits_valid = T::BITS as u32; //zpf 16

        let num_bits_in_mask = bits_per_block; //zpf 2
        let bit_mask = 1_u32.checked_shl(bits_per_block).unwrap() - 1; //zpf 3 as u32
        let bit_mask = T::cast_from(bit_mask);
        //zpf 3 as i16

        Self {
            data: value, //zpf 23i16
            bit_mask, //zpf 3
            num_bits_in_mask, //zpf 2
            num_bits_valid, //zpf 16
            limit, //zpf None
            padding_bit, //zpf i32::ZERO
        }
    }
}
```

##### `crate::shortint::ClientKey::encrypt`

`src/shortint/client_key/mod.rs`

```rust
impl ClientKey {
    pub fn encrypt(&self, message: u64) -> Ciphertext {
        ShortintEngine::with_thread_local_mut(|engine| engine.encrypt(self, message))
    }
}
```

`src/shortint/engine/mod.rs`

```rust
thread_local! {
    static LOCAL_ENGINE: RefCell<ShortintEngine> = RefCell::new(ShortintEngine::new());
}

impl ShortintEngine {
    /// Safely gives access to the `thead_local` shortint engine
    /// to call one (or many) of its method.
    #[inline]
    pub fn with_thread_local_mut<F, R>(func: F) -> R
    where
        F: FnOnce(&mut Self) -> R,
    {
        LOCAL_ENGINE.with(|engine_cell| func(&mut engine_cell.borrow_mut()))
    }
}
```

`|engine| engine.encrypt(self, message)`

`src/shortint/engine/client_side.rs`

```rust
impl ShortintEngine {
    pub fn encrypt(&mut self, client_key: &ClientKey, message: u64) -> Ciphertext {
        self.encrypt_with_message_modulus(
            client_key,
            message, //zpf: 3 | 1 | 1 | 0 ...
            client_key.parameters.message_modulus(), //zpf 4
        )
    }

    pub(crate) fn encrypt_with_message_modulus(
        &mut self,
        client_key: &ClientKey,
        message: u64,
        message_modulus: MessageModulus,
    ) -> Ciphertext {
        let params_op_order: PBSOrder = client_key.parameters.encryption_key_choice().into();

        let (encryption_lwe_sk, encryption_noise_distribution) =
            client_key.encryption_key_and_noise();

        let ct = self.encrypt_inner_ct(
            &client_key.parameters,
            &encryption_lwe_sk,
            encryption_noise_distribution,
            message,
            message_modulus,
        );

        //This ensures that the space message_modulus*carry_modulus < param.message_modulus *
        // param.carry_modulus
        let carry_modulus = (client_key.parameters.message_modulus().0
            * client_key.parameters.carry_modulus().0)
            / message_modulus.0;

        Ciphertext::new(
            ct,
            Degree::new(message_modulus.0 - 1),
            NoiseLevel::NOMINAL,
            message_modulus,
            CarryModulus(carry_modulus),
            params_op_order,
        )
    }
}
```

### 总结

`FheInt16::encrypt(23i16, &client_key)` 加密过程如下：

- 将`23i16`分解为`8`个`2bit`的`block`，即`[00, 00, 00, 00, 00, 01, 01, 11]`
- 调用`encrypt_words_radix_impl`加密每个`block`，得到`8`个`block`的`ciphertext`
- 调用`Ciphertext::new`构造最终的`ciphertext`，其中`message_modulus`为`4`，`carry_modulus`为`4`
- 返回最终的`ciphertext`是一个`Vec<Vec<u64>>`

这种情况下如何做运算，比如`+`如何计算？

- `plaintext`大小是多少？这里就是`2bits`？
- 因为是一个`Torus`，进位又是如何表示？

