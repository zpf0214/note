# latex

$$\begin{aligned}
TLWE\quad ciphertext\quad \mathscr{c} &\leftarrow TLWE_{\mathscr{s}}(u)\in \mathbb{T}_{q}^{n+1}\\
key\quad \mathscr{s} &=(s_1, s_2, \dots, s_{n}) \in \mathbb{B}^n\\
bootstrappingKey\quad vector\quad bsk&=(bsk[1], bsk[2], \dots, bsk[n])\\
bsk[j]&\leftarrow TGGSW_{\mathfrak{s}^\prime}(s_j) \\
\mathfrak{s}^\prime &= (\mathfrak{s}_1^\prime, \mathfrak{s}_2^\prime, \dots, \mathfrak{s}_{k}^\prime) \in \mathbb{B}[X]^n\\
\end{aligned}$$

其中将`TLWE/TGGSW/`展开

$$\begin{aligned}
TLWE_{\mathscr{s}}(u)&=(a_1, a_2, \dots, a_{n}, b)\in \mathbb{T}_{q}^{n+1}\\
b&=\sum_{j=1}^n a_j\cdot s_j + u^*,\quad(a_1, a_2, \dots, a_{n})\stackrel{\$}{\leftarrow}\mathbb{T}^n_q\\
u^*&=u+e,\quad e\leftarrow \hat{\mathcal{X}}\\
TGGSW_{\mathfrak{s}^\prime}(s_j)&=\mathfrak{L} + m\cdot G^T \in \mathbb{T}_{N,q}[X]^{(k+1)l\times (k+1)}\\
\mathfrak{L}&\leftarrow (TGLWE_{\mathfrak{s}^\prime}(0), TGLWE_{\mathfrak{s}^\prime}(0), \dots, TGLWE_{\mathfrak{s}^\prime}(0))^T\quad ((k+1)l\quad rows) \\
G^T &= I_{k+1}\otimes g^T,\quad g^T=(\frac{1}{B}, \frac{1}{B^2}, \cdots, \frac{1}{B^{j}})^T, \quad B^j=p\\
TGLWE_{\mathfrak{s}^\prime}(0)&=(\mathfrak{a}_1, \cdots, \mathfrak{a}_k, \mathfrak{b}) \in \mathbb{T}_{N,q}[{X}]^{k+1}\\
\mathfrak{b}&=\sum_{j=1}^k \mathfrak{s}^\prime_j\cdot \mathfrak{a}_j + \mathfrak{u}^*,\quad (\mathfrak{a}_1, \cdots, \mathfrak{a}_k) \stackrel{\$}{\leftarrow}\mathbb{T}_{N,q}[X]^k\\
\mathfrak{u}^* &= 0 + \mathfrak{e},\quad \mathfrak{e}\leftarrow \hat{\mathcal{X}}\in q^{-1}\mathbb{Z}_N[X]\\
\end{aligned}$$
