# Blind Rotation

In this section, we will explain how to perform blind rotation using TFHE.

## Introduction

我们在给出定义之后，会给出例子，来说明如何使用TFHE进行盲转。

$$\begin{aligned}
\mathbf{s} &= (s_1,\cdots,s_n) \in \mathbb{B}^n \\
(a_1,\cdots,a_n) &\stackrel{\$}{\leftarrow}\mathbb{T}_q^{n} \\
\mathbf{c} &\leftarrow TLWE_s(u) = (a_1,\cdots,a_n, b) \in \mathbb{T}_q^{n+1} \\
\end{aligned}$$
