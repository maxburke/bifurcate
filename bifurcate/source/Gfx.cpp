#pragma warning(push, 0)
#define NOMINMAX
#define D3D_DEBUG_INFO
#include <d3d11.h>
#pragma warning(pop)

#include "Config.h"
#include "Core.h"
#include "Gfx.h"
#include "Anim.h"
#include "Mesh.h"
#include "Camera.h"
#include <math.h>

namespace bg
{
    static TCHAR WINDOW_CLASS[] = TEXT("WindowClass");
    static TCHAR WINDOW_NAME[] = TEXT("Magnificent Mike and the Scourge of Dr. Pengoblin");
    HWND gWindowHandle;

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
static FreeCam *gFreeCam;
static ID3D11VertexShader *gDebugVertexShader;
static ID3D11InputLayout *gDebugInputLayout;
    #define SAFE_RELEASE(x) if (x) { (x)->Release(); } else (void)0

    void *GfxGetWindowHandle()
    {
        return gWindowHandle;
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
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
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

        D3D11_TEXTURE2D_DESC depthDesc = {};
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

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = depthDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        if (FAILED(gDevice->CreateDepthStencilView(gDepthStencil, &depthStencilViewDesc, &gDepthStencilView)))
            SignalErrorAndReturn(false, "Unable to create depth/stencil view.");

        gContext->OMSetRenderTargets(1, &gBackBufferView, gDepthStencilView);

        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<FLOAT>(width);
        viewport.Height = static_cast<FLOAT>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        gContext->RSSetViewports(1, &viewport);

        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;

        ID3D11RasterizerState *rasterizerState;
        gDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        gContext->RSSetState(rasterizerState);
        rasterizerState->Release();

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

        /// DEBUG VERTEX SHADER ONLY
        {
            bc::AutoMemMap debugVertexShader("C:/Users/max/opt/bifurcate/bifurcate/assets/Debug.vsh.o");
            if (!debugVertexShader.Valid())
                SignalErrorAndReturn(false, "Unable to map vertex shader.");

            if (FAILED(gDevice->CreateVertexShader(debugVertexShader.Mem(), debugVertexShader.Size(), NULL, &gDebugVertexShader)))
                SignalErrorAndReturn(false, "Unable to create vertex shader.");

            D3D11_INPUT_ELEMENT_DESC debugInputElements[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            if (FAILED(gDevice->CreateInputLayout(debugInputElements, sizeof(debugInputElements) / sizeof(debugInputElements[0]), debugVertexShader.Mem(), debugVertexShader.Size(), &gDebugInputLayout)))
                SignalErrorAndReturn(false, "Unable to create input layout.");
        }
        /// END DEBUG VERTEX SHADER ONLY

        {
            bc::AutoMemMap vertexShader("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.vsh.o");
            if (!vertexShader.Valid())
                SignalErrorAndReturn(false, "Unable to map vertex shader.");

            if (FAILED(gDevice->CreateVertexShader(vertexShader.Mem(), vertexShader.Size(), NULL, &gSkinnedVertexShader)))
                SignalErrorAndReturn(false, "Unable to create vertex shader.");

            if (FAILED(gDevice->CreateInputLayout(inputElements, sizeof(inputElements) / sizeof(inputElements[0]), vertexShader.Mem(), vertexShader.Size(), &gSkinnedMeshInputLayout)))
                SignalErrorAndReturn(false, "Unable to create input layout.");
        }

        {
            bc::AutoMemMap pixelShader("C:/Users/max/opt/bifurcate/bifurcate/assets/Skinned.fsh.o");
            if (!pixelShader.Valid())
                SignalErrorAndReturn(false, "Unable to map vertex shader.");

            if (FAILED(gDevice->CreatePixelShader(pixelShader.Mem(), pixelShader.Size(), NULL, &gDebugMaterial)))
                SignalErrorAndReturn(false, "Unable to create vertex shader.");
        }

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
        gProjMatrix.v[10] = -zf / (zf - zn);
        gProjMatrix.v[11] = -1;
        gProjMatrix.v[14] = zn * zf / (zn - zf);
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
gFreeCam = FreeCamCreate(width, height);
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
FreeCamUpdate(gFreeCam);
Mat4x4 viewMatrix;
FreeCamFetchViewMatrix(gFreeCam, &viewMatrix);
Mat4x4Multiply(static_cast<Mat4x4 *>(mappedResource.pData), &viewMatrix, &gProjMatrix);

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

    VertexBuffer *VertexBufferCreate(int numVertices, size_t vertexSize, const void *vertices)
    {
        ID3D11Buffer *vertexBuffer;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = static_cast<UINT>(numVertices) * vertexSize;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = vertices;

        if (FAILED(gDevice->CreateBuffer(&desc, &data, &vertexBuffer)))
            SignalErrorAndReturn(NULL, "Unable to create vertex buffer.");

        return reinterpret_cast<VertexBuffer *>(vertexBuffer);
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

#define USE_DEBUG_SKELETON_RENDER 1
#define USE_DEBUG_QUAD_DRAW 0
#if USE_DEBUG_SKELETON_RENDER

    static void TransformJoints(Mat4x4 * __restrict poseMatrices, const Mat4x4 * __restrict rawMatrices, const AnimData *animData)
    {
        int numJoints = animData->mNumJoints;
        const AnimJoint *joints = animData->mJoints;

        memcpy(poseMatrices, rawMatrices, numJoints * sizeof(Mat4x4));

        for (int i = 1; i < numJoints; ++i)
        {
            int parent = joints[i].mParentIndex;
            //Mat4x4 pose(poseMatrices[i]);
            Mat4x4Multiply(poseMatrices + i, poseMatrices + i, poseMatrices + parent);
        }
    }

    void DrawSkinnedMesh(const SkinnedMeshData *, const AnimData *animData, const SoaQuatPos *poseData)
    {
        int numJoints = poseData->mNumElements;

        ID3D11Buffer *vertexBuffer = NULL;
        ID3D11Buffer *indexBuffer = NULL;
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = static_cast<UINT>(numJoints - 1) * sizeof(float) * 3;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(gDevice->CreateBuffer(&desc, NULL, &vertexBuffer)))
            __debugbreak();

        const int numIndices = (numJoints - 1) * 2;
        unsigned short *indices = static_cast<unsigned short *>(AllocaAligned(16, sizeof(unsigned short) * numIndices));
        for (int i = 0; i < numIndices; ++i)
            indices[i] = static_cast<unsigned short>(i);
        indexBuffer = reinterpret_cast<ID3D11Buffer *>(IndexBufferCreate(numIndices, indices));

        size_t matricesAllocSize = sizeof(Mat4x4) * numJoints;

        Mat4x4 *poseMatrices = static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        Mat4x4 *rawMatrices = static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        poseData->ConvertToMat4x4(rawMatrices);

        TransformJoints(poseMatrices, rawMatrices, animData);

        // TODO: You might need to flip the order of this multiplication, try inverseBindPose * poseMatrices instead!!
//        MultiplyInverseBindPose(matrices, numJoints, meshData->mInverseBindPose, poseMatrices);

        const AnimJoint *joints = animData->mJoints;
        size_t positionBufferSize = numIndices * 3 * sizeof(float);
        float *positionBuffer = static_cast<float *>(AllocaAligned(16, positionBufferSize));

        // When rendering the skeletons you just need to use the pose matrices, but for the skinned
        // models (below), you have to multiply by the inverse bind pose.
        Mat4x4 *mat = poseMatrices;
        for (int i = 1, positionIndex = 0; i < numJoints; ++i)
        {
            int parent = joints[i].mParentIndex;
            positionBuffer[positionIndex++] = mat[parent].v[12];
            positionBuffer[positionIndex++] = mat[parent].v[13];
            positionBuffer[positionIndex++] = mat[parent].v[14];
            positionBuffer[positionIndex++] = mat[i].v[12];
            positionBuffer[positionIndex++] = mat[i].v[13];
            positionBuffer[positionIndex++] = mat[i].v[14];
        }

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (FAILED(gContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            __debugbreak();
        memcpy(mappedResource.pData, positionBuffer, positionBufferSize);
        gContext->Unmap(vertexBuffer, 0);

        gContext->VSSetShader(gDebugVertexShader, NULL, 0);
        gContext->PSSetShader(gDebugMaterial, NULL, 0);
        gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        gContext->IASetInputLayout(gDebugInputLayout);

        gContext->VSSetConstantBuffers(0, 1, &gViewProjConstants);

        UINT offsets[1] = { 0 };
        UINT vertexStrides[1] = { sizeof(float) * 3 };
        gContext->IASetVertexBuffers(0, 1, &vertexBuffer, vertexStrides, offsets);
        gContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
        gContext->DrawIndexed(static_cast<UINT>(numIndices), 0, 0);
        
        vertexBuffer->Release();
        indexBuffer->Release();
    }

#elif USE_DEBUG_QUAD_DRAW

    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const AnimData *, const SoaQuatPos *poseData)
    {
        static bool initializedDebugDrawData = false;
        static ID3D11Buffer *vertexBuffer;
        static ID3D11Buffer *indexBuffer;

        UNUSED(meshData);
        UNUSED(poseData);
        if (!initializedDebugDrawData)
        {
            unsigned short indices[] = { 0, 1, 2, 2, 3, 0 };
            indexBuffer = reinterpret_cast<ID3D11Buffer *>(IndexBufferCreate(6, indices));
            float vertices[] = {
                -1, 1, -2,
                1, 1, -2,
                1, -1, -2,
                -1, -1, -2
            };
            vertexBuffer = reinterpret_cast<ID3D11Buffer *>(VertexBufferCreate(4, 12, vertices));
            initializedDebugDrawData = true;
        }

        gContext->VSSetShader(gDebugVertexShader, NULL, 0);
        gContext->PSSetShader(gDebugMaterial, NULL, 0);
        gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gContext->IASetInputLayout(gDebugInputLayout);

        ID3D11Buffer *vsBuffers[] = { gViewProjConstants };
        gContext->VSSetConstantBuffers(0, sizeof vsBuffers / sizeof vsBuffers[0], vsBuffers);

        UINT offsets[1] = { 0 };
        UINT vertexStrides[1] = { 12 };
        gContext->IASetVertexBuffers(0, 1, &vertexBuffer, vertexStrides, offsets);
        gContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
        gContext->DrawIndexed(6, 0, 0);
    }

#else
    ////////////////////////////////////////////////////////////////////

    static void UploadMatrices(const Mat4x4 *matrices, int numJoints)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        size_t matricesAllocSize = sizeof(Mat4x4) * numJoints;
        if (FAILED(gContext->Map(gSkinningMatrices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            SignalErrorAndReturn(, "Unable to map joint buffer for writing.");
        memcpy(mappedResource.pData, matrices, matricesAllocSize);
        gContext->Unmap(gSkinningMatrices, 0);
        ID3D11Buffer *vsBuffers[] = { gViewProjConstants, gSkinningMatrices };
        gContext->VSSetConstantBuffers(0, sizeof vsBuffers / sizeof vsBuffers[0], vsBuffers);
    }

    static void TransformJoints(Mat4x4 * __restrict poseMatrices, const Mat4x4 * __restrict rawMatrices, const AnimData *animData)
    {
        int numJoints = animData->mNumJoints;
        const AnimJoint *joints = animData->mJoints;

        for (int i = 1; i < numJoints; ++i)
        {
            int parent = joints[i].mParentIndex;
            Mat4x4Multiply(poseMatrices + i, rawMatrices + i, rawMatrices + parent);
        }
    }

    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const AnimData *animData, const SoaQuatPos *poseData)
    {
        int *numTris = meshData->mNumTris;
        int numMeshes = meshData->mNumMeshes;
        int numJoints = poseData->mNumElements;

        size_t matricesAllocSize = sizeof(Mat4x4) * numJoints;
        Mat4x4 *poseMatrices = static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        Mat4x4 *rawMatrices = static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        Mat4x4 *matrices =  static_cast<Mat4x4 *>(AllocaAligned(16, matricesAllocSize));
        poseData->ConvertToMat4x4(rawMatrices);
        TransformJoints(poseMatrices, rawMatrices, animData);
        MultiplyInverseBindPose(matrices, numJoints, poseMatrices, meshData->mInverseBindPose);
        // TODO: DEBUG
        // Let's see what happens when we use an identity transformation matrix.
        //MultiplyInverseBindPose(matrices, numJoints, meshData->mBindPose, meshData->mInverseBindPose);
        UploadMatrices(matrices, numJoints);

        gContext->VSSetShader(gSkinnedVertexShader, NULL, 0);
        gContext->PSSetShader(gDebugMaterial, NULL, 0);
        gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gContext->IASetInputLayout(gSkinnedMeshInputLayout);

        ID3D11Buffer **indexBuffers = reinterpret_cast<ID3D11Buffer **>(meshData->mIndexBuffers);
        ID3D11Buffer **vertexBuffers = reinterpret_cast<ID3D11Buffer **>(meshData->mVertexBuffers);
        MeshWeightedPositionBuffer **pxBuffers = meshData->mWeightedPositionBuffers;

        for (int i = 0; i < numMeshes; ++i)
        {
            UINT offsets[1] = { 0 };
            UINT vertexStrides[1] = { sizeof(MeshVertex) };
            ID3D11Buffer *vertexBuffer[1] = { vertexBuffers[i] };
            gContext->IASetVertexBuffers(0, 1, vertexBuffer, vertexStrides, offsets);
            gContext->IASetIndexBuffer(indexBuffers[i], DXGI_FORMAT_R16_UINT, 0);

            ID3D11ShaderResourceView *shaderResourceViews[] = {
                pxBuffers[i]->mWeightedPositionsRV,
                pxBuffers[i]->mJointIndicesRV
            };
            gContext->VSSetShaderResources(0, sizeof shaderResourceViews / sizeof shaderResourceViews[i], shaderResourceViews);
            gContext->DrawIndexed(static_cast<UINT>(numTris[i]) * 3, 0, 0);
        }
    }
#endif
}
