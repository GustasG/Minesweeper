#include "pch.h"

#include "ui/render.h"
#include "ui/window.h"

void
CalculateRenderInfo(
    _In_ const RECT* clientRect,
    _In_ uint32_t minefieldWidth,
    _In_ uint32_t minefieldHeight,
    _Out_ RenderInfo* renderInfo)
{
    if (minefieldWidth == 0 || minefieldHeight == 0)
    {
        renderInfo->cellSize = MIN_CELL_SIZE;
        renderInfo->boardWidth = 0;
        renderInfo->boardHeight = 0;
        return;
    }

    uint32_t availableWidth = (uint32_t)(clientRect->right - clientRect->left) - (MARGIN * 2);
    uint32_t availableHeight = (uint32_t)(clientRect->bottom - clientRect->top) - (MARGIN * 2);

    uint32_t cellSizeByWidth = availableWidth / minefieldWidth;
    uint32_t cellSizeByHeight = availableHeight / minefieldHeight;

    uint32_t cellSize = max(cellSizeByWidth, cellSizeByHeight);
    cellSize = max(cellSize, MIN_CELL_SIZE);

    if (cellSize * minefieldWidth > availableWidth || cellSize * minefieldHeight > availableHeight)
    {
        cellSize = min(cellSizeByWidth, cellSizeByHeight);
        cellSize = max(cellSize, MIN_CELL_SIZE);
    }

    renderInfo->cellSize = cellSize;
    renderInfo->boardWidth = minefieldWidth * cellSize;
    renderInfo->boardHeight = minefieldHeight * cellSize;
}

static HBITMAP
GetCellBackground(_In_ const Application* app, _In_ const Cell* cell, _In_ uint32_t x, _In_ uint32_t y)
{
    if (app->minefield.state == GAME_LOST && cell->state == CELL_HIDDEN && cell->hasMine)
    {
        if (x != app->minefield.blastX || y != app->minefield.blastY)
        {
            return app->cellResources.mine;
        }
    }

    switch (cell->state)
    {
        case CELL_HIDDEN:
        {
            return app->cellResources.up;
        }
        case CELL_REVEALED:
        {
            if (cell->hasMine)
            {
                if (app->minefield.state == GAME_LOST && x == app->minefield.blastX && y == app->minefield.blastY)
                {
                    return app->cellResources.blast;
                }

                return app->cellResources.mine;
            }

            return app->cellResources.down;
        }
        case CELL_FLAGGED:
        {
            if (app->minefield.state == GAME_LOST && !cell->hasMine)
            {
                return app->cellResources.falseMine;
            }

            return app->cellResources.flag;
        }
    }

    return NULL;
}

static HBITMAP
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

    if (backgroundBitmap != NULL)
    {
        BITMAP bm;
        GetObjectW(backgroundBitmap, sizeof(BITMAP), &bm);

        HBITMAP hOldBitmap = SelectObject(hdcMem, backgroundBitmap);

        StretchBlt(
            hdc,
            cellRect->left,
            cellRect->top,
            cellWidth,
            cellHeight,
            hdcMem,
            0,
            0,
            bm.bmWidth,
            bm.bmHeight,
            SRCCOPY);

        SelectObject(hdcMem, hOldBitmap);
    }

    if (numberBitmap != NULL)
    {
        BITMAP bm;
        GetObjectW(numberBitmap, sizeof(BITMAP), &bm);

        HBITMAP hOldBitmap = SelectObject(hdcMem, numberBitmap);

        StretchBlt(
            hdc,
            cellRect->left,
            cellRect->top,
            cellWidth,
            cellHeight,
            hdcMem,
            0,
            0,
            bm.bmWidth,
            bm.bmHeight,
            SRCCOPY);

        SelectObject(hdcMem, hOldBitmap);
    }
}

void
RenderMinefield(_In_ const Application* app, _In_ HDC hdc)
{
    HDC hdcMem = CreateCompatibleDC(hdc);

    for (uint32_t y = 0; y < app->minefield.height; y++)
    {
        for (uint32_t x = 0; x < app->minefield.width; x++)
        {
            RECT cellRect = {
                (LONG)(MARGIN + x * app->renderInfo.cellSize),
                (LONG)(MARGIN + y * app->renderInfo.cellSize),
                (LONG)(MARGIN + (x + 1) * app->renderInfo.cellSize),
                (LONG)(MARGIN + (y + 1) * app->renderInfo.cellSize),
            };

            RenderSingleCell(app, hdc, hdcMem, &cellRect, x, y);
        }
    }

    DeleteDC(hdcMem);
}
