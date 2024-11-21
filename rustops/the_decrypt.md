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


