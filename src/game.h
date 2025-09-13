#pragma once

#include <sal.h>

#include <stdbool.h>
#include <stdint.h>

#define MAX_CELLS_VERTICALLY 100
#define MAX_CELLS_HORIZONTALLY 100

typedef enum
{
    CELL_HIDDEN,
    CELL_REVEALED,
    CELL_FLAGGED
} CellState;

typedef struct
{
    CellState state;
    uint8_t neighborMines;
    bool hasMine;
} Cell;

typedef enum
{
    GAME_PLAYING,
    GAME_WON,
    GAME_LOST
} GameState;

typedef enum
{
    DIFFICULTY_BEGINNER,
    DIFFICULTY_INTERMEDIATE,
    DIFFICULTY_EXPERT,
    DIFFICULTY_CUSTOM
} Difficulty;

typedef struct
{
    Cell cells[MAX_CELLS_VERTICALLY * MAX_CELLS_HORIZONTALLY];
    uint64_t startTime;
    uint64_t endTime;
    uint32_t width;
    uint32_t height;
    uint32_t totalMines;
    uint32_t flaggedCells;
    uint32_t revealedCells;
    GameState state;
    Difficulty difficulty;
    uint32_t blastX;
    uint32_t blastY;
    bool firstClick;
} Minefield;

bool CreateMinefield(_Out_ Minefield* field, _In_ Difficulty difficulty);

bool CreateCustomMinefield(_Out_ Minefield* field, _In_ uint32_t width, _In_ uint32_t height, _In_ uint32_t totalMines);

bool RevealCell(_Inout_ Minefield* field, _In_ uint32_t x, _In_ uint32_t y);

bool ToggleFlag(_Inout_ Minefield* field, _In_ uint32_t x, _In_ uint32_t y);

_Ret_maybenull_ const Cell* GetCell(_In_ const Minefield* field, _In_ uint32_t x, _In_ uint32_t y);
