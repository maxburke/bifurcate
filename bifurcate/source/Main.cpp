#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "Config.h"
#include "Gfx.h"
#include "Anim.h"

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdline, int showCmd)
{
    UNUSED(instance);
    UNUSED(prevInstance);
    UNUSED(cmdline);
    UNUSED(showCmd);

    if (bg::gfx_initialize(instance, 1280, 720))
        return 1;

    //bx::load_map("z:/doom3data/maps/game/recycling1.proc");
    bg::load_anim("z:/doom3data/models/md5/monsters/hellknight/walk7.md5anim");

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        bg::gfx_begin_scene();
        bg::gfx_end_scene();
    }

    bg::gfx_shutdown();

    return 0;
}
