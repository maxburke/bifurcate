#pragma warning(push, 0)
#define NOMINMAX
#define D3D_DEBUG_INFO
#include <d3d9.h>
#pragma warning(pop)

#include "Config.h"
#include "Core.h"
#include "Gfx.h"
#include "Mesh.h"

namespace bg
{
#if B_WIN32
    static TCHAR WINDOW_CLASS[] = TEXT("WindowClass");
    static TCHAR WINDOW_NAME[] = TEXT("Magnificent Mike and the Scourge of Dr. Pengoblin");
    static HWND gWindowHandle;
    static LPDIRECT3D9 gD3d;
    static LPDIRECT3DDEVICE9 gDevice;
    static LPDIRECT3DVERTEXDECLARATION9 gSkinnedMeshVertexDeclaration;
    static Mat4x4 gWorldViewProjection = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

#define SAFE_RELEASE(x) if (x) x->Release();

    void AlertGetLastMessage(void)
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

    static LRESULT CALLBACK WindowCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
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

    static int RegisterWindowClass(HINSTANCE instance)
    {
        WNDCLASS windowClass;
        ATOM rv;

        ZeroMemory(&windowClass, sizeof windowClass);
        windowClass.lpfnWndProc = WindowCallback;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = WINDOW_CLASS;
        windowClass.hCursor = LoadIcon(instance, IDC_ARROW);
        rv = RegisterClass(&windowClass);

        return rv == 0;
    }

    static int CreateGameWindow(HINSTANCE instance, int width, int height)
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
            AlertGetLastMessage();
            return 0;
        }
        else
        {
            ShowWindow(gWindowHandle, SW_SHOWDEFAULT);
            UpdateWindow(gWindowHandle);
        }

        return gWindowHandle == NULL;
    }

    static int InitializeDevice(HWND window_handle, int width, int height)
    {
        gD3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (gD3d == NULL)
            SignalErrorAndReturn(1, "Unable to create D3D.");

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
            SignalErrorAndReturn(1, "Unable to create D3D device.");

        return 0;
    }

    static bool CreateSkinnedMeshVertexDeclaration()
    {
        D3DVERTEXELEMENT9 vertexElements[] = {
            { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            { 0, 8, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
            D3DDECL_END()
        };

        if (FAILED(gDevice->CreateVertexDeclaration(vertexElements, &gSkinnedMeshVertexDeclaration)))
            SignalErrorAndReturn(false, "Unable to create vertex declaration");
        return true;
    }

    IDirect3DVertexShader9 *gSkinnedVertexShader;
    IDirect3DPixelShader9 *gRedPixelShader;
    static bool CreateSimpleSkinnedShaders()
    {
        using namespace bc;
        AutoMemMap fsh("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.fsh.o");
        AutoMemMap vsh("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.vsh.o");

        if (!fsh.Valid() || !vsh.Valid())
            SignalErrorAndReturn(false, "Unable to map fragment and/or vertex shader assets.");

        if (FAILED(gDevice->CreateVertexShader(static_cast<const DWORD *>(vsh.Mem()), &gSkinnedVertexShader)))
            SignalErrorAndReturn(false, "Unable to create vertex shader.");

        if (FAILED(gDevice->CreatePixelShader(static_cast<const DWORD *>(fsh.Mem()), &gRedPixelShader)))
            SignalErrorAndReturn(false, "Unable to create vertex shader.");

        return true;
    }

    int GfxInitialize(void *instance, int width, int height)
    {
        HINSTANCE hinstance = static_cast<HINSTANCE>(instance);
        if (RegisterWindowClass(hinstance)
            || CreateGameWindow(hinstance, width, height)
            || InitializeDevice(gWindowHandle, width, height))
            return 1;

        if (!CreateSkinnedMeshVertexDeclaration())
            return 1;

        // TODO: fix this to use a proper asset loading system
        if (!CreateSimpleSkinnedShaders())
            return 1;

        // TODO: fix this to use a proper asset loading system.
        if (!CreateSimpleSkinnedShaders())
            return 1;

        return 0;
    }
    
    void GfxShutdown()
    {
    }

    void GfxBeginScene()
    {
        gDevice->BeginScene();
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
            D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
            D3DFMT_INDEX16,
            D3DPOOL_DEFAULT,
            &buffer,
            NULL)))
            SignalErrorAndReturn(NULL, "Unable to create index buffer.");

        void *data;
        if (FAILED(buffer->Lock(0, 0, &data, D3DLOCK_DISCARD)))
            SignalErrorAndReturn(NULL, "Unable to lock index buffer.");

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
            D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
            0,
            D3DPOOL_DEFAULT,
            &buffer,
            NULL)))
            SignalErrorAndReturn(NULL, "Unable to create vertex buffer.");

        void *data;
        if (FAILED(buffer->Lock(0, 0, &data, D3DLOCK_DISCARD)))
            SignalErrorAndReturn(NULL, "Unable to lock vertex buffer.");

        memcpy(data, vertices, bufferSize);
        buffer->Unlock();

        return reinterpret_cast<MeshVertexBuffer *>(buffer);
    }

    struct MeshWeightedPositionBuffer
    {
        int mNumPositions;
        IDirect3DTexture9 *mPositionTexture;
        IDirect3DTexture9 *mJointTexture;
    };

    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices)
    {
        MeshWeightedPositionBuffer *buffer = static_cast<MeshWeightedPositionBuffer *>(MemAlloc(bc::POOL_GFX, sizeof(*buffer)));
        IDirect3DTexture9 *positionTexture;
        IDirect3DTexture9 *jointTexture;
        UINT width = static_cast<UINT>(numPositions);

        if (FAILED(gDevice->CreateTexture(width, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &positionTexture, NULL)))
            SignalErrorAndReturn(NULL, "Unable to create position texture.");

        if (FAILED(gDevice->CreateTexture(width, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &jointTexture, NULL)))
            SignalErrorAndReturn(NULL, "Unable to create joint texture.");

        D3DLOCKED_RECT rect;
        if (FAILED(positionTexture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD)))
            SignalErrorAndReturn(NULL, "Unable to lock position texture.");
        memcpy(rect.pBits, weightedPositions, numPositions * sizeof(*weightedPositions));
        positionTexture->UnlockRect(0);

        if (FAILED(jointTexture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD)))
            SignalErrorAndReturn(NULL, "Unable to lock joint texture.");
        memcpy(rect.pBits, jointIndices, numPositions * sizeof(*jointIndices));
        jointTexture->UnlockRect(0);

        buffer->mPositionTexture = positionTexture;
        buffer->mJointTexture = jointTexture;
        buffer->mNumPositions = numPositions;
        return buffer;
    }

    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const SoaQuatPos *poseData)
    {
        int numMeshes = meshData->mNumMeshes;

        IndexBuffer **indexBuffers = meshData->mIndexBuffers;
        MeshVertexBuffer **vertexBuffers = meshData->mVertexBuffers;
        MeshWeightedPositionBuffer **pxBuffers = meshData->mWeightedPositionBuffers;
        int *numTris = meshData->mNumTris;

        Mat4x3 *matrices = static_cast<Mat4x3 *>(AllocaAligned(16, sizeof(Mat4x3) * poseData->mNumElements));
        poseData->ConvertToMat4x3(matrices);

        gDevice->SetVertexDeclaration(gSkinnedMeshVertexDeclaration);
        gDevice->SetVertexShader(gSkinnedVertexShader);
        gDevice->SetVertexShaderConstantF(0, gWorldViewProjection.v, 4);
        // Set vertex shader here

        for (int i = 0; i < numMeshes; ++i)
        {
            // Engine idea:
            // Instead of actually doing the draw call here, add to a batch which
            // can later be sorted and then "flushed" after everything has been
            // processed.

            gDevice->SetIndices(reinterpret_cast<IDirect3DIndexBuffer9 *>(indexBuffers[i]));
            gDevice->SetStreamSource(0, 
                reinterpret_cast<IDirect3DVertexBuffer9 *>(vertexBuffers[i]),
                0,
                sizeof(MeshVertex));

            gDevice->SetTexture(0, pxBuffers[i]->mPositionTexture);
            gDevice->SetTexture(1, pxBuffers[i]->mJointTexture);

            const int numPositions = pxBuffers[i]->mNumPositions;
            int positionVector[4] = { numPositions, numPositions, numPositions, numPositions };
            gDevice->SetVertexShaderConstantI(0, positionVector, 1);

            // set material/pixel shader here
            gDevice->SetPixelShader(gRedPixelShader);
            int tris = numTris[i];
            int verts = tris * 3;
            gDevice->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                0,
                0,
                static_cast<UINT>(verts),
                0,
                static_cast<UINT>(tris));
        }
    }

#endif
}
