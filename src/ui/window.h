#pragma once

#include <sal.h>

#define CELL_SIZE 24

#define BORDER_WIDTH 12
#define BORDER_HEIGHT 12

#define COUNTER_BORDER_WIDTH 2
#define COUNTER_AREA_HEIGHT 40
#define COUNTER_MARGIN 4
#define COUNTER_HEIGHT 28
#define COUNTER_DIGIT_WIDTH 16

#define FACE_SIZE 28

_Ret_maybenull_ HWND CreateMainWindow(_In_ HINSTANCE hInstance);
