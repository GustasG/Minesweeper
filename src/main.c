#include "pch.h"

#include "resource.h"
#include "ui/window.h"

static void
ShowWindowCreationError(void)
{
    DWORD error = GetLastError();
    wchar_t errorMsg[1024];

    if (error == 0)
    {
        wcscpy_s(
            errorMsg,
            ARRAYSIZE(errorMsg),
            L"Failed to start Minesweeper - unknown error occurred during window creation!");
    }
    else
    {
        wchar_t* errorText = NULL;
        DWORD result = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&errorText,
            0,
            NULL);

        if (result > 0 && errorText != NULL)
        {
            swprintf_s(
                errorMsg,
                ARRAYSIZE(errorMsg),
                L"Failed to start Minesweeper!\n\nError %lu: %s",
                error,
                errorText);
            LocalFree(errorText);
        }
        else
        {
            swprintf_s(errorMsg, ARRAYSIZE(errorMsg), L"Failed to start Minesweeper!\n\nError: %lu", error);
        }
    }

    MessageBoxW(NULL, errorMsg, L"Application startup error", MB_OK | MB_ICONERROR);
}

int APIENTRY
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);

    HWND hWnd = CreateMainWindow(hInstance);

    if (hWnd == NULL)
    {
        ShowWindowCreationError();
        return 1;
    }

    HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}
