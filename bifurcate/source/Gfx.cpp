#pragma warning(push, 0)
#define NOMINMAX
#include <d3d9.h>
#pragma warning(pop)

#include "Config.h"
#include "Gfx.h"

namespace bg
{
    struct draw_element_buffer
    {
    };

    draw_element_buffer *deb_create(int, int)
    {
        return NULL;
    }

    void deb_set_indices(draw_element_buffer *deb, unsigned short *indices)
    {
        UNUSED(indices);
        UNUSED(deb);
    }

    void deb_set_vertices(draw_element_buffer *deb, vertex *vertices)
    {
        UNUSED(deb);
        UNUSED(vertices);
    }

#if B_WIN32
    static TCHAR WINDOW_CLASS[] = TEXT("WindowClass");
    static TCHAR WINDOW_NAME[] = TEXT("Magnificent Mike and the Scourge of Dr. Pengoblin");
    static HWND g_window_handle;
    static LPDIRECT3D9 g_d3d;
    static LPDIRECT3DDEVICE9 g_device;

#define SAFE_RELEASE(x) if (x) x->Release();

    void alert_get_last_message(void)
    {
        LPTSTR buffer;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            GetLastError(),
            0,
            (LPTSTR)&buffer,
            1024,
            NULL);
    
        __debugbreak();
    }

    static LRESULT CALLBACK window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch(msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(window, msg, wparam, lparam);
        }

        return 0;
    }

    static int register_window_class(HINSTANCE instance)
    {
        WNDCLASS window_class;
        ATOM rv;

        ZeroMemory(&window_class, sizeof window_class);
        window_class.lpfnWndProc = window_callback;
        window_class.hInstance = instance;
        window_class.lpszClassName = WINDOW_CLASS;
        window_class.hCursor = LoadIcon(instance, IDC_ARROW);
        rv = RegisterClass(&window_class);

        return rv == 0;
    }

    static int create_window(HINSTANCE instance, int width, int height)
    {
        g_window_handle = CreateWindow(
            WINDOW_CLASS,
            WINDOW_NAME,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width,
            height,
            NULL,
            NULL,
            instance,
            NULL);

        if (g_window_handle == NULL)
        {
            alert_get_last_message();
            return 0;
        }
        else
        {
            ShowWindow(g_window_handle, SW_SHOWDEFAULT);
            UpdateWindow(g_window_handle);
        }

        return g_window_handle == NULL;
    }

    static int initialize_device(HWND window_handle, int width, int height)
    {
        g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (g_d3d == NULL)
            return 1;

        D3DPRESENT_PARAMETERS presentation_parameters;
        ZeroMemory(&presentation_parameters, sizeof presentation_parameters);
        presentation_parameters.BackBufferWidth = (UINT)width;
        presentation_parameters.BackBufferHeight = (UINT)height;
        presentation_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
        presentation_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentation_parameters.hDeviceWindow = window_handle;
        presentation_parameters.Windowed = TRUE;
        presentation_parameters.EnableAutoDepthStencil = TRUE;
        presentation_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
        presentation_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

        if (FAILED(g_d3d->CreateDevice(
            D3DADAPTER_DEFAULT, 
            D3DDEVTYPE_HAL,
            window_handle,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &presentation_parameters,
            &g_device)))
            return 1;

        return 0;
    }

    int gfx_initialize(void *instance, int width, int height)
    {
        HINSTANCE hinstance = static_cast<HINSTANCE>(instance);
        if (register_window_class(hinstance)
            || create_window(hinstance, width, height)
            || initialize_device(g_window_handle, width, height))
            return 1;

        return 0;
    }
    
    void gfx_shutdown()
    {
    }

    void gfx_begin_scene()
    {
        g_device->Clear(
            0,
            NULL,
            D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            D3DCOLOR_RGBA(0, 0, 0, 0),
            1.0f,
            0);
    }

    void gfx_end_scene()
    {
        g_device->Present(NULL, NULL, NULL, NULL);
    }
#endif
}