/*
 * dx.cpp
 *
 */
#include "dx.h"
#include "d2fx.h"
#include <d3d11.h>
#include "gfx.h"
#include "testdata.h"
#include "tcache.h"




#ifdef _DEBUG
#define EXTRA_DEVICE_FLAGS D3D11_CREATE_DEVICE_DEBUG
#else
#define EXTRA_DEVICE_FLAGS 0 
#endif


 // Helpers
#define DXFAIL(_dxcall) if((hr = (_dxcall)) != S_OK) {  DebugBreak(); }


HRESULT hr;
IDXGIFactory* pDxgiFactory;
IDXGIAdapter* pTargetAdapter;
IDXGIOutput* pTargetOutput;
ID3D11Device* pDevice;
ID3D11DeviceContext* pDevCtx;
IDXGISwapChain* pSwapChain;
ID3D11InputLayout* pInputLayout;
ID3D11Buffer* pVertexBuffer;
ID3D11Buffer* pIndexBuffer;
ID3D11Texture1D* pPalette;
ID3D11ShaderResourceView* pPaletteSrv;
ID3D11Buffer* pConstantBuffer;
ID3D11RenderTargetView* pBackBufferRtv;
ID3D11BlendState* pBlend[4];
ID3D11Texture2D* pDepthSurface;
ID3D11DepthStencilView* pDepthSurfaceDsv;
ID3D11Texture2D* pTexCacheTexture;
ID3D11ShaderResourceView* pTexCacheTextureSrv;


// Shader types.
#define SHADER_TYPE_PIXEL   0
#define SHADER_TYPE_VERTEX  1

// Our shaders.
struct
{
    const u8 ShaderType;
    const char* pFile;

    void* pShaderData;
    u32 ShaderSz;

    union
    {
        ID3D11PixelShader* pps;
        ID3D11VertexShader* pvs;
    };
} Shaders[] =
{
    { SHADER_TYPE_VERTEX, "vs" },
    { SHADER_TYPE_PIXEL, "ps" },
};

#define GetShader(_shader_id) Shaders[_shader_id]
#define SHADER_VS  0
#define SHADER_PS  1


void* GetResource(const char* pName, u32* pSize)
{
    HRSRC hRes;
    HGLOBAL hgRes;

    if(!(hRes = FindResourceA(GetModuleHandle(IMAGE_FILE_NAME), pName, RT_RCDATA)) || !(hgRes = LoadResource(GetModuleHandle(IMAGE_FILE_NAME), hRes)))
        return NULL;

    if(pSize)
        *pSize = SizeofResource(GetModuleHandle(IMAGE_FILE_NAME), hRes);

    return LockResource(hgRes);
}


static bool CreateShaders()
{
    for(uint i = 0; i < ARRAYSIZE(Shaders); i++)
    {
        if(!(Shaders[i].pShaderData = GetResource(Shaders[i].pFile, &Shaders[i].ShaderSz)))
        {
            return false;
        }

        switch(Shaders[i].ShaderType)
        {
            case SHADER_TYPE_PIXEL: DXFAIL(pDevice->CreatePixelShader(Shaders[i].pShaderData, Shaders[i].ShaderSz, NULL, &Shaders[i].pps)); break;
            case SHADER_TYPE_VERTEX: DXFAIL(pDevice->CreateVertexShader(Shaders[i].pShaderData, Shaders[i].ShaderSz, NULL, &Shaders[i].pvs)); break;
        }
    }

    return true;
}


bool DxInitialize(void* hWindow)
{
    HRESULT (WINAPI* pfCreateDXGIFactory)(REFIID riid, _COM_Outptr_ void** ppFactory);
    HRESULT (WINAPI* pfD3D11CreateDevice)(IDXGIAdapter * pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, CONST D3D_FEATURE_LEVEL * pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device * *ppDevice, D3D_FEATURE_LEVEL * pFeatureLevel, ID3D11DeviceContext * *ppImmediateContext);


    // Load implementation.
    const HMODULE hDXGI = LoadLibrary("dxgi.dll");
    const HMODULE hD3D11 = LoadLibrary("d3d11.dll");

    if(!hDXGI || !(*((void**) &pfCreateDXGIFactory) = GetProcAddress(hDXGI, "CreateDXGIFactory")))
        return false;

    if(!hD3D11 || !(*((void**) &pfD3D11CreateDevice) = GetProcAddress(hD3D11, "D3D11CreateDevice")))
        return false;

    DXFAIL(pfCreateDXGIFactory(IID_PPV_ARGS(&pDxgiFactory)));

    // Create our D3D device.
    D3D_FEATURE_LEVEL RequestedFeatureLevels[] =
    {
         D3D_FEATURE_LEVEL_11_0,
    };

    DXFAIL(pfD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, EXTRA_DEVICE_FLAGS | D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS, RequestedFeatureLevels, ARRAYSIZE(RequestedFeatureLevels), D3D11_SDK_VERSION, &pDevice, NULL, &pDevCtx));

    DXGI_SWAP_CHAIN_DESC scd;

    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 0;
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.OutputWindow = (HWND) hWindow;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = NULL;

    DXFAIL(pDxgiFactory->CreateSwapChain(pDevice, &scd, &pSwapChain));

    // Create shaders
    if(!CreateShaders())
        return false;

    // Create our single vertex ring buffer and describe the layout.
    D3D11_BUFFER_DESC bd;
    bd.ByteWidth = 0x100000; /* 64K */
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = NULL;
    bd.StructureByteStride = 0;

    DXFAIL(pDevice->CreateBuffer(&bd, NULL, &pVertexBuffer));

    // Create our index buffer.
    bd.ByteWidth = 0x100000; /* 64K */
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = NULL;
    bd.StructureByteStride = 0;

    DXFAIL(pDevice->CreateBuffer(&bd, NULL, &pIndexBuffer));

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R16G16_SINT,    0, offsetof(gfx_vert, x),  D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "DEPTH",    0, DXGI_FORMAT_R16_UNORM,      0, offsetof(gfx_vert, z),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R8G8_UINT,      0, offsetof(gfx_vert, u),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, offsetof(gfx_vert, rgba), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TILEID",   0, DXGI_FORMAT_R16_UINT,       0, offsetof(gfx_vert, tid), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DXFAIL(pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), GetShader(SHADER_VS).pShaderData, GetShader(SHADER_VS).ShaderSz, &pInputLayout));

    // Create palette.
    D3D11_TEXTURE1D_DESC paldesc;

    paldesc.Width = 0x100;
    paldesc.MipLevels = 1;
    paldesc.ArraySize = 1;
    paldesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    paldesc.Usage = D3D11_USAGE_DEFAULT;
    paldesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    paldesc.CPUAccessFlags = 0;
    paldesc.MiscFlags = 0;

    DXFAIL(pDevice->CreateTexture1D(&paldesc, NULL, &pPalette));
    DXFAIL(pDevice->CreateShaderResourceView(pPalette, NULL, &pPaletteSrv));

    // Get real backbuffer dimensions.
    DXGI_SWAP_CHAIN_DESC xx;
    pSwapChain->GetDesc(&xx);


    // Create depth-buffer
    D3D11_TEXTURE2D_DESC texdesc;
    texdesc.Width = xx.BufferDesc.Width;
    texdesc.Height = xx.BufferDesc.Height;
    texdesc.MipLevels = 1;
    texdesc.ArraySize = 1;
    texdesc.Format = DXGI_FORMAT_D16_UNORM;
    texdesc.SampleDesc.Count = 1;
    texdesc.SampleDesc.Quality = 0;
    texdesc.Usage = D3D11_USAGE_DEFAULT;
    texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texdesc.CPUAccessFlags = 0;
    texdesc.MiscFlags = 0;

    DXFAIL(pDevice->CreateTexture2D(&texdesc, NULL, &pDepthSurface));
    DXFAIL(pDevice->CreateDepthStencilView(pDepthSurface, NULL, &pDepthSurfaceDsv));


    // Create texture cache.
    texdesc.Width = 256;
    texdesc.Height = 256;
    texdesc.MipLevels = 1;
    texdesc.ArraySize = MAX_TILES;
    texdesc.Format = DXGI_FORMAT_R8_UINT;
    texdesc.Format = DXGI_FORMAT_R8_UINT;
    texdesc.SampleDesc.Count = 1;
    texdesc.SampleDesc.Quality = 0;
    texdesc.Usage = D3D11_USAGE_DEFAULT;
    texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texdesc.CPUAccessFlags = NULL;
    texdesc.MiscFlags = NULL;

    DXFAIL(pDevice->CreateTexture2D(&texdesc, NULL, &pTexCacheTexture));
    DXFAIL(pDevice->CreateShaderResourceView(pTexCacheTexture, NULL, &pTexCacheTextureSrv));

    {
        

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = 16;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;

        float screen_size[2];
        screen_size[0] = xx.BufferDesc.Width;
        screen_size[1] = xx.BufferDesc.Height;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = screen_size;
        data.SysMemPitch = sizeof(screen_size);
        data.SysMemSlicePitch = 0;

        DXFAIL(pDevice->CreateBuffer(&desc, &data, &pConstantBuffer));

        D3D11_VIEWPORT vp;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width = xx.BufferDesc.Width;
        vp.Height = xx.BufferDesc.Height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;

        pDevCtx->RSSetViewports(1, &vp);
    }


    // Setup device state.
    pDevCtx->IASetInputLayout(pInputLayout);
    pDevCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pDevCtx->VSSetShader(GetShader(SHADER_VS).pvs, NULL, 0);
    pDevCtx->PSSetShader(GetShader(SHADER_PS).pps, NULL, 0);

    UINT VertexStride = sizeof(gfx_vert);
    UINT VertexOffset = 0;

    pDevCtx->IASetVertexBuffers(0, 1, &pVertexBuffer, &VertexStride, &VertexOffset);
    pDevCtx->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Bind resources
    pDevCtx->VSSetConstantBuffers(0, 1, &pConstantBuffer);


    ID3D11Resource* pBackBuffer;
    DXFAIL(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));
    DXFAIL(pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pBackBufferRtv));


    {
        //
        // Rasterizer state.
        //
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        rd.ScissorEnable = FALSE;
       

        ID3D11RasterizerState* pRasterState;
        DXFAIL(pDevice->CreateRasterizerState(&rd, &pRasterState));

        pDevCtx->RSSetState(pRasterState);


        //
        // Depth test state.
        //
        D3D11_DEPTH_STENCIL_DESC depth ={};
        depth.DepthEnable = TRUE;
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth.DepthFunc = D3D11_COMPARISON_GREATER;
        depth.StencilEnable = FALSE;

        ID3D11DepthStencilState* pDepthState;
        DXFAIL(pDevice->CreateDepthStencilState(&depth, &pDepthState));

        pDevCtx->OMSetDepthStencilState(pDepthState, 0);
    }


     

    D3D11_BLEND_DESC BlendDesc[] =
    {
        {
            FALSE,
            FALSE,

            { FALSE, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE },
        },

        {
            FALSE,
            FALSE,

            { TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE },
        },

        {
            FALSE,
            FALSE,

            { TRUE, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE },
        },

        {
            FALSE,
            FALSE,

            { TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE },
        },
    };

    
    for(int i = 0; i < ARRAYSIZE(pBlend); i++)
    {
        DXFAIL(pDevice->CreateBlendState(&BlendDesc[i], &pBlend[i]));
    }


    //
    // -- Initialization Done --
    //



    //u8 width = 128;
    //u8 height = 128;

    //s16 x = 1;
    //s16 y = 1;

    //float xxx = 256.0f - 0.5f;

    //u8 z = xxx;

    //u8 tex_u = width;
    //u8 tex_v = height;

    //gfx_vert vertices[] =
    //{
    //    { x, y, 0, 0 },
    //    { x+width, y, tex_u, 0 },
    //    { x, y+height, 0, tex_v},
    //    { x+width, y, tex_u, 0 },
    //    { x+width, y+height, tex_u, tex_v },
    //    { x, y+height, 0, tex_v }
    //};


    //void* pTestTex;
    //void* pTestTexSrv;

    //DxCreateTexture(128, 128, rawData_sprite, &pTestTex, &pTestTexSrv);

    //DxStateSetPalette(rawData_pal);


    return true;
}


void DxTextureCacheUpload(u32 ArrayIdx, u16 DstX, u16 DstY, u16 TexWidth, u16 TexHeight, const void* data)
{
    D3D11_BOX rc;
    rc.left = DstX;
    rc.top = DstY;
    rc.right = rc.left + TexWidth;
    rc.bottom = rc.top + TexHeight;
    rc.back = 1;
    rc.front = 0;

    pDevCtx->UpdateSubresource(pTexCacheTexture, ArrayIdx, &rc, data, TexWidth, 0);
}





void DxCreateTexture(u32 Width, u32 Height, const void* pData, void** ppTexture, void** ppSrv)
{
    D3D11_TEXTURE2D_DESC texdesc;
    texdesc.Width = Width;
    texdesc.Height = Height;
    texdesc.MipLevels = 1;
    texdesc.ArraySize = 1;
    texdesc.Format = DXGI_FORMAT_R8_UINT;
    texdesc.SampleDesc.Count = 1;
    texdesc.SampleDesc.Quality = 0;
    texdesc.Usage = D3D11_USAGE_IMMUTABLE;
    texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texdesc.CPUAccessFlags = NULL;
    texdesc.MiscFlags = NULL;

    D3D11_SUBRESOURCE_DATA res;
    res.pSysMem = pData;
    res.SysMemPitch = Width * 1;
    res.SysMemSlicePitch = 0;

    DXFAIL(pDevice->CreateTexture2D(&texdesc, &res, (ID3D11Texture2D**) ppTexture));
    DXFAIL(pDevice->CreateShaderResourceView((ID3D11Resource*)(*ppTexture), NULL, (ID3D11ShaderResourceView**)ppSrv));

    LOG("Allocate Texture: 0x%X, 0x%X", *ppTexture, *ppSrv);
}


void DxDestroyTexture(void* pTexture, void* pSrv)
{
    LOG("Destroy Texture: 0x%X, 0x%X", pTexture, pSrv);

    if(((ID3D11ShaderResourceView*) pSrv)->Release() != 0)
    {
       // DebugBreak();
    }

    if(((ID3D11Texture2D*) pTexture)->Release() != 0)
    {
        // TODO: breaks Nsight
       // DebugBreak();
    }
}


void DxStateSetTexture(void* pTextureSrv)
{
    pDevCtx->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**) &pTextureSrv);
}


void DxStateSetPalette(void* pPaletteData)
{
    pDevCtx->UpdateSubresource(pPalette, 0, NULL, pPaletteData, 256 * sizeof(u32), 0);
    pDevCtx->PSSetShaderResources(1, 1, &pPaletteSrv);
}


void DumpDxVerts(gfx_vert* v, u32 c)
{
    LOG("DxVerts:");
    for(u32 i = 0; i < c; i++)
    {
        LOG("   %d, %d (%u, %u)", v[i].x, v[i].y, v[i].u, v[i].v);
    }
}

void DxDraw(gfx_vert* pVertices, u32 Count)
{
    D3D11_MAPPED_SUBRESOURCE map;

   // DumpDxVerts(pVertices, Count);

    DXFAIL(pDevCtx->Map(pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
    memcpy(map.pData, pVertices, Count * sizeof(gfx_vert));
    pDevCtx->Unmap(pVertexBuffer, 0);

    pDevCtx->Draw(Count, 0);
}

void DxSetBlend(u8 Blend)
{
    pDevCtx->OMSetBlendState(pBlend[Blend], NULL, ~0);
}

void DxBegin()
{
    float clear[4] = { 0 };

    pDevCtx->OMSetRenderTargets(1, &pBackBufferRtv, pDepthSurfaceDsv);
    pDevCtx->ClearRenderTargetView(pBackBufferRtv, clear);
    pDevCtx->ClearDepthStencilView(pDepthSurfaceDsv, D3D11_CLEAR_DEPTH, 0, 0);
}


void DxEndAndPresent()
{
    pSwapChain->Present(0, 0);
}


void DxStartVertices(void*& pVB, void*& pIB)
{
    D3D11_MAPPED_SUBRESOURCE map;


    DXFAIL(pDevCtx->Map(pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
    pVB = map.pData;

    DXFAIL(pDevCtx->Map(pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
    pIB = map.pData;
}

void DxEndVertices()
{
    pDevCtx->Unmap(pVertexBuffer, 0);
    pDevCtx->Unmap(pIndexBuffer, 0);
}

void DxDrawIndexed(u32 Offset, u32 Count)
{
    pDevCtx->DrawIndexed(Count, Offset, 0);
}



void DxStateSetTextureCache()
{
    pDevCtx->PSSetShaderResources(0, 1, &pTexCacheTextureSrv);
}


void* DxGetTextureCacheSrv()
{
    return pTexCacheTextureSrv;
}