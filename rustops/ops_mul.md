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
    /// let b = FheInt16::encrypt(7849i16, &client_key);
    ///
    /// let result = &a * &b;
    /// let result: i16 = result.decrypt(&client_key);
    /// assert_eq!(result, 3i16.wrapping_mul(7849i16));
```


