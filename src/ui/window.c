#include "pch.h"

#include "application.h"
#include "game.h"
#include "resource.h"
#include "ui/render.h"
#include "ui/window.h"

#define TICK_TIMER_ID 1

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

    if (app->metrics.cellWidth == 0u || app->metrics.cellHeight == 0u)
        return false;

    int32_t cellWidth = (int32_t)app->metrics.cellWidth;
    int32_t cellHeight = (int32_t)app->metrics.cellHeight;
    uint32_t cellX = (uint32_t)(relX / cellWidth);
    uint32_t cellY = (uint32_t)(relY / cellHeight);

    if (cellX >= app->minefield.width || cellY >= app->minefield.height)
        return false;

    *outX = cellX;
    *outY = cellY;

    return true;
}

static void
GetFaceRect(_In_ const Application* app, _Out_ LPRECT lpRect)
{
    int32_t contentLeft = (int32_t)app->metrics.borderWidth;
    int32_t contentTop = (int32_t)app->metrics.borderHeight;
    int32_t boardWidth = (int32_t)(app->minefield.width * app->metrics.cellWidth);
    int32_t faceWidth = (int32_t)app->metrics.faceWidth;
    int32_t faceHeight = (int32_t)app->metrics.faceHeight;

    int32_t faceLeft = contentLeft + (boardWidth - faceWidth) / 2;
    int32_t faceTop = contentTop + ((int32_t)app->metrics.counterAreaHeight - faceHeight) / 2;

    lpRect->left = faceLeft;
    lpRect->top = faceTop;
    lpRect->right = faceLeft + faceWidth;
    lpRect->bottom = faceTop + faceHeight;
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
    BOOL useDark = IsDarkModeEnabled();
    (void)DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    (void)DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    DWORD backdrop = DWMSBT_MAINWINDOW;
    (void)DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
}

static uint32_t
ScaleValue(_In_ uint32_t value, _In_ uint32_t numerator, _In_ uint32_t denominator)
{
    if (value == 0u || denominator == 0u)
        return value;

    int scaled = MulDiv((int)value, (int)numerator, (int)denominator);

    return (uint32_t)max(1, scaled);
}

static SIZE
GetMinefieldClientSize(_In_ const Application* app, _In_ const LayoutMetrics* metrics)
{
    uint64_t boardWidth = (uint64_t)app->minefield.width * metrics->cellWidth;
    uint64_t boardHeight = (uint64_t)app->minefield.height * metrics->cellHeight;

    SIZE size = {
        .cx = (LONG)(boardWidth + 2u * metrics->borderWidth),
        .cy = (LONG)(boardHeight + (3u * metrics->borderHeight) + metrics->counterAreaHeight)
    };

    return size;
}

static void
GetClientSizeLimits(
    _In_ const Application* app,
    _Out_opt_ uint32_t* minWidth,
    _Out_opt_ uint32_t* minHeight)
{
    uint32_t resolvedMinWidth = max(app->minClientWidth, 1u);
    uint32_t resolvedMinHeight = max(app->minClientHeight, 1u);

    if (minWidth != NULL)
        *minWidth = resolvedMinWidth;

    if (minHeight != NULL)
        *minHeight = resolvedMinHeight;
}

static bool
GetWindowRectForClient(_In_ HWND hWnd, _In_ uint32_t clientWidth, _In_ uint32_t clientHeight, _Out_ LPRECT lpRect)
{
    if (lpRect == NULL)
        return false;

    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = (LONG)clientWidth;
    lpRect->bottom = (LONG)clientHeight;

    UINT dpi = GetDpiForWindow(hWnd);
    return AdjustWindowRectExForDpi(lpRect, GetWindowStyle(hWnd), TRUE, GetWindowExStyle(hWnd), dpi);
}

static bool
SetWindowClientSize(_In_ HWND hWnd, _In_ uint32_t width, _In_ uint32_t height)
{
    RECT windowRect;

    if (!GetWindowRectForClient(hWnd, width, height, &windowRect))
        return false;

    return SetWindowPos(
               hWnd,
               NULL,
               0,
               0,
               windowRect.right - windowRect.left,
               windowRect.bottom - windowRect.top,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void
UpdateScaledMetricsForClient(_Inout_ Application* app, _In_ uint32_t clientWidth, _In_ uint32_t clientHeight)
{
    uint32_t minWidth = 0;
    uint32_t minHeight = 0;
    const LayoutMetrics* base = &app->baseMetrics;

    GetClientSizeLimits(app, &minWidth, &minHeight);

    clientWidth = max(clientWidth, minWidth);
    clientHeight = max(clientHeight, minHeight);

    app->clientWidth = clientWidth;
    app->clientHeight = clientHeight;

    LayoutMetrics scaled = {
        .cellWidth = ScaleValue(base->cellWidth, clientWidth, minWidth),
        .cellHeight = ScaleValue(base->cellHeight, clientHeight, minHeight),
        .borderWidth = ScaleValue(base->borderWidth, clientWidth, minWidth),
        .borderHeight = ScaleValue(base->borderHeight, clientHeight, minHeight),
        .counterBorderWidth = ScaleValue(base->counterBorderWidth, clientWidth, minWidth),
        .counterAreaHeight = ScaleValue(base->counterAreaHeight, clientHeight, minHeight),
        .counterMargin = ScaleValue(base->counterMargin, clientWidth, minWidth),
        .counterHeight = ScaleValue(base->counterHeight, clientHeight, minHeight),
        .counterDigitWidth = ScaleValue(base->counterDigitWidth, clientWidth, minWidth),
        .faceWidth = ScaleValue(base->faceWidth, clientWidth, minWidth),
        .faceHeight = ScaleValue(base->faceHeight, clientHeight, minHeight),
    };

    app->metrics = scaled;
}

static void
UpdateLayoutMetricsForWindow(_Inout_ Application* app, _In_ HWND hWnd)
{
    UINT dpi = GetDpiForWindow(hWnd);
    LayoutMetrics* base = &app->baseMetrics;

    base->cellWidth = max(1u, (uint32_t)MulDiv(CELL_SIZE, dpi, USER_DEFAULT_SCREEN_DPI));
    base->cellHeight = base->cellWidth;
    base->borderWidth = max(1u, (uint32_t)MulDiv(BORDER_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI));
    base->borderHeight = max(1u, (uint32_t)MulDiv(BORDER_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI));
    base->counterBorderWidth = max(1u, (uint32_t)MulDiv(COUNTER_BORDER_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI));
    base->counterAreaHeight = max(1u, (uint32_t)MulDiv(COUNTER_AREA_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI));
    base->counterMargin = (uint32_t)MulDiv(COUNTER_MARGIN, dpi, USER_DEFAULT_SCREEN_DPI);
    base->counterHeight = max(1u, (uint32_t)MulDiv(COUNTER_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI));
    base->counterDigitWidth = max(1u, (uint32_t)MulDiv(COUNTER_DIGIT_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI));
    base->faceWidth = max(1u, (uint32_t)MulDiv(FACE_SIZE, dpi, USER_DEFAULT_SCREEN_DPI));
    base->faceHeight = base->faceWidth;

    SIZE minClient = GetMinefieldClientSize(app, base);
    app->minClientWidth = (uint32_t)minClient.cx;
    app->minClientHeight = (uint32_t)minClient.cy;
}

_Success_(
    return) static bool AdjustWindowRectForCellSize(_In_ const Application* app, _In_ HWND hWnd, _Out_ LPRECT lpRect)
{
    UINT dpi = GetDpiForWindow(hWnd);
    SIZE minClient = GetMinefieldClientSize(app, &app->baseMetrics);

    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = (LONG)minClient.cx;
    lpRect->bottom = (LONG)minClient.cy;

    return AdjustWindowRectExForDpi(lpRect, GetWindowStyle(hWnd), TRUE, GetWindowExStyle(hWnd), dpi);
}

static bool
ResizeWindowForMinefield(_Inout_ Application* app, _In_ HWND hWnd, _In_ bool forceMinimum)
{
    SIZE minClient = GetMinefieldClientSize(app, &app->baseMetrics);
    app->minClientWidth = (uint32_t)minClient.cx;
    app->minClientHeight = (uint32_t)minClient.cy;

    RECT clientRect = {0};

    if (!GetClientRect(hWnd, &clientRect))
        ZeroMemory(&clientRect, sizeof(clientRect));

    uint32_t currentWidth = (uint32_t)(clientRect.right - clientRect.left);
    uint32_t currentHeight = (uint32_t)(clientRect.bottom - clientRect.top);

    uint32_t minWidth = 0;
    uint32_t minHeight = 0;

    GetClientSizeLimits(app, &minWidth, &minHeight);

    uint32_t targetWidth = forceMinimum ? minWidth : max(currentWidth, minWidth);
    uint32_t targetHeight = forceMinimum ? minHeight : max(currentHeight, minHeight);

    if (targetWidth != currentWidth || targetHeight != currentHeight)
    {
        if (!SetWindowClientSize(hWnd, targetWidth, targetHeight))
        {
            RECT fallbackRect;

            if (!AdjustWindowRectForCellSize(app, hWnd, &fallbackRect))
                return false;

            if (!SetWindowPos(
                    hWnd,
                    NULL,
                    0,
                    0,
                    fallbackRect.right - fallbackRect.left,
                    fallbackRect.bottom - fallbackRect.top,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE))
            {
                return false;
            }
        }

        if (!GetClientRect(hWnd, &clientRect))
            return false;

        currentWidth = (uint32_t)(clientRect.right - clientRect.left);
        currentHeight = (uint32_t)(clientRect.bottom - clientRect.top);
    }

    UpdateScaledMetricsForClient(app, currentWidth, currentHeight);
    return true;
}

static bool
InitNewGame(_Inout_ Application* app, _In_ HWND hWnd, _In_ bool forceMinimum)
{
    if (!ResizeWindowForMinefield(app, hWnd, forceMinimum))
    {
        MessageBoxW(hWnd, L"Minefield too large for display!", L"Error", MB_OK | MB_ICONWARNING);
        return false;
    }

    app->isLeftMouseDown = false;
    app->hoverCellX = (uint32_t)-1;
    app->hoverCellY = (uint32_t)-1;

    InvalidateRect(hWnd, NULL, TRUE);
    return true;
}

static void
StartNewGame(_In_ Application* app, _In_ HWND hWnd, _In_ Difficulty difficulty)
{
    uint32_t previousWidth = app->minefield.width;
    uint32_t previousHeight = app->minefield.height;
    Difficulty previousDifficulty = app->minefield.difficulty;

    if (difficulty == DIFFICULTY_CUSTOM)
    {
        uint32_t w = app->minefield.width;
        uint32_t h = app->minefield.height;
        uint32_t m = app->minefield.totalMines;

        if (!CreateCustomMinefield(&app->minefield, w, h, m))
        {
            MessageBoxW(hWnd, L"Failed to create grid. Check your settings", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
    }
    else
    {
        if (!CreateMinefield(&app->minefield, difficulty))
        {
            MessageBoxW(hWnd, L"Failed to create grid. Check your settings", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    bool sizeChanged = (app->minefield.width != previousWidth) || (app->minefield.height != previousHeight);
    bool forceResize = false;

    if (difficulty == DIFFICULTY_CUSTOM)
    {
        forceResize = sizeChanged;
    }
    else
    {
        forceResize = sizeChanged || (previousDifficulty != difficulty);
    }

    InitNewGame(app, hWnd, forceResize);
}

static void
StartNewGameCustom(
    _In_ Application* app,
    _In_ HWND hWnd,
    _In_ uint32_t width,
    _In_ uint32_t height,
    _In_ uint32_t mines)
{
    uint32_t previousWidth = app->minefield.width;
    uint32_t previousHeight = app->minefield.height;

    if (!CreateCustomMinefield(&app->minefield, width, height, mines))
    {
        MessageBoxW(hWnd, L"Failed to create grid. Check your settings", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    bool sizeChanged = (app->minefield.width != previousWidth) || (app->minefield.height != previousHeight);

    InitNewGame(app, hWnd, sizeChanged);
}

static INT_PTR CALLBACK
CustomGameDlgProc(_In_ HWND hDlg, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            Application* app = (Application*)lParam;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)app);

            if (app != NULL)
            {
                SetDlgItemInt(hDlg, IDC_CUSTOM_WIDTH, app->minefield.width, FALSE);
                SetDlgItemInt(hDlg, IDC_CUSTOM_HEIGHT, app->minefield.height, FALSE);
                SetDlgItemInt(hDlg, IDC_CUSTOM_MINES, app->minefield.totalMines, FALSE);
            }
            else
            {
                SetDlgItemInt(hDlg, IDC_CUSTOM_WIDTH, 9, FALSE);
                SetDlgItemInt(hDlg, IDC_CUSTOM_HEIGHT, 9, FALSE);
                SetDlgItemInt(hDlg, IDC_CUSTOM_MINES, 10, FALSE);
            }

            return TRUE;
        }
        case WM_COMMAND:
        {
            Application* app = (Application*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            HWND hParent = GetParent(hDlg);

            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    BOOL ok1 = FALSE, ok2 = FALSE, ok3 = FALSE;
                    UINT w = GetDlgItemInt(hDlg, IDC_CUSTOM_WIDTH, &ok1, FALSE);
                    UINT h = GetDlgItemInt(hDlg, IDC_CUSTOM_HEIGHT, &ok2, FALSE);
                    UINT m = GetDlgItemInt(hDlg, IDC_CUSTOM_MINES, &ok3, FALSE);

                    if (!ok1 || !ok2 || !ok3)
                    {
                        MessageBoxW(hDlg, L"Please enter valid numbers.", L"Invalid Input", MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    w = max(1u, min((UINT)MAX_CELLS_HORIZONTALLY, w));
                    h = max(1u, min((UINT)MAX_CELLS_VERTICALLY, h));

                    UINT maxMines = w * h - 1;
                    if (m > maxMines)
                        m = maxMines;
                    if (m < 1)
                        m = 1;

                    if (app && hParent)
                    {
                        StartNewGameCustom(app, hParent, (uint32_t)w, (uint32_t)h, (uint32_t)m);
                    }

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }

            break;
        }
    }

    return FALSE;
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
        case WM_GETMINMAXINFO:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (app != NULL && app->minClientWidth > 0 && app->minClientHeight > 0)
            {
                uint32_t minWidth = 0;
                uint32_t minHeight = 0;

                GetClientSizeLimits(app, &minWidth, &minHeight);

                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                RECT trackRect = {0};

                if (GetWindowRectForClient(hWnd, minWidth, minHeight, &trackRect))
                {
                    mmi->ptMinTrackSize.x = trackRect.right - trackRect.left;
                    mmi->ptMinTrackSize.y = trackRect.bottom - trackRect.top;
                }

                return 0;
            }

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

            SetTimer(hWnd, TICK_TIMER_ID, 1000, NULL);

            return 0;
        }
        case WM_DESTROY:
        {
            KillTimer(hWnd, TICK_TIMER_ID);
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
        case WM_TIMER:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            switch (wParam)
            {
                case TICK_TIMER_ID:
                {
                    if (app != NULL)
                    {
                        if (app->minefield.state == GAME_PLAYING && !app->minefield.firstClick)
                        {
                            InvalidateRect(hWnd, NULL, FALSE);
                        }
                    }

                    return 0;
                }
            }

            break;
        }
        case WM_SIZE:
        {
            Application* app = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (wParam != SIZE_MINIMIZED)
            {
                if (app != NULL)
                {
                    uint32_t width = LOWORD(lParam);
                    uint32_t height = HIWORD(lParam);
                    UpdateScaledMetricsForClient(app, width, height);
                }
            }

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
                {
                    DialogBoxParamW(
                        GetWindowInstance(hWnd),
                        MAKEINTRESOURCEW(IDD_CUSTOM_GAME),
                        hWnd,
                        CustomGameDlgProc,
                        (LPARAM)app);
                    break;
                }
                case IDM_GAME_EXIT:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
                case IDM_HELP_ABOUT:
                {
                    wchar_t about[512];

                    swprintf(
                        about,
                        ARRAYSIZE(about),
                        L"Minesweeper\n"
                        L"Version 1.0\n"
                        L"Built %S %S\n\n"
                        L"GitHub: https://github.com/GustasG/Minesweeper\n"
                        L"License: MIT\n\n"
                        L"Controls:\n"
                        L"- Left-click: Reveal\n"
                        L"- Right-click: Flag/Unflag\n"
                        L"- F2: New Game\n\n",
                        __DATE__,
                        __TIME__);

                    MessageBoxW(hWnd, about, L"About Minesweeper", MB_OK | MB_ICONINFORMATION);
                    break;
                }
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

                if (ResizeWindowForMinefield(app, hWnd, false))
                    InvalidateRect(hWnd, NULL, FALSE);
            }

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
        .style = CS_HREDRAW | CS_VREDRAW,
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
        WS_OVERLAPPEDWINDOW,
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
