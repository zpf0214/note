# decrypt

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

根据 ***summary_of_ops_add.md*** 我们已知`let result = &a + &b;` 的结果，这个结构与我们之前考虑的并不一致，而且整个过程并没有使用到`LUT` ，所以我们要看看`decrypt` 这个过程是如何处理进位问题的

## `let result: i16 = result.decrypt(&client_key);`

`src/high_level_api/integers/signed/encrypt.rs`
```rust
impl<Id, ClearType> FheDecrypt<ClearType> for FheInt<Id>
where
    Id: FheIntId,
    ClearType: RecomposableSignedInteger,
{
    /// Decrypts a [FheInt] to a signed type.
    ///
    /// The unsigned type has to be explicit.
    ///
    /// # Example
    /// use tfhe::prelude::*;
    /// use tfhe::{generate_keys, set_server_key, ConfigBuilder, FheInt16};
    ///
    /// let (client_key, server_key) = generate_keys(ConfigBuilder::default());
    /// set_server_key(server_key);
    ///
    /// let a = FheInt16::encrypt(7288i16, &client_key);
    ///
    /// // i16 is explicit
    /// let decrypted: i16 = a.decrypt(&client_key);
    /// assert_eq!(decrypted, 7288i16);
    ///
    /// // i32 is explicit
    /// let decrypted: i32 = a.decrypt(&client_key);
    /// assert_eq!(decrypted, 7288i32);
    fn decrypt(&self, key: &ClientKey) -> ClearType { //zpf we find the T in decrypt_radix_impl is ClearType
    //zpf what is ClearType? In here it mean i32?
        key.key.key.decrypt_signed_radix(&self.ciphertext.on_cpu())
    }
}
```

`key.key.key.decrypt_signed_radix(&self.ciphertext.on_cpu())`
`src/integer/client_key/mod.rs`
```rust
impl<const N: usize> RecomposableSignedInteger for StaticSignedBigInt<N> {}

/// This function takes a signed integer of type `T` for which `num_bits_set`
/// have been set.
///
/// It will set the most significant bits to the value of the bit
/// at pos `num_bits_set - 1`.
///
/// This is used to correctly decrypt a signed radix ciphertext into a clear type
/// that has more bits than the original ciphertext.
///
/// This is like doing i8 as i16, i16 as i64, i6 as i8, etc
pub(in crate::integer) fn sign_extend_partial_number<T>(unpadded_value: T, num_bits_set: u32) -> T //zpf 到处都是泛型参数，搞得我很难受
where
    T: RecomposableSignedInteger,
{
    if num_bits_set >= T::BITS as u32 {
        return unpadded_value;
    }
    let sign_bit_pos = num_bits_set - 1;
    let sign_bit = (unpadded_value >> sign_bit_pos) & T::ONE;

    // Creates a padding mask
    // where bits above num_bits_set
    // are 1s if sign bit is `1` else `0`
    let padding = (T::MAX * sign_bit) << num_bits_set;
    padding | unpadded_value
}

impl ClientKey {
    pub fn decrypt_signed_radix<T>(&self, ctxt: &SignedRadixCiphertext) -> T //zpf T is return type
    where
        T: RecomposableSignedInteger,
    {
        self.decrypt_signed_radix_impl(ctxt, crate::shortint::ClientKey::decrypt_message_and_carry)
    }

    pub fn decrypt_signed_radix_impl<T, F>(
        &self,
        ctxt: &SignedRadixCiphertext, //zpf [cipher(6), cipher(1), cipher(1), ...]
        decrypt_block: F, //zpf crate::shortint::ClientKey::decrypt_message_and_carry
    ) -> T //zpf T is return type
    where
        T: RecomposableSignedInteger,
        F: Fn(&crate::shortint::ClientKey, &crate::shortint::Ciphertext) -> u64,
    {
        let message_modulus = self.parameters().message_modulus().0; //zpf 4
        assert!(message_modulus.is_power_of_two());

        // Decrypting a signed value is the same as decrypting an unsigned value
        // but, in the signed case,
        // we have to take care of the case when the clear type T has more bits
        // than what the ciphertext encrypts.
        let unpadded_value = self.decrypt_radix_impl(&ctxt.blocks, decrypt_block);

        let num_bits_in_message = message_modulus.ilog2();
        let num_bits_in_ctxt = num_bits_in_message * ctxt.blocks.len() as u32;
        sign_extend_partial_number(unpadded_value, num_bits_in_ctxt)
    }

    /// Decrypts a ciphertext in radix decomposition into 64bits
    ///
    /// The words are assumed to be in little endian order.
    fn decrypt_radix_impl<T, F>(
        &self,
        blocks: &[crate::shortint::Ciphertext], //zpf [cipher(6), cipher(1), cipher(1), ...]
        decrypt_block: F, //zpf crate::shortint::ClientKey::decrypt_message_and_carry
    ) -> T //zpf T 与self有关吗？
           //zpf 但是我们知道T是返回值
    where
        T: RecomposableFrom<u64>,
        F: Fn(&crate::shortint::ClientKey, &crate::shortint::Ciphertext) -> u64,
    {
        if blocks.is_empty() {
            return T::ZERO;
        }

        let bits_in_block = self.key.parameters.message_modulus().0.ilog2(); //zpf 2
        let mut recomposer = BlockRecomposer::<T>::new(bits_in_block);

        for encrypted_block in blocks {
            let decrypted_block = decrypt_block(&self.key, encrypted_block); //zpf 6u64, 1, 1, 0, ...
            if !recomposer.add_unmasked(decrypted_block) {
                // End of T::BITS reached no need to try more
                // recomposition
                break;
            };
        }

        recomposer.value() //zpf return 26
    }
}

impl<const N: usize> RecomposableSignedInteger for StaticSignedBigInt<N> {}

/// This function takes a signed integer of type `T` for which `num_bits_set`
/// have been set.
///
/// It will set the most significant bits to the value of the bit
/// at pos `num_bits_set - 1`.
///
/// This is used to correctly decrypt a signed radix ciphertext into a clear type
/// that has more bits than the original ciphertext.
///
/// This is like doing i8 as i16, i16 as i64, i6 as i8, etc
pub(in crate::integer) fn sign_extend_partial_number<T>(unpadded_value: T, num_bits_set: u32) -> T
where
    T: RecomposableSignedInteger,
{
    if num_bits_set >= T::BITS as u32 {
        return unpadded_value;
    }
    let sign_bit_pos = num_bits_set - 1;
    let sign_bit = (unpadded_value >> sign_bit_pos) & T::ONE;

    // Creates a padding mask
    // where bits above num_bits_set
    // are 1s if sign bit is `1` else `0`
    let padding = (T::MAX * sign_bit) << num_bits_set;
    padding | unpadded_value
}
```

`crate::shortint::ClientKey::decrypt_message_and_carry`
`src/shortint/client_key/mod.rs`
```rust
impl ClientKey {
    pub fn decrypt_message_and_carry(&self, ct: &Ciphertext) -> u64 { //zpf cipher(6)
        let decrypted_u64: u64 = self.decrypt_no_decode(ct); //zpf Plaintext(plaintext)

        let delta = (1_u64 << 63)
            / (self.parameters.message_modulus().0 * self.parameters.carry_modulus().0) as u64;

        //The bit before the message
        let rounding_bit = delta >> 1;

        //compute the rounding bit
        let rounding = (decrypted_u64 & rounding_bit) << 1;

        (decrypted_u64.wrapping_add(rounding)) / delta
    }

    pub(crate) fn decrypt_no_decode(&self, ct: &Ciphertext) -> u64 {
        let lwe_decryption_key = match ct.pbs_order {
            PBSOrder::KeyswitchBootstrap => self.large_lwe_secret_key(),
            PBSOrder::BootstrapKeyswitch => self.small_lwe_secret_key(),
        };
        decrypt_lwe_ciphertext(&lwe_decryption_key, &ct.ct).0
    }
}
```

`decrypt_lwe_ciphertext`
`src/core_crypto/algorithms/lwe_encryption.rs`
```rust
/// Decrypt an [`LWE ciphertext`](`LweCiphertext`) and return a noisy plaintext.
///
/// See [`encrypt_lwe_ciphertext`] for usage.
///
/// # Formal Definition
///
/// See the [`LWE ciphertext formal definition`](`LweCiphertext#lwe-decryption`) for the definition
/// of the encryption algorithm.
pub fn decrypt_lwe_ciphertext<Scalar, KeyCont, InputCont>(
    lwe_secret_key: &LweSecretKey<KeyCont>,
    lwe_ciphertext: &LweCiphertext<InputCont>,
) -> Plaintext<Scalar>
where
    Scalar: UnsignedInteger,
    KeyCont: Container<Element = Scalar>,
    InputCont: Container<Element = Scalar>,
{
    let ciphertext_modulus = lwe_ciphertext.ciphertext_modulus();

    if ciphertext_modulus.is_compatible_with_native_modulus() {
        decrypt_lwe_ciphertext_native_mod_compatible(lwe_secret_key, lwe_ciphertext)
    } else {
        decrypt_lwe_ciphertext_other_mod(lwe_secret_key, lwe_ciphertext)
    }
}

pub fn decrypt_lwe_ciphertext_native_mod_compatible<Scalar, KeyCont, InputCont>(
    lwe_secret_key: &LweSecretKey<KeyCont>,
    lwe_ciphertext: &LweCiphertext<InputCont>,
) -> Plaintext<Scalar>
where
    Scalar: UnsignedInteger,
    KeyCont: Container<Element = Scalar>,
    InputCont: Container<Element = Scalar>,
{
    assert!(
        lwe_ciphertext.lwe_size().to_lwe_dimension() == lwe_secret_key.lwe_dimension(),
        "Mismatch between LweDimension of output ciphertext and input secret key. \
        Got {:?} in output, and {:?} in secret key.",
        lwe_ciphertext.lwe_size().to_lwe_dimension(),
        lwe_secret_key.lwe_dimension()
    );

    let ciphertext_modulus = lwe_ciphertext.ciphertext_modulus();

    assert!(ciphertext_modulus.is_compatible_with_native_modulus());

    let (mask, body) = lwe_ciphertext.get_mask_and_body();

    let mask_key_dot_product = slice_wrapping_dot_product(mask.as_ref(), lwe_secret_key.as_ref());
    let plaintext = (*body.data).wrapping_sub(mask_key_dot_product);

    match ciphertext_modulus.kind() {
        CiphertextModulusKind::Native => Plaintext(plaintext),
        CiphertextModulusKind::NonNativePowerOfTwo => {
            // Manage power of 2 encoding
            Plaintext(
                plaintext
                    .wrapping_div(ciphertext_modulus.get_power_of_two_scaling_to_native_torus()),
            )
        }
        CiphertextModulusKind::Other => unreachable!(),
    }
}
```

`BlockRecomposer`
`src/integer/block_decomposition.rs`
```rust
pub struct BlockRecomposer<T> {
    data: T,
    bit_mask: T,
    num_bits_in_block: u32,
    bit_pos: u32,
}

```



总结调用链

```rust

decrypt(key)
    = decrypt_signed_radix(ciphertext)
    = decrypt_signed_radix_impl(ciphertext, Func)
    = decrypt_radix_impl(ciphertext, Func) //zpf Func = decrypt_message_and_carry
                                           //zpf 该函数处理了进位问题？
    = decrypt_message_and_carry(ciphertext) //zpf 这里比较关键，决定了我们最终得到的值 6u64, 1, 1, 0, ...
    = decrypt_no_decode(ciphertext)
    = decrypt_lwe_ciphertext(lwe_decryption_key, ciphertext)
    = decrypt_lwe_ciphertext_native_mod_compatible(lwe_secret_key, lwe_ciphertext)
```

- 还是不清楚6u64是如何被整合进`BlockRecomposer` 当中的，这个细节很重要，已经处理 ***BlockRecomposer.md***
- 追了好久，但是还是不知道`BlockRecomposer<T>` 中的`T` 泛型是什么类别 T = i16

## Summary

到此，我们已经基本了解`encode -> encrypt -> decrypt -> decode`的过程，但是这个例子的处理逻辑中并不包含`LUT`这一部分，我们可以继续梳理与此有关的逻辑分支

