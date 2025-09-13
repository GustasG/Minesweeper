#include "pch.h"

#include "ui/render.h"

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
            switch (app->minefield.state)
            {
                case GAME_PLAYING:
                    break;
                case GAME_WON:
                    break;
                case GAME_LOST:
                {
                    if (cell->hasMine)
                    {
                        if (x == app->minefield.blastX && y == app->minefield.blastY)
                            return app->cellResources.blast;

                        return app->cellResources.mine;
                    }

                    break;
                }
            }

            return app->cellResources.down;
        }
        case CELL_FLAGGED:
        {
            switch (app->minefield.state)
            {
                case GAME_PLAYING:
                    break;
                case GAME_WON:
                    break;
                case GAME_LOST:
                {
                    if (cell->hasMine)
                        return app->cellResources.falseMine;

                    break;
                }
            }

            return app->cellResources.flag;
        }
    }

    return NULL;
}

static _Ret_notnull_ HBITMAP
GetFaceBitmap(_In_ const Application* app)
{
    switch (app->minefield.state)
    {
        case GAME_PLAYING:
        {
            if (app->isLeftMouseDown)
            {
                if (app->isFaceHot)
                    return app->faceResources.smileFaceDown;

                return app->faceResources.click;
            }

            break;
        }
        case GAME_WON:
            return app->faceResources.win;
        case GAME_LOST:
            return app->faceResources.lost;
    }

    return app->faceResources.smile;
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
RenderGridCell(
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
RenderBorders(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t cellSize = app->metrics.cellSize;
    uint32_t boardWidth = app->minefield.width * cellSize;
    uint32_t boardHeight = app->minefield.height * app->metrics.cellSize;
    uint32_t gridRightEdge = app->metrics.borderWidth + boardWidth;
    uint32_t originY = 2u * app->metrics.borderHeight + app->metrics.counterAreaHeight;
    uint32_t bottomEdge = originY + boardHeight;

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.topLeft,
        0,
        0,
        app->metrics.borderWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.top,
        app->metrics.borderWidth,
        0,
        boardWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.topRight,
        gridRightEdge,
        0,
        app->metrics.borderWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.left,
        0,
        app->metrics.borderHeight,
        app->metrics.borderWidth,
        app->metrics.counterAreaHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.right,
        gridRightEdge,
        app->metrics.borderHeight,
        app->metrics.borderWidth,
        app->metrics.counterAreaHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.middleLeft,
        0,
        originY - app->metrics.borderHeight,
        app->metrics.borderWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.top,
        app->metrics.borderWidth,
        originY - app->metrics.borderHeight,
        boardWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.middleRight,
        gridRightEdge,
        originY - app->metrics.borderHeight,
        app->metrics.borderWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(hdc, hdcMem, app->borderResources.left, 0, originY, app->metrics.borderWidth, boardHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.right,
        gridRightEdge,
        originY,
        app->metrics.borderWidth,
        boardHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.bottomLeft,
        0,
        bottomEdge,
        app->metrics.borderWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.bottom,
        app->metrics.borderWidth,
        bottomEdge,
        boardWidth,
        app->metrics.borderHeight);

    BlitBitmapScaled(
        hdc,
        hdcMem,
        app->borderResources.bottomRight,
        gridRightEdge,
        bottomEdge,
        app->metrics.borderWidth,
        app->metrics.borderHeight);
}

static void
RenderFace(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t contentLeft = app->metrics.borderWidth;
    uint32_t contentTop = app->metrics.borderHeight;
    uint32_t boardWidth = app->minefield.width * app->metrics.cellSize;

    uint32_t faceSize = app->metrics.faceSize;
    uint32_t faceLeft = contentLeft + (boardWidth - faceSize) / 2;
    uint32_t faceTop = contentTop + (app->metrics.counterAreaHeight - faceSize) / 2;

    HBITMAP face = GetFaceBitmap(app);
    BlitBitmapScaled(hdc, hdcMem, face, faceLeft, faceTop, faceSize, faceSize);
}

static void
RenderCounterArea(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    RenderFace(app, hdc, hdcMem);
}

static void
RenderGrid(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t cellSize = app->metrics.cellSize;
    uint32_t originX = app->metrics.borderWidth;
    uint32_t originY = 2u * app->metrics.borderHeight + app->metrics.counterAreaHeight;

    for (uint32_t y = 0; y < app->minefield.height; y++)
    {
        for (uint32_t x = 0; x < app->minefield.width; x++)
        {
            uint32_t left = originX + (x * cellSize);
            uint32_t top = originY + (y * cellSize);

            RECT cellRect = {
                .left = (LONG)left,
                .top = (LONG)top,
                .right = (LONG)(left + cellSize),
                .bottom = (LONG)(top + cellSize),
            };

            RenderGridCell(app, hdc, hdcMem, &cellRect, x, y);
        }
    }
}

void
RenderGameWindow(_In_ const Application* app, _In_ HDC hdc)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    int oldMode = SetStretchBltMode(hdc, HALFTONE);

    RenderBorders(app, hdc, hdcMem);
    RenderCounterArea(app, hdc, hdcMem);
    RenderGrid(app, hdc, hdcMem);

    SetStretchBltMode(hdc, oldMode);
    DeleteDC(hdcMem);
}
