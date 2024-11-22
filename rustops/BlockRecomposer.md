# BlockRecomposer

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

6u64 是如何成为 `BlockRecomposer` 这一步非常重要，他决定了进位是如何表示的

我们可以先尝试写段代码看看究竟发生了什么


`decrypt_radix_impl`
`src/integer/client_key/mod.rs`
```rust
        let mut recomposer = BlockRecomposer::<T>::new(bits_in_block); // bits_in_block = 2

        for encrypted_block in blocks {
            let decrypted_block = decrypt_block(&self.key, encrypted_block); //zpf 6u64, 1, 1, 0, ...
            if !recomposer.add_unmasked(decrypted_block) {
                // End of T::BITS reached no need to try more
                // recomposition
                break;
            };
        }
```

`src/integer/block_decomposition.rs`
```rust
pub struct BlockRecomposer<T> {
    data: T, //zpf T is i16
    bit_mask: T,
    num_bits_in_block: u32,
    bit_pos: u32,
}

impl<T> BlockRecomposer<T>
where
    T: Recomposable,
{
    pub fn new(bits_per_block: u32) -> Self {
        let num_bits_in_block = bits_per_block;
        let bit_pos = 0;
        let bit_mask = 1_u32.checked_shl(bits_per_block).unwrap() - 1;
        let bit_mask = T::cast_from(bit_mask);

        Self {
            data: T::ZERO,
            bit_mask, //zpf 3
            num_bits_in_block, //zpf 2
            bit_pos, //zpf 0
        }
    }

    pub fn add_unmasked<V>(&mut self, block: V) -> bool
    where
        T: CastFrom<V>,
    {
        let casted_block = T::cast_from(block); //zpf 6i16
        self.add(casted_block)
    }

    fn add(&mut self, mut block: T) -> bool {
        if self.bit_pos >= T::BITS as u32 { //zpf i16::BITS = 16
            return false;
        }

        block <<= self.bit_pos; //zpf 6
        self.data = self.data.recomposable_wrapping_add(block); //zpf 6
        self.bit_pos += self.num_bits_in_block; //zpf 2

        true
    }
}

macro_rules! impl_recomposable_decomposable {
    (
        $($type:ty),* $(,)?
    ) => {
        $(
            impl Decomposable for $type { }
            impl Recomposable for $type {
                #[inline]
                fn recomposable_wrapping_add(self, other: Self) -> Self {
                    self.wrapping_add(other) //zpf can not find this function
                }
            }
            impl RecomposableFrom<u64> for $type { }
            impl DecomposableInto<u64> for $type { }
            impl RecomposableFrom<u8> for $type { }
            impl DecomposableInto<u8> for $type { }
        )*
    };
}

impl<const N: usize> Decomposable for StaticSignedBigInt<N> {}
impl<const N: usize> Recomposable for StaticSignedBigInt<N> {
    #[inline]
    fn recomposable_wrapping_add(mut self, other: Self) -> Self {
        self.add_assign(other);
        self
    }
}
```

- 这里再度进行不下去，这里的加法最后值是多少？猜测还是 `6i16`

## Summary

`BlockRecomposer` 行为与我们预想当中的一致，`[6u64, 1, 1, ...]`可以理解为：

`6 + 1*4 + 1 * 4^2`
