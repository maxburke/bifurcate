#pragma warning(push, 0)
#define NOMINMAX
#include <d3d9.h>
#pragma warning(pop)

#include "Config.h"
#include "Core.h"
#include "Gfx.h"

namespace bg
{
#if B_WIN32
    static TCHAR WINDOW_CLASS[] = TEXT("WindowClass");
    static TCHAR WINDOW_NAME[] = TEXT("Magnificent Mike and the Scourge of Dr. Pengoblin");
    static HWND gWindowHandle;
    static LPDIRECT3D9 gD3d;
    static LPDIRECT3DDEVICE9 gDevice;

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

    static int register_windowClass(HINSTANCE instance)
    {
        WNDCLASS windowClass;
        ATOM rv;

        ZeroMemory(&windowClass, sizeof windowClass);
        windowClass.lpfnWndProc = window_callback;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = WINDOW_CLASS;
        windowClass.hCursor = LoadIcon(instance, IDC_ARROW);
        rv = RegisterClass(&windowClass);

        return rv == 0;
    }

    static int create_window(HINSTANCE instance, int width, int height)
    {
        gWindowHandle = CreateWindow(
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

        if (gWindowHandle == NULL)
        {
            alert_get_last_message();
            return 0;
        }
        else
        {
            ShowWindow(gWindowHandle, SW_SHOWDEFAULT);
            UpdateWindow(gWindowHandle);
        }

        return gWindowHandle == NULL;
    }

    static int initialize_device(HWND window_handle, int width, int height)
    {
        gD3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (gD3d == NULL)
            return 1;

        D3DPRESENT_PARAMETERS presentationParameters;
        ZeroMemory(&presentationParameters, sizeof presentationParameters);
        presentationParameters.BackBufferWidth = (UINT)width;
        presentationParameters.BackBufferHeight = (UINT)height;
        presentationParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
        presentationParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentationParameters.hDeviceWindow = window_handle;
        presentationParameters.Windowed = TRUE;
        presentationParameters.EnableAutoDepthStencil = TRUE;
        presentationParameters.AutoDepthStencilFormat = D3DFMT_D24S8;
        presentationParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

        if (FAILED(gD3d->CreateDevice(
            D3DADAPTER_DEFAULT, 
            D3DDEVTYPE_HAL,
            window_handle,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &presentationParameters,
            &gDevice)))
            return 1;

        return 0;
    }

    int GfxInitialize(void *instance, int width, int height)
    {
        HINSTANCE hinstance = static_cast<HINSTANCE>(instance);
        if (register_windowClass(hinstance)
            || create_window(hinstance, width, height)
            || initialize_device(gWindowHandle, width, height))
            return 1;

        return 0;
    }
    
    void GfxShutdown()
    {
    }

    void GfxBeginScene()
    {
        gDevice->Clear(
            0,
            NULL,
            D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            D3DCOLOR_RGBA(0, 0, 0, 0),
            1.0f,
            0);
    }

    void GfxEndScene()
    {
        gDevice->Present(NULL, NULL, NULL, NULL);
    }

    IndexBuffer *IndexBufferCreate(int numIndices, unsigned short *indices)
    {
        IDirect3DIndexBuffer9 *buffer;
        const size_t bufferSize = numIndices * sizeof(*indices);
        if (FAILED(gDevice->CreateIndexBuffer(
            bufferSize,
            0,
            D3DFMT_INDEX16,
            D3DPOOL_DEFAULT,
            &buffer,
            NULL)))
            return NULL;

        void *data;
        if (FAILED(buffer->Lock(0, 0, &data, D3DLOCK_DISCARD)))
            return NULL;

        memcpy(data, indices, bufferSize);
        buffer->Unlock();

        return reinterpret_cast<IndexBuffer *>(buffer);
    }

    MeshVertexBuffer *MeshVertexBufferCreate(int num_vertices, MeshVertex *vertices)
    {
        IDirect3DVertexBuffer9 *buffer;

        const size_t bufferSize = num_vertices * sizeof(*vertices);
        if (FAILED(gDevice->CreateVertexBuffer(
            bufferSize,
            0,
            0,
            D3DPOOL_DEFAULT,
            &buffer,
            NULL)))
            return NULL;

        void *data;
        if (FAILED(buffer->Lock(0, 0, &data, D3DLOCK_DISCARD)))
            return NULL;

        memcpy(data, vertices, bufferSize);
        buffer->Unlock();

        return reinterpret_cast<MeshVertexBuffer *>(buffer);
    }

    struct MeshWeightedPositionBuffer
    {
        IDirect3DTexture9 *mPositionTexture;
        IDirect3DTexture9 *mJointTexture;
    };

    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices)
    {
        MeshWeightedPositionBuffer *buffer = static_cast<MeshWeightedPositionBuffer *>(MemAlloc(bc::POOL_GFX, sizeof(*buffer)));
        UINT width = static_cast<UINT>(numPositions);

        if (FAILED(gDevice->CreateTexture(width, 1, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &buffer->mPositionTexture, NULL)))
            return NULL;

        if (FAILED(gDevice->CreateTexture(width, 1, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT, &buffer->mJointTexture, NULL)))
            return NULL;

        UNUSED(jointIndices);
        UNUSED(weightedPositions);
        return buffer;
    }

#endif
}
