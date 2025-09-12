#include "pch.h"

#include "application.h"
#include "game.h"
#include "resource.h"
#include "ui/render.h"
#include "ui/window.h"

_Success_(
    return) static bool AdjustWindowRectForCellSize(_In_ const Application* app, _In_ HWND hWnd, _Out_ LPRECT lpRect)
{
    UINT dpi = GetDpiForWindow(hWnd);

    int cellSize = MulDiv(CELL_SIZE, dpi, 96);
    int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

    uint32_t minClientWidth = app->minefield.width * (uint32_t)cellSize + (uint32_t)(2 * borderWidth);
    uint32_t minClientHeight = app->minefield.height * (uint32_t)cellSize + (uint32_t)(2 * borderHeight);

    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = (LONG)minClientWidth;
    lpRect->bottom = (LONG)minClientHeight;

    return AdjustWindowRectExForDpi(lpRect, GetWindowStyle(hWnd), TRUE, GetWindowExStyle(hWnd), dpi);
}

static bool
ResizeWindowForMinefield(_In_ const Application* app, _In_ HWND hWnd)
{
    RECT windowRect;

    if (!AdjustWindowRectForCellSize(app, hWnd, &windowRect))
        return false;

    return SetWindowPos(
        hWnd,
        NULL,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_NOMOVE | SWP_NOZORDER);
}

static void
ShowGameOverMessage(_In_ HWND hWnd, _In_ GameState state)
{
    switch (state)
    {
        case GAME_WON:
            MessageBoxW(hWnd, L"Congratulations! You won!", L"Victory", MB_OK | MB_ICONINFORMATION);
            break;
        case GAME_LOST:
            MessageBoxW(hWnd, L"Game Over! You hit a mine!", L"Game Over", MB_OK | MB_ICONEXCLAMATION);
            break;
        default:
            break;
    }
}

static void
HandleMouseMove(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    if (!app->isLeftMouseDown)
        return;

    uint32_t prevX = app->hoverCellX;
    uint32_t prevY = app->hoverCellY;

    UINT dpi = GetDpiForWindow(hWnd);

    int cellSize = MulDiv(CELL_SIZE, dpi, 96);
    int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

    if (x >= borderWidth && y >= borderHeight)
    {
        uint32_t cellX = (uint32_t)((x - borderWidth) / cellSize);
        uint32_t cellY = (uint32_t)((y - borderHeight) / cellSize);

        if (cellX < app->minefield.width && cellY < app->minefield.height)
        {
            app->hoverCellX = cellX;
            app->hoverCellY = cellY;
        }
        else
        {
            app->hoverCellX = (uint32_t)-1;
            app->hoverCellY = (uint32_t)-1;
        }
    }
    else
    {
        app->hoverCellX = (uint32_t)-1;
        app->hoverCellY = (uint32_t)-1;
    }

    if (app->hoverCellX != prevX || app->hoverCellY != prevY)
    {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

static void
HandleLeftMouseDown(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    SetCapture(hWnd);
    app->isLeftMouseDown = true;

    UINT dpi = GetDpiForWindow(hWnd);

    int cellSize = MulDiv(CELL_SIZE, dpi, 96);
    int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

    if (x >= borderWidth && y >= borderHeight)
    {
        uint32_t cellX = (uint32_t)((x - borderWidth) / cellSize);
        uint32_t cellY = (uint32_t)((y - borderHeight) / cellSize);

        if (cellX < app->minefield.width && cellY < app->minefield.height)
        {
            app->hoverCellX = cellX;
            app->hoverCellY = cellY;
        }
        else
        {
            app->hoverCellX = (uint32_t)-1;
            app->hoverCellY = (uint32_t)-1;
        }
    }
    else
    {
        app->hoverCellX = (uint32_t)-1;
        app->hoverCellY = (uint32_t)-1;
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

static void
HandleLeftMouseUp(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    bool revealed = false;

    if (app->isLeftMouseDown)
    {
        ReleaseCapture();
        app->isLeftMouseDown = false;

        UINT dpi = GetDpiForWindow(hWnd);

        int cellSize = MulDiv(CELL_SIZE, dpi, 96);
        int borderWidth = MulDiv(BORDER_WIDTH, dpi, 96);
        int borderHeight = MulDiv(BORDER_HEIGHT, dpi, 96);

        if (x >= borderWidth && y >= borderHeight)
        {
            uint32_t cellX = (uint32_t)((x - borderWidth) / cellSize);
            uint32_t cellY = (uint32_t)((y - borderHeight) / cellSize);

            if (cellX < app->minefield.width && cellY < app->minefield.height)
            {
                if (RevealCell(&app->minefield, cellX, cellY))
                {
                    revealed = true;
                }
            }
        }
    }

    app->hoverCellX = (uint32_t)-1;
    app->hoverCellY = (uint32_t)-1;

    InvalidateRect(hWnd, NULL, FALSE);

    if (revealed)
    {
        if (app->minefield.state == GAME_WON || app->minefield.state == GAME_LOST)
        {
            ShowGameOverMessage(hWnd, app->minefield.state);
        }
    }
}

static void
HandleRightMouseClick(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    UINT dpi = GetDpiForWindow(hWnd);

    int scaledCell = MulDiv(CELL_SIZE, dpi, 96);
    int borderW = MulDiv(BORDER_WIDTH, dpi, 96);
    int borderH = MulDiv(BORDER_HEIGHT, dpi, 96);

    if (x < borderW || y < borderH)
    {
        return;
    }

    uint32_t cellX = (uint32_t)(x - borderW) / (uint32_t)scaledCell;
    uint32_t cellY = (uint32_t)(y - borderH) / (uint32_t)scaledCell;

    if (cellX >= app->minefield.width || cellY >= app->minefield.height)
    {
        return;
    }

    if (ToggleFlag(&app->minefield, cellX, cellY))
    {
        InvalidateRect(hWnd, NULL, false);
    }
}

static void
StartNewGame(_In_ Application* app, _In_ HWND hWnd, _In_ Difficulty difficulty)
{
    if (!CreateMinefield(&app->minefield, difficulty))
    {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];

        switch (error)
        {
            case ERROR_NOT_ENOUGH_MEMORY:
                wcscpy_s(errorMsg, ARRAYSIZE(errorMsg), L"Not enough memory to create minefield!");
                break;
            case ERROR_INVALID_PARAMETER:
                wcscpy_s(errorMsg, ARRAYSIZE(errorMsg), L"Invalid difficulty settings!");
                break;
            case ERROR_ARITHMETIC_OVERFLOW:
                wcscpy_s(errorMsg, ARRAYSIZE(errorMsg), L"Minefield too large!");
                break;
            default:
                swprintf_s(errorMsg, ARRAYSIZE(errorMsg), L"Failed to create minefield! Error: %lu", error);
                break;
        }

        MessageBoxW(hWnd, errorMsg, L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (!ResizeWindowForMinefield(app, hWnd))
    {
        MessageBoxW(hWnd, L"Minefield too large for display!", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    app->isLeftMouseDown = false;
    app->hoverCellX = (uint32_t)-1;
    app->hoverCellY = (uint32_t)-1;

    InvalidateRect(hWnd, NULL, true);
}

static LRESULT CALLBACK
WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
            Application* app = (Application*)pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)app);

            break;
        }
        case WM_CREATE:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                StartNewGame(app, hWnd, DIFFICULTY_BEGINNER);
            }

            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);

            return 0;
        }
        case WM_NCDESTROY:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            DestroyApplication(app);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL);

            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                RenderGameWindow(app, hWnd, hdc);
            }

            EndPaint(hWnd, &ps);

            return 0;
        }
        case WM_COMMAND:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            switch (LOWORD(wParam))
            {
                case IDM_GAME_NEW:
                    StartNewGame(app, hWnd, app->minefield.difficulty);
                    break;
                case IDM_GAME_BEGINNER:
                    StartNewGame(app, hWnd, DIFFICULTY_BEGINNER);
                    break;
                case IDM_GAME_INTERMEDIATE:
                    StartNewGame(app, hWnd, DIFFICULTY_INTERMEDIATE);
                    break;
                case IDM_GAME_EXPERT:
                    StartNewGame(app, hWnd, DIFFICULTY_EXPERT);
                    break;
                case IDM_GAME_CUSTOM:
                    MessageBoxW(hWnd, L"Custom difficulty not implemented yet", L"Info", MB_OK);
                    break;
                case IDM_GAME_BEST_TIMES:
                    MessageBoxW(hWnd, L"Leaderboard clicked!", L"Info", MB_OK);
                    break;
                case IDM_GAME_EXIT:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
                case IDM_HELP_ABOUT:
                    MessageBoxW(hWnd, L"Minesweeper v1.0\nBuilt with Win32 API", L"About", MB_OK);
                    break;
                default:
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }

            return 0;
        }
        case WM_MOUSEMOVE:
        {
            int32_t x = GET_X_LPARAM(lParam);
            int32_t y = GET_Y_LPARAM(lParam);
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                HandleMouseMove(app, hWnd, x, y);
            }

            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            int32_t x = GET_X_LPARAM(lParam);
            int32_t y = GET_Y_LPARAM(lParam);
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                HandleLeftMouseDown(app, hWnd, x, y);
            }

            return 0;
        }
        case WM_LBUTTONUP:
        {
            int32_t x = GET_X_LPARAM(lParam);
            int32_t y = GET_Y_LPARAM(lParam);
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                HandleLeftMouseUp(app, hWnd, x, y);
            }

            return 0;
        }
        case WM_RBUTTONDOWN:
        {
            int32_t x = GET_X_LPARAM(lParam);
            int32_t y = GET_Y_LPARAM(lParam);
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                HandleRightMouseClick(app, hWnd, x, y);
            }

            return 0;
        }
        case WM_DPICHANGED:
        {
            RECT* const prcNewWindow = (RECT*)lParam;
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            SetWindowPos(
                hWnd,
                NULL,
                prcNewWindow->left,
                prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            if (app != NULL)
            {
                ResizeWindowForMinefield(app, hWnd);
            }

            return 0;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

_Ret_maybenull_ HWND
CreateMainWindow(_In_ HINSTANCE hInstance)
{
    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = WindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName = NULL,
        .lpszClassName = L"Minesweeper",
        .hIconSm = NULL,
    };

    ATOM classAtom = RegisterClassExW(&wc);

    if (classAtom == 0)
        return NULL;

    Application* app = CreateApplication(hInstance);

    if (app == NULL)
    {
        UnregisterClassW(MAKEINTATOM(classAtom), hInstance);
        return NULL;
    }

    HWND hWnd = CreateWindowExW(
        0,
        MAKEINTATOM(classAtom),
        L"Minesweeper",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        NULL,
        LoadMenuW(hInstance, MAKEINTRESOURCEW(IDR_MENU1)),
        hInstance,
        app);

    if (hWnd == NULL)
    {
        UnregisterClassW(MAKEINTATOM(classAtom), hInstance);
        DestroyApplication(app);

        return NULL;
    }

    return hWnd;
}
