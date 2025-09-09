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
    uint32_t cellSize;
    uint32_t boardWidth;
    uint32_t boardHeight;
} RenderInfo;

typedef struct
{
    Minefield minefield;
    CellResources cellResources;
    FaceResources faceResources;
    RenderInfo renderInfo;
} Application;

_Ret_maybenull_ Application* CreateApplication(_In_ HINSTANCE hInstance);

void DestroyApplication(_In_opt_ Application* app);
