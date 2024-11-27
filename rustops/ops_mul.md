# mul

`src/high_level_api/integers/signed/ops.rs`
```rust
    /// use tfhe::prelude::*;
    /// use tfhe::{generate_keys, set_server_key, ConfigBuilder, FheInt16};
    ///
    /// let (client_key, server_key) = generate_keys(ConfigBuilder::default());
    /// set_server_key(server_key);
    ///
    /// let a = FheInt16::encrypt(3i16, &client_key);
    /// let b = FheInt16::encrypt(23i16, &client_key);
    ///
    /// let result = &a * &b;
    /// let result: i16 = result.decrypt(&client_key);
    /// assert_eq!(result, 3i16.wrapping_mul(23i16));
```

`generic_integer_impl_operation`
`src/high_level_api/integers/signed/ops.rs`
```rust
generic_integer_impl_operation!(
    rust_trait: Mul(mul),
    implem: {
        |lhs: &FheInt<_>, rhs: &FheInt<_>| {
            global_state::with_internal_keys(|key| match key {
                InternalServerKey::Cpu(cpu_key) => {
                    let inner_result = cpu_key
                        .pbs_key()
                        .mul_parallelized(&*lhs.ciphertext.on_cpu(), &*rhs.ciphertext.on_cpu());
                    FheInt::new(inner_result, cpu_key.tag.clone())
                },
                #[cfg(feature = "gpu")]
                InternalServerKey::Cuda(cuda_key) => {
                     with_thread_local_cuda_streams(|streams| {
                        let inner_result = cuda_key.key.key
                            .mul(&*lhs.ciphertext.on_gpu(), &*rhs.ciphertext.on_gpu(), streams);
                        FheInt::new(inner_result, cuda_key.tag.clone())
                    })
                }
            })
        }
    },
);
```

`mul_parallelized`
`src/integer/server_key/radix_parallel/mul.rs`
```rust
impl ServerKey {
    /// Computes homomorphically a multiplication between two ciphertexts encrypting integer values.
    ///
    /// The result is assigned to the `ct_left` ciphertext.
    ///
    /// This function, like all "default" operations (i.e. not smart, checked or unchecked), will
    /// check that the input ciphertexts block carries are empty and clears them if it's not the
    /// case and the operation requires it. It outputs a ciphertext whose block carries are always
    /// empty.
    ///
    /// This means that when using only "default" operations, a given operation (like add for
    /// example) has always the same performance characteristics from one call to another and
    /// guarantees correctness by pre-emptively clearing carries of output ciphertexts.
    ///
    /// # Warning
    ///
    /// - Multithreaded
    ///
    /// # Example
    ///
    /// use tfhe::integer::gen_keys_radix;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let num_blocks = 4;
    /// let (cks, sks) = gen_keys_radix(PARAM_MESSAGE_2_CARRY_2_KS_PBS, num_blocks);
    ///
    /// let clear_1 = 170;
    /// let clear_2 = 6;
    ///
    /// // Encrypt two messages
    /// let ctxt_1 = cks.encrypt(clear_1);
    /// let ctxt_2 = cks.encrypt(clear_2);
    ///
    /// // Compute homomorphically a multiplication
    /// let ct_res = sks.mul_parallelized(&ctxt_1, &ctxt_2);
    /// // Decrypt
    /// let res: u64 = cks.decrypt(&ct_res);
    /// assert_eq!((clear_1 * clear_2) % 256, res);
    /// //zpf 注意看这里需要mod 2^8 ; num_blocks * 2bits
    pub fn mul_parallelized<T>(&self, ct1: &T, ct2: &T) -> T
    where
        T: IntegerRadixCiphertext,
    {
        let mut ct_res = ct1.clone();
        self.mul_assign_parallelized(&mut ct_res, ct2);
        ct_res
    }

    pub fn mul_assign_parallelized<T>(&self, ct1: &mut T, ct2: &T)
    where
        T: IntegerRadixCiphertext,
    {
        let mut tmp_rhs;

        let (lhs, rhs) = match (ct1.block_carries_are_empty(), ct2.block_carries_are_empty()) {
            (true, true) => (ct1, ct2),
            (true, false) => {
                tmp_rhs = ct2.clone();
                self.full_propagate_parallelized(&mut tmp_rhs);
                //zpf 这个函数我们在处理Add的时候已经处理过了
                (ct1, &tmp_rhs)
            }
            (false, true) => {
                self.full_propagate_parallelized(ct1);
                (ct1, ct2)
            }
            (false, false) => {
                tmp_rhs = ct2.clone();
                rayon::join(
                    || self.full_propagate_parallelized(ct1),
                    || self.full_propagate_parallelized(&mut tmp_rhs),
                );
                (ct1, &tmp_rhs)
            }
        };

        self.unchecked_mul_assign_parallelized(lhs, rhs);
    }

    /// Computes homomorphically a multiplication between two ciphertexts encrypting integer values.
    ///
    /// This function computes the operation without checking if it exceeds the capacity of the
    /// ciphertext.
    ///
    /// The result is assigned to the `ct_left` ciphertext.
    ///
    /// # Warning
    ///
    /// - Multithreaded
    ///
    /// # Example
    ///
    /// use tfhe::integer::gen_keys_radix;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let num_blocks = 4;
    /// let (cks, sks) = gen_keys_radix(PARAM_MESSAGE_2_CARRY_2_KS_PBS, num_blocks);
    ///
    /// let clear_1 = 255;
    /// let clear_2 = 143;
    ///
    /// // Encrypt two messages
    /// let ctxt_1 = cks.encrypt(clear_1);
    /// let ctxt_2 = cks.encrypt(clear_2);
    ///
    /// // Compute homomorphically a multiplication
    /// let ct_res = sks.unchecked_mul_parallelized(&ctxt_1, &ctxt_2);
    ///
    /// // Decrypt
    /// let res: u64 = cks.decrypt(&ct_res);
    /// assert_eq!((clear_1 * clear_2) % 256, res);
    pub fn unchecked_mul_assign_parallelized<T>(&self, lhs: &mut T, rhs: &T)
    where
        T: IntegerRadixCiphertext,
    {
        if rhs.holds_boolean_value() {
            self.zero_out_if_condition_is_false(lhs, &rhs.blocks()[0]);
            return;
        }

        if lhs.holds_boolean_value() {
            let mut cloned_rhs = rhs.clone();
            self.zero_out_if_condition_is_false(&mut cloned_rhs, &lhs.blocks()[0]);
            *lhs = cloned_rhs;
            return;
        }

        let terms = self.compute_terms_for_mul_low(lhs, rhs);
        // zpf 仍然是这边在做计算

        if let Some(result) = self.unchecked_sum_ciphertexts_vec_parallelized(terms) {
            *lhs = result;
        } else {
            self.create_trivial_zero_assign_radix(lhs);
        }
    }

    /// This functions computes the terms resulting from multiplying each block
    /// of rhs with lhs. When summed these terms will give the low part of the result.
    /// i.e. in a (lhs: Nbit * rhs: Nbit) multiplication, summing the terms will give a N bit result
    fn compute_terms_for_mul_low<T>(&self, lhs: &T, rhs: &T) -> Vec<T>
    where
        T: IntegerRadixCiphertext,
    {
        let message_modulus = self.key.message_modulus.0;

        let lsb_block_mul_lut = self
            .key
            .generate_lookup_table_bivariate(|x, y| (x * y) % message_modulus as u64);
            //zpf 这个是最为关键的地方，双参数LUT 具体是怎么实现乘法的
            //zpf 论文包括Add实现都是单参数，双参数是怎么变为单参数的？

        let msb_block_mul_lut = self
            .key
            .generate_lookup_table_bivariate(|x, y| (x * y) / message_modulus as u64);

        let message_part_terms_generator = rhs
            .blocks()
            .par_iter()
            .enumerate()
            .filter(|(_, block)| block.degree.get() != 0)
            .map(|(i, rhs_block)| {
                let mut result = self.blockshift(lhs, i);
                result.blocks_mut()[i..]
                    .par_iter_mut()
                    .filter(|block| block.degree.get() != 0)
                    .for_each(|lhs_block| {
                        self.key.unchecked_apply_lookup_table_bivariate_assign(
                            lhs_block,
                            rhs_block,
                            &lsb_block_mul_lut,
                        );
                    });

                result
            });

        if self.message_modulus().0 > 2 {
            // Multiplying 2 blocks generates some part this is in the carry
            // we have to compute them.
            message_part_terms_generator
                .chain(
                    rhs.blocks()[..rhs.blocks().len() - 1] // last block carry would be thrown away
                        .par_iter()
                        .enumerate()
                        .filter(|(_, block)| block.degree.get() != 0)
                        .map(|(i, rhs_block)| {
                            // Here we are doing (a * b) / modulus
                            // that is, getting the carry part of the block multiplication
                            // so the shift is one block longer
                            let mut result = self.blockshift(lhs, i + 1);
                            result.blocks_mut()[i + 1..]
                                .par_iter_mut()
                                .filter(|block| block.degree.get() != 0)
                                .for_each(|lhs_block| {
                                    self.key.unchecked_apply_lookup_table_bivariate_assign(
                                        lhs_block,
                                        rhs_block,
                                        &msb_block_mul_lut,
                                    );
                                });

                            result
                        }),
                )
                .collect::<Vec<_>>()
        } else {
            message_part_terms_generator.collect::<Vec<_>>()
        }
    }


}
```

`generate_lookup_table_bivariate`
`src/shortint/server_key/bivariate_pbs.rs`
```rust
impl ServerKey {
    /// Constructs the lookup table for a given bivariate function as input.
    ///
    /// # Example
    ///
    /// ```rust
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let msg_1 = 3;
    /// let msg_2 = 2;
    ///
    /// let ct1 = cks.encrypt(msg_1);
    /// let ct2 = cks.encrypt(msg_2);
    ///
    /// let f = |x, y| (x + y) % 4;
    ///
    /// let acc = sks.generate_lookup_table_bivariate(f);
    ///
    /// let ct_res = sks.apply_lookup_table_bivariate(&ct1, &ct2, &acc);
    ///
    /// let dec = cks.decrypt(&ct_res);
    /// assert_eq!(dec, f(msg_1, msg_2));
    /// ```
    pub fn generate_lookup_table_bivariate<F>(&self, f: F) -> BivariateLookupTableOwned
    where
        F: Fn(u64, u64) -> u64,
    {
        self.generate_lookup_table_bivariate_with_factor(f, self.message_modulus)
    }

    /// Generates a bivariate accumulator
    pub fn generate_lookup_table_bivariate_with_factor<F>(
        &self,
        f: F,
        left_message_scaling: MessageModulus,
    ) -> BivariateLookupTableOwned
    where
        F: Fn(u64, u64) -> u64,
    {
        // Depending on the factor used, rhs and / or lhs may have carries
        // (degree >= message_modulus) which is why we need to apply the message_modulus
        // to clear them
        let factor_u64 = left_message_scaling.0 as u64;
        let message_modulus = self.message_modulus.0 as u64;
        let wrapped_f = |input: u64| -> u64 {
            //zpf input = lhs * factor_u64 + rhs
            //zpf 没有信息上的损失
            let lhs = (input / factor_u64) % message_modulus;
            let rhs = (input % factor_u64) % message_modulus;

            f(lhs, rhs)
        };
        let accumulator = self.generate_lookup_table(wrapped_f);
        //zpf 再次回到我们熟悉的地方了，wrapped_f 是一个单参数函数
        //可能要把具体细节带入进去才能知道具体是如何操作的

        BivariateLookupTable {
            acc: accumulator,
            ct_right_modulus: left_message_scaling,
        }
    }

    pub fn unchecked_apply_lookup_table_bivariate_assign(
        &self,
        ct_left: &mut Ciphertext,
        ct_right: &Ciphertext,
        acc: &BivariateLookupTableOwned,
    ) {
        let modulus = (ct_right.degree.get() + 1) as u64;
        assert!(modulus <= acc.ct_right_modulus.0 as u64);

        self.unchecked_scalar_mul_assign(ct_left, acc.ct_right_modulus.0 as u8);

        unchecked_add_assign(ct_left, ct_right);
        //zpf 我们看到竟然有Add的存在，看来大概率是拆成了加法？

        // Compute the PBS
        self.apply_lookup_table_assign(ct_left, &acc.acc);
    }

}
```

`unchecked_scalar_mul_assign`
`src/shortint/server_key/scalar_mul.rs`
```rust
pub(crate) fn unchecked_scalar_mul_assign(ct: &mut Ciphertext, scalar: u8) {
    ct.set_noise_level(ct.noise_level() * scalar as usize);
    ct.degree = Degree::new(ct.degree.get() * scalar as usize);

    match scalar {
        0 => {
            trivially_encrypt_lwe_ciphertext(&mut ct.ct, Plaintext(0));
        }
        1 => {
            // Multiplication by one is the identidy
        }
        scalar => {
            let scalar = u64::from(scalar);
            let cleartext_scalar = Cleartext(scalar);
            lwe_ciphertext_cleartext_mul_assign(&mut ct.ct, cleartext_scalar);
        }
    }
}
```

`lwe_ciphertext_cleartext_mul_assign`
`src/core_crypto/algorithms/lwe_linear_algebra.rs`
```rust
/// Multiply the left-hand side [`LWE ciphertext`](`LweCiphertext`) by the right-hand side cleartext
/// updating it in-place.
///
/// # Example
///
/// use tfhe::core_crypto::prelude::*;
///
/// // DISCLAIMER: these toy example parameters are not guaranteed to be secure or yield correct
/// // computations
/// // Define parameters for LweCiphertext creation
/// let lwe_dimension = LweDimension(742);
/// let lwe_noise_distribution =
///     Gaussian::from_dispersion_parameter(StandardDev(0.000007069849454709433), 0.0);
/// let ciphertext_modulus = CiphertextModulus::new_native();
///
/// // Create the PRNG
/// let mut seeder = new_seeder();
/// let seeder = seeder.as_mut();
/// let mut encryption_generator =
///     EncryptionRandomGenerator::<ActivatedRandomGenerator>::new(seeder.seed(), seeder);
/// let mut secret_generator =
///     SecretRandomGenerator::<ActivatedRandomGenerator>::new(seeder.seed());
///
/// // Create the LweSecretKey
/// let lwe_secret_key =
///     allocate_and_generate_new_binary_lwe_secret_key(lwe_dimension, &mut secret_generator);
///
/// // Create the plaintext
/// let msg = 3u64;
/// let plaintext = Plaintext(msg << 60);
/// let mul_cleartext = 2;
///
/// // Create a new LweCiphertext
/// let mut lwe = allocate_and_encrypt_new_lwe_ciphertext(
///     &lwe_secret_key,
///     plaintext,
///     lwe_noise_distribution,
///     ciphertext_modulus,
///     &mut encryption_generator,
/// );
///
/// lwe_ciphertext_cleartext_mul_assign(&mut lwe, Cleartext(mul_cleartext));
///
/// let decrypted_plaintext = decrypt_lwe_ciphertext(&lwe_secret_key, &lwe);
///
/// // Round and remove encoding
/// // First create a decomposer working on the high 4 bits corresponding to our encoding.
/// let decomposer = SignedDecomposer::new(DecompositionBaseLog(4), DecompositionLevelCount(1));
///
/// let rounded = decomposer.closest_representable(decrypted_plaintext.0);
///
/// // Remove the encoding
/// let cleartext = rounded >> 60;
///
/// // Check we recovered the expected result
/// assert_eq!(cleartext, msg * mul_cleartext);
pub fn lwe_ciphertext_cleartext_mul_assign<Scalar, InCont>(
    lhs: &mut LweCiphertext<InCont>,
    rhs: Cleartext<Scalar>,
) where
    Scalar: UnsignedInteger,
    InCont: ContainerMut<Element = Scalar>,
{
    slice_wrapping_scalar_mul_assign(lhs.as_mut(), rhs.0);
}
```

