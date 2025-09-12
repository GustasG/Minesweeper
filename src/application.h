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
    HBITMAP right;
    HBITMAP top;
    HBITMAP topLeft;
    HBITMAP topRight;
} BorderResources;

typedef struct
{
    Minefield minefield;
    CellResources cellResources;
    FaceResources faceResources;
    BorderResources borderResources;
    uint32_t hoverCellX;
    uint32_t hoverCellY;
    bool isLeftMouseDown;
} Application;

_Ret_maybenull_ Application* CreateApplication(_In_ HINSTANCE hInstance);

void DestroyApplication(_In_opt_ Application* app);
