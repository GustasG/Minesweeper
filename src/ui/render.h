#pragma once

#include "application.h"

void CalculateRenderInfo(
    _In_ const RECT* clientRect,
    _In_ uint32_t minefieldWidth,
    _In_ uint32_t minefieldHeight,
    _Out_ RenderInfo* renderInfo);

void RenderMinefield(_In_ const Application* app, _In_ HDC hdc);
