# `lookup_table`

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

仍然以 EXAMPLE 1 为例，`let result = &a + &b;` 执行完之后，`result`的 `degree` 便大于 `message_modulus` ，那么自然而然`result.block_carries_are_empty() = false`，就会进入到`lookup_table` 过程之中
即`let result = &result + &a` 那么我们就会进入到`lookup_table`的过程之中

`Add` 调用链中`generate_lookup_table`出现的位置
`src/integer/server_key/radix_parallel/add.rs`
```rust
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
```

我们从这里开始梳理整个流程

似乎不太好，还是要了解整个流程
