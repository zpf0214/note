# message encode

以下是rust版本中给出的例子，我们沿着这个例子看看`message`如何encode到`plaintext`，然后encrypt到`ciphertext`

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

## `FheInt16::encrypt(23i16, &client_key)`

`FheInt16`将值限制到16bits，因此如果是`i32`范围的值也只会被截取低16bits。

`message = 23i16` 会被分解为`8 * 2bits`，即`encoded_message = [3u64, 1, 1, 0, 0, 0, 0, 0]`，此时`message_modulus = 4`，

`encoded_message`中的元素后续分别被encode到`plaintext`(`plaintext_modulus = 16`), `plaintext_blocks = [Plaintext(3u64), Plaintext(1), ...]`(`Plaintext(u) = u * (1<<63) / plaintext_modulus`)

`encrypt`则是将`plaintext_blocks` 中的元素分别加密到`ciphertext`，即`ciphertext_blocks = [Ciphertext(Plaintext(3u64)), ...]`

更为详细的过程见`rustops/encrypt.md`
