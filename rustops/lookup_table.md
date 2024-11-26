# `lookup_table`

## 遗留

- `fill_accumulator`具体实现细节，这里目前其实是没有搞清楚的(2024年11月26日)
- `message_blocks/carry_blocks` 是`Ciphertext(6)`，还是`6u64`?

EXAMPLE 1

`src/high_level_api/integers/signed/ops.rs`
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

- 通过***ops_add.md*** 等，我们知道，整个运算过程并不涉及`LUT`等概念，那么何时才会需要用到呢？

> `i16`被分解为`2bits * 8` 是因为`i16`还是因为`FheInt16`？
> 因为`FheInt16`，具体逻辑见***ops_add.md***

EXAMPLE 2

`src/shortint/server_key/mod.rs`
```rust
impl ServerKey {
    /// Constructs the lookup table given a function as input.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let msg = 3;
    ///
    /// let ct = cks.encrypt(msg);
    ///
    /// // Generate the lookup table for the function f: x -> x*x mod 4
    /// let f = |x: u64| x.pow(2) % 4;
    /// let lut = sks.generate_lookup_table(f);
    /// let ct_res = sks.apply_lookup_table(&ct, &lut);
    ///
    /// let dec = cks.decrypt(&ct_res);
    /// // 3**2 mod 4 = 1
    /// assert_eq!(dec, f(msg));
    pub fn generate_lookup_table<F>(&self, f: F) -> LookupTableOwned
    where
        F: Fn(u64) -> u64,
    {
        generate_lookup_table(
            self.bootstrapping_key.glwe_size(),
            self.bootstrapping_key.polynomial_size(),
            self.ciphertext_modulus,
            self.message_modulus,
            self.carry_modulus,
            f,
        )
    }
}
```

- 以上例子有如何直接使用`lookup_table` 的方式，但是重载的`+` 是如何使用的呢？
- 是否可以给个例子看看这个过程是如何被使用的

## `message_extract`

仍然以 EXAMPLE 1 为例，`let result = &a + &b;` 执行完之后，`result`的 `degree` 便大于 `message_modulus` ，那么自然而然`result.block_carries_are_empty() = false`，就会进入到`lookup_table` 过程之中
即`let result = &result + &a` 那么我们就会进入到`lookup_table`的过程之中

> 如何判断`result.block_carries_are_empty()` 是一个非常重要的问题。似乎并不是以carry被占满来判断


`add_parallelized(&*lhs.ciphertext.on_cpu(), &*rhs.ciphertext.on_cpu());`
`src/integer/server_key/radix_parallel/add.rs`
```rust
impl ServerKey {
    pub fn add_parallelized<T>(&self, ct_left: &T, ct_right: &T) -> T
    /// zpf ct_left = FheInt16::encrypt(23i16, &client_key);
    /// zpf ct_right = FheInt16::encrypt(3i16, &client_key);
    where
        T: IntegerRadixCiphertext,
    {
        let mut ct_res = ct_left.clone();
        self.add_assign_parallelized(&mut ct_res, ct_right);
        ct_res
    }

    pub fn add_assign_parallelized<T>(&self, ct_left: &mut T, ct_right: &T)
    where
        T: IntegerRadixCiphertext,
    {
        let mut tmp_rhs: T;

        let (lhs, rhs) = match (
            ct_left.block_carries_are_empty(), //zpf true
            ct_right.block_carries_are_empty(), //zpf true
        ) {
            (true, true) => (ct_left, ct_right),
            (true, false) => {
                tmp_rhs = ct_right.clone();
                self.full_propagate_parallelized(&mut tmp_rhs);
                (ct_left, &tmp_rhs)
            }
            //zpf let result = &result + &a;
            //zpf result.degree = 6;
            //zpf result = [Ciphertext(6), Ciphertext(1), Ciphertext(1), Ciphertext(0), ...]
            (false, true) => {
                self.full_propagate_parallelized(ct_left);
                (ct_left, ct_right)
            }
            (false, false) => {
                tmp_rhs = ct_right.clone();
                rayon::join(
                    || self.full_propagate_parallelized(ct_left),
                    || self.full_propagate_parallelized(&mut tmp_rhs),
                );
                (ct_left, &tmp_rhs)
            }
        };

        self.add_assign_with_carry_parallelized(lhs, rhs, None);
    }
}
```

`full_propagate_parallelized`
`src/integer/server_key/radix_parallel/mod.rs`
```rust
impl ServerKey {
    pub fn full_propagate_parallelized<T>(&self, ctxt: &mut T)
    where
        T: IntegerRadixCiphertext,
    {
        let Some(start_index) = ctxt
            .blocks()
            .iter()
            .position(|block| !block.carry_is_empty())
        else {
            // No block has any carries, do nothing
            return;
        };
        self.partial_propagate_parallelized(ctxt, start_index);
    }

    /// Propagates carries starting from start_index.
    ///
    /// Does nothing if start_index >= ctxt.len() or ctxt is empty
    pub fn partial_propagate_parallelized<T>(&self, ctxt: &mut T, mut start_index: usize)
    where
        T: IntegerRadixCiphertext,
    {
        if start_index >= ctxt.blocks().len() || ctxt.blocks().is_empty() {
            return;
        }

        // Extract message blocks and carry blocks from the
        // input block slice.
        // Carries Vec has one less block than message Vec
        let extract_message_and_carry_blocks = |blocks: &[crate::shortint::Ciphertext]| { //zpf 这是一个闭包函数，主要是在处理carry_blocks/message_blocks
            let num_blocks = blocks.len();

            rayon::join(
                || {
                    blocks
                        .par_iter()
                        .map(|block| self.key.message_extract(block))
                        .collect::<Vec<_>>()
                },
                || {
                    let mut carry_blocks = Vec::with_capacity(num_blocks);
                    // No need to compute the carry of the last block, we would just throw it away
                    blocks[..num_blocks - 1]
                        .par_iter()
                        .map(|block| self.key.carry_extract(block))
                        .collect_into_vec(&mut carry_blocks);
                    carry_blocks
                },
            )
        };

        if self.is_eligible_for_parallel_single_carry_propagation(ctxt.blocks().len()) { //zpf ture
            //zpf 判断是否进位以及执行进位加法
            let highest_degree = ctxt.blocks()[start_index..]
                .iter()
                .max_by(|block_a, block_b| block_a.degree.get().cmp(&block_b.degree.get()))
                .map(|block| block.degree.get())
                .unwrap(); // We checked for emptiness earlier

            if highest_degree >= (self.key.message_modulus.0 - 1) * 2 { //zpf true
                // At least one of the blocks has more than one carry,
                // we need to extract message and carries, then add + propagate
                let (mut message_blocks, carry_blocks) = //zpf message_blocks/carry_blocks 是Ciphertext(6)，还是6u64
                                                         //zpf 这里是在生成lookup_table，我们在进行值计算的时候应该是6u64
                    extract_message_and_carry_blocks(&ctxt.blocks()[start_index..]);

                ctxt.blocks_mut()[start_index] = message_blocks.remove(0);
                let mut lhs = T::from(message_blocks);
                let rhs = T::from(carry_blocks);
                self.add_assign_with_carry_parallelized(&mut lhs, &rhs, None); //zpf 进位之后的加法
                ctxt.blocks_mut()[start_index + 1..].clone_from_slice(lhs.blocks());
            } else {
                self.propagate_single_carry_parallelized(&mut ctxt.blocks_mut()[start_index..]);
            }
        } else {
            let maybe_highest_degree = ctxt
                // We do not care about degree of 'first' block as it won't receive any carries
                .blocks()[start_index + 1..]
                .iter()
                .max_by(|block_a, block_b| block_a.degree.get().cmp(&block_b.degree.get()))
                .map(|block| block.degree.get());

            if maybe_highest_degree.is_some_and(|degree| degree > self.key.max_degree.get()) {
                // At least one of the blocks than can receive a carry, won't be able too
                // so we need to do a first 'partial' round
                let (mut message_blocks, carry_blocks) =
                    extract_message_and_carry_blocks(&ctxt.blocks()[start_index..]);
                ctxt.blocks_mut()[start_index..].swap_with_slice(&mut message_blocks);
                for (block, carry) in ctxt.blocks_mut()[start_index + 1..]
                    .iter_mut()
                    .zip(carry_blocks.iter())
                {
                    self.key.unchecked_add_assign(block, carry);
                }
                // We can start propagation one index later as we already did the first block
                start_index += 1;
            }

            let len = ctxt.blocks().len();
            // If start_index >= len, the range is considered empty
            for i in start_index..len {
                let _ = self.propagate_parallelized(ctxt, i);
            }
        }
    }
}
```

`carry_is_empty`
`src/shortint/ciphertext/standard.rs`
```rust
impl Ciphertext {
    pub fn carry_is_empty(&self) -> bool {
        self.degree.get() < self.message_modulus.0
    }
}
```

`message_extract`
`src/shortint/server_key/mod.rs`
```rust
impl ServerKey {
    /// Extract a new ciphertext containing only the message i.e., with a cleared carry buffer.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let clear = 9;
    ///
    /// // Encrypt a message
    /// let ct = cks.unchecked_encrypt(clear);
    ///
    /// zpf 变成下面这种形式是合理的，因为 9 我们并没有按照2bits来拆分，那么直接encrypt就会变成下面这种形式
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  1 0  |   0 1   |
    ///
    /// // Compute homomorphically the message extraction
    /// let ct_res = sks.message_extract(&ct);
    ///
    /// // |     ct_res      |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  0 0  |   0 1   |
    ///
    /// // Decrypt:
    /// let res = cks.decrypt(&ct_res);
    /// assert_eq!(1, res);
    pub fn message_extract(&self, ct: &Ciphertext) -> Ciphertext {
        let mut result = ct.clone();
        self.message_extract_assign(&mut result);
        result
    }

    /// Clears the carry buffer of the input ciphertext.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let clear = 9;
    ///
    /// // Encrypt a message
    /// let mut ct = cks.unchecked_encrypt(clear);
    ///
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  1 0  |   0 1   |
    ///
    /// // Compute homomorphically the message extraction
    /// sks.message_extract_assign(&mut ct);
    ///
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  0 0  |   0 1   |
    ///
    /// // Decrypt:
    /// let res = cks.decrypt(&ct);
    /// assert_eq!(1, res);
    pub fn message_extract_assign(&self, ct: &mut Ciphertext) {
        let acc = self.generate_msg_lookup_table(|x| x, ct.message_modulus);

        self.apply_lookup_table_assign(ct, &acc);
    }

    /// Given a function as input, constructs the lookup table working on the message bits
    /// Carry bits are ignored
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let msg = 3;
    ///
    /// let ct = cks.encrypt(msg);
    ///
    /// // Generate the lookup table on message for the function f: x -> x*x
    /// let f = |x: u64| x.pow(2);
    ///
    /// let lut = sks.generate_msg_lookup_table(f, ct.message_modulus);
    /// let ct_res = sks.apply_lookup_table(&ct, &lut);
    ///
    /// let dec = cks.decrypt(&ct_res);
    /// // 3^2 mod 4 = 1
    /// assert_eq!(dec, f(msg) % 4);
    pub fn generate_msg_lookup_table<F>(&self, f: F, modulus: MessageModulus) -> LookupTableOwned
    where
        F: Fn(u64) -> u64, //zpf f: |x| x
    {
        self.generate_lookup_table(|x| f(x % modulus.0 as u64) % modulus.0 as u64)
    }

    /// Constructs the lookup table given a function as input.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let msg = 3;
    ///
    /// let ct = cks.encrypt(msg);
    ///
    /// // Generate the lookup table for the function f: x -> x*x mod 4
    /// let f = |x: u64| x.pow(2) % 4;
    /// let lut = sks.generate_lookup_table(f);
    /// let ct_res = sks.apply_lookup_table(&ct, &lut);
    ///
    /// let dec = cks.decrypt(&ct_res);
    /// // 3**2 mod 4 = 1
    /// assert_eq!(dec, f(msg));
    pub fn generate_lookup_table<F>(&self, f: F) -> LookupTableOwned
    where
        F: Fn(u64) -> u64, //zpf |x| f(x % modulus.0 as u64) % modulus.0 as u64
    {
        generate_lookup_table(
            self.bootstrapping_key.glwe_size(), //zpf 1 ?
            self.bootstrapping_key.polynomial_size(), //zpf 2048
            self.ciphertext_modulus, //zpf 16
            self.message_modulus, //zpf 4
            self.carry_modulus, //zpf 4
            f,
        )
    }

    pub fn apply_lookup_table_assign(&self, ct: &mut Ciphertext, acc: &LookupTableOwned) {
        if ct.is_trivial() { //zpf false
            self.trivial_pbs_assign(ct, acc);
            return;
        }

        ShortintEngine::with_thread_local_mut(|engine| {
            let (mut ciphertext_buffers, buffers) = engine.get_buffers(self);
            match self.pbs_order {
                PBSOrder::KeyswitchBootstrap => {
                    keyswitch_lwe_ciphertext(
                    //zpf 我们略过这个函数的细节，因为我们主要关注的是pbs
                        &self.key_switching_key,
                        &ct.ct,
                        &mut ciphertext_buffers.buffer_lwe_after_ks,
                    );

                    apply_programmable_bootstrap(
                        &self.bootstrapping_key,
                        &ciphertext_buffers.buffer_lwe_after_ks,
                        &mut ct.ct,
                        &acc.acc,
                        buffers,
                    );
                }
                PBSOrder::BootstrapKeyswitch => {
                    apply_programmable_bootstrap(
                        &self.bootstrapping_key,
                        &ct.ct,
                        &mut ciphertext_buffers.buffer_lwe_after_pbs,
                        &acc.acc,
                        buffers,
                    );

                    keyswitch_lwe_ciphertext(
                        &self.key_switching_key,
                        &ciphertext_buffers.buffer_lwe_after_pbs,
                        &mut ct.ct,
                    );
                }
            }
        });

        ct.degree = acc.degree;
        ct.set_noise_level(NoiseLevel::NOMINAL);
    }
}

pub fn generate_lookup_table<F>(
    glwe_size: GlweSize, //zpf 1
    polynomial_size: PolynomialSize, //zpf 2048
    ciphertext_modulus: CiphertextModulus, //zpf 16
    message_modulus: MessageModulus, //zpf 4
    carry_modulus: CarryModulus, //zpf 4
    f: F,
) -> LookupTableOwned
where
    F: Fn(u64) -> u64, //zpf: |x|x -> |x| f(x % modulus.0 as u64) % modulus.0 as u64
{
    let mut acc = GlweCiphertext::new(0, glwe_size, polynomial_size, ciphertext_modulus);
    //zpf 为什么返回值是 GlweCiphertext ？虽然返回值确实是一个数组，但是 GlweCiphertext 是一个二维数组
    let max_value = fill_accumulator(
        &mut acc,
        polynomial_size,
        glwe_size,
        message_modulus,
        carry_modulus,
        f,
    );

    LookupTableOwned {
        acc,
        degree: Degree::new(max_value as usize),
    }
}
```

`fill_accumulator`
`src/shortint/engine/mod.rs`
```rust
pub(crate) fn fill_accumulator<F, C>(
    accumulator: &mut GlweCiphertext<C>,
    polynomial_size: PolynomialSize, //zpf 2048
    glwe_size: GlweSize, //zpf 1
    message_modulus: MessageModulus, //zpf 4
    carry_modulus: CarryModulus, //zpf 4
    f: F, //zpf x -> x
) -> u64
where
    C: ContainerMut<Element = u64>,
    F: Fn(u64) -> u64,
{
    assert_eq!(accumulator.polynomial_size(), polynomial_size);
    assert_eq!(accumulator.glwe_size(), glwe_size);

    let mut accumulator_view = accumulator.as_mut_view();

    accumulator_view.get_mut_mask().as_mut().fill(0);

    // Modulus of the msg contained in the msg bits and operations buffer
    let modulus_sup = message_modulus.0 * carry_modulus.0;

    // N/(p/2) = size of each block
    // zpf 作用是什么？
    let box_size = polynomial_size.0 / modulus_sup; //zpf 2^11 / 2^4

    // Value of the shift we multiply our messages by
    let delta = (1_u64 << 63) / (message_modulus.0 * carry_modulus.0) as u64;

    let mut body = accumulator_view.get_mut_body();
    let accumulator_u64 = body.as_mut();

    // Tracking the max value of the function to define the degree later
    let mut max_value = 0;

    for i in 0..modulus_sup {
        let index = i * box_size;
        let f_eval = f(i as u64);
        max_value = max_value.max(f_eval);
        accumulator_u64[index..index + box_size].fill(f_eval * delta);
    } //zpf 生成programmable test polynomial

    let half_box_size = box_size / 2;

    // Negate the first half_box_size coefficients
    // zpf 为什么取反？什么理论支持的？
    // 是一个负rotate 过程，所以要取反, 参考论文
    for a_i in accumulator_u64[0..half_box_size].iter_mut() {
        *a_i = (*a_i).wrapping_neg();
    }

    // Rotate the accumulator
    // zpf rotate_left 函数搜不到，在crate.io 上有包含例子的定义
    // 就是我们所理解的rotate，不过这里还是没有给出解释为什么这样做
    // 这里的rotate 次数是定死的，所以是为什么？
    accumulator_u64.rotate_left(half_box_size);

    max_value
}
```

`GlweCiphertext`
`src/core_crypto/entities/glwe_ciphertext.rs`
```rust
pub struct GlweCiphertext<C: Container>
where
    C::Element: UnsignedInteger,
{
    data: C,
    polynomial_size: PolynomialSize,
    ciphertext_modulus: CiphertextModulus<C::Element>,
}
```

`get_buffers`
`src/shortint/engine/mod.rs`
```rust
impl ShortintEngine {
    /// Return the [`BuffersRef`] and [`ComputationBuffers`] for the given `ServerKey`
    pub fn get_buffers(
        &mut self,
        server_key: &ServerKey,
    ) -> (BuffersRef<'_>, &mut ComputationBuffers) {
        (
            self.ciphertext_buffers.as_buffers(
                server_key
                    .key_switching_key
                    .output_lwe_size()
                    .to_lwe_dimension(),
                server_key.bootstrapping_key.output_lwe_dimension(),
                server_key.ciphertext_modulus,
            ),
            &mut self.computation_buffers,
        )
    }
}
```

`BuffersRef`
`src/shortint/engine/mod.rs`
```rust
pub struct BuffersRef<'a> {
    // For the intermediate keyswitch result in the case of a big ciphertext
    pub(crate) buffer_lwe_after_ks: LweCiphertextMutView<'a, u64>,
    // For the intermediate PBS result in the case of a smallciphertext
    pub(crate) buffer_lwe_after_pbs: LweCiphertextMutView<'a, u64>,
}
```

`apply_programmable_bootstrap`
`src/shortint/server_key/mod.rs`
```rust
pub(crate) fn apply_programmable_bootstrap<InputCont, OutputCont>(
    bootstrapping_key: &ShortintBootstrappingKey,
    in_buffer: &LweCiphertext<InputCont>,
    out_buffer: &mut LweCiphertext<OutputCont>,
    acc: &GlweCiphertext<Vec<u64>>,
    buffers: &mut ComputationBuffers,
) where
    InputCont: Container<Element = u64>,
    OutputCont: ContainerMut<Element = u64>,
{
    let mut glwe_out: GlweCiphertext<_> = acc.clone();

    apply_blind_rotate(bootstrapping_key, in_buffer, &mut glwe_out, buffers);

    extract_lwe_sample_from_glwe_ciphertext(&glwe_out, out_buffer, MonomialDegree(0));
}

pub(crate) fn apply_blind_rotate<Scalar, InputCont, OutputCont>(
    bootstrapping_key: &ShortintBootstrappingKey,
    in_buffer: &LweCiphertext<InputCont>,
    acc: &mut GlweCiphertext<OutputCont>,
    buffers: &mut ComputationBuffers,
) where
    Scalar: UnsignedTorus + CastInto<usize> + CastFrom<usize> + Sync,
    InputCont: Container<Element = Scalar>,
    OutputCont: ContainerMut<Element = Scalar>,
{
    #[cfg(feature = "pbs-stats")]
    let _ = PBS_COUNT.fetch_add(1, Ordering::Relaxed);

    match bootstrapping_key {
        ShortintBootstrappingKey::Classic(fourier_bsk) => {
            let fft = Fft::new(fourier_bsk.polynomial_size());
            let fft = fft.as_view();
            buffers.resize(
                programmable_bootstrap_lwe_ciphertext_mem_optimized_requirement::<u64>(
                    fourier_bsk.glwe_size(),
                    fourier_bsk.polynomial_size(),
                    fft,
                )
                .unwrap()
                .unaligned_bytes_required(),
            );
            let stack = buffers.stack();

            // Compute the blind rotation
            blind_rotate_assign_mem_optimized(in_buffer, acc, fourier_bsk, fft, stack);
        }
        ShortintBootstrappingKey::MultiBit {
            fourier_bsk,
            thread_count,
            deterministic_execution,
        } => {
            multi_bit_blind_rotate_assign(
                in_buffer,
                acc,
                fourier_bsk,
                *thread_count,
                *deterministic_execution,
            );
        }
    };
}
```

### 总结调用链

```rust
add_parallelized(result, a)
-> full_propagate_parallelized(ct_left)
-> partial_propagate_parallelized(ctxt, start_index)
-> message_extract(block) //zpf 这个过程并没有进位处理，猜测这里做的rotate是为了去掉carry信息
-> message_extract_assign(&mut result);
-> apply_lookup_table_assign(ct, &acc)      | -> generate_msg_lookup_table(|x| x, ct.message_modulus)
                                            | -> generate_lookup_table
                                            | -> fill_accumulator
-> apply_programmable_bootstrap
```

最为关键的地方就是在做`message_extract`的时候做了一个定死了大小的rotate，从而修改了test polynomial

## `carry_extract`

`carry_extract`
`src/shortint/server_key/mod.rs`
```rust
impl ServerKey {
    /// Extract a new ciphertext encrypting the input carry buffer.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let clear = 9;
    ///
    /// // Encrypt a message
    /// let ct = cks.unchecked_encrypt(clear);
    ///
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  1 0  |   0 1   |
    ///
    /// // Compute homomorphically carry extraction
    /// let ct_res = sks.carry_extract(&ct);
    ///
    /// // |     ct_res      |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  0 0  |   1 0   |
    ///
    /// // Decrypt:
    /// let res = cks.decrypt(&ct_res);
    /// assert_eq!(2, res);
    pub fn carry_extract(&self, ct: &Ciphertext) -> Ciphertext {
        let mut result = ct.clone();
        self.carry_extract_assign(&mut result);
        result
    }

    /// Replace the input encrypted message by the value of its carry buffer.
    ///
    /// # Example
    ///
    /// use tfhe::shortint::gen_keys;
    /// use tfhe::shortint::parameters::PARAM_MESSAGE_2_CARRY_2_KS_PBS;
    ///
    /// // Generate the client key and the server key:
    /// let (cks, sks) = gen_keys(PARAM_MESSAGE_2_CARRY_2_KS_PBS);
    ///
    /// let clear = 9;
    ///
    /// // Encrypt a message
    /// let mut ct = cks.unchecked_encrypt(clear);
    ///
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  1 0  |   0 1   |
    ///
    /// // Compute homomorphically carry extraction
    /// sks.carry_extract_assign(&mut ct);
    ///
    /// // |       ct        |
    /// // | carry | message |
    /// // |-------|---------|
    /// // |  0 0  |   1 0   |
    ///
    /// // Decrypt:
    /// let res = cks.decrypt_message_and_carry(&ct);
    /// assert_eq!(2, res);
    pub fn carry_extract_assign(&self, ct: &mut Ciphertext) {
        let modulus = ct.message_modulus.0 as u64;

        let lookup_table = self.generate_lookup_table(|x| x / modulus);
        //zpf 我们再一次看到了这两个函数，验证了我之前的看法

        self.apply_lookup_table_assign(ct, &lookup_table);
    }

}
```


## 总结

本质上加法与`lookup_table`无关，还是基本的加法思路

至此，我们已经梳理清楚加法的所有内容。加法实现与论文并没有区别，也不会借用到pbs。

之所以代码中使用到了`lookup_table` ，是想要控制noise，否则直接相加也不会有什么问题
