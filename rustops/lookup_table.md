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

仍然以 EXAMPLE 1 为例，`let result = &a + &b;` 执行完之后，`result`的 `degree` 便大于 `message_modulus` ，那么自然而然`result.block_carries_are_empty() = false`，就会进入到`lookup_table` 过程之中
即`let result = &result + &a` 那么我们就会进入到`lookup_table`的过程之中


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

