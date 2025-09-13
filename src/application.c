#include "pch.h"

#include "application.h"
#include "resource.h"
#include "ui/window.h"

static void
UnloadCellResources(_In_ CellResources* resources)
{
    for (size_t i = 0; i < ARRAYSIZE(resources->cells); i++)
    {
        if (resources->cells[i] != NULL)
            DeleteObject(resources->cells[i]);
    }

    if (resources->blast != NULL)
        DeleteObject(resources->blast);

    if (resources->down != NULL)
        DeleteObject(resources->down);

    if (resources->flag != NULL)
        DeleteObject(resources->flag);

    if (resources->mine != NULL)
        DeleteObject(resources->mine);

    if (resources->up != NULL)
        DeleteObject(resources->up);

    if (resources->falseMine != NULL)
        DeleteObject(resources->falseMine);
}

static void
UnloadBorderResources(_In_ BorderResources* resources)
{
    if (resources->bottom != NULL)
        DeleteObject(resources->bottom);

    if (resources->bottomLeft != NULL)
        DeleteObject(resources->bottomLeft);

    if (resources->bottomRight != NULL)
        DeleteObject(resources->bottomRight);

    if (resources->counterLeft != NULL)
        DeleteObject(resources->counterLeft);

    if (resources->counterMiddle != NULL)
        DeleteObject(resources->counterMiddle);

    if (resources->counterRight != NULL)
        DeleteObject(resources->counterRight);

    if (resources->left != NULL)
        DeleteObject(resources->left);

    if (resources->middleLeft != NULL)
        DeleteObject(resources->middleLeft);

    if (resources->middleRight != NULL)
        DeleteObject(resources->middleRight);

    if (resources->right != NULL)
        DeleteObject(resources->right);

    if (resources->top != NULL)
        DeleteObject(resources->top);

    if (resources->topLeft != NULL)
        DeleteObject(resources->topLeft);

    if (resources->topRight != NULL)
        DeleteObject(resources->topRight);
}

static void
UnloadFaceResources(_In_ FaceResources* resources)
{
    if (resources->click != NULL)
        DeleteObject(resources->click);

    if (resources->lost != NULL)
        DeleteObject(resources->lost);

    if (resources->smile != NULL)
        DeleteObject(resources->smile);

    if (resources->smileFaceDown != NULL)
        DeleteObject(resources->smileFaceDown);

    if (resources->win != NULL)
        DeleteObject(resources->win);
}

static void
UnloadAssets(_In_ Application* app)
{
    UnloadCellResources(&app->cellResources);
    UnloadBorderResources(&app->borderResources);
    UnloadFaceResources(&app->faceResources);
}

static bool
LoadCellResources(_In_ CellResources* resources, _In_ HINSTANCE hInstance)
{
    static const int bitmapNames[] = {
        IDB_CELL1,
        IDB_CELL2,
        IDB_CELL3,
        IDB_CELL4,
        IDB_CELL5,
        IDB_CELL6,
        IDB_CELL7,
        IDB_CELL8,
    };

    for (size_t i = 0; i < ARRAYSIZE(bitmapNames); i++)
    {
        if ((resources->cells[i] = LoadBitmapW(hInstance, MAKEINTRESOURCE(bitmapNames[i]))) == NULL)
            return false;
    }

    if ((resources->blast = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BLAST))) == NULL)
        return false;

    if ((resources->down = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_CELLDOWN))) == NULL)
        return false;

    if ((resources->flag = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_CELLFLAG))) == NULL)
        return false;

    if ((resources->mine = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_CELLMINE))) == NULL)
        return false;

    if ((resources->up = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_CELLUP))) == NULL)
        return false;

    if ((resources->falseMine = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_FALSEMINE))) == NULL)
        return false;

    return true;
}

static bool
LoadBorderResources(_In_ BorderResources* resources, _In_ HINSTANCE hInstance)
{
    if ((resources->bottom = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_BOTTOM))) == NULL)
        return false;

    if ((resources->bottomLeft = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_BOTTOM_LEFT))) == NULL)
        return false;

    if ((resources->bottomRight = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_BOTTOM_RIGHT))) == NULL)
        return false;

    if ((resources->counterLeft = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_COUNTER_LEFT))) == NULL)
        return false;

    if ((resources->counterMiddle = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_COUNTER_MIDDLE))) == NULL)
        return false;

    if ((resources->counterRight = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_COUNTER_RIGHT))) == NULL)
        return false;

    if ((resources->left = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_LEFT))) == NULL)
        return false;

    if ((resources->middleLeft = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_MIDDLE_LEFT))) == NULL)
        return false;

    if ((resources->middleRight = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_MIDDLE_RIGHT))) == NULL)
        return false;

    if ((resources->right = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_RIGHT))) == NULL)
        return false;

    if ((resources->top = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_TOP))) == NULL)
        return false;

    if ((resources->topLeft = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_TOP_LEFT))) == NULL)
        return false;

    if ((resources->topRight = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_BORDER_TOP_RIGHT))) == NULL)
        return false;

    return true;
}

static bool
LoadFaceResources(_In_ FaceResources* resources, _In_ HINSTANCE hInstance)
{
    if ((resources->click = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_CLICK_FACE))) == NULL)
        return false;

    if ((resources->lost = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_LOST_FACE))) == NULL)
        return false;

    if ((resources->smile = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_SMILE_FACE))) == NULL)
        return false;

    if ((resources->smileFaceDown = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_SMILE_FACE_DOWN))) == NULL)
        return false;

    if ((resources->win = LoadBitmapW(hInstance, MAKEINTRESOURCE(IDB_WIN_FACE))) == NULL)
        return false;

    return true;
}

static bool
LoadAssets(_In_ Application* app, _In_ HINSTANCE hInstance)
{
    if (!LoadCellResources(&app->cellResources, hInstance))
        goto fail;

    if (!LoadBorderResources(&app->borderResources, hInstance))
        goto fail;

    if (!LoadFaceResources(&app->faceResources, hInstance))
        goto fail;

    return true;

fail:
    UnloadAssets(app);

    return false;
}

_Ret_maybenull_ Application*
CreateApplication(_In_ HINSTANCE hInstance)
{
    HANDLE hHeap = GetProcessHeap();
    Application* app = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(Application));

    if (app == NULL)
        return NULL;

    if (!LoadAssets(app, hInstance))
    {
        HeapFree(hHeap, 0, app);
        return NULL;
    }

    app->metrics.cellSize = (uint32_t)CELL_SIZE;
    app->metrics.borderWidth = (uint32_t)BORDER_WIDTH;
    app->metrics.borderHeight = (uint32_t)BORDER_HEIGHT;
    app->metrics.counterBorderWidth = (uint32_t)COUNTER_BORDER_WIDTH;
    app->metrics.counterAreaHeight = (uint32_t)COUNTER_AREA_HEIGHT;
    app->metrics.counterMargin = (uint32_t)COUNTER_MARGIN;
    app->metrics.counterHeight = (uint32_t)COUNTER_HEIGHT;
    app->metrics.counterDigitWidth = (uint32_t)COUNTER_DIGIT_WIDTH;
    app->metrics.faceSize = (uint32_t)FACE_SIZE;

    app->hoverCellX = (uint32_t)-1;
    app->hoverCellY = (uint32_t)-1;
    app->isFaceHot = false;

    return app;
}

void
DestroyApplication(_In_opt_ Application* app)
{
    if (app == NULL)
        return;

    UnloadAssets(app);
    HeapFree(GetProcessHeap(), 0, app);
}
