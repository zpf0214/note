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


