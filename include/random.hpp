#pragma once

static uint g_seed = 324223426;

inline uint rand_xorshift(uint seed)
{
    // Xorshift algorithm from George Marsaglia's paper
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
}

inline float randf()
{
    g_seed = rand_xorshift(g_seed);
    uint seed = g_seed;
    // Faster on cuda probably
    //return seed * 2.3283064365387e-10f;

    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    seed &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    seed |= ieeeOne;                          // Add fractional part to 1.0

    float  f = reinterpret_cast<float&>(seed);       // Range [1:2]
    return f - 1.0f;                        // Range [0:1]
}

