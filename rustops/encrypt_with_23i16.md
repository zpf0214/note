# `FheInt16::encrypt(23i16, &client_key)`

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

```
a = FheInt16::encrypt(23i16, &client_key)
    = FheInt16::try_encrypt(23i16, &client_key)
    = encrypt_signed_radix(23i16, 8) //zpf Id::num_blocks has been searched
    = encrypt_words_radix_impl(&client_key, 23i16, 8, crate::shortint::ClientKey::encrypt)
    = create_clear_radix_block_iterator(23i16, 4, 8)

```

到这里我们来到了最为核心的地方`create_clear_radix_block_iterator`
`src/integers/encryption.rs`
```rust
pub(crate) fn create_clear_radix_block_iterator<T>(
    message: T,
    message_modulus: MessageModulus,
    num_blocks: usize,
) -> ClearRadixBlockIterator<T>
where
    T: DecomposableInto<u64>,
{
    let bits_in_block = message_modulus.0.ilog2();
    let decomposer = BlockDecomposer::new(message, bits_in_block);

    decomposer
        .iter_as::<u64>()
        .chain(std::iter::repeat(0u64))
        .take(num_blocks)
}
```

```
decomposer = BlockDecomposer::new(message, bits_in_block);
           = BlockDecomposer::new(23i16, 2)
           = BlockDecomposer{
                   data: 23i16,
                   bit_mask: 3, 
                   num_bits_in_block: 2,
                   num_bits_valid: 16, //T::BITS
                   limit: Option<i16>,
                   padding_bit: i16,
               }
```

我们写了一段代码验证以上讨论：
```rust
use tfhe::integer::block_decomposition::BlockDecomposer;
//use tfhe::prelude::*;
//use tfhe::{generate_keys, set_server_key, ConfigBuilder, FheInt16};

fn main() {
    let message = 23i16;
    let bits_in_block = 2;
    let num_blocks = 8;

    let decomposer = BlockDecomposer::new(message, bits_in_block);

    let clear_block_iterator = decomposer
        .iter_as::<u64>()
        .chain(std::iter::repeat(0u64))
        .take(num_blocks);

    let blocks = clear_block_iterator.collect::<Vec<_>>();

    println!("{:?}", blocks);
}
```
以上代码显示：
```rust
[3, 1, 1, 0, 0, 0, 0, 0]
```
与我们预期一致

