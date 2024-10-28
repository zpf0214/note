# bootstrap

cpp的源代码似乎有一些问题，我需要梳理整个流程，从encode -> encryption -> decryption -> decode.

同时注意一些关键步骤是否按照论文中去实现的，如果不是按照论文去实现的，是否有问题？

## 问题

在看`src/tests/small_size_test.cpp`中关于bootstratp的测试代码，其中有一段代码：

```cpp
        // 3 生成测试多项式 //zpf: 从这里的生成方式来看并不是论文中的test polynomial
        TorusPolynomial *test_vec = new_TorusPolynomial(N);
        for( int32_t i = 0 ; i < N; ++i){
            test_vec->coefsT[i] = message;
        }
```

这里的生成方式并不是按照论文中的test polynomial，而是直接将message作为测试多项式的系数。这种方式是否正确？

因为代码实现和论文并不一致，论文是放在Torus上的，但是实现的时候放到了int32_t上。那么如何完成从Torus到int32_t的转换？

针对这个问题，我们需要一个完整的过程，从`encode -> encryption -> decryption -> decode`。

我们去看源码中已有的测试，找找是否有相关的测试。

```shell
src/test/fakes/lwe-bootstrapping.h:260:        TorusPolynomial *testvect = new_TorusPolynomial(N);
src/test/fakes/lwe-bootstrapping.h:270:        for (int32_t i = 0; i < N; i++) testvect->coefsT[i] = mu;
src/test/fakes/lwe-bootstrapping.h:272:        //fake_tfhe_blindRotateAndExtract(u, testvect, bk->bk, barb, bara, n, bk_params);
src/test/fakes/lwe-bootstrapping.h:273:        fake_tfhe_blindRotateAndExtract(result, testvect, bk->bk, barb, bara, n, bk_params);
src/test/fakes/lwe-bootstrapping.h:279:        delete_TorusPolynomial(testvect);
src/test/fakes/lwe-bootstrapping-fft.h:255:        TorusPolynomial *testvect = new_TorusPolynomial(N);
src/test/fakes/lwe-bootstrapping-fft.h:265:        for (int32_t i = 0; i < N; i++) testvect->coefsT[i] = mu;
src/test/fakes/lwe-bootstrapping-fft.h:267:        //fake_tfhe_blindRotateAndExtract_FFT(u, testvect, bkFFT->bkFFT, barb, bara, n, bk_params);
src/test/fakes/lwe-bootstrapping-fft.h:268:        fake_tfhe_blindRotateAndExtract_FFT(result, testvect, bkFFT->bkFFT, barb, bara, n, bk_params);
src/test/fakes/lwe-bootstrapping-fft.h:274:        delete_TorusPolynomial(testvect);
src/libtfhe/tgsw-fft-operations.cpp:176:    TorusPolynomial* testvect=new_TorusPolynomial(N);
src/libtfhe/tgsw-fft-operations.cpp:182:       testvect->coefsT[i]=aa;
src/libtfhe/tgsw-fft-operations.cpp:184:       testvect->coefsT[i]=-aa;
src/libtfhe/tgsw-fft-operations.cpp:185:    torusPolynomialMulByXai(testvectbis, barb, testvect);
src/libtfhe/tgsw-fft-operations.cpp:189:    // Accumulateur acc = fft((0, testvect))
src/libtfhe/tgsw-fft-operations.cpp:220:        torusPolynomialMulByXai(testvectbis, correctOffset, testvect); //celui-ci, c'est la phase idéale (calculée sans bruit avec la clé privée)
src/libtfhe/tgsw-fft-operations.cpp:245:    delete_TorusPolynomial(testvect);
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:181:    TorusPolynomial *testvect = new_TorusPolynomial(N);
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:192:    for (int32_t i = 0; i < N; i++) testvect->coefsT[i] = mu;
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:195:    tfhe_blindRotateAndExtract_FFT(result, testvect, bk->bkFFT, barb, bara, n, bk_params);
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:199:    delete_TorusPolynomial(testvect);
src/libtfhe/lwe-bootstrapping-functions.cpp:150:    TorusPolynomial *testvect = new_TorusPolynomial(N);
src/libtfhe/lwe-bootstrapping-functions.cpp:162:    for (int32_t i = 0; i < N; i++) testvect->coefsT[i] = mu;
src/libtfhe/lwe-bootstrapping-functions.cpp:164:    tfhe_blindRotateAndExtract(result, testvect, bk->bk, barb, bara, n, bk_params);
src/libtfhe/lwe-bootstrapping-functions.cpp:167:    delete_TorusPolynomial(testvect);
```

含有`testvect`名称的地方就只有这几个，而且生成方式除了`testvect->coefsT[i]=aa;`是在做`blindRotate`之外，其他地方都是直接将`message`(是明文还是密文？)作为系数来对`testvect`进行赋值(初始化)。

但是这与论文中的`test polynomial`的生成方式不一致，而且`test polynomial`的生成方式并没有在源码中找到。

去看看使用`testvect`的地方是否有什么论文中没有提到的技巧。或者真的是cpp源码的实现方式有问题。

### `src/test/fakes/lwe-bootstrapping.h`

`testvect`位于函数`old_fake_tfhe_bootstrap`中，从名字可以看出已经被废弃了。`grep`了一下，确实没有被使用到。

### `src/test/fakes/lwe-bootstrapping-fft.h`

同上，`testvect`位于函数`old_fake_tfhe_bootstrap_FFT`中，从名字可以看出已经被废弃了。`grep`了一下，确实没有被使用到。

### `src/libtfhe/tgsw-fft-operations.cpp`

代码中与`testvect`相关的地方全部被注释掉了，说明`testvect`并没有被使用到。

### `src/libtfhe/lwe-bootstrapping-functions-fft.cpp`

`testvect`位于函数`tfhe_bootstrap_woKS_FFT`中，我们需要看看这个函数的调用逻辑，同时注意每个函数的输入输出并梳理出从`encode -> encryption -> decryption -> decode`的整个流程。

```cpp
/**
 * result = LWE(mu) iff phase(x)>0, LWE(-mu) iff phase(x)<0
 * @param result The resulting LweSample
 * @param bk The bootstrapping + keyswitch key
 * @param mu The output message (if phase(x)>0) // zpf: 什么意思？首先mu是明文还是密文？
 * @param x The input sample
 */
EXPORT void tfhe_bootstrap_woKS_FFT(LweSample *result,
                                    const LweBootstrappingKeyFFT *bk,
                                    Torus32 mu,
                                    const LweSample *x) {
```

`tfhe_bootstrap_woKS_FFT` <- `tfhe_bootstrap_FFT`

除了以上关系，同时还有：

```shell
src/test/fakes/lwe-bootstrapping-fft.h:203:    static inline void tfhe_bootstrap_woKS_FFT(LweSample *result, const LweBootstrappingKeyFFT *bkFFT, Torus32 mu, const LweSample *x) {\
src/test/bootstrapping_test_fft.cpp:300:            tfhe_bootstrap_woKS_FFT(result, bkFFT, TEST_MU, insample);
src/include/tfhe.h:44:EXPORT void tfhe_bootstrap_woKS_FFT(LweSample* result, const LweBootstrappingKeyFFT* bk, Torus32 mu, const LweSample* x);
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:169:EXPORT void tfhe_bootstrap_woKS_FFT(LweSample *result,
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:220:    tfhe_bootstrap_woKS_FFT(u, bk, mu, x);
src/libtfhe/boot-gates.cpp:347:    tfhe_bootstrap_woKS_FFT(u1, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:355:    tfhe_bootstrap_woKS_FFT(u2, bk->bkFFT, MU, temp_result);
```

我们也需要去这些地方看看，但是现在先关注 `tfhe_bootstrap_woKS_FFT` <- `tfhe_bootstrap_FFT`

#### `tfhe_bootstrap_FFT`

```cpp
/**
 * result = LWE(mu) iff phase(x)>0, LWE(-mu) iff phase(x)<0
 * @param result The resulting LweSample
 * @param bk The bootstrapping + keyswitch key
 * @param mu The output message (if phase(x)>0)
 * @param x The input sample
 */
EXPORT void tfhe_bootstrap_FFT(LweSample *result,
                               const LweBootstrappingKeyFFT *bk,
                               Torus32 mu,
                               const LweSample *x) {
```

- `Torus32 mu` 输入的mu是明文还是密文？

调用`tfhe_bootstrap_FFT`的地方有：

```shell
src/test/fakes/lwe-bootstrapping-fft.h:229:    static inline void tfhe_bootstrap_FFT(LweSample *result, const LweBootstrappingKeyFFT *bkFFT, Torus32 mu, const LweSample *x) {\
src/test/bootstrapping_test_fft.cpp:386:            tfhe_bootstrap_FFT(result, bkFFT, TEST_MU, insample);
src/test/test-bootstrapping-fft.cpp:68:        tfhe_bootstrap_FFT(test_out + i, keyset->cloud.bkFFT, mu_boot, test_in + i);
src/include/tgsw_functions.h:87:EXPORT void tfhe_bootstrap_FFT(LweSample *result, const LweBootstrappingKeyFFT *bk, Torus32 mu, const LweSample *x);
src/include/tfhe.h:45:EXPORT void tfhe_bootstrap_FFT(LweSample* result, const LweBootstrappingKeyFFT* bk, Torus32 mu, const LweSample* x);
src/libtfhe/lwe-bootstrapping-functions-fft.cpp:213:EXPORT void tfhe_bootstrap_FFT(LweSample *result,
src/libtfhe/boot-gates.cpp:49:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:75:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:101:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:127:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:153:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:212:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:238:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:264:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:290:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
src/libtfhe/boot-gates.cpp:316:    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);
```

我们想看整个调用流程是什么样的，所以我们需要去看这些地方的调用逻辑(前面没有看其它地方如何调用`tfhe_bootstrap_woKS_FFT`也是基于以上原因)。
这里我们暂时略过与测试相关的地方，因为测试中看不到整个从`encode -> encryption -> decryption -> decode`的流程。

##### `src/libtfhe/boot-gates.cpp`

```cpp
/*
 * Homomorphic bootstrapped NAND gate
 * Takes in input 2 LWE samples (with message space [-1/8,1/8], noise<1/16)
 * Outputs a LWE bootstrapped sample (with message space [-1/8,1/8], noise<1/16)
*/
EXPORT void
bootsNAND(LweSample *result, const LweSample *ca, const LweSample *cb, const TFheGateBootstrappingCloudKeySet *bk) {
    static const Torus32 MU = modSwitchToTorus32(1, 8);
    const LweParams *in_out_params = bk->params->in_out_params;

    LweSample *temp_result = new_LweSample(in_out_params);

    //compute: (0,1/8) - ca - cb
    static const Torus32 NandConst = modSwitchToTorus32(1, 8);
    lweNoiselessTrivial(temp_result, NandConst, in_out_params);
    lweSubTo(temp_result, ca, in_out_params);
    lweSubTo(temp_result, cb, in_out_params);

    //if the phase is positive, the result is 1/8
    //if the phase is positive, else the result is -1/8
    tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);

    delete_LweSample(temp_result);
}
```

- 理解该代码中MU的作用是什么

从以上代码我们可以看到所有系数全部填`MU`的合理性，因为`NAND`的值只有两个，要么是\(\frac{1}{8}\)要么是\(-\frac{1}{8}\)。所以`test polynomial`的生成方式中将系数全部设置为`message`是合理的。除了`boot-gates.cpp`之外没有其它地方会使用到。

### `src/libtfhe/lwe-bootstrapping-functions.cpp`

相关代码中使用了`testvect`，但是`tfhe_bootstrap`并未在实际代码中使用到，所以可以忽略。

## 总结

> `test polynomial`的生成方式并不是按照论文中的test polynomial，而是直接将message作为测试多项式的系数。

这种方式在cpp源码中是可以使用的，因为主要是为搭建逻辑门服务的。即如果我们要通过逻辑门方式实现`TFHE`，那么按照源码实现是没有问题的。

## 遗留

正常生成`test polynomial`的方式应该是怎样的？