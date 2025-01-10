# Blind Rotation

In this section, we will explain how to perform blind rotation using TFHE.

## notation

$nearest(x) \coloneqq \lfloor x\rceil$
根据代码，有$nearest(0.5)=0$。

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

由以上计算我们发现$v_j = \{0, \frac{1}{4}, \frac{2}{4}\}.$ 不包含$\frac{3}{4}$。但是考虑到$-\frac{1}{4}=\frac{3}{4}$，所以我们有：
$$\begin{aligned}
\{\pm v_0, \cdots, \pm v_{N-1}\} &= \mathbb{T}_4 \\
\end{aligned}$$

不失一般性，我们选择$N=8$。于是有：
$$\begin{aligned}
(\tilde{a}_1,\tilde{a}_2) &= (2, 15) \\
\tilde{b} &= \lfloor 2Nb \rceil \ mod \ 2N = 14 + \lfloor 16e \rceil \ mod \ 2N \\
-\tilde{u}^* &= -\tilde{b} + \sum_{j=1}^n \tilde{a}_j\cdot s_j = 3 - \lfloor 16e \rceil \ mod \ 2N \\
\end{aligned}$$

已知$e \in \frac{1}{32}\mathbb{Z}, \lvert e\rvert < \frac{1}{2p}=\frac{1}{8}$，我们有：
$$\begin{aligned}
e &= \{0, \frac{\pm 1}{32}, \frac{\pm 2}{32}, \frac{\pm 3}{32}\} \\
-\tilde{u}^* &= 3 - \lfloor 16e \rceil \ mod \ 2N = \{5,4,3,2\} \\
\end{aligned}$$

`test polynomial` $v$：
$$\begin{aligned}
v(X) &=  \frac{1}{4}X^3 + \frac{1}{4}X^4 + \frac{1}{4}X^5 + \frac{1}{4}X^6 + \frac{2}{4}X^7 \\
\end{aligned}$$

可以验证，对于所有$-\tilde{u}^* =3 - \lfloor 16e \rceil \ mod \ 2N = \{5,4,3,2\}$，都可以正确的计算出$u$。

我们可以看到，`Rescaling`操作是如何引入新的误差的。

#### Example2

当\(u = 0\)时，我们有：
\[\begin{aligned}
-\tilde{u}^* &= 15 - \lfloor 16e \rceil \ mod \ 2N  \\
&=\{0, 1, 14, 15\}
\end{aligned}\]
我们发现当\(-\tilde{u}^* = 1\)时，常数项系数为：
\[\begin{aligned}
X^1 \times v(x) &= -\frac{2}{4} + \cdots \neq  0 + \cdots
\end{aligned}\]

从这里我们可以看出，当$u=0$时，即使误差在可接受的范围值内，也不能保证被正确的计算出。

这主要是因为`test polynomial`只能表示\(\mathbb{T}_p[X]\)中一半的元素。
那么0以及\(\frac{2}{4}\)都存在半个误差区间无法被正确计算的情况。

这也是为什么我们做bootstrap的时候0偶尔会出错的原因。

## BlindRotate

我们有如下定义：

$$\begin{aligned}
TLWE \ ciphertext \ \mathbf{c} &\leftarrow {TLWE}_{\mathbf{s}}(u) \in \mathbb{T}_q^{n+1} \\
key \ \mathbf{s} &= (s_1,\cdots,s_n) \in \mathbb{B}^n \\
bootstrapping-key \ vector \ bsk &= (bsk[1], \cdots, bsk[n]) \\
bsk[j] &\leftarrow TGGSW_{\mathbf{{\zeta^{\prime}}}}(s_j) \in \mathbb{T}_{N,q}[X]^{(k+1)l\times (k+1)} \ with \ B^l = p\\
\mathbf{{\zeta^{\prime}}} &= (\zeta^{\prime}_1,\cdots,\zeta^{\prime}_k) \in \mathbb{B}_N[X]^k \\
v \coloneqq v(X) &= \sum_{j=0}^{N-1} v_j X^j \quad with \ v_j = \frac{\lfloor \frac{pj}{2N}\rceil \ mod \ p}{p} \in \mathbb{T}_p\\
\end{aligned}$$

我们通过`BlindRotate`得到$TGLWE \ ciphertext \ \mathbf{c'} \leftarrow TGLWE_{\mathbf{\zeta^{\prime}}}(X^{-\bar{u}^*}\cdot v(X)) \in \mathbb{T}_{N,q}[X]^{k+1}$：

1. define $\vec{\mathbf{c}}(X) \coloneqq (0, \cdots, 0, v)$ and $\tilde{\mathbf{c}} \coloneqq (\tilde{a}_1, \cdots, \tilde{a}_n, \tilde{b}) \leftarrow \lfloor \mathbf{c}2N \rceil \ mod \ 2N$;
2. do $\left\{ \begin{aligned}
    \mathbf{c}_0^{\prime} & \leftarrow X^{-\tilde{b}} \cdot \vec{\mathbf{c}}(X) \\
    \mathbf{c}_j^{\prime} & \leftarrow CMux(bsk[j], \mathbf{c}_{j-1}^{\prime}, X^{\tilde{a}_j}\cdot \mathbf{c}_{j-1}^{\prime}) \quad for \ j=1,\cdots,n \\
    \end{aligned} \right.$
    and  set $\mathbf{c^{\prime}} \coloneqq \mathbf{c}_n^{\prime}$;

我们将`CMux(.)`具体展开，同时注意所需要的乘法运算次数：

$$\begin{aligned}
CMux(bsk[j],  \mathbf{c}_{j-1}^{\prime}, X^{\tilde{a}_j}\cdot \mathbf{c}_{j-1}^{\prime}) &\leftarrow bsk[j] \boxdot (X^{\tilde{a}_j}\cdot \mathbf{c}_{j-1}^{\prime} - \mathbf{c}_{j-1}^{\prime}) + \mathbf{c}_{j-1}^{\prime} \\
&\leftarrow bsk[j] \boxdot (X^{\tilde{a}_j} - 1)\mathbf{c}_{j-1}^{\prime} + \mathbf{c}_{j-1}^{\prime} \\
&=G^{-1}((X^{\tilde{a}_j} - 1)\mathbf{c}_{j-1}^{\prime}) \cdot  bsk[j] +  \mathbf{c}_{j-1}^{\prime} \\
\end{aligned}$$

我们假设$G^{-1}((X^{\tilde{a}_j} - 1)\mathbf{c}_{j-1}^{\prime})$需要的乘法运算次数为$o(G)$。

$$\begin{aligned}
G^{-1}((X^{\tilde{a}_j} - 1)\mathbf{c}_{j-1}^{\prime}) \cdot  bsk[j] \leftarrow \mathbb{Z}_{N}[X]^{(k+1)l} \times \mathbb{T}_{N,q}[X]^{(k+1)l\times (k+1)}
\end{aligned}$$

### Example

仍然使用之前的例子，假设 $p=4,q=32,n=2,N=8,B=2,l=2,k=1$。

$$\begin{aligned}
\vec{\mathbf{c}}(X) &\coloneqq (0, v) \\
&\quad v \coloneqq v(X) =  \frac{1}{4}X^3 + \frac{1}{4}X^4 + \frac{1}{4}X^5 + \frac{1}{4}X^6 + \frac{2}{4}X^7 \\
\tilde{\mathbf{c}} &\coloneqq (\tilde{a}_1, \cdots, \tilde{a}_n, \tilde{b}) \leftarrow \lfloor \mathbf{c}2N \rceil \ mod \ 2N \\
&= (2, 15, 14 + \lfloor 16e \rceil ) \ mod \ 2N\\
\end{aligned}$$

计算$bootstrapping-key \ vector \ bsk$：

$$\begin{aligned}
\mathbf{g}^\top &= (1/2, 1/4)^\top\\
\mathbf{G}^{\top} &= \mathbf{I}_{\mathbf{k+1}} \otimes \mathbf{g}^\top \\
\mathbf{{\zeta^{\prime}}} &= (\zeta^{\prime}_1,\cdots,\zeta^{\prime}_k) \in \mathbb{B}_N[X]^k =(\zeta^{\prime}_1) \\
&=(1+X+X^2+X^3+X^4+X^5+X^6+X^7) \\
TGLWE_{\mathbf{\zeta^{\prime}}}(0) &=(\vec{a}_1(X), \vec{l}(X)) \ with \ \vec{a}_1(X) = \frac{3}{4}X^5 \\
&= (\frac{3}{4}X^5, \vec{l}(X)) \\
\vec{l}_i(X) &= \frac{-3}{4}+\frac{-3}{4}X+\frac{-3}{4}X^2+\frac{-3}{4}X^3+\frac{-3}{4}X^4 \\
&\qquad +\frac{3}{4}X^5+\frac{3}{4}X^6+\frac{3}{4}X^7\\
&\qquad + \mathbf{e_i}(X) \\
bsk[j] &\leftarrow TGGSW_{\mathbf{{\zeta^{\prime}}}}(s_j) \in \mathbb{T}_{N,q}[X]^{(k+1)l\times (k+1)} \ with \ B^l = p\\
&bsk[i] = \begin{pmatrix}
\frac{1}{2} + \frac{3}{4}X^5 & \vec{l}_i(X)  \\
\frac{1}{4} + \frac{3}{4}X^5 & \vec{l}_i(X)  \\
\frac{3}{4}X^5 & \frac{1}{2} +\vec{l}_i(X)  \\
\frac{3}{4}X^5 & \frac{1}{4} +\vec{l}_i(X)  
\end{pmatrix}_{4\times 2}
 \\
bsk &= (bsk[1], bsk[2]) \in \mathbb{T}_{N=8,q=32}[X]^{4\times 2 \times 2} \\
\end{aligned}$$

详细展开step2：

$$\begin{aligned}
\mathbf{c}_0^{\prime} & \leftarrow X^{-\tilde{b}} \cdot \vec{\mathbf{c}}(X) \\
&=(0, \frac{1}{4}X^{5-\lfloor 16e \rceil}(1+X+X^2+X^3+2X^4)) \\
&=(0, v_0(X)) \\
&\qquad v_0(X) = \frac{1}{4}X^{5-\lfloor 16e \rceil}(1+X+X^2+X^3+2X^4) \\
\mathbf{c}_1^{\prime} & \leftarrow CMux(bsk[1], \mathbf{c}_{1-1}^{\prime}, X^{\tilde{a}_1}\cdot \mathbf{c}_{1-1}^{\prime}) \\
&=G^{-1}((X^{\tilde{a}_1} - 1)\mathbf{c}_{0}^{\prime}) \cdot  bsk[1] +  \mathbf{c}_{0}^{\prime} \\
&=G^{-1}((X^{2} - 1)\mathbf{c}_{0}^{\prime}) \cdot  bsk[1] +  \mathbf{c}_{0}^{\prime} \\
&=(0,0,0,v_1(X)) \cdot  bsk[1] +  \mathbf{c}_{0}^{\prime} \\
&\qquad (v_1(X) = X^{5-\lfloor 16e \rceil}(X^2-1)(1+X+X^2+X^3+2X^4))\\
&\qquad (v_1(X) = 4(X^2-1)v_0(X)) \\
&= (\frac{3}{4}X^5\cdot v_1(X),(\frac{1}{4} +\vec{l}_1(X))\cdot v_1(X))+  \mathbf{c}_{0}^{\prime} \\
&= (3X^5(X^2-1)\cdot v_0(X),(1 +4\vec{l}_1(X))(X^2-1)\cdot v_0(X))+  \mathbf{c}_{0}^{\prime} \\
&= (3X^5(X^2-1)\cdot v_0(X),((1 +4\vec{l}_1)(X^2-1)+1)\cdot v_0(X)) \\
&= (p_1(X)\cdot v_0(X),q_1(X)\cdot v_0(X)) \\
&= (p_1\cdot v_0,q_1\cdot v_0) \\
\mathbf{c}_2^{\prime} & \leftarrow CMux(bsk[2], \mathbf{c}_{2-1}^{\prime}, X^{\tilde{a}_2}\cdot \mathbf{c}_{2-1}^{\prime}) \\
&=G^{-1}((X^{\tilde{a}_2} - 1)\mathbf{c}_{1}^{\prime}) \cdot  bsk[2] +  \mathbf{c}_{1}^{\prime} \\
&=G^{-1}((X^{15} - 1)\mathbf{c}_{1}^{\prime}) \cdot  bsk[2] +  \mathbf{c}_{1}^{\prime} \\
&=G^{-1}((-X^{7} - 1)\mathbf{c}_{1}^{\prime}) \cdot  bsk[2] +  \mathbf{c}_{1}^{\prime} \\
&=4(-X^{7} - 1) \cdot v_0 \cdot (0, p_1 , 0,q_1)\cdot  bsk[2] +  \mathbf{c}_{1}^{\prime} \\
&=4(-X^{7} - 1) \cdot (\frac{1}{4}(+3X^5)p_1 + \frac{3}{4}X^5q_1, \frac{1}{4}q_1 + p_1 \vec{l}_2 + q_1 \vec{l}_2) +  \mathbf{c}_{1}^{\prime} \\
\mathbf{c^{\prime}} &\coloneqq \mathbf{c}_2^{\prime}
\end{aligned}$$

已知$TGLWE \ ciphertext \ \mathbf{c'} \leftarrow TGLWE_{\mathbf{\zeta^{\prime}}}(X^{-\bar{u}^*}\cdot v(X)) = TGLWE_{\mathbf{\zeta^{\prime}}}(u + \cdots) \in \mathbb{T}_{N,q}[X]^{k+1}$. 这里如果我们进行`decrypt`操作，理论上是可以获得$u$的。

> 以上的计算量过大，而且也没能够体现出需要的乘法次数。
