#include "pch.h"

#include "application.h"
#include "resource.h"

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
UnloadFaceResources(_In_ FaceResources* resources)
{
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
LoadFaceResources(_In_ FaceResources* resources, _In_ HINSTANCE hInstance)
{
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
