# Blind Rotation

In this section, we will explain how to perform blind rotation using TFHE.

## Introduction

我们在给出定义之后，会给出例子，来说明如何使用TFHE进行盲转。

$$\begin{aligned}
u &\in \mathbb{T}_p \\
\mathbf{s} &= (s_1,\cdots,s_n) \in \mathbb{B}^n \\
(a_1,\cdots,a_n) &\stackrel{\$}{\leftarrow}\mathbb{T}_q^{n} \\
\mathbf{c} &\leftarrow TLWE_s(u) = (a_1,\cdots,a_n, b) \in \mathbb{T}_q^{n+1} \\
u^* &= u+e \\
b &= \sum_{i=1}^n a_i\cdot s_i+u^* \\
\end{aligned}$$

以上包含了TLWE所需要的参数以及计算过程。为了能够更直观的理解，我们给出一个具体的例子。
### Example

假设我们要将一个`2-bit`精确度的Torus元素$u$转换为`5-bit`精确度的Torus元素$u^*$。

即$p=4,q=32$,不失一般性，我们选择$n=2$。

我们有如下例子：

$$\begin{aligned}
u &=\frac{3}{4}\in \mathbb{T}_4 \\
\mathbf{s} &= (s_1,s_2)=(1,1) \in \mathbb{B}^n \\
(a_1,a_2) &=(\frac{5}{32},\frac{31}{32})\stackrel{\$}{\leftarrow}\mathbb{T}_{32}^{2} \\
\mathbf{c} &\leftarrow TLWE_s(u) = (a_1,a_2, b) \in \mathbb{T}_{32}^{3} \\
u^* &= u+e = \frac{3}{4}+e \\
b &= \sum_{i=1}^n a_i\cdot s_i+u^* =\frac{28}{32}+e\\
\end{aligned}$$

#### noise error $e$

上述例子中，我们没有给出$e$的具体值，我们希望看到`BlindRotate`过程中$e$是如何变化的。这里我们简单讨论一下$e$的生成方法和取值范围。

$$\begin{aligned}
distribution\ X &\in R\\
e_0 &\leftarrow X \\
\bar{e}&=round(qe_0) \in \mathbb{Z} \\
e &= \frac{\bar{e}}{q} \in \frac{1}{q}\mathbb{Z} \\
\end{aligned}$$

将$q=32$代入上式，我们有：

$$\begin{aligned}
e\in \frac{1}{32}\mathbb{Z} \\
\end{aligned}$$

为了可以正确的`Decryption`，我们需要保证$e$的取值范围在`Decrypt`时不会溢出。即：
$$\begin{aligned}
\lvert e\rvert < \frac{1}{2p}=\frac{1}{8} \\
\end{aligned}$$

## Rounding with polynomial

根据多项式环的外积性质可以将$u^*$转换为$u$（不是$u+e$）。

理论上，我们假设$N \geq q$, 这样$\mathbb{T}_q$中的所有元素都可以被表示为$N$次多项式的系数。

$$\begin{aligned}
u^* &\in \mathbb{T}_q \\
\bar{u}^* &\coloneqq \lfloor qu^*\rceil \quad mod \quad q \\
v_j &\coloneqq \frac{\lfloor (p\bar{u}^*)/q\rceil \quad mod \quad p}{p}\quad ,\quad 0\leq j< q \\
\end{aligned}$$

其中$v_j$表示多项式`test polynomial`$v(X)=v_0+v_1X+v_2X^2+\cdots+v_{N-1}X^{N-1} \in \mathbb{T}_{N,p}[X]=\mathbb{T}_p[X]/(X^N+1)$的第$j$个系数。

根据多项式环的外积性质$X^{-j}\cdot v(X)=v_j+\cdots,0\leq j<N$，我们有：

$$\begin{aligned}
X^{-\bar{u}^*}\cdot v(X) &= \frac{\lfloor (p\bar{u}^*)/q\rceil \quad mod \quad p}{p} + \cdots \\
&= \frac{\lfloor p{u}^*\rceil \quad mod \quad p}{p} + \cdots \\
&=u + \cdots \\
\end{aligned}$$

### Rescaling

以上假设$N \geq q$在实际应用中并不满足。在实际中，$N$可能远远小于$q$。

考虑到$X^{2N}=1$, 我们有$X^{-\bar{u}^*}\cdot v(X)$中的$-\bar{u}^*$是定义在模`2N`中的元素。

因此，我们需要对$\bar{u}^*$进行`Rescaling`操作。

记$\bar{u}^*=\lfloor qu^*\rceil \quad mod \quad q$，$\bar{a}_j=\lfloor qa_j\rceil \quad mod \quad q$，$\bar{b}=\lfloor qb \rceil \quad mod \quad q$。则有：

$$\begin{aligned}
\bar{u}^* &= -\bar{b} + \sum_{j=1}^n \bar{a}_j\cdot s_j \quad (mod \quad q) \\
\end{aligned}$$

以上公式在不引入新的error的情况下，将$u^*$转换为$u$。

但是实际应用中无法满足$N \geq q$的假设。我们只能使用会引入一定误差的近似公式：

$$\begin{aligned}
-\tilde{u}^* = -\tilde{b} + \sum_{j=1}^n \tilde{a}_j\cdot s_j \quad (mod \quad 2N) \\
\end{aligned}$$

其中$\tilde{a}_j=\lfloor 2N{a}_j\rceil \ mod \ 2N$，$\tilde{b}=\lfloor 2N{b}\rceil \ mod \ 2N$。

`test polynomial`$v$的系数$\{v_j\}$可以由`Rescaling`公式计算得到:

$$\begin{aligned}
v \coloneqq v(X) = \sum_{j=0}^{N-1} v_j X^j \quad with \ v_j = \frac{\lfloor \frac{pj}{2N}\rceil \ mod \ p}{p} \in \mathbb{T}_p\\
\end{aligned}$$

> 为了控制误差，我们必须小心的选择N，使得误差仍然在可接受的范围内。

#### Example

仍然使用之前的例子，假设$p=4,q=32,n=2$。计算新的`test polynomial v`的系数。

$$\begin{aligned}
v_j &= \frac{\lfloor \frac{pj}{2N}\rceil \quad mod \ p}{p} \quad with \ j=0,1,\cdots ,N-1 \\
&= \frac{\lfloor \frac{4\cdot j}{2\cdot N}\rceil \ mod \ 4}{4} \\
&= \frac{\lfloor \frac{2j}{N}\rceil \ mod \ 4}{4} \\
\end{aligned}$$

由以上计算我们发现我们永远无法得到$u$的值，无论$N$取何值。