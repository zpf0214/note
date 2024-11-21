# add

总结性质，不再过分关注细节

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

我们已知`23i16`在`encrypt`之前会被按照2个bit一组拆成`8*2`数组: `[3，1，1，0，0，0，0，0]`

`Plaintext = 3 * 2^63 / 2^4`, 其中`2^4`是`plaintext modulus`

`encrypt` 就是分别对`[3，1，1，0，0，0，0，0]`每一个元素加密

- 如何`encrypt`？
- 3, 1用的是相同的`random vector`?，用的是相同的`noise`？
- 对`degree`的初始化可以参考 **encrypt.md** , 做了比较详细的处理



## `let result = &a + &b;`

现在，我们对
`let a = FheInt16::encrypt(23i16, &client_key);`
已经有了直观的认识，我们可以在此基础上进一步讨论`Add`是如何做的


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

根据 ***ops_add.md*** 中的讨论，我们知道两者相加不会产生进位，如果有进位，也只会被`modulus`掉，注意看下方代码的注释：

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
/// 预先处理了 ERROR 整个调用链上也并没有进位操作
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

## 小结，没有同态计算过的两个`ciphertext`相加并不需要`LUT`


对于 `let result = &a + &b;` ，计算过程中并不会使用到`LUT`，想要使用到，估计得 `let result = &a + &result;`
可能这样才会触发相关路径

再看大致调用链：
```rust
result = &a + &b;
       = add_parallelizeda(a, b)
       = add_assign_parallelized(a, b)
       = add_assign_with_carry_parallelized(a, b, None)
       = advanced_add_assign_sequential_at_least_4_bits(a, b, None, None)
       = unchecked_add_assign(a, b)
       = lwe_ciphertext_add_assign(a, b)
       = lwe_ciphertext_add_assign_native_mod_compatible(a, b)
       = slice_wrapping_add_assign(a, b)
```

- 整个过程也不会产生进位，那么相加溢出怎么处理？
- 相加溢出是留到`decrypt` 的时候才开始处理吗？
- 如果需要用到`LUT`，应该是一个什么样的路径？
- 整个过程中`degree`是如何变化的？已经解决，参考 ***ops_add.md***

## summary

message1 = `23i16` -> `[3, 1, 1, 0, 0, 0, 0, 0]` -> plaintext1 = `[Plaintext(3), ...]` -> ciphertext1 = `[ciphertext(3), ciphertext(1), ...]`

message2 = `3i16` -> `[3, 0, 0, 0, 0, 0, 0, 0]` -> plaintext2 = `[Plaintext(3), ...]` -> ciphertext2 = `[ciphertext(3), ciphertext(0), ...]`

message1 + message2 = `[3u64, 1, 1, 0, 0, 0, 0, 0] + [3u64, 0, 0, 0, 0, 0, 0, 0]` -> `[6u64, 1, 1, 0, 0, 0, 0, 0]` -> `[2(mod 4), 2, 1, 0, 0, 0, 0, 0]` -> `2*1 + 2*4 + 1*16` -> `23 + 3`

## 问题遗留

- 好像整个过程没有看到bootstrap？
