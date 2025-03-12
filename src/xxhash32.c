
#include "xxhash32.h"
#define PRIME32_1 2654435761U
#define PRIME32_2 2246822519U
#define PRIME32_3 3266489917U
#define PRIME32_4 668265263U
#define PRIME32_5 374761393U

static inline uint32_t XXH32_rotl(uint32_t x, int r) {
    return (x << r) | (x >> (32 - r));
}


static inline uint32_t XXH32_readLE32(const void* ptr) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(val));  // 避免未对齐内存访问
    return val;
}

// 内联函数：核心哈希轮函数
static inline uint32_t XXH32_round(uint32_t seed, uint32_t input) {
    seed += input * PRIME32_2;
    seed = XXH32_rotl(seed, 13);
    seed *= PRIME32_1;
    return seed;
}

// 对 buffer 进行哈希计算
uint32_t xxhash32(const void* input, size_t len, uint32_t seed) 
{
    const uint8_t* p = (const uint8_t*)input;
    const uint8_t* const bEnd = p + len;
    uint32_t h32;

    if (len >= 16) {
        const uint8_t* const limit = bEnd - 16;
        uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
        uint32_t v2 = seed + PRIME32_2;
        uint32_t v3 = seed + 0;
        uint32_t v4 = seed - PRIME32_1;

        // 主循环：每次处理 16 字节
        do {
            v1 = XXH32_round(v1, XXH32_readLE32(p)); p += 4;
            v2 = XXH32_round(v2, XXH32_readLE32(p)); p += 4;
            v3 = XXH32_round(v3, XXH32_readLE32(p)); p += 4;
            v4 = XXH32_round(v4, XXH32_readLE32(p)); p += 4;
        } while (p <= limit);

        h32 = XXH32_rotl(v1, 1) + 
              XXH32_rotl(v2, 7) + 
              XXH32_rotl(v3, 12) + 
              XXH32_rotl(v4, 18);
    } else {
        h32 = seed + PRIME32_5;
    }

    h32 += (uint32_t)len;

    // 处理剩余的字节
    while (p + 4 <= bEnd) {
        h32 += XXH32_readLE32(p) * PRIME32_3;
        h32 = XXH32_rotl(h32, 17) * PRIME32_4;
        p += 4;
    }

    while (p < bEnd) {
        h32 += (*p) * PRIME32_5;
        h32 = XXH32_rotl(h32, 11) * PRIME32_1;
        p++;
    }

    // 最终混合
    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}
