#include "pch.h"

#include "application.h"
#include "game.h"
#include "resource.h"
#include "ui/render.h"
#include "ui/window.h"

#define CLASS_NAME L"Minesweeper"

static bool
AdjustWindowRectForCellSize(_In_ const Application* app, _In_ HWND hWnd, _Out_ LPRECT lpRect)
{
    uint32_t minClientWidth = app->minefield.width * MIN_CELL_SIZE + MARGIN * 2;
    uint32_t minClientHeight = app->minefield.height * MIN_CELL_SIZE + MARGIN * 2;

    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = (LONG)minClientWidth;
    lpRect->bottom = (LONG)minClientHeight;

    WINDOWINFO wi = {
        .cbSize = sizeof(WINDOWINFO),
    };

    if (GetWindowInfo(hWnd, &wi))
    {
        if (AdjustWindowRectEx(lpRect, wi.dwStyle, TRUE, wi.dwExStyle))
        {
            return true;
        }
    }

    return false;
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
HandleLeftMouseClick(_In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (x < (int32_t)MARGIN || y < (int32_t)MARGIN)
    {
        return;
    }

    uint32_t cellX = (uint32_t)(x - MARGIN) / app->renderInfo.cellSize;
    uint32_t cellY = (uint32_t)(y - MARGIN) / app->renderInfo.cellSize;

    if (cellX >= app->minefield.width || cellY >= app->minefield.height)
    {
        return;
    }

    if (RevealCell(&app->minefield, cellX, cellY))
    {
        InvalidateRect(hWnd, NULL, false);

        if (app->minefield.state == GAME_WON || app->minefield.state == GAME_LOST)
        {
            ShowGameOverMessage(hWnd, app->minefield.state);
        }
    }
}

static void
HandleRightMouseClick(_In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (x < (int32_t)MARGIN || y < (int32_t)MARGIN)
    {
        return;
    }

    uint32_t cellX = (uint32_t)(x - MARGIN) / app->renderInfo.cellSize;
    uint32_t cellY = (uint32_t)(y - MARGIN) / app->renderInfo.cellSize;

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

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    CalculateRenderInfo(&clientRect, app->minefield.width, app->minefield.height, &app->renderInfo);

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
            SetWindowLongPtr(hWnd, GWLP_HINSTANCE, (LONG_PTR)NULL);

            return 0;
        }
        case WM_SIZE:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                RECT clientRect;
                GetClientRect(hWnd, &clientRect);

                CalculateRenderInfo(&clientRect, app->minefield.width, app->minefield.height, &app->renderInfo);
                InvalidateRect(hWnd, NULL, FALSE);
            }

            return 0;
        }
        case WM_PAINT:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            if (app != NULL)
            {
                RenderMinefield(app, hdc);
            }

            EndPaint(hWnd, &ps);

            return 0;
        }
        case WM_GETMINMAXINFO:
        {
            const Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL)
            {
                RECT windowRect;
                if (AdjustWindowRectForCellSize(app, hWnd, &windowRect))
                {
                    MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
                    pMinMaxInfo->ptMinTrackSize.x = windowRect.right - windowRect.left;
                    pMinMaxInfo->ptMinTrackSize.y = windowRect.bottom - windowRect.top;

                    return 0;
                }
            }

            return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
        case WM_LBUTTONDOWN:
        {
            int32_t x = LOWORD(lParam);
            int32_t y = HIWORD(lParam);

            HandleLeftMouseClick(hWnd, x, y);
            return 0;
        }
        case WM_RBUTTONDOWN:
        {
            int32_t x = LOWORD(lParam);
            int32_t y = HIWORD(lParam);

            HandleRightMouseClick(hWnd, x, y);
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
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName = NULL,
        .lpszClassName = CLASS_NAME,
        .hIconSm = NULL,
    };

    if (!RegisterClassExW(&wc))
        return NULL;

    Application* app = CreateApplication(hInstance);

    if (app == NULL)
    {
        UnregisterClassW(CLASS_NAME, hInstance);
        return NULL;
    }

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Minesweeper",
        WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
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
        UnregisterClassW(CLASS_NAME, hInstance);
        DestroyApplication(app);

        return NULL;
    }

    return hWnd;
}
