#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "Config.h"
#include "Gfx.h"
#include "Anim.h"
#include "Mesh.h"

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdline, int showCmd)
{
    UNUSED(instance);
    UNUSED(prevInstance);
    UNUSED(cmdline);
    UNUSED(showCmd);

    if (bg::GfxInitialize(instance, 1280, 720))
        return 1;

    //bx::load_map("z:/doom3data/maps/game/recycling1.proc");
    bg::LoadAnim("z:/doom3data/models/md5/monsters/hellknight/walk7.md5anim");
    bg::LoadMesh("z:/doom3data/models/md5/monsters/hellknight/hellknight.md5mesh");

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        bg::GfxBeginScene();
        bg::GfxEndScene();
    }

    bg::GfxShutdown();

    return 0;
}
