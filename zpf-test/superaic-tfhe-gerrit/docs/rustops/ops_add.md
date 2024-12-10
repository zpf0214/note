# `ops::Add`

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

通过之前的讨论，我们知道`FheInt16::encrypt(23i16, &client_key)` 加密过程如下：

- 将`23i16`分解为`8`个`2bit`的`block`，即`[00, 00, 00, 00, 00, 01, 01, 11]`
- 调用`encrypt_words_radix_impl`加密每个`block`，得到`8`个`block`的`ciphertext`
- 调用`Ciphertext::new`构造最终的`ciphertext`，其中`message_modulus`为`4`，`carry_modulus`为`4`
- 返回最终的`ciphertext`是一个`Vec<Vec<u64>>`
- `degree` 是如何设置的？ 已经解决，参考 ***encrypt.md***

这种情况下如何做运算，比如`+`如何计算？

- `plaintext`大小是多少？这里就是`2bits`？
- 因为是一个`Torus`，进位又是如何表示？

## `let result = &a + &b;`

通过之前的讨论，我们知道`&a + &b`中的`+`被重载了，我们顺着这里继续往下讨论

`src/high_level_api/integers/signed/ops.rs`
```rust
generic_integer_impl_operation!(
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
)
```

`InternalServerKey`
`src/high_level_api/keys/server.rs`
```rust
pub enum InternalServerKey {
    Cpu(ServerKey),
    #[cfg(feature = "gpu")]
    Cuda(CudaServerKey),
}
```

`cpu_key: ServerKey`
`src/high_level_api/keys/server.rs`
```rust
pub struct ServerKey {
    pub(crate) key: Arc<IntegerServerKey>,
    pub(crate) tag: Tag,
}
```

`pbs_key()`
`src/high_level_api/keys/server.rs`
```rust
impl ServerKey {
    pub(in crate::high_level_api) fn pbs_key(&self) -> &crate::integer::ServerKey {
        self.key.pbs_key()
    }
}
```

注意该函数返回`&crate::integer::ServerKey`

`src/integer/server_key/mod.rs`
```rust
pub struct ServerKey {
    pub(crate) key: crate::shortint::ServerKey,
}
```

`crate::shortint::ServerKey`
`src/shortint/server_key/mod.rs`
```rust
pub struct ServerKey {
    pub key_switching_key: LweKeyswitchKeyOwned<u64>,
    pub bootstrapping_key: ShortintBootstrappingKey,
    // Size of the message buffer
    pub message_modulus: MessageModulus,
    // Size of the carry buffer
    pub carry_modulus: CarryModulus,
    // Maximum number of operations that can be done before emptying the operation buffer
    pub max_degree: MaxDegree,
    pub max_noise_level: MaxNoiseLevel,
    // Modulus use for computations on the ciphertext
    pub ciphertext_modulus: CiphertextModulus,
    pub pbs_order: PBSOrder,
}
```

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

`ct_left.block_carries_are_empty(),`
`src/integer/ciphertext/integer_ciphertext.rs`
```rust
pub trait IntegerRadixCiphertext: IntegerCiphertext + Sync + Send + From<Vec<Ciphertext>> {
    fn block_carries_are_empty(&self) -> bool {
        self.blocks().iter().all(Ciphertext::carry_is_empty)
    }
}
```

`Ciphertext::carry_is_empty`
`src/shortint/ciphertext/standard.rs`
```rust
pub struct Ciphertext {
    pub ct: LweCiphertextOwned<u64>,
    pub degree: Degree, //zpf usize
    // zpf Degree 初始化的时候会被设置成message_modulus -1
    // 在计算的时候值会随着计算次数而增加
    pub(crate) noise_level: NoiseLevel,
    pub message_modulus: MessageModulus,
    pub carry_modulus: CarryModulus,
    pub pbs_order: PBSOrder,
}


impl Ciphertext {
    pub fn carry_is_empty(&self) -> bool {
        self.degree.get() < self.message_modulus.0
    }
}
```

> 对于`carry_is_empty`, 我们需要知道`degree`是什么时候被赋值的，又是如何变化的, `encrypt`的时候给`degree`赋值了吗？写个程序验证一下


该方法 `block_carries_are_empty` 的主要功能是检查当前的加密整数（`Ciphertext`）中的每一个块是否都有空的进位。该方法通过遍历所有块并调用每个块的 `carry_is_empty` 方法来实现这一点。最终，如果所有块的进位都为空，则返回 `true`；否则返回 `false`。


`self.full_propagate_parallelized(&mut tmp_rhs);`
`src/integer/server_key/radix_parallel/mod.rs`
`src/integer/server_key/radix_parallel/add.rs`
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

    /// Does lhs += (rhs + carry)
    pub fn add_assign_with_carry_parallelized<T>(
        &self,
        lhs: &mut T,
        rhs: &T,
        input_carry: Option<&BooleanBlock>, //zpf None
    ) where
        T: IntegerRadixCiphertext,
    {
        if !lhs.block_carries_are_empty() {
            self.full_propagate_parallelized(lhs);
        }

        let mut cloned_rhs;

        let rhs = if rhs.block_carries_are_empty() { //zpf return rhs
            rhs
        } else {
            cloned_rhs = rhs.clone();
            self.full_propagate_parallelized(&mut cloned_rhs);
            &cloned_rhs
        };

        self.advanced_add_assign_with_carry_parallelized( //zpf return Option<BooleanBlock>
            lhs.blocks_mut(),
            rhs.blocks(),
            input_carry, //zpf none
            OutputFlag::None,
        );
    }

    /// Computes the result of `lhs += rhs + input_carry`
    ///
    /// This will selects what seems to be best algorithm to propagate carries
    /// (fully parallel vs sequential) by looking at the number of blocks and
    /// number of threads.
    ///
    /// - `lhs` and `rhs` must have the same `len()`, empty is allowed
    /// - `blocks of lhs` and `rhs` must all be without carry
    /// - blocks must have at least one bit of message and one bit of carry
    ///
    /// Returns `Some(...)` if requested_flag != ComputationFlags::None
    pub(crate) fn advanced_add_assign_with_carry_parallelized(
        &self,
        lhs: &mut [Ciphertext],
        rhs: &[Ciphertext],
        input_carry: Option<&BooleanBlock>,  //zpf None
        requested_flag: OutputFlag,          //zpf None
    ) -> Option<BooleanBlock> {
        if self.is_eligible_for_parallel_single_carry_propagation(lhs.len()) {
            self.advanced_add_assign_with_carry_at_least_4_bits(
                lhs,
                rhs,
                input_carry,
                requested_flag,
            )
        } else {
            self.advanced_add_assign_with_carry_sequential_parallelized(
                lhs,
                rhs,
                input_carry,
                requested_flag,
            )
        }
    }

    /// Does lhs += (rhs + carry)
    ///
    /// acts like the ADC assembly op, except, the flags have to be explicitly requested
    /// as they incur additional PBSes
    ///
    /// - Parameters must have at least 2 bits of message, 2 bits of carry
    /// - blocks of lhs and rhs must be clean (no carries)
    /// - lhs and rhs must have the same length
    pub(crate) fn advanced_add_assign_with_carry_at_least_4_bits(
        &self,
        lhs: &mut [Ciphertext],
        rhs: &[Ciphertext],
        input_carry: Option<&BooleanBlock>,  //zpf None
        requested_flag: OutputFlag,          //zpf None
    ) -> Option<BooleanBlock> {
        // Empty rhs is a specially allowed 'weird' case to
        // act like a 'propagate single carry' function.
        // This is not made explicit in the docs as we have a
        // `propagate_single_carry_parallelized` function which wraps this special case
        if rhs.is_empty() {
            // Technically, CarryFlag is computable, but OverflowFlag is not
            assert_eq!(
                requested_flag,
                OutputFlag::None,
                "Cannot compute flags when called in propagation mode"
            );
        } else {
            assert_eq!(
                lhs.len(),
                rhs.len(),
                "Both operands must have the same number of blocks"
            );
        }

        if lhs.is_empty() {
            // Then both are empty
            if requested_flag == OutputFlag::None {
                return None;
            }
            return Some(self.create_trivial_boolean_block(false));
        }

        let saved_last_blocks = if requested_flag == OutputFlag::Overflow {
            Some((lhs.last().cloned().unwrap(), rhs.last().cloned().unwrap()))
        } else {
            None
        };

        // Perform the block additions
        // zpf 终于终于我们看到加法被执行了
        for (lhs_b, rhs_b) in lhs.iter_mut().zip(rhs.iter()) {
            self.key.unchecked_add_assign(lhs_b, rhs_b);
        }
        if let Some(carry) = input_carry {
            self.key.unchecked_add_assign(&mut lhs[0], &carry.0);
        }

        let blocks = lhs;
        let num_blocks = blocks.len();

        let message_modulus = self.message_modulus().0 as u64;
        let num_bits_in_message = message_modulus.ilog2() as u64;

        let block_modulus = self.message_modulus().0 * self.carry_modulus().0;
        let num_bits_in_block = block_modulus.ilog2();

        // Just in case we compare with max noise level, but it should always be num_bits_in_blocks
        // with the parameters we provide
        let grouping_size = (num_bits_in_block as usize).min(self.key.max_noise_level.get());

        let mut output_flag = None;

        // First step
        let (shifted_blocks, block_states) = match requested_flag {
            OutputFlag::None => {
                let (shifted_blocks, mut block_states) =
                    self.compute_shifted_blocks_and_block_states(blocks);
                let _ = block_states.pop().unwrap();
                (shifted_blocks, block_states)
            }
            OutputFlag::Overflow => {
                let (block, (shifted_blocks, block_states)) = rayon::join(
                    || {
                        // When used on the last block of `lhs` and `rhs`, this will create a
                        // block that encodes the 2 values needed to later know if overflow did
                        // happen depending on the input carry of the last block.
                        let lut = self.key.generate_lookup_table_bivariate(|lhs, rhs| {
                            overflow_flag_preparation_lut(lhs, rhs, num_bits_in_message)
                        });
                        let (last_lhs_block, last_rhs_block) = saved_last_blocks.as_ref().unwrap();
                        self.key.unchecked_apply_lookup_table_bivariate(
                            last_lhs_block,
                            last_rhs_block,
                            &lut,
                        )
                    },
                    || {
                        let (shifted_blocks, mut block_states) =
                            self.compute_shifted_blocks_and_block_states(blocks);
                        let _ = block_states.pop().unwrap();
                        (shifted_blocks, block_states)
                    },
                );

                output_flag = Some(block);
                (shifted_blocks, block_states)
            }
            OutputFlag::Carry => {
                let (shifted_blocks, mut block_states) =
                    self.compute_shifted_blocks_and_block_states(blocks);
                let last_block_state = block_states.pop().unwrap();
                output_flag = Some(last_block_state);
                (shifted_blocks, block_states)
            }
        };

        // Second step
        let (mut prepared_blocks, resolved_carries) = {
            let (propagation_simulators, resolved_carries) = self
                .compute_propagation_simulators_and_groups_carries(grouping_size, &block_states);

            let mut prepared_blocks = shifted_blocks;
            prepared_blocks
                .iter_mut()
                .zip(propagation_simulators.iter())
                .for_each(|(block, simulator)| {
                    self.key.unchecked_add_assign(block, simulator);
                });

            match requested_flag {
                OutputFlag::None => {}
                OutputFlag::Overflow => {
                    let block = output_flag.as_mut().unwrap();
                    self.key
                        .unchecked_add_assign(block, &propagation_simulators[num_blocks - 1]);
                }
                OutputFlag::Carry => {
                    let block = output_flag.as_mut().unwrap();
                    self.key
                        .unchecked_add_assign(block, &propagation_simulators[num_blocks - 1]);
                }
            }

            (prepared_blocks, resolved_carries)
        };

        // Final step: adding resolved carries and cleaning result
        let mut add_carries_and_cleanup = || {
            let message_extract_lut = self
                .key
                .generate_lookup_table(|block| (block >> 1) % message_modulus);

            prepared_blocks
                .par_iter_mut()
                .enumerate()
                .for_each(|(i, block)| {
                    let grouping_index = i / grouping_size;
                    let carry = &resolved_carries[grouping_index];
                    self.key.unchecked_add_assign(block, carry);

                    self.key
                        .apply_lookup_table_assign(block, &message_extract_lut)
                });
        };

        match requested_flag {
            OutputFlag::None => {
                add_carries_and_cleanup();
            }
            OutputFlag::Overflow => {
                let overflow_flag_lut = self.key.generate_lookup_table(|block| {
                    let input_carry = (block >> 1) & 1;
                    let does_overflow_if_carry_is_1 = (block >> 3) & 1;
                    let does_overflow_if_carry_is_0 = (block >> 2) & 1;
                    if input_carry == 1 {
                        does_overflow_if_carry_is_1
                    } else {
                        does_overflow_if_carry_is_0
                    }
                });
                rayon::join(
                    || {
                        let block = output_flag.as_mut().unwrap();
                        self.key.unchecked_add_assign(
                            block,
                            &resolved_carries[resolved_carries.len() - 1],
                        );
                        self.key
                            .apply_lookup_table_assign(block, &overflow_flag_lut);
                    },
                    add_carries_and_cleanup,
                );
            }
            OutputFlag::Carry => {
                let carry_flag_lut = self.key.generate_lookup_table(|block| (block >> 2) & 1);

                rayon::join(
                    || {
                        let block = output_flag.as_mut().unwrap();
                        self.key.unchecked_add_assign(
                            block,
                            &resolved_carries[resolved_carries.len() - 1],
                        );
                        self.key.apply_lookup_table_assign(block, &carry_flag_lut);
                    },
                    add_carries_and_cleanup,
                );
            }
        }

        blocks.clone_from_slice(&prepared_blocks);

        match requested_flag {
            OutputFlag::None => None,
            OutputFlag::Overflow | OutputFlag::Carry => {
                output_flag.map(BooleanBlock::new_unchecked)
            }
        }
    }
    pub(crate) fn advanced_add_assign_with_carry_sequential_parallelized(
        &self,
        lhs: &mut [Ciphertext],
        rhs: &[Ciphertext],
        input_carry: Option<&BooleanBlock>, //zpf None
        requested_flag: OutputFlag,         //zpf None
    ) -> Option<BooleanBlock> {
        assert_eq!(
            lhs.len(),
            rhs.len(),
            "Both operands must have the same number of blocks"
        );

        if lhs.is_empty() {
            return if requested_flag == OutputFlag::None {
                None
            } else {
                Some(self.create_trivial_boolean_block(false))
            };
        }

        let carry =
            input_carry.map_or_else(|| self.create_trivial_boolean_block(false), Clone::clone);

        // 2_2, 3_3, 4_4
        // If we have at least 2 bits and at least as much carries
        //
        // The num blocks == 1 + requested_flag == OverflowFlag will actually result in one more
        // PBS of latency than num_blocks == 1 && requested_flag != OverflowFlag
        //
        // It happens because the computation of the overflow flag requires 2 steps,
        // and we insert these two steps in parallel to normal carry propagation.
        // The first step is done when processing the first block,
        // the second step is done when processing the last block.
        // So if the number of block is smaller than 2 then,
        // the overflow computation adds additional layer of PBS.
        if self.key.message_modulus.0 >= 4 && self.key.carry_modulus.0 >= self.key.message_modulus.0 //zpf true
        {
            self.advanced_add_assign_sequential_at_least_4_bits(
                requested_flag,
                lhs,
                rhs,
                carry,
                input_carry,
            )
        } else if self.key.message_modulus.0 == 2
            && self.key.carry_modulus.0 >= self.key.message_modulus.0
        {
            self.advanced_add_assign_sequential_at_least_2_bits(lhs, rhs, carry, requested_flag)
        } else {
            panic!(
                "Invalid combo of message modulus ({}) and carry modulus ({}) \n\
                This function requires the message modulus >= 2 and carry modulus >= message_modulus \n\
                I.e. PARAM_MESSAGE_X_CARRY_Y where X >= 1 and Y >= X.",
                self.key.message_modulus.0, self.key.carry_modulus.0
            );
        }
    }

    fn advanced_add_assign_sequential_at_least_4_bits(
        &self,
        requested_flag: OutputFlag,
        lhs: &mut [Ciphertext],
        rhs: &[Ciphertext],
        carry: BooleanBlock,
        input_carry: Option<&BooleanBlock>,
    ) -> Option<BooleanBlock> {
        let mut carry = carry.0;

        let mut overflow_flag = if requested_flag == OutputFlag::Overflow {
            let mut block = self
                .key
                .unchecked_scalar_mul(lhs.last().as_ref().unwrap(), self.message_modulus().0 as u8);
            self.key
                .unchecked_add_assign(&mut block, rhs.last().as_ref().unwrap());
            Some(block)
        } else {
            None
        };

        // Handle the first block
        self.key.unchecked_add_assign(&mut lhs[0], &rhs[0]);
        self.key.unchecked_add_assign(&mut lhs[0], &carry);

        // To be able to use carry_extract_assign in it
        carry.clone_from(&lhs[0]);
        rayon::scope(|s| {
            s.spawn(|_| {
                self.key.message_extract_assign(&mut lhs[0]);
            });

            s.spawn(|_| {
                self.key.carry_extract_assign(&mut carry);
            });

            if requested_flag == OutputFlag::Overflow {
                s.spawn(|_| {
                    // Computing the overflow flag requires an extra step for the first block

                    let overflow_flag = overflow_flag.as_mut().unwrap();
                    let num_bits_in_message = self.message_modulus().0.ilog2() as u64;
                    let lut = self.key.generate_lookup_table(|lhs_rhs| {
                        let lhs = lhs_rhs / self.message_modulus().0 as u64;
                        let rhs = lhs_rhs % self.message_modulus().0 as u64;
                        overflow_flag_preparation_lut(lhs, rhs, num_bits_in_message)
                    });
                    self.key.apply_lookup_table_assign(overflow_flag, &lut);
                });
            }
        });

        let num_blocks = lhs.len();

        // We did the first block before, the last block is done after this if,
        // so we need 3 blocks at least to enter this
        if num_blocks >= 3 {
            for (lhs_b, rhs_b) in lhs[1..num_blocks - 1]
                .iter_mut()
                .zip(rhs[1..num_blocks - 1].iter())
            {
                self.key.unchecked_add_assign(lhs_b, rhs_b);
                self.key.unchecked_add_assign(lhs_b, &carry);

                carry.clone_from(lhs_b);
                rayon::join(
                    || self.key.message_extract_assign(lhs_b),
                    || self.key.carry_extract_assign(&mut carry),
                );
            }
        }

        if num_blocks >= 2 {
            // Handle the last block
            self.key
                .unchecked_add_assign(&mut lhs[num_blocks - 1], &rhs[num_blocks - 1]);
            self.key
                .unchecked_add_assign(&mut lhs[num_blocks - 1], &carry);
        }

        if let Some(block) = overflow_flag.as_mut() {
            if num_blocks == 1 && input_carry.is_some() {
                self.key
                    .unchecked_add_assign(block, input_carry.map(|b| &b.0).unwrap());
            } else if num_blocks > 1 {
                self.key.unchecked_add_assign(block, &carry);
            }
        }

        // Note that here when num_blocks == 1 && requested_flag != Overflow nothing
        // will actually be spawned.
        rayon::scope(|s| {
            if num_blocks >= 2 {
                // To be able to use carry_extract_assign in it
                carry.clone_from(&lhs[num_blocks - 1]);

                // These would already have been done when the first block was processed
                s.spawn(|_| {
                    self.key.message_extract_assign(&mut lhs[num_blocks - 1]);
                });

                s.spawn(|_| {
                    self.key.carry_extract_assign(&mut carry);
                });
            }

            if requested_flag == OutputFlag::Overflow {
                s.spawn(|_| {
                    let overflow_flag_block = overflow_flag.as_mut().unwrap();
                    // Computing the overflow flag requires and extra step for the first block
                    let overflow_flag_lut = self.key.generate_lookup_table(|block| {
                        let input_carry = block & 1;
                        let does_overflow_if_carry_is_1 = (block >> 3) & 1;
                        let does_overflow_if_carry_is_0 = (block >> 2) & 1;
                        if input_carry == 1 {
                            does_overflow_if_carry_is_1
                        } else {
                            does_overflow_if_carry_is_0
                        }
                    });

                    self.key
                        .apply_lookup_table_assign(overflow_flag_block, &overflow_flag_lut);
                });
            }
        });

        match requested_flag {
            OutputFlag::None => None,
            OutputFlag::Overflow => {
                assert!(
                    overflow_flag.is_some(),
                    "internal error, overflow_flag should exist"
                );
                overflow_flag.map(BooleanBlock::new_unchecked)
            }
            OutputFlag::Carry => {
                carry.degree = Degree::new(1);
                Some(BooleanBlock::new_unchecked(carry))
            }
        }
    }

}
```

`full_propagate_parallelized` 函数的主要功能是实现同态加密操作中的进位传播。该函数的工作流程包括：

检查给定的 `IntegerRadixCiphertext` 类型的密文块，寻找第一个含有进位的块。
若找到这样的块，则调用另一个方法 `partial_propagate_parallelized` 对进位进行处理和传播。
如果没有块包含进位，该函数将提前返回而不进行任何其他操作。
通过并行计算的方式，函数优化了密文中多个块的进位传播，使得同态计算更加高效。整体上，这一功能是实现加密数据加法计算的重要组成部分。

`add_assign_with_carry_parallelized`
并行的带进位加法，如果有进位先处理进位(什么是进位，ciphertext下的进位具体指的是什么？如何知道需要进位了？按道理`23i16 + 3i16`是能够看到进位情况的)
确保无进位之后执行加法


`unchecked_add_assign`
`shr/shortint/server_key/add.rs`
```rust
pub(crate) fn unchecked_add_assign(ct_left: &mut Ciphertext, ct_right: &Ciphertext) {
    lwe_ciphertext_add_assign(&mut ct_left.ct, &ct_right.ct);
    //zpf degree 这里我们首次看到degree 的变化，这可能与LUT 有关
    ct_left.degree = Degree::new(ct_left.degree.get() + ct_right.degree.get());
    ct_left.set_noise_level(ct_left.noise_level() + ct_right.noise_level());
}
```


`lwe_ciphertext_add_assign`
`src/core_crypto/algorithms/lwe_linear_algebra.rs`
```rust
pub fn lwe_ciphertext_add_assign<Scalar, LhsCont, RhsCont>(
    lhs: &mut LweCiphertext<LhsCont>,
    rhs: &LweCiphertext<RhsCont>,
) where
    Scalar: UnsignedInteger,
    LhsCont: ContainerMut<Element = Scalar>,
    RhsCont: Container<Element = Scalar>,
{
    let ciphertext_modulus = rhs.ciphertext_modulus();
    if ciphertext_modulus.is_compatible_with_native_modulus() {
        lwe_ciphertext_add_assign_native_mod_compatible(lhs, rhs);
    } else {
        lwe_ciphertext_add_assign_other_mod(lhs, rhs);
    }
}
pub fn lwe_ciphertext_add_assign_native_mod_compatible<Scalar, LhsCont, RhsCont>(
    lhs: &mut LweCiphertext<LhsCont>,
    rhs: &LweCiphertext<RhsCont>,
) where
    Scalar: UnsignedInteger,
    LhsCont: ContainerMut<Element = Scalar>,
    RhsCont: Container<Element = Scalar>,
{
    assert_eq!(
        lhs.ciphertext_modulus(),
        rhs.ciphertext_modulus(),
        "Mismatched moduli between lhs ({:?}) and rhs ({:?}) LweCiphertext",
        lhs.ciphertext_modulus(),
        rhs.ciphertext_modulus()
    );
    let ciphertext_modulus = rhs.ciphertext_modulus();
    assert!(ciphertext_modulus.is_compatible_with_native_modulus());

    slice_wrapping_add_assign(lhs.as_mut(), rhs.as_ref());
}
```

`slice_wrapping_add_assign`
`src/core_crypto/algorithms/slice_algorithms.rs`
```rust
/// Add a slice containing unsigned integers to another one element-wise and in place.
///
/// # Note
///
/// Computations wrap around (similar to computing modulo $2^{n\_{bits}}$) when exceeding the
/// unsigned integer capacity.
///
/// # Example
///
/// use tfhe::core_crypto::algorithms::slice_algorithms::*;
/// let mut first = vec![1u8, 2, 3, 4, 5, 6];
/// let second = vec![255u8, 255, 255, 1, 2, 3];
/// slice_wrapping_add_assign(&mut first, &second);
/// assert_eq!(&first, &[0u8, 1, 2, 5, 7, 9]);
/// zpf: TODO 这里并没有看到进位操作，从调用链看进位操作已经被
/// 预先处理了
pub fn slice_wrapping_add_assign<Scalar>(lhs: &mut [Scalar], rhs: &[Scalar])
where
    Scalar: UnsignedInteger,
{
    assert!(
        lhs.len() == rhs.len(),
        "lhs (len: {}) and rhs (len: {}) must have the same length",
        lhs.len(),
        rhs.len()
    );

    lhs.iter_mut()
        .zip(rhs.iter())
        .for_each(|(lhs, &rhs)| *lhs = (*lhs).wrapping_add(rhs));
}
```

`wrapping_add` is a function in UnsignedInteger triat(`src/core_crypto/commons/numeric/unsigned.rs`)

### `generate_lookup_table`

这里在创建`programmable test polynomial`，这正是我们最终想要的，而且看起来与运算本身并没有关系，这里需要好好看看

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


