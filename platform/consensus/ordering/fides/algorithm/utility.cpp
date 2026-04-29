#include <cstring>
#include <cstdint>
#include <vector>
#include <random>    // std::mt19937_64

static constexpr uint64_t P = 0xFFFFFFFFFFFFFFC5ULL;  // 模 P，保证 P>n

// 快速幂取模
static uint64_t modpow(uint64_t a, uint64_t e) {
    uint64_t r = 1;
    while (e) {
        if (e & 1) r = (__uint128_t)r * a % P;
        a = (__uint128_t)a * a % P;
        e >>= 1;
    }
    return r;
}

// 计算第 i 个 Lagrange 基多项式在 0 点的值
static uint64_t lagrange_coeff(int i, int t, const std::vector<uint64_t>& xs) {
    __uint128_t num = 1, den = 1;
    uint64_t xi = xs[i];
    for (int j = 0; j < t; ++j) {
        if (j == i) continue;
        uint64_t xj = xs[j];
        num = num * (P - xj) % P;                // (0 - xj)
        den = den * ((xi + P - xj) % P) % P;     // (xi - xj)
    }
    return (uint64_t)( num * modpow((uint64_t)den, P-2) % P );
}

// 根据 secret s 拆分成 n 份 t-of-n 分享
static std::vector<uint64_t> shamir_share(uint64_t s, int n, int t) {
    std::vector<uint64_t> coeff(t);
    coeff[0] = s;
    std::mt19937_64 eng(s);
    for (int i = 1; i < t; ++i)
        coeff[i] = eng() % P;
    std::vector<uint64_t> shares(n);
    for (int i = 0; i < n; ++i) {
        __uint128_t xp = 1, y = 0;
        uint64_t x = i + 1;
        for (int j = 0; j < t; ++j) {
            y  = (y + xp * coeff[j]) % P;
            xp = xp * x % P;
        }
        shares[i] = (uint64_t)y;
    }
    return shares;
}

// 用 t 份分享和 Lagrange 插值恢复 secret
static uint64_t shamir_recover(const std::vector<uint64_t>& xs,
                               const std::vector<uint64_t>& ys, int t) {
    __uint128_t s = 0;
    for (int i = 0; i < t; ++i) {
        uint64_t L = lagrange_coeff(i, t, xs);
        s = (s + (__uint128_t)ys[i] * L) % P;
    }
    return (uint64_t)s;
}

uint32_t SimpleShamirCoin(int r, int n, int t) {
    uint64_t secret = static_cast<uint64_t>(r);
    auto shares = shamir_share(secret, n, t);

    std::vector<uint64_t> xs(t), ys(t);
    for (int i = 0; i < t; ++i) {
        xs[i] = i + 1;
        ys[i] = shares[i];
    }
    uint64_t rec = shamir_recover(xs, ys, t);
    return static_cast<uint32_t>(rec % n);
}


