#include "pch.h"

#include "game.h"
#include "random.h"

static void
GetDifficultySettings(_In_ Difficulty difficulty, _Out_ uint32_t* width, _Out_ uint32_t* height, _Out_ uint32_t* mines)
{
    switch (difficulty)
    {
        case DIFFICULTY_BEGINNER:
            *width = 9;
            *height = 9;
            *mines = 10;
            break;
        case DIFFICULTY_INTERMEDIATE:
            *width = 16;
            *height = 16;
            *mines = 40;
            break;
        case DIFFICULTY_EXPERT:
            *width = 30;
            *height = 16;
            *mines = 99;
            break;
        case DIFFICULTY_CUSTOM:
            *width = 9;
            *height = 9;
            *mines = 10;
            break;
        default:
            *width = 9;
            *height = 9;
            *mines = 10;
            break;
    }
}

static void
RevealConnectedCells(_Inout_ Minefield* field, _In_ uint32_t x, _In_ uint32_t y)
{
    if (x >= field->width || y >= field->height)
        return;

    Cell* cell = &field->cells[y * field->width + x];

    if (cell->state != CELL_HIDDEN || cell->hasMine)
        return;

    cell->state = CELL_REVEALED;
    field->revealedCells++;

    if (cell->neighborMines > 0)
        return;

    for (int32_t dy = -1; dy <= 1; dy++)
    {
        for (int32_t dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
                continue;

            int32_t nx = (int32_t)x + dx;
            int32_t ny = (int32_t)y + dy;

            if (nx >= 0 && ny >= 0 && nx < (int32_t)field->width && ny < (int32_t)field->height)
            {
                RevealConnectedCells(field, (uint32_t)nx, (uint32_t)ny);
            }
        }
    }
}

static void
PlaceMines(_Inout_ Minefield* field, _In_ uint32_t excludeX, _In_ uint32_t excludeY)
{
    if (excludeX >= field->width || excludeY >= field->height)
        return;

    struct xorshift32_state state = {
        .a = (uint32_t)GetTickCount64(),
    };

    uint32_t minesPlaced = 0;
    uint32_t attempts = 0;
    uint32_t maxAttempts = field->width * field->height * 10;

    while (minesPlaced < field->totalMines && attempts < maxAttempts)
    {
        uint32_t x = xorshift32(&state) % field->width;
        uint32_t y = xorshift32(&state) % field->height;

        attempts++;

        if (x == excludeX && y == excludeY)
            continue;

        Cell* cell = &field->cells[y * field->width + x];

        if (!cell->hasMine)
        {
            cell->hasMine = true;
            minesPlaced++;
        }
    }
}

static void
CalculateNeighborMines(_Inout_ Minefield* field)
{
    for (uint32_t y = 0; y < field->height; y++)
    {
        for (uint32_t x = 0; x < field->width; x++)
        {
            Cell* cell = &field->cells[y * field->width + x];

            if (cell->hasMine)
                continue;

            uint8_t count = 0;

            for (int32_t dy = -1; dy <= 1; dy++)
            {
                for (int32_t dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0)
                        continue;

                    int32_t nx = (int32_t)x + dx;
                    int32_t ny = (int32_t)y + dy;

                    if (nx >= 0 && nx < (int32_t)field->width && ny >= 0 && ny < (int32_t)field->height)
                    {
                        if (field->cells[ny * field->width + nx].hasMine)
                        {
                            count++;
                        }
                    }
                }
            }

            cell->neighborMines = count;
        }
    }
}

bool
CreateMinefield(_Out_ Minefield* field, _In_ Difficulty difficulty)
{
    ZeroMemory(field, sizeof(Minefield));
    GetDifficultySettings(difficulty, &field->width, &field->height, &field->totalMines);

    uint32_t maxMines = field->width * field->height - 1;

    if (field->totalMines > maxMines)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (field->width > MAX_CELLS_HORIZONTALLY || field->height > MAX_CELLS_VERTICALLY)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    field->difficulty = difficulty;
    field->state = GAME_PLAYING;
    field->flaggedCells = 0;
    field->revealedCells = 0;
    field->firstClick = true;
    field->startTime = 0;

    return true;
}

bool
CreateCustomMinefield(_Out_ Minefield* field, _In_ uint32_t width, _In_ uint32_t height, _In_ uint32_t totalMines)
{
    ZeroMemory(field, sizeof(Minefield));
    field->width = width;
    field->height = height;
    field->totalMines = totalMines;

    uint32_t maxMines = (width * height);

    if (width == 0 || height == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (totalMines >= maxMines)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (width > MAX_CELLS_HORIZONTALLY || height > MAX_CELLS_VERTICALLY)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    field->difficulty = DIFFICULTY_CUSTOM;
    field->state = GAME_PLAYING;
    field->flaggedCells = 0;
    field->revealedCells = 0;
    field->firstClick = true;
    field->startTime = 0;

    return true;
}

bool
RevealCell(_Inout_ Minefield* field, _In_ uint32_t x, _In_ uint32_t y)
{
    if (x >= field->width || y >= field->height || field->state != GAME_PLAYING)
    {
        return false;
    }

    Cell* cell = &field->cells[y * field->width + x];

    if (cell->state != CELL_HIDDEN)
        return false;

    if (field->firstClick)
    {
        field->firstClick = false;
        field->startTime = GetTickCount64();

        PlaceMines(field, x, y);
        CalculateNeighborMines(field);
    }

    if (cell->hasMine)
    {
        cell->state = CELL_REVEALED;
        field->state = GAME_LOST;
        field->blastX = x;
        field->blastY = y;
        field->endTime = GetTickCount64();

        return true;
    }

    RevealConnectedCells(field, x, y);

    uint32_t totalCells = field->width * field->height;
    if (field->revealedCells == totalCells - field->totalMines)
    {
        field->state = GAME_WON;
        field->endTime = GetTickCount64();
    }

    return true;
}

bool
ToggleFlag(_Inout_ Minefield* field, _In_ uint32_t x, _In_ uint32_t y)
{
    if (x >= field->width || y >= field->height)
        return false;

    if (field->state != GAME_PLAYING)
        return false;

    Cell* cell = &field->cells[y * field->width + x];

    if (cell->state == CELL_REVEALED)
        return false;

    if (cell->state == CELL_FLAGGED)
    {
        cell->state = CELL_HIDDEN;

        if (field->flaggedCells > 0)
        {
            field->flaggedCells--;
        }
    }
    else if (cell->state == CELL_HIDDEN)
    {
        cell->state = CELL_FLAGGED;
        field->flaggedCells++;
    }

    return true;
}

_Ret_maybenull_ const Cell*
GetCell(_In_ const Minefield* field, _In_ uint32_t x, _In_ uint32_t y)
{
    if (x >= field->width || y >= field->height)
        return NULL;

    return &field->cells[y * field->width + x];
}
