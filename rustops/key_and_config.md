# generate key with default key

2024年11月14日
我们希望将rust的key生成方式移植到cpp中，为此我们需要了解rust的生成方式

我们仍然从这个经典例子开始

`src/high_level_api/integers/signed/ops.rs`
```rust
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
```

注意到我们虽然生成了`server_key`，但是并没有使用它，它的作用是什么之前并没有仔细考虑，使我们要传给`server`端的`public_key`吗？

lwe ciphertext `(a0, a1, ..., an, b)` can be represented as `(seed, b): ai <- generator(seed)`

通过之前的梳理，我们知道`ConfigBuilder::default()`最终返回：

`src/shortint/parameters/mod.rs`:
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

没有看到`tlwe`的相关参数，是放在`glwe_dimension`中了吗？

## `generate_keys`

`let (client_key, server_key) = generate_keys(ConfigBuilder::default());`

`src/high_level_api/keys/mod.rs`
```rust
pub fn generate_keys<C: Into<Config>>(config: C) -> (ClientKey, ServerKey) {
    let client_kc = ClientKey::generate(config);
    let server_kc = client_kc.generate_server_key();

    (client_kc, server_kc)
}
```

> 我们想知道`client_kc/server_kc`分别是什么以及之间的关系

### `ClientKey::generate(config);`

`src/high_level_api/keys/client.rs`
```rust
pub struct ClientKey {
    pub(crate) key: IntegerClientKey,
    pub(crate) tag: Tag,
}

impl ClientKey {
    /// Generates a new key from the given config.
    pub fn generate<C: Into<Config>>(config: C) -> Self {
        let config: Config = config.into();
        Self {
            key: IntegerClientKey::from(config.inner),
            tag: Tag::default(),
        }
    }
}
```

`IntegerClientKey::from(config.inner)`

`src/high_level_api/keys/inner.rs`
```rust
#[derive(Clone, Debug, serde::Serialize, serde::Deserialize, Versionize)]
#[versionize(IntegerClientKeyVersions)]
pub(crate) struct IntegerClientKey {
    pub(crate) key: crate::integer::ClientKey,
    pub(crate) dedicated_compact_private_key: Option<CompactPrivateKey>,
    pub(crate) compression_key: Option<CompressionPrivateKeys>,
}

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

`crate::integer::ClientKey::new(config.block_parameters);`

`src/integer/client_key/mod.rs`
```rust
pub struct ClientKey {
    pub(crate) key: ShortintClientKey,
}

impl ClientKey {
    /// Creates a Client Key.
    ///
    /// # Example
    ///
    /// use tfhe::integer::ClientKey;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key, that can encrypt in
    /// // radix and crt decomposition, where each block of the decomposition
    /// // have over 2 bits of message modulus.
    /// let cks = ClientKey::new(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    pub fn new<P>(parameter_set: P) -> Self
    where
        P: TryInto<ShortintParameters>,
        <P as TryInto<ShortintParameters>>::Error: std::fmt::Debug,
    {
        Self {
            key: ShortintClientKey::new(parameter_set),
        }
    }
}
```

`ShortintClientKey::new(parameter_set),`

`src/shortint/client_key/mod.rs`
```rust
pub struct ClientKey {
    pub(crate) glwe_secret_key: GlweSecretKeyOwned<u64>,
    /// Key used as the output of the keyswitch operation
    pub(crate) lwe_secret_key: LweSecretKeyOwned<u64>,
    pub parameters: ShortintParameterSet,
}

impl ClientKey {
    /// Generate a client key.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::client_key::ClientKey;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key:
    /// let cks = ClientKey::new(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    pub fn new<P>(parameters: P) -> Self
    where
        P: TryInto<ShortintParameterSet>,
        <P as TryInto<ShortintParameterSet>>::Error: Debug,
    {
        ShortintEngine::with_thread_local_mut(|engine| {
            engine.new_client_key(parameters.try_into().unwrap())
        })
    }
}
```

`engine.new_client_key(parameters.try_into().unwrap())`

`src/shortint/engine/client_side.rs`
```rust
impl ShortintEngine {
    pub fn new_client_key(&mut self, parameters: ShortintParameterSet) -> ClientKey {
        // generate the lwe secret key
        let lwe_secret_key = allocate_and_generate_new_binary_lwe_secret_key(
            parameters.lwe_dimension(),
            &mut self.secret_generator,
        );

        // generate the rlwe secret key
        let glwe_secret_key = allocate_and_generate_new_binary_glwe_secret_key(
            parameters.glwe_dimension(),
            parameters.polynomial_size(),
            &mut self.secret_generator,
        );

        // pack the keys in the client key set
        ClientKey {
            glwe_secret_key,
            lwe_secret_key,
            parameters,
        }
    }
}
```

> `lwe_secret_key, glwe_secret_key`共用同一个`secret_generator`?

`ShortintEngine`

`src/shortint/engine/mod.rs`
```rust
/// ShortintEngine
///
/// This 'engine' holds the necessary engines from [`core_crypto`](crate::core_crypto)
/// as well as the buffers that we want to keep around to save processing time.
///
/// This structs actually implements the logics into its methods.
pub struct ShortintEngine {
    /// A structure containing a single CSPRNG to generate secret key coefficients.
    pub(crate) secret_generator: SecretRandomGenerator<ActivatedRandomGenerator>,
    /// A structure containing two CSPRNGs to generate material for encryption like public masks
    /// and secret errors.
    ///
    /// The [`EncryptionRandomGenerator`] contains two CSPRNGs, one publicly seeded used to
    /// generate mask coefficients and one privately seeded used to generate errors during
    /// encryption.
    pub(crate) encryption_generator: EncryptionRandomGenerator<ActivatedRandomGenerator>,
    /// A seeder that can be called to generate 128 bits seeds, useful to create new
    /// [`EncryptionRandomGenerator`] to encrypt seeded types.
    pub(crate) seeder: DeterministicSeeder<ActivatedRandomGenerator>,
    #[cfg(feature = "zk-pok")]
    pub(crate) random_generator: RandomGenerator<ActivatedRandomGenerator>,
    pub(crate) computation_buffers: ComputationBuffers,
    ciphertext_buffers: Memory,
}
```

`let lwe_secret_key = allocate_and_generate_new_binary_lwe_secret_key`

`src/core_crypto/algorithms/lwe_secret_key_generation.rs`
```rust
pub fn allocate_and_generate_new_binary_lwe_secret_key<Scalar, Gen>(
    lwe_dimension: LweDimension,
    generator: &mut SecretRandomGenerator<Gen>,
) -> LweSecretKeyOwned<Scalar>
where
    Scalar: RandomGenerable<UniformBinary> + Numeric,
    Gen: ByteRandomGenerator,
{
    let mut lwe_secret_key = LweSecretKeyOwned::new_empty_key(Scalar::ZERO, lwe_dimension);

    generate_binary_lwe_secret_key(&mut lwe_secret_key, generator);

    lwe_secret_key
}

pub fn generate_binary_lwe_secret_key<Scalar, InCont, Gen>(
    lwe_secret_key: &mut LweSecretKey<InCont>,
    generator: &mut SecretRandomGenerator<Gen>,
) where
    Scalar: RandomGenerable<UniformBinary>,
    InCont: ContainerMut<Element = Scalar>,
    Gen: ByteRandomGenerator,
{
    generator.fill_slice_with_random_uniform_binary(lwe_secret_key.as_mut());
}
```

`generator.fill_slice_with_random_uniform_binary(lwe_secret_key.as_mut());`

`src/core_crypto/commons/generators/secret.rs`
```rust
/// A random number generator which can be used to generate secret keys.
pub struct SecretRandomGenerator<G: ByteRandomGenerator>(RandomGenerator<G>);

impl<G: ByteRandomGenerator> SecretRandomGenerator<G> {
    pub(crate) fn fill_slice_with_random_uniform_binary<Scalar>(&mut self, slice: &mut [Scalar])
    where
        Scalar: RandomGenerable<UniformBinary>,
    {
        self.0.fill_slice_with_random_uniform_binary(slice);
    }
}

```

`fill_slice_with_random_uniform_binary(slice);`

`src/core_crypto/commons/math/random/generator.rs`
```rust
pub struct RandomGenerator<G: ByteRandomGenerator>(G);

impl<G: ByteRandomGenerator> RandomGenerator<G> {
    /// Fill a slice with random uniform binary values.
    ///
    /// # Example
    ///
    /// use concrete_csprng::generators::SoftwareRandomGenerator;
    /// use concrete_csprng::seeders::Seed;
    /// use tfhe::core_crypto::commons::math::random::RandomGenerator;
    /// let mut generator = RandomGenerator::<SoftwareRandomGenerator>::new(Seed(0));
    /// let mut vec = vec![0u32; 1000];
    /// generator.fill_slice_with_random_uniform_binary(&mut vec);
    /// assert!(vec.iter().any(|&x| x != 0));
    pub fn fill_slice_with_random_uniform_binary<Scalar>(&mut self, output: &mut [Scalar])
    where
        Scalar: RandomGenerable<UniformBinary>,
    {
        Scalar::fill_slice(self, UniformBinary, output);
    }
}
```

借助该例子，我们大致明白了`client_key`是如何产生的。

#### Summary

```rust
ClientKey::generate(config)
    -> IntegerClientKey::from(config.inner)
    -> crate::integer::ClientKey::new(config.block_parameters)
    -> ShortintClientKey::new(parameter_set)
    -> engine.new_client_key(parameters.try_into().unwrap())
    -> allocate_and_generate_new_binary_lwe_secret_key(parameters.lwe_dimension(), &mut self.secret_generator,)
    -> generate_binary_lwe_secret_key(&mut lwe_secret_key, generator)
    -> generator.fill_slice_with_random_uniform_binary(lwe_secret_key.as_mut())
    -> fill_slice_with_random_uniform_binary(slice)
    -> Scalar::fill_slice(self, UniformBinary, output)
```

这个过程实在太复杂了,为什么这么复杂呢？目的是什么？

### `client_kc.generate_server_key();`

`server_key`由`client_key`产生，那么产生了什么？

`src/high_level_api/keys/client.rs`
```rust
pub struct ClientKey {
    pub(crate) key: IntegerClientKey,
    pub(crate) tag: Tag,
}

impl ClientKey {
    /// Generates a new ServerKey
    ///
    /// The `ServerKey` generated is meant to be used to initialize the global state
    /// using [crate::high_level_api::set_server_key].
    pub fn generate_server_key(&self) -> ServerKey {
        ServerKey::new(self)
    }
}
```

`ServerKey::new(self)`

`src/high_level_api/keys/server.rs`
```rust
pub struct ServerKey {
    pub(crate) key: Arc<IntegerServerKey>,
    pub(crate) tag: Tag,
}

impl ServerKey {
    pub fn new(keys: &ClientKey) -> Self {
        Self {
            key: Arc::new(IntegerServerKey::new(&keys.key)),
            tag: keys.tag.clone(),
        }
    }
}
```

`IntegerServerKey::new(&keys.key)`

`src/high_level_api/keys/inner.rs`
```rust
impl IntegerServerKey {
    pub(in crate::high_level_api) fn new(client_key: &IntegerClientKey) -> Self {
        let cks = &client_key.key;

        let (compression_key, decompression_key) = client_key.compression_key.as_ref().map_or_else(
            || (None, None),
            |a| {
                let (compression_key, decompression_key) =
                    cks.new_compression_decompression_keys(a);
                (Some(compression_key), Some(decompression_key))
            },
        );

        let base_integer_key = crate::integer::ServerKey::new_radix_server_key(cks);

        let cpk_key_switching_key_material =
            client_key
                .dedicated_compact_private_key
                .as_ref()
                .map(|(private_key, ksk_params)| {
                    let build_helper =
                        crate::integer::key_switching_key::KeySwitchingKeyBuildHelper::new(
                            (private_key, None),
                            (cks, &base_integer_key),
                            *ksk_params,
                        );

                    build_helper.into()
                });
        Self {
            key: base_integer_key,
            cpk_key_switching_key_material,
            compression_key,
            decompression_key,
        }
    }
}
```

```rust
/// A structure containing the server public key.
///
/// The server key is generated by the client and is meant to be published: the client
/// sends it to the server so it can compute homomorphic circuits.
```

根据这个解释我们可以知道`server_key`确实是我们想要的`public key`

以上虽然梳理了过程，但是由于没有给具体的例子，还是不清楚`server_key`是如何由`client_key`生成的

> 为什么`FheInt16::encrypt(23i16, &client_key);`没有用到`server_key`？


#### Summary

```rust
server_key = client_kc.generate_server_key()
    -> ServerKey::new(self)
    -> IntegerServerKey::new(&keys.key)
    -> The server key is generated by the client and is meant to be published: the client sends it to the server so it can compute homomorphic circuits.
```

### Summary

基本上就是通过`config`生成对应的`key`，cpp中也可以这样设置，当然`client_key`的生成要放在`client`端

## 遗留

> 为什么`FheInt16::encrypt(23i16, &client_key);`没有用到`server_key`？

