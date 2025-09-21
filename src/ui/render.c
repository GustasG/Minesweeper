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
        {
            if (app->isLeftMouseDown)
            {
                if (app->isFaceHot)
                    return app->faceResources.smileFaceDown;
            }

            return app->faceResources.win;
        }
        case GAME_LOST:
        {
            if (app->isLeftMouseDown)
            {
                if (app->isFaceHot)
                    return app->faceResources.smileFaceDown;
            }

            return app->faceResources.lost;
        }
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
    uint32_t cellWidth = app->metrics.cellWidth;
    uint32_t cellHeight = app->metrics.cellHeight;
    uint32_t boardWidth = app->minefield.width * cellWidth;
    uint32_t boardHeight = app->minefield.height * cellHeight;
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
    uint32_t boardWidth = app->minefield.width * app->metrics.cellWidth;

    uint32_t faceWidth = app->metrics.faceWidth;
    uint32_t faceHeight = app->metrics.faceHeight;
    int32_t faceLeft = (int32_t)contentLeft + ((int32_t)boardWidth - (int32_t)faceWidth) / 2;
    int32_t faceTop = (int32_t)contentTop + ((int32_t)app->metrics.counterAreaHeight - (int32_t)faceHeight) / 2;

    HBITMAP face = GetFaceBitmap(app);
    BlitBitmapScaled(hdc, hdcMem, face, faceLeft, faceTop, faceWidth, faceHeight);
}

static void
RenderRemainingMines(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t contentLeft = app->metrics.borderWidth;
    uint32_t contentTop = app->metrics.borderHeight;
    uint32_t top = contentTop + (app->metrics.counterAreaHeight - app->metrics.counterHeight) / 2u;
    uint32_t height = app->metrics.counterHeight;

    // Left border
    uint32_t leftLeft = contentLeft + app->metrics.counterMargin;
    uint32_t leftWidth = app->metrics.counterBorderWidth;
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterLeft, leftLeft, top, leftWidth, height);

    // Middle border (backdround)
    uint32_t middleLeft = leftLeft + leftWidth;
    uint32_t middleWidth = 3u * app->metrics.counterDigitWidth;
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterMiddle, middleLeft, top, middleWidth, height);

    // Right border
    uint32_t rightLeft = middleLeft + middleWidth;
    uint32_t rightWidth = app->metrics.counterBorderWidth;
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterRight, rightLeft, top, rightWidth, height);

    int32_t value = (int32_t)app->minefield.totalMines - (int32_t)app->minefield.flaggedCells;
    value = max(-99, min(999, value));

    HBITMAP s0 = NULL, s1 = NULL, s2 = NULL;

    if (value < 0)
    {
        value = min(99, -value);

        s0 = app->counterResources.minus;
        s1 = app->counterResources.digits[(value / 10) % 10];
        s2 = app->counterResources.digits[value % 10];
    }
    else
    {
        s0 = app->counterResources.digits[(value / 100) % 10];
        s1 = app->counterResources.digits[(value / 10) % 10];
        s2 = app->counterResources.digits[value % 10];
    }

    uint32_t counterWidth = app->metrics.counterDigitWidth;
    BlitBitmapScaled(hdc, hdcMem, s0, middleLeft + 0u * counterWidth, top + 3, counterWidth, height - 3);
    BlitBitmapScaled(hdc, hdcMem, s1, middleLeft + 1u * counterWidth, top + 3, counterWidth, height - 3);
    BlitBitmapScaled(hdc, hdcMem, s2, middleLeft + 2u * counterWidth, top + 3, counterWidth, height - 3);
}

static void
RenderTimer(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t contentLeft = app->metrics.borderWidth;
    uint32_t contentTop = app->metrics.borderHeight;
    uint32_t boardWidth = app->minefield.width * app->metrics.cellWidth;

    uint32_t top = contentTop + (app->metrics.counterAreaHeight - app->metrics.counterHeight) / 2u;
    uint32_t height = app->metrics.counterHeight;

    uint32_t rightWidth = app->metrics.counterBorderWidth;
    uint32_t middleWidth = 3u * app->metrics.counterDigitWidth;
    uint32_t leftWidth = app->metrics.counterBorderWidth;

    int32_t rightLeft = contentLeft + boardWidth - app->metrics.counterMargin - rightWidth;
    uint32_t middleLeft = rightLeft - middleWidth;
    uint32_t leftLeft = middleLeft - leftWidth;

    // Left border
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterLeft, leftLeft, top, leftWidth, height);

    // Middle background
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterMiddle, middleLeft, top, middleWidth, height);

    // Right border
    BlitBitmapScaled(hdc, hdcMem, app->borderResources.counterRight, rightLeft, top, rightWidth, height);

    int32_t seconds = 0;

    if (!app->minefield.firstClick)
    {
        uint64_t diff = 0;

        if (app->minefield.state == GAME_PLAYING)
        {
            diff = GetTickCount64() - app->minefield.startTime;
        }
        else
        {
            diff = app->minefield.endTime - app->minefield.startTime;
        }

        seconds = (int32_t)(diff / 1000ULL);
    }

    seconds = max(0, min(999, seconds));

    uint32_t counterWidth = app->metrics.counterDigitWidth;

    HBITMAP s0 = app->counterResources.digits[(seconds / 100) % 10];
    BlitBitmapScaled(hdc, hdcMem, s0, middleLeft + 0u * counterWidth, top + 3, counterWidth, height - 3);

    HBITMAP s1 = app->counterResources.digits[(seconds / 10) % 10];
    BlitBitmapScaled(hdc, hdcMem, s1, middleLeft + 1u * counterWidth, top + 3, counterWidth, height - 3);

    HBITMAP s2 = app->counterResources.digits[seconds % 10];
    BlitBitmapScaled(hdc, hdcMem, s2, middleLeft + 2u * counterWidth, top + 3, counterWidth, height - 3);
}

static void
RenderCounterArea(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    RenderRemainingMines(app, hdc, hdcMem);
    RenderTimer(app, hdc, hdcMem);
    RenderFace(app, hdc, hdcMem);
}

static void
RenderGrid(_In_ const Application* app, _In_ HDC hdc, _In_ HDC hdcMem)
{
    uint32_t cellWidth = app->metrics.cellWidth;
    uint32_t cellHeight = app->metrics.cellHeight;
    uint32_t originX = app->metrics.borderWidth;
    uint32_t originY = 2u * app->metrics.borderHeight + app->metrics.counterAreaHeight;

    for (uint32_t y = 0; y < app->minefield.height; y++)
    {
        for (uint32_t x = 0; x < app->minefield.width; x++)
        {
            uint32_t left = originX + (x * cellWidth);
            uint32_t top = originY + (y * cellHeight);

            RECT cellRect = {
                .left = (LONG)left,
                .top = (LONG)top,
                .right = (LONG)(left + cellWidth),
                .bottom = (LONG)(top + cellHeight),
            };

            RenderGridCell(app, hdc, hdcMem, &cellRect, x, y);
        }
    }
}

void
RenderGameWindow(_In_ const Application* app, _In_ HDC hdc)
{
    HWND hwnd = WindowFromDC(hdc);
    RECT clientRect = {0};

    if (hwnd != NULL)
    {
        if (!GetClientRect(hwnd, &clientRect))
            return;
    }
    else
    {
        clientRect.right = (LONG)app->clientWidth;
        clientRect.bottom = (LONG)app->clientHeight;
    }

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (width <= 0 || height <= 0)
        return;

    HDC hdcBack = CreateCompatibleDC(hdc);
    HDC hdcMem = CreateCompatibleDC(hdc);

    if (hdcBack == NULL || hdcMem == NULL)
    {
        if (hdcBack != NULL)
            DeleteDC(hdcBack);

        if (hdcMem != NULL)
            DeleteDC(hdcMem);

        return;
    }

    HBITMAP hbmBack = CreateCompatibleBitmap(hdc, width, height);

    if (hbmBack == NULL)
    {
        DeleteDC(hdcBack);
        DeleteDC(hdcMem);
        return;
    }

    HBITMAP oldBack = (HBITMAP)SelectObject(hdcBack, hbmBack);
    int oldMode = SetStretchBltMode(hdcBack, HALFTONE);

    RECT clearRect = {0, 0, width, height};
    HBRUSH brush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);

    if (brush != NULL)
        FillRect(hdcBack, &clearRect, brush);
    else
        PatBlt(hdcBack, 0, 0, width, height, WHITENESS);

    RenderBorders(app, hdcBack, hdcMem);
    RenderCounterArea(app, hdcBack, hdcMem);
    RenderGrid(app, hdcBack, hdcMem);

    BitBlt(hdc, 0, 0, width, height, hdcBack, 0, 0, SRCCOPY);

    SetStretchBltMode(hdcBack, oldMode);
    SelectObject(hdcBack, oldBack);

    DeleteObject(hbmBack);
    DeleteDC(hdcBack);
    DeleteDC(hdcMem);
}
