#pragma once

#include <cstdint>

/** SimpleXorCoin: XOR 聚合 t 个 PRNG 输出，返回 [0,n) */
uint32_t SimpleXorCoin(int r, int n, int t);

/** SimpleShamirCoin: Shamir t-of-n 分享恢复后 mod n，返回 [0,n) */
uint32_t SimpleShamirCoin(int r, int n, int t);
