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

- 将`23i16`分解为`8`个`2bit`的`block`，即`[3u64, 1, 1, 0, 0, 0, 0, 0]`
- 调用`encrypt_words_radix_impl`加密每个`block`，得到`8`个`block`的`ciphertext`
- 调用`Ciphertext::new`构造最终的`ciphertext`，其中`message_modulus`为`4`，`carry_modulus`为`4`
- 返回最终的`ciphertext`是一个`Vec<Vec<u64>>`

## `result = &a + &b`

### `degree < MessageModulus`

根据rust代码，当`degree`在可接受范围内，直接两者相加(与论文一致)，例子中的`result = &a + &b` 就是直接相加，结果类似于`[Ciphertext(Plaintext(3u64 + 3u64)), ...]`，可以看到`6u64`已经超过`message_modulus`但是仍然小于`plaintext_modulus`

decrypt到plaintext中，有`plaintext_blocks = [Plaintext(6u64), ...]`

decode到message, 有`encoded_message = [6, 1, 1, 0, 0, 0, 0, 0]`

`message = 6 + 1*4 + 1*16 = 26 = 23i16 + 3i16`

### `degree >= MessageModulus`

当计算次数过多时，虽然noise仍然可以通过bootstrap控制，但是值可能会溢出，比如`result + result + result`，那么结果类似于`[18, 3, 3, 0, ...] = [2, 3, 3, 0, ...] (mod 16)`，decrypt得到的值就不正确

此时我们需要处理进位，`result = [01_10, 00_01, 00_01]`，高位2bits称为`CarryModulus`，低2bits称为`MessageModulus`，rust通过pbs将`result`分解为

`message_blocks = [10, 01, 01, ...]`(f = x % MessageModulus), 

`carry_blocks = [00, 01, 00, ...]`(f = x / MessageModulus), 需要进位

重新组合回`result = message_blocks + carry_blocks = [00_10, 00_10, 00_01, ...] = 2 + 2*4 + 1*16 = 26 = 23i16 + 3i16`

以上做法可以保证不会溢出

细节见`rustops/ops_add.md`

