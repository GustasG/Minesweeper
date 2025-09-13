#pragma once

#include <sal.h>

#include <stdint.h>

struct xorshift32_state
{
    uint32_t a;
};

uint32_t xorshift32(_In_ struct xorshift32_state* state);
