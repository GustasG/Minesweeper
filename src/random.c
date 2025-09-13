#include "pch.h"

#include "random.h"

uint32_t
xorshift32(_In_ struct xorshift32_state* state)
{
    uint32_t x = state->a;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return state->a = x;
}
