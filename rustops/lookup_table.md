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


