# generic_integer_impl_operation

这个宏用于实现泛型整数的操作。重载了运算符，使得密文运算可以通过宏直接生成。

接下啦我们借由这个宏来梳理rust代码的逻辑。目的是理清楚`TFHE`的实现流程：`message space -> plaintext space -> ciphertext space -> plaintext space -> message space`。

```rust
generic_integer_impl_operation!(
    /// Adds two [FheInt]
    ///
    /// The operation is modular, i.e. on overflow it wraps around.
    ///
    /// # Example
    ///
    /// ```rust
    /// use tfhe::prelude::*;
    /// use tfhe::{generate_keys, set_server_key, ConfigBuilder, FheInt16};
    ///
    /// let (client_key, server_key) = generate_keys(ConfigBuilder::default());
    /// set_server_key(server_key);
    ///
    /// let a = FheInt16::encrypt(23i16, &client_key);
    /// let b = FheInt16::encrypt(3i16, &client_key);
    ///
    /// let result = &a + &b;
    /// let result: i16 = result.decrypt(&client_key);
    /// assert_eq!(result, 23i16 + 3i16);
    /// ```
    rust_trait: Add(add),
    implem: {
        |lhs: &FheInt<_>, rhs: &FheInt<_>| {
            global_state::with_internal_keys(|key| match key {
                InternalServerKey::Cpu(cpu_key) => {
                    let inner_result = cpu_key
                        .pbs_key()
                        .add_parallelized(&*lhs.ciphertext.on_cpu(), &*rhs.ciphertext.on_cpu());
                    FheInt::new(inner_result, cpu_key.tag.clone())
                },
                #[cfg(feature = "gpu")]
                InternalServerKey::Cuda(cuda_key) => {
                    with_thread_local_cuda_streams(|streams| {
                        let inner_result = cuda_key.key.key
                            .add(&*lhs.ciphertext.on_gpu(), &*rhs.ciphertext.on_gpu(), streams);
                        FheInt::new(inner_result, cuda_key.tag.clone())
                    })
                }
            })
        }
    },
);

```

为了能够更好的理解`TFHE`的整个操作以及`rust`的实现方法，我们需要：

- 将文档的内容全部理解
- 理解这个宏的实现逻辑

## `/// let (client_key, server_key) = generate_keys(ConfigBuilder::default());`

代码实现位于`src/high_level_api/keys/mod.rs`:

```rust
/// Generates keys using the provided config.
///
/// # Example
///
/// ```rust
/// use tfhe::{generate_keys, ConfigBuilder};
///
/// let config = ConfigBuilder::default().build();
/// let (client_key, server_key) = generate_keys(config);
/// ```
pub fn generate_keys<C: Into<Config>>(config: C) -> (ClientKey, ServerKey) {
    let client_kc = ClientKey::generate(config);
    let server_kc = client_kc.generate_server_key();

    (client_kc, server_kc)
}
```

### `ConfigBuilder::default()`

相关代码位于`src/high_level_api/config.rs`:

```rust
/// The builder to create your config
///
/// The configuration is needed to select parameters you wish to use for these types
/// (whether it is the default parameters or some custom parameters).
/// The default parameters are specialized for GPU execution
/// in case the gpu feature is activated.
#[derive(Clone)]
pub struct ConfigBuilder {
    config: Config,
}

impl Default for ConfigBuilder {
    fn default() -> Self {
        Self::default_with_big_encryption()
    }
}
impl ConfigBuilder {
    /// Use default parameters with big encryption
    /// The returned parameters are specialized for the GPU
    /// in case the gpu feature is activated
    ///
    /// For more information see [crate::core_crypto::prelude::PBSOrder::KeyswitchBootstrap]
    pub fn default_with_big_encryption() -> Self {
        Self {
            config: Config {
                inner: IntegerConfig::default_big(),
            },
        }
    }
}
```

#### `IntegerConfig::default_big()`

相关代码位于`src/high_level_api/keys/inner.rs`:

```rust
// Clippy complained that fields end in _parameters, :roll_eyes:
#[derive(Copy, Clone, Debug, serde::Serialize, serde::Deserialize, Versionize)]
#[versionize(IntegerConfigVersions)]
#[allow(clippy::struct_field_names)]
pub(crate) struct IntegerConfig {
    pub(crate) block_parameters: crate::shortint::PBSParameters,
    pub(crate) dedicated_compact_public_key_parameters: Option<(
        crate::shortint::parameters::CompactPublicKeyEncryptionParameters,
        crate::shortint::parameters::ShortintKeySwitchingParameters,
    )>,
    pub(crate) compression_parameters: Option<CompressionParameters>,
}

impl IntegerConfig {
    pub(in crate::high_level_api) fn default_big() -> Self {
        #[cfg(not(feature = "gpu"))]
        let params = crate::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS.into();
        #[cfg(feature = "gpu")]
        let params =
            crate::shortint::parameters::PARAM_GPU_MULTI_BIT_MESSAGE_2_CARRY_2_GROUP_3_KS_PBS
                .into();
        Self {
            block_parameters: params,
            dedicated_compact_public_key_parameters: None,
            compression_parameters: None,
        }
    }
}
```

1. `shortint::PBSParameters`: (struct IntegerConfig其它参数与此类似都存储了所需的值，不再copy到本文)
```rust
#[derive(Serialize, Copy, Clone, Deserialize, Debug, PartialEq, Versionize)]
#[versionize(PBSParametersVersions)]
pub enum PBSParameters {
    PBS(ClassicPBSParameters),
    MultiBitPBS(MultiBitPBSParameters),
}
```

其中：
- `ClassicPBSParameters`:
```rust
#[derive(Serialize, Copy, Clone, Deserialize, Debug, PartialEq, Versionize)]
#[versionize(ClassicPBSParametersVersions)]
pub struct ClassicPBSParameters {
    pub lwe_dimension: LweDimension, //zpf: usize
    pub glwe_dimension: GlweDimension, //zpf: usize
    pub polynomial_size: PolynomialSize, //zpf: usize
    pub lwe_noise_distribution: DynamicDistribution<u64>, //zpf: {tag: u64, DynamicDistributionPayload}
    pub glwe_noise_distribution: DynamicDistribution<u64>,
    pub pbs_base_log: DecompositionBaseLog, //zpf: usize
    pub pbs_level: DecompositionLevelCount, //zpf: usize
    pub ks_base_log: DecompositionBaseLog, //zpf: usize
    pub ks_level: DecompositionLevelCount, //zpf: usize
    pub message_modulus: MessageModulus, //zpf: usize
    pub carry_modulus: CarryModulus, //zpf: usize
    pub max_noise_level: MaxNoiseLevel, //zpf: usize
    pub log2_p_fail: f64,
    pub ciphertext_modulus: CiphertextModulus, //zpf: enum{Native, Custome(NonZeroU128)}
    pub encryption_key_choice: EncryptionKeyChoice, //zpf: enum{Big, Small}
}
```

- `MultiBitPBSParameters`:
```rust
#[derive(Serialize, Copy, Clone, Deserialize, Debug, PartialEq, Versionize)]
#[versionize(MultiBitPBSParametersVersions)]
pub struct MultiBitPBSParameters {
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
    pub grouping_factor: LweBskGroupingFactor, //zpf: usize
    pub deterministic_execution: bool,
}
```

> 这样很难看到代码之间的联系，不如画流程图，流程图也不太好画


2. `crate::shortint::parameters::PARAM_GPU_MULTI_BIT_MESSAGE_2_CARRY_2_GROUP_3_KS_PBS`
代码位于`src/shortint/parameters/mod.rs`:
```rust
pub const PARAM_MESSAGE_2_CARRY_2_KS_PBS: ClassicPBSParameters =
    PARAM_MESSAGE_2_CARRY_2_KS_PBS_GAUSSIAN_2M64;
```

其中：
```rust
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


以上我们得到什么信息？
- `ConfigBuilder::default()`生成了`Config`结构体`PARAM_MESSAGE_2_CARRY_2_KS_PBS_GAUSSIAN_2M64`，包含了算法可以公开的参数。


### `let (client_key, server_key) = generate_keys(config);`

代码位于`src/high_level_api/keys/mod.rs`:

```rust
pub use client::ClientKey;
pub fn generate_keys<C: Into<Config>>(config: C) -> (ClientKey, ServerKey) {
    let client_kc = ClientKey::generate(config);
    let server_kc = client_kc.generate_server_key();

    (client_kc, server_kc)
}
```

#### `ClientKey::generate(config);`

代码位于`src/high_level_api/keys/client.rs`:
```rust
use crate::high_level_api::keys::{CompactPrivateKey, IntegerClientKey};

/// Generates a new key from the given config.
pub fn generate<C: Into<Config>>(config: C) -> Self {
    let config: Config = config.into();
    Self {
        key: IntegerClientKey::from(config.inner),
        tag: Tag::default(),
    }
}
```

##### `IntegerClientKey::from(config.inner),`

代码位于`src/high_level_api/keys/inner.rs`:
```rust
impl From<IntegerConfig> for IntegerClientKey {
    fn from(config: IntegerConfig) -> Self {
        assert!(
            (config.block_parameters.message_modulus().0) == 2 || config.block_parameters.message_modulus().0 == 4,
            "This API only supports parameters for which the MessageModulus is 2 or 4 (1 or 2 bits per block)",
        );

        let key = crate::integer::ClientKey::new(config.block_parameters);

        let dedicated_compact_private_key = config
            .dedicated_compact_public_key_parameters
            .map(|p| (crate::integer::CompactPrivateKey::new(p.0), p.1));

        let compression_key = config
            .compression_parameters
            .map(|params| key.new_compression_private_key(params));

        Self {
            key,
            dedicated_compact_private_key,
            compression_key,
        }
    }
}
```

###### `crate::integer::ClientKey::new(config.block_parameters);`

代码位于`src/integer/client_key/mod.rs`:

```rust
use crate::shortint::{
    Ciphertext, ClientKey as ShortintClientKey, ShortintParameterSet as ShortintParameters, //zpf: 这也是为什么我们找不到ShortintClientKey的原因
};

pub struct ClientKey {
    pub(crate) key: ShortintClientKey,
}

pub fn new<P>(parameter_set: P) -> Self
where
    P: TryInto<ShortintParameters>,
    <P as TryInto<ShortintParameters>>::Error: std::fmt::Debug,
{
    Self {
        key: ShortintClientKey::new(parameter_set),
    }
}
```

- `ShortintClientKey::new(parameter_set),`
```rust
pub fn new<P>(parameters: P) -> Self
where
    P: TryInto<ShortintParameterSet>,
    <P as TryInto<ShortintParameterSet>>::Error: Debug,
{
    ShortintEngine::with_thread_local_mut(|engine| {
        engine.new_client_key(parameters.try_into().unwrap())
    })
}
```


### 小结

根据默认参数生成了`client_key`，`server_key`.

## `set_server_key(server_key);`

`src/high_level_api/global_state.rs`:

```rust
thread_local! {
    static INTERNAL_KEYS: RefCell<Option<InternalServerKey>> = const { RefCell::new(None) };
}

/// The function used to initialize internal keys.
///
/// As each thread has its own set of keys,
/// this function must be called at least once on each thread to initialize its keys. //zpf: 和我的理解是不一样的，这里考虑的是多线程情况要如何处理
///
///
/// # Example
///
/// Only working in the `main` thread
///
/// ```rust
/// use tfhe::{generate_keys, ConfigBuilder};
///
/// let config = ConfigBuilder::default().build();
/// let (client_key, server_key) = generate_keys(config);
///
/// tfhe::set_server_key(server_key);
/// // Now we can do operations on homomorphic types
/// ```
///
///
/// Working with multiple threads
///
/// ```rust
/// use std::thread;
/// use tfhe::ConfigBuilder;
///
/// let config = ConfigBuilder::default().build();
/// let (client_key, server_key) = tfhe::generate_keys(config);
/// let server_key_2 = server_key.clone();
///
/// let th1 = thread::spawn(move || {
///     tfhe::set_server_key(server_key);
///     // Now, this thread we can do operations on homomorphic types
/// });
///
/// let th2 = thread::spawn(move || {
///     tfhe::set_server_key(server_key_2);
///     // Now, this thread we can do operations on homomorphic types
/// });
///
/// th2.join().unwrap();
/// th1.join().unwrap();
/// ```
pub fn set_server_key<T: Into<InternalServerKey>>(keys: T) {
    INTERNAL_KEYS.with(|internal_keys| internal_keys.replace_with(|_old| Some(keys.into())));
}
```

### 小结

这个函数 set_server_key 的作用是将传入的 keys 设置为线程局部静态变量 INTERNAL_KEYS 的值。


## `let a = FheInt16::encrypt(23i16, &client_key);`

### `FheInt16`

由宏定义得到，我们需要仔细分析这个宏，代码位于`src/high_level_api/mod.rs`:

```rust
// internal helper macro to make it easier to `pub use`
// all necessary stuff tied to a FheUint/FheInt from the given `module_path`.
#[allow(unused)]
macro_rules! expand_pub_use_fhe_type(
    (
        pub use $module_path:path { $($fhe_type_name:ident),* $(,)? };
    )=> {

        ::paste::paste! {
            pub use $module_path::{
                $(
                    $fhe_type_name,
                    [<Compressed $fhe_type_name>],
                    [<$fhe_type_name Id>],

                    // ConformanceParams
                    [<$fhe_type_name ConformanceParams>],
                )*
            };
        }
    }
);

expand_pub_use_fhe_type!(
    pub use crate::high_level_api::integers{
        FheUint2, FheUint4, FheUint6, FheUint8, FheUint10, FheUint12, FheUint14, FheUint16,
        FheUint32, FheUint64, FheUint128, FheUint160, FheUint256, FheUint512, FheUint1024, FheUint2048,

        FheInt2, FheInt4, FheInt6, FheInt8, FheInt10, FheInt12, FheInt14, FheInt16,
        FheInt32, FheInt64, FheInt128, FheInt160, FheInt256
    };
);
```

以上的宏并不是在定义`FheInt16`，而是利用宏公开使用路径，类似于`pub use crate::high_level_api::integers::FheInt16;`。

所以这里看不到`FheInt16`的定义。我们需要去其它地方找。

#### `generic_integer_impl_scalar_div_rem!`

`src/high_level_api/integers/signed/scalar_ops.rs`:
这里也不像是定义的地方，定义没道理会这么麻烦。

#### `create_integer_wrapper_type!(name: FheUint2, clear_scalar_type: u8);`

`src/c_api/high_level_api/integers.rs`

这个宏定义特别长，但是感觉应该也不是定义的地方，这个宏可以单独拿出来仔细分析一下。

> 2024年11月5日，这个`FheInt16`好难找啊，还是没有找到
