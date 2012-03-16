#pragma warning(push, 0)
#define NOMINMAX
#define D3D_DEBUG_INFO
#include <d3d11.h>
#pragma warning(pop)

#include "Config.h"
#include "Core.h"
#include "Gfx.h"
#include "Mesh.h"
#include <math.h>

namespace bg
{
    static TCHAR WINDOW_CLASS[] = TEXT("WindowClass");
    static TCHAR WINDOW_NAME[] = TEXT("Magnificent Mike and the Scourge of Dr. Pengoblin");
    static HWND gWindowHandle;

    static IDXGISwapChain *gSwapChain;
    static ID3D11Device *gDevice;
    static ID3D11DeviceContext *gContext;
    static ID3D11RenderTargetView *gBackBufferView;
    static ID3D11Texture2D *gDepthStencil;
    static ID3D11DepthStencilView *gDepthStencilView;

    static ID3D11Buffer *gViewProjConstants;
    static ID3D11Buffer *gSkinningMatrices;
    static ID3D11InputLayout *gSkinnedMeshInputLayout;
    static ID3D11VertexShader *gSkinnedVertexShader;
    static ID3D11PixelShader *gDebugMaterial;

    static Mat4x4 gProjMatrix;

    #define SAFE_RELEASE(x) if (x) { (x)->Release(); } else (void)0

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

    static bool RegisterWindowClass(HINSTANCE instance)
    {
        WNDCLASS windowClass;
        ATOM rv;

        ZeroMemory(&windowClass, sizeof windowClass);
        windowClass.lpfnWndProc = WindowCallback;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = WINDOW_CLASS;
        windowClass.hCursor = LoadIcon(instance, IDC_ARROW);
        rv = RegisterClass(&windowClass);

        return rv != 0;
    }

    static bool CreateGameWindow(HINSTANCE instance, int width, int height)
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
            LPTSTR buffer;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                NULL,
                GetLastError(),
                0,
                (LPTSTR)&buffer,
                1024,
                NULL);
            SignalErrorAndReturn(false, buffer);
        }
        else
        {
            ShowWindow(gWindowHandle, SW_SHOWDEFAULT);
            UpdateWindow(gWindowHandle);
        }

        return gWindowHandle != NULL;
    }

    static void InitializeSwapChainDesc(DXGI_SWAP_CHAIN_DESC *desc, HWND window, int width, int height)
    {
        ZeroMemory(desc, sizeof *desc);
        desc->BufferDesc.Width = static_cast<UINT>(width);
        desc->BufferDesc.Height = static_cast<UINT>(height);
        desc->BufferDesc.RefreshRate.Numerator = 60;
        desc->BufferDesc.RefreshRate.Denominator = 1;
        desc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc->BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc->BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        desc->SampleDesc.Count = 1;
        desc->SampleDesc.Quality = 0;
        desc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc->BufferCount = 2;
        desc->OutputWindow = window;
        desc->Windowed = TRUE;
        desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    static bool InitializeDevice(HWND window, int width, int height)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        InitializeSwapChainDesc(&swapChainDesc, window, width, height);

        D3D_FEATURE_LEVEL featureLevel;
        if (FAILED(D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            0,
            NULL,
            0,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &gSwapChain,
            &gDevice,
            &featureLevel,
            &gContext)))
            SignalErrorAndReturn(false, "Unable to create device and swap chain.");

        ID3D11Texture2D *backBuffer;
        if (FAILED(gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer)))
            SignalErrorAndReturn(false, "Unable to get back buffer.");

        if (FAILED(gDevice->CreateRenderTargetView(backBuffer, NULL, &gBackBufferView)))
            SignalErrorAndReturn(false, "Unable to create back buffer render target view.");

        SAFE_RELEASE(backBuffer);

        D3D11_TEXTURE2D_DESC depthDesc;
        ZeroMemory(&depthDesc, sizeof depthDesc);
        depthDesc.Width = static_cast<UINT>(width);
        depthDesc.Height = static_cast<UINT>(height);
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (FAILED(gDevice->CreateTexture2D(&depthDesc, NULL, &gDepthStencil)))
            SignalErrorAndReturn(false, "Unable to create depth/stencil buffer.");

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
        ZeroMemory(&depthStencilViewDesc, sizeof depthStencilViewDesc);
        depthStencilViewDesc.Format = depthDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        if (FAILED(gDevice->CreateDepthStencilView(gDepthStencil, &depthStencilViewDesc, &gDepthStencilView)))
            SignalErrorAndReturn(false, "Unable to create depth/stencil view.");

        gContext->OMSetRenderTargets(1, &gBackBufferView, gDepthStencilView);

        D3D11_VIEWPORT viewport;
        ZeroMemory(&viewport, sizeof viewport);
        viewport.Width = static_cast<FLOAT>(width);
        viewport.Height = static_cast<FLOAT>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        gContext->RSSetViewports(1, &viewport);

        return true;
    }

    static bool CreateSkinnedMeshRenderData()
    {
        D3D11_BUFFER_DESC jointDesc = {};
        jointDesc.ByteWidth = sizeof(Mat4x4) * MAX_NUM_JOINTS;
        jointDesc.Usage = D3D11_USAGE_DYNAMIC;
        jointDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        jointDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(gDevice->CreateBuffer(&jointDesc, NULL, &gSkinningMatrices)))
            SignalErrorAndReturn(false, "Unable to create joint buffer.");

        D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R16_UINT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 2, DXGI_FORMAT_R16_UINT, 0, 10, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        bc::AutoMemMap vertexShader("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.vsh.o");
        if (!vertexShader.Valid())
            SignalErrorAndReturn(false, "Unable to map vertex shader.");

        if (FAILED(gDevice->CreateVertexShader(vertexShader.Mem(), vertexShader.Size(), NULL, &gSkinnedVertexShader)))
            SignalErrorAndReturn(false, "Unable to create vertex shader.");

        bc::AutoMemMap pixelShader("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.fsh.o");
        if (!pixelShader.Valid())
            SignalErrorAndReturn(false, "Unable to map vertex shader.");

        if (FAILED(gDevice->CreatePixelShader(pixelShader.Mem(), pixelShader.Size(), NULL, &gDebugMaterial)))
            SignalErrorAndReturn(false, "Unable to create vertex shader.");

        if (FAILED(gDevice->CreateInputLayout(inputElements, sizeof(inputElements) / sizeof(inputElements[0]), vertexShader.Mem(), vertexShader.Size(), &gSkinnedMeshInputLayout)))
            SignalErrorAndReturn(false, "Unable to create input layout.");

        D3D11_BUFFER_DESC viewProjDesc = {};
        viewProjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        viewProjDesc.ByteWidth = sizeof(Mat4x4);
        viewProjDesc.Usage = D3D11_USAGE_DYNAMIC;
        viewProjDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(gDevice->CreateBuffer(&viewProjDesc, NULL, &gViewProjConstants)))
            SignalErrorAndReturn(false, "Unable to create view/projection constant buffer.");

        return true;
    }

    static void CreateProjectionMatrix(int width, int height)
    {
        // This creates a lefthand perspective projection matrix
        const float fov = 3.1415926535f / 4;
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        const float zn = 1.0f;
        const float zf = 2000.0f;

        // xScale     0          0               0
        // 0        yScale       0               0
        // 0          0       zf/(zf-zn)         1
        // 0          0       -zn*zf/(zf-zn)     0
        // where:
        // yScale = cot(fovY/2)
        // xScale = yScale / aspect ratio

        const float yscale = 1 / tanf(fov / 2);
        const float xscale = yscale / aspectRatio;

        gProjMatrix.v[0] = xscale;
        gProjMatrix.v[5] = yscale;
        gProjMatrix.v[10] = zf / (zf - zn);
        gProjMatrix.v[11] = 1;
        gProjMatrix.v[14] = -zn * zf / (zf - zn);
    }

    bool GfxInitialize(void *instance, int width, int height)
    {
        HINSTANCE hinstance = static_cast<HINSTANCE>(instance);
        if (!RegisterWindowClass(hinstance))
            SignalErrorAndReturn(false, "Unable to register window class.");

        if (!CreateGameWindow(hinstance, width, height))
            SignalErrorAndReturn(false, "Unable to create game window.");

        if (!InitializeDevice(gWindowHandle, width, height))
            SignalErrorAndReturn(false, "Unable to initialize graphics device.");

        if (!CreateSkinnedMeshRenderData())
            SignalErrorAndReturn(false, "Unable to create skinned mesh render data.");

        CreateProjectionMatrix(width, height);

        return true;
    }

    void GfxBeginScene()
    {
        // clearColor is initialized automagically to { 0, 0, 0, 0 } 
        static const FLOAT clearColor[4];
        gContext->ClearRenderTargetView(gBackBufferView, clearColor);
        gContext->ClearDepthStencilView(gDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (FAILED(gContext->Map(gViewProjConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            SignalErrorAndReturn(, "Unable to map view/proj constant buffer for writing.");

static int frame;
if ((++frame % 30) == 0)
    OutputDebugStringA("Change this matrix shit below to use a proper view/projection matrix instead of just the projection matrix;\n");

        memcpy(mappedResource.pData, &gProjMatrix, sizeof(Mat4x4));
        gContext->Unmap(gViewProjConstants, 0);
    }

    void GfxEndScene()
    {
        gSwapChain->Present(1, 0);
    }

    void GfxShutdown()
    {
        SAFE_RELEASE(gDepthStencilView);
        SAFE_RELEASE(gDepthStencil);
        SAFE_RELEASE(gBackBufferView);
        SAFE_RELEASE(gContext);
        SAFE_RELEASE(gSwapChain);
        SAFE_RELEASE(gDevice);
    }

    IndexBuffer *IndexBufferCreate(int numIndices, unsigned short *indices)
    {
        ID3D11Buffer *indexBuffer;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = numIndices * sizeof(unsigned short);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = indices;

        if (FAILED(gDevice->CreateBuffer(&desc, &data, &indexBuffer)))
            SignalErrorAndReturn(NULL, "Unable to create index buffer.");

        return reinterpret_cast<IndexBuffer *>(indexBuffer);
    }

    MeshVertexBuffer *MeshVertexBufferCreate(int numVertices, MeshVertex *vertices)
    {
        ID3D11Buffer *vertexBuffer;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = numVertices * sizeof(MeshVertex);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = vertices;

        if (FAILED(gDevice->CreateBuffer(&desc, &data, &vertexBuffer)))
            SignalErrorAndReturn(NULL, "Unable to create vertex buffer.");

        return reinterpret_cast<MeshVertexBuffer *>(vertexBuffer);
    }

    struct MeshWeightedPositionBuffer
    {
        ID3D11Buffer *mWeightedPositions;
        ID3D11ShaderResourceView *mWeightedPositionsRV;
        ID3D11Buffer *mJointIndices;
        ID3D11ShaderResourceView *mJointIndicesRV;
    };

    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices)
    {
        MeshWeightedPositionBuffer *px = static_cast<MeshWeightedPositionBuffer *>(MemAlloc(bc::POOL_GFX, sizeof(MeshWeightedPositionBuffer)));

        // Allocate weighted position buffer
        D3D11_BUFFER_DESC pxBufDesc = {};
        pxBufDesc.ByteWidth = numPositions * sizeof(Vec4);
        pxBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
        pxBufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA pxBufData = {};
        pxBufData.pSysMem = weightedPositions;

        if (FAILED(gDevice->CreateBuffer(&pxBufDesc, &pxBufData, &px->mWeightedPositions)))
            SignalErrorAndReturn(NULL, "Unable to create weighted position buffer.");

        D3D11_SHADER_RESOURCE_VIEW_DESC pxBufferSRVDesc = {};
        pxBufferSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        pxBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        pxBufferSRVDesc.Buffer.NumElements = static_cast<UINT>(numPositions);
        if (FAILED(gDevice->CreateShaderResourceView(px->mWeightedPositions, &pxBufferSRVDesc, &px->mWeightedPositionsRV)))
            SignalErrorAndReturn(NULL, "Unable to create weighted position SRV.");

        D3D11_BUFFER_DESC jointIndicesDesc = {};
        jointIndicesDesc.ByteWidth = static_cast<UINT>(numPositions);
        jointIndicesDesc.Usage = D3D11_USAGE_IMMUTABLE;
        jointIndicesDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA jointIndicesData = {};
        jointIndicesData.pSysMem = jointIndices;

        if (FAILED(gDevice->CreateBuffer(&jointIndicesDesc, &jointIndicesData, &px->mJointIndices)))
            SignalErrorAndReturn(NULL, "Unable to create joint indices buffer.");

        D3D11_SHADER_RESOURCE_VIEW_DESC jointBufferSRVDesc = {};
        jointBufferSRVDesc.Format = DXGI_FORMAT_R8_UINT;
        jointBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        jointBufferSRVDesc.Buffer.NumElements = static_cast<UINT>(numPositions);
        if (FAILED(gDevice->CreateShaderResourceView(px->mJointIndices, &jointBufferSRVDesc, &px->mJointIndicesRV)))
            SignalErrorAndReturn(NULL, "Unable to create joint indices SRV.");

        return px;
    }

    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const SoaQuatPos *poseData)
    {
        ID3D11Buffer **indexBuffers = reinterpret_cast<ID3D11Buffer **>(meshData->mIndexBuffers);
        ID3D11Buffer **vertexBuffers = reinterpret_cast<ID3D11Buffer **>(meshData->mVertexBuffers);
        MeshWeightedPositionBuffer **pxBuffers = meshData->mWeightedPositionBuffers;
        int *numTris = meshData->mNumTris;
        int numMeshes = meshData->mNumMeshes;

        size_t matricesAllocSize = sizeof(Mat4x4) * poseData->mNumElements;
        Mat4x4 *matrices = static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        poseData->ConvertToMat4x4(matrices);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (FAILED(gContext->Map(gSkinningMatrices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            SignalErrorAndReturn(, "Unable to map joint buffer for writing.");
        memcpy(mappedResource.pData, matrices, matricesAllocSize);
        gContext->Unmap(gSkinningMatrices, 0);

        ID3D11Buffer *vsBuffers[] = { gViewProjConstants, gSkinningMatrices };
        gContext->VSSetConstantBuffers(0, sizeof vsBuffers / sizeof vsBuffers[0], vsBuffers);
        gContext->VSSetShader(gSkinnedVertexShader, NULL, 0);
        gContext->PSSetShader(gDebugMaterial, NULL, 0);

        UINT offsets[1] = { 0 };
        UINT vertexStrides[1] = { sizeof(MeshVertex) };

        gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gContext->IASetInputLayout(gSkinnedMeshInputLayout);

        for (int i = 0; i < numMeshes; ++i)
        {
            ID3D11Buffer *vertexBuffer[1] = { vertexBuffers[i] };
            gContext->IASetIndexBuffer(indexBuffers[i], DXGI_FORMAT_R16_UINT, 0);
            gContext->IASetVertexBuffers(0, 1, vertexBuffer, vertexStrides, offsets);
            ID3D11ShaderResourceView *shaderResourceViews[] = {
                pxBuffers[i]->mWeightedPositionsRV,
                pxBuffers[i]->mJointIndicesRV
            };
            gContext->VSSetShaderResources(0, sizeof shaderResourceViews / sizeof shaderResourceViews[i], shaderResourceViews);
            gContext->DrawIndexed(static_cast<UINT>(numTris[i]), 0, 0);
        }
    }

#if 0

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
        int *c = meshData->mNumTris;

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

#endif
}
