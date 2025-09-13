#pragma once

#include <Windows.h>

#include "game.h"

typedef struct
{
    HBITMAP cells[8];
    HBITMAP blast;
    HBITMAP down;
    HBITMAP flag;
    HBITMAP mine;
    HBITMAP up;
    HBITMAP falseMine;
} CellResources;

typedef struct
{
    HBITMAP click;
    HBITMAP lost;
    HBITMAP smile;
    HBITMAP smileFaceDown;
    HBITMAP win;
} FaceResources;

typedef struct
{
    HBITMAP bottom;
    HBITMAP bottomLeft;
    HBITMAP bottomRight;
    HBITMAP left;
    HBITMAP middleLeft;
    HBITMAP middleRight;
    HBITMAP counterLeft;
    HBITMAP counterMiddle;
    HBITMAP counterRight;
    HBITMAP right;
    HBITMAP top;
    HBITMAP topLeft;
    HBITMAP topRight;
} BorderResources;

typedef struct
{
    uint32_t cellSize;
    uint32_t borderWidth;
    uint32_t borderHeight;
    uint32_t counterBorderWidth;
    uint32_t counterAreaHeight;
    uint32_t counterMargin;
    uint32_t counterHeight;
    uint32_t counterDigitWidth;
    uint32_t faceSize;
} LayoutMetrics;

typedef struct
{
    Minefield minefield;
    CellResources cellResources;
    FaceResources faceResources;
    BorderResources borderResources;
    LayoutMetrics metrics;
    uint32_t hoverCellX;
    uint32_t hoverCellY;
    bool isLeftMouseDown;
    bool isFaceHot;
} Application;

_Ret_maybenull_ Application* CreateApplication(_In_ HINSTANCE hInstance);

void DestroyApplication(_In_opt_ Application* app);
