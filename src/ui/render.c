#include "pch.h"

#include "ui/render.h"
#include "ui/window.h"

static void
BlitBitmapScaled(
    _In_ HDC hdcDest,
    _In_ HDC hdcSrc,
    _In_opt_ HBITMAP bitmap,
    _In_ int xDest,
    _In_ int yDest,
    _In_ int wDest,
    _In_ int hDest)
{
    if (bitmap == NULL || wDest <= 0 || hDest <= 0)
        return;

    BITMAP bm;
    GetObjectW(bitmap, sizeof(BITMAP), &bm);

    HBITMAP old = (HBITMAP)SelectObject(hdcSrc, bitmap);
    StretchBlt(hdcDest, xDest, yDest, wDest, hDest, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject(hdcSrc, old);
}

static _Ret_maybenull_ HBITMAP
GetCellBackground(_In_ const Application* app, _In_ const Cell* cell, _In_ uint32_t x, _In_ uint32_t y)
{
    switch (cell->state)
    {
        case CELL_HIDDEN:
        {
            switch (app->minefield.state)
            {
                case GAME_PLAYING:
                {
                    if (app->isLeftMouseDown && x == app->hoverCellX && y == app->hoverCellY)
                        return app->cellResources.down;

                    break;
                }
                case GAME_WON:
                    break;
                case GAME_LOST:
                {
                    if (cell->hasMine)
                    {
                        if (x != app->minefield.blastX || y != app->minefield.blastY)
                            return app->cellResources.mine;
                    }

                    break;
                }
            }

            return app->cellResources.up;
        }
        case CELL_REVEALED:
        {
            if (cell->hasMine)
            {
                if (app->minefield.state == GAME_LOST && x == app->minefield.blastX && y == app->minefield.blastY)
                    return app->cellResources.blast;

                return app->cellResources.mine;
            }

            return app->cellResources.down;
        }
        case CELL_FLAGGED:
        {
            if (app->minefield.state == GAME_LOST && !cell->hasMine)
                return app->cellResources.falseMine;

            return app->cellResources.flag;
        }
    }

    return NULL;
}

static _Ret_maybenull_ HBITMAP
GetCellNumber(_In_ const Application* app, _In_ const Cell* cell)
{
    if (cell->state == CELL_REVEALED && !cell->hasMine && cell->neighborMines > 0 &&
        cell->neighborMines <= ARRAYSIZE(app->cellResources.cells))
    {
        return app->cellResources.cells[cell->neighborMines - 1];
    }

    return NULL;
}

static void
RenderSingleCell(
    _In_ const Application* app,
    _In_ HDC hdc,
    _In_ HDC hdcMem,
    _In_ const RECT* cellRect,
    _In_ uint32_t x,
    _In_ uint32_t y)
{
    const Cell* cell = GetCell(&app->minefield, x, y);

    if (cell == NULL)
        return;

    HBITMAP backgroundBitmap = GetCellBackground(app, cell, x, y);
    HBITMAP numberBitmap = GetCellNumber(app, cell);
    int cellWidth = cellRect->right - cellRect->left;
    int cellHeight = cellRect->bottom - cellRect->top;

    BlitBitmapScaled(hdc, hdcMem, backgroundBitmap, cellRect->left, cellRect->top, cellWidth, cellHeight);
    BlitBitmapScaled(hdc, hdcMem, numberBitmap, cellRect->left, cellRect->top, cellWidth, cellHeight);
}

static void
RenderBorders(_In_ const Application* app, _In_ HWND hWnd, _In_ HDC hdc, _In_ HDC hdcMem)
{
    UINT dpi = GetDpiForWindow(hWnd);

    int cellSize = MulDiv(CELL_SIZE, dpi, 96);
    int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

    int boardWidth = (int)(app->minefield.width * cellSize);
    int boardHeight = (int)(app->minefield.height * cellSize);
    int rightEdge = borderWidth + boardWidth;
    int bottomEdge = borderHeight + boardHeight;

    BlitBitmapScaled(hdc, hdcMem, app->borderResources.topLeft, 0, 0, borderWidth, borderHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.topRight, rightEdge, 0, borderWidth, borderHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.bottomLeft, 0, bottomEdge, borderWidth, borderHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.bottomRight, rightEdge, bottomEdge, borderWidth, borderHeight);

    BlitBitmapScaled(hdc, hdcMem, app->borderResources.top, borderWidth, 0, boardWidth, borderHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.bottom, borderWidth, bottomEdge, boardWidth, borderHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.left, 0, borderHeight, borderWidth, boardHeight);
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.right, rightEdge, borderHeight, borderWidth, boardHeight);
}

static void
RenderCells(_In_ const Application* app, _In_ HWND hWnd, _In_ HDC hdc, _In_ HDC hdcMem)
{
    UINT dpi = GetDpiForWindow(hWnd);

    int cellSize = MulDiv(CELL_SIZE, dpi, 96);
    int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

    for (uint32_t y = 0; y < app->minefield.height; y++)
    {
        for (uint32_t x = 0; x < app->minefield.width; x++)
        {
            RECT cellRect = {
                .left = (LONG)(borderWidth + (int)(x * cellSize)),
                .top = (LONG)(borderHeight + (int)(y * cellSize)),
                .right = (LONG)(borderWidth + (int)((x + 1) * cellSize)),
                .bottom = (LONG)(borderHeight + (int)((y + 1) * cellSize)),
            };

            RenderSingleCell(app, hdc, hdcMem, &cellRect, x, y);
        }
    }
}

void
RenderGameWindow(_In_ const Application* app, _In_ HWND hWnd, _In_ HDC hdc)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    int oldMode = SetStretchBltMode(hdc, HALFTONE);

    RenderBorders(app, hWnd, hdc, hdcMem);
    RenderCells(app, hWnd, hdc, hdcMem);

    SetStretchBltMode(hdc, oldMode);
    DeleteDC(hdcMem);
}
