#include "pch.h"

#include "application.h"
#include "game.h"
#include "resource.h"
#include "ui/render.h"
#include "ui/window.h"

_Success_(return) static bool TryGetCellFromPoint(
    _In_ const Application* app,
    _In_ int32_t x,
    _In_ int32_t y,
    _Out_ uint32_t* outX,
    _Out_ uint32_t* outY)
{
    int32_t originX = (int32_t)app->metrics.borderWidth;
    int32_t originY = (int32_t)(app->metrics.borderHeight + app->metrics.counterAreaHeight + app->metrics.borderHeight);

    if (x < originX || y < originY)
        return false;

    int32_t relX = x - originX;
    int32_t relY = y - originY;
    int32_t cellSize = (int32_t)app->metrics.cellSize;
    uint32_t cellX = (uint32_t)(relX / cellSize);
    uint32_t cellY = (uint32_t)(relY / cellSize);

    if (cellX >= app->minefield.width || cellY >= app->minefield.height)
        return false;

    *outX = cellX;
    *outY = cellY;

    return true;
}

static void
GetFaceRect(_In_ const Application* app, _Out_ LPRECT lpRect)
{
    uint32_t contentLeft = app->metrics.borderWidth;
    uint32_t contentTop = app->metrics.borderHeight;
    uint32_t boardWidth = app->minefield.width * app->metrics.cellSize;
    uint32_t faceSize = app->metrics.faceSize;

    uint32_t faceLeft = contentLeft + (boardWidth - faceSize) / 2;
    uint32_t faceTop = contentTop + (app->metrics.counterAreaHeight - faceSize) / 2;

    lpRect->left = faceLeft;
    lpRect->top = faceTop;
    lpRect->right = faceLeft + faceSize;
    lpRect->bottom = faceTop + faceSize;
}

static bool
IsPointInFace(_In_ const Application* app, _In_ int32_t x, _In_ int32_t y)
{
    RECT fr;
    POINT pt = {x, y};

    GetFaceRect(app, &fr);

    return PtInRect(&fr, pt);
}

static void
UpdateFaceHot(_Inout_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    bool prev = app->isFaceHot;
    app->isFaceHot = IsPointInFace(app, x, y);

    if (app->isFaceHot != prev)
    {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

static bool
IsDarkModeEnabled(void)
{
    DWORD value = 1; // 1 = light, 0 = dark
    HKEY hKey;

    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0,
            KEY_READ,
            &hKey) == ERROR_SUCCESS)
    {
        DWORD size = sizeof(value);
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }

    return value == 0;
}

static void
ApplyDwmWindowAttributes(_In_ HWND hWnd)
{
    const BOOL useDark = IsDarkModeEnabled();
    (void)DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

    const DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    (void)DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    const int backdrop = DWMWA_NCRENDERING_POLICY;
    (void)DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
}

static void
UpdateLayoutMetricsForWindow(_Inout_ Application* app, _In_ HWND hWnd)
{
    UINT dpi = GetDpiForWindow(hWnd);

    app->metrics.cellSize = (uint32_t)MulDiv(CELL_SIZE, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.borderWidth = (uint32_t)MulDiv(BORDER_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.borderHeight = (uint32_t)MulDiv(BORDER_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.counterBorderWidth = (uint32_t)MulDiv(COUNTER_BORDER_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.counterAreaHeight = (uint32_t)MulDiv(COUNTER_AREA_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.counterMargin = (uint32_t)MulDiv(COUNTER_MARGIN, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.counterHeight = (uint32_t)MulDiv(COUNTER_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.counterDigitWidth = (uint32_t)MulDiv(COUNTER_DIGIT_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
    app->metrics.faceSize = (uint32_t)MulDiv(FACE_SIZE, dpi, USER_DEFAULT_SCREEN_DPI);
}

_Success_(
    return) static bool AdjustWindowRectForCellSize(_In_ const Application* app, _In_ HWND hWnd, _Out_ LPRECT lpRect)
{
    UINT dpi = GetDpiForWindow(hWnd);

    uint32_t boardWidth = app->minefield.width * app->metrics.cellSize;
    uint32_t boardHeight = app->minefield.height * app->metrics.cellSize;

    // left border + grid + right border
    uint32_t minClientWidth = boardWidth + 2u * app->metrics.borderWidth;

    // top border + counter area + middle part (border) + grid + bottom border
    uint32_t minClientHeight = boardHeight + (3u * app->metrics.borderHeight) + app->metrics.counterAreaHeight;

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

    InvalidateRect(hWnd, NULL, TRUE);
}

static void
HandleMouseMove(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    if (!app->isLeftMouseDown)
        return;

    uint32_t prevX = app->hoverCellX;
    uint32_t prevY = app->hoverCellY;
    uint32_t cellX, cellY;

    if (TryGetCellFromPoint(app, x, y, &cellX, &cellY))
    {
        app->hoverCellX = cellX;
        app->hoverCellY = cellY;
    }
    else
    {
        app->hoverCellX = (uint32_t)-1;
        app->hoverCellY = (uint32_t)-1;
    }

    UpdateFaceHot(app, hWnd, x, y);

    if (app->hoverCellX != prevX || app->hoverCellY != prevY)
    {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

static void
HandleLeftMouseDown(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    uint32_t cellX, cellY;
    app->isLeftMouseDown = true;

    SetCapture(hWnd);
    UpdateFaceHot(app, hWnd, x, y);

    if (TryGetCellFromPoint(app, x, y, &cellX, &cellY))
    {
        app->hoverCellX = cellX;
        app->hoverCellY = cellY;
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
    if (app->isLeftMouseDown)
    {
        uint32_t cellX, cellY;
        app->isLeftMouseDown = false;

        ReleaseCapture();

        if (IsPointInFace(app, x, y))
        {
            app->isFaceHot = false;
            StartNewGame(app, hWnd, app->minefield.difficulty);
        }
        else if (TryGetCellFromPoint(app, x, y, &cellX, &cellY))
        {
            RevealCell(&app->minefield, cellX, cellY);
        }
    }

    app->hoverCellX = (uint32_t)-1;
    app->hoverCellY = (uint32_t)-1;
    app->isFaceHot = false;

    InvalidateRect(hWnd, NULL, FALSE);
}

static void
HandleRightMouseClick(_In_ Application* app, _In_ HWND hWnd, _In_ int32_t x, _In_ int32_t y)
{
    uint32_t cellX, cellY;

    if (TryGetCellFromPoint(app, x, y, &cellX, &cellY) && ToggleFlag(&app->minefield, cellX, cellY))
    {
        InvalidateRect(hWnd, NULL, FALSE);
    }
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
            ApplyDwmWindowAttributes(hWnd);

            if (app != NULL)
            {
                UpdateLayoutMetricsForWindow(app, hWnd);
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
                RenderGameWindow(app, hdc);
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
                UpdateLayoutMetricsForWindow(app, hWnd);
                ResizeWindowForMinefield(app, hWnd);
            }

            InvalidateRect(hWnd, NULL, FALSE);

            return 0;
        }
        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
        {
            ApplyDwmWindowAttributes(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);

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
        .hbrBackground = GetStockObject(LTGRAY_BRUSH),
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
