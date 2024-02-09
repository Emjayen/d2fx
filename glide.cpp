
#include "3dfx/glide.h"
#include "dx.h"
#include "d2fx.h"
#include "gfx.h"
#include <Windows.h>



extern void tcache_init();
// - The palette entry for the conventional transparent (color key) is index zero.
// - The color at 0,0 is zero (indexed) for all textures.



// Configuration
#define TMU_BANK_SZ   (4 * 1024 * 1024)
#define TMU_TEX_ALIGN 0x40


struct vert
{
    float x; // GR_PARAM_XY 
    float y; // ^ 
    u32 c;   // GR_PARAM_PARGB
    float q; // GR_PARAM_Q0, GR_PARAM_Q1
    float u; // GR_PARAM_ST0, GR_PARAM_ST1
    float v; // ^

    u32 reserved;
};


struct tmu_vad
{
    gfx_tex* tex;
};


// 3dfx device state.
struct
{
    tmu_vad vads[TMU_BANK_SZ / TMU_TEX_ALIGN];


    // tmu_tex* tex;
    gfx_tex* tex;
    u32 constant_color;
    u32 constant_write_mask;
    u32 shift;

    gfx_blend blend;

    u32 color_palette[0x100];

} fxs;



#define PFX_COLOR_STATE(function, factor, local, other) u16(function | (factor << 5) | (local << 9) | (other << 11))
#define PFX_BLEND_STATE(rgb_sf, rgb_df, alpha_sf, alpha_df) u16(rgb_sf | (rgb_df << 4) | (alpha_sf << 8) | (alpha_df << 12))



// Prototypes
void GetTexDim(const GrTexInfo* info, FxU32* w, FxU32* h);



void vtv(gfx_vert& out, vert& in)
{
    u16 u = in.u;
    u16 v = in.v;

    u -= (u >> 8);
    v -= (v >> 8);

    out.x = (s16) in.x;
    out.y = (s16) in.y;
    out.u = u >> fxs.shift;
    out.v = v >> fxs.shift;

    out.rgba = (in.c & 0x00FFFFFF) | (fxs.constant_color & fxs.constant_write_mask);
}




//
// Initialization
//
FX_ENTRY GrContext_t FX_CALL grSstWinOpen
(
    FxU32                hWnd,
    GrScreenResolution_t screen_resolution,
    GrScreenRefresh_t    refresh_rate,
    GrColorFormat_t      color_format,
    GrOriginLocation_t   origin_location,
    int                  nColBuffers,
    int                  nAuxBuffers
)
{
    static bool Done;

    if(Done)
    {
        LOG("Already called SstWinOpen() ???");
        return (GrContext_t) 1;
    }

    tcache_init();

    Done = true;

    SetProcessDPIAware();

    if(!DxInitialize((void*) hWnd))
        DebugBreak();

    return (GrContext_t) 1;
}


extern bool bShouldBeRendering;

//
// Command submission
//
FX_ENTRY void FX_CALL grBufferSwap(FxU32 swap_interval)
{
    gfx_flush();
    DxEndAndPresent();
    DxBegin();
}


FX_ENTRY void FX_CALL grDrawVertexArray
(
    FxU32 mode,
    FxU32 Count,
    void* pointers
)
{
    gfx_vert* dst = gfx_vb_write_begin();
    vert** src = (vert**) pointers;


    if(bShouldBeRendering != true)
        DebugBreak();


    for(u8 i = 0; i < Count; i++)
    {
        vtv(dst[i], *src[i]);
    }

    switch(mode)
    {
        case GR_TRIANGLE_FAN:
        {
            for(u8 i = 2; i < Count; i++)
            {
                gfx_draw_tri(fxs.tex, fxs.blend, 0, i-1, i);
            }
        } break;

        case GR_TRIANGLE_STRIP:
        {
            for(u8 i = 0; i < Count - 2; i++)
            {
                gfx_draw_tri(fxs.tex, fxs.blend, i, i+1, i+2);
            }
        } break;
    }

    gfx_vb_write_end(Count);
}


FX_ENTRY void FX_CALL grDrawVertexArrayContiguous
(
    FxU32 mode,
    FxU32 Count,
    void* pointers,
    FxU32 stride
)
{
    gfx_vert* dst = gfx_vb_write_begin();
    vert* src = (vert*) pointers;

    for(u8 i = 0; i < Count; i++)
    {
        vtv(dst[i], src[i]);
    }

    switch(mode)
    {
        case GR_TRIANGLE_FAN:
        {
            for(u8 i = 2; i < Count; i++)
            {
                gfx_draw_tri(fxs.tex, fxs.blend, 0, i-1, i);
            }
        } break;

        case GR_TRIANGLE_STRIP:
        {
            for(u8 i = 0; i < Count - 2; i++)
            {
                gfx_draw_tri(fxs.tex, fxs.blend, i, i+1, i+2);
            }
        } break;
    }

    gfx_vb_write_end(Count);
}


FX_ENTRY void FX_CALL grDrawPoint(const void* pt)
{

}

FX_ENTRY void FX_CALL grDrawLine(const void* v1, const void* v2)
{

}


//
// Pipeline state setting.
//
FX_ENTRY void FX_CALL grTexSource
(
    GrChipID_t tmu,
    FxU32      startAddress,
    FxU32      evenOdd,
    GrTexInfo* info
)
{
    FxU32 width;
    FxU32 height;
   
    GetTexDim(info, &width, &height);
    u32 size = width*height;


    ASSERT((startAddress+size <= TMU_BANK_SZ - TMU_TEX_ALIGN) && (startAddress & TMU_TEX_ALIGN-1) == 0);

    DWORD Idx;
    _BitScanReverse(&Idx, max(width, height));
    fxs.shift = 8 - Idx;

    // Set texture as current state. Note that the texture will still not be resident in vidmem
    // until it's actually referenced in work submission.
    fxs.tex = fxs.vads[startAddress/TMU_TEX_ALIGN].tex;

    ASSERT(fxs.tex != nullptr);
}


FX_ENTRY void FX_CALL grTexDownloadMipMap
(
    GrChipID_t tmu,
    FxU32 startAddress,
    FxU32 evenOdd,
    GrTexInfo* info
)
{
    // Compute linear size of texture memory.
    FxU32 width;
    FxU32 height;
    GetTexDim(info, &width, &height);
    const auto size = width * height;

    ASSERT((startAddress+size <= TMU_BANK_SZ - TMU_TEX_ALIGN) && (startAddress & TMU_TEX_ALIGN-1) == 0 && size >= TMU_TEX_ALIGN && (size & TMU_TEX_ALIGN-1) == 0);


    // Implicitly releases any overwritten surfaces.
    for(u32 i = 0; i < size/TMU_TEX_ALIGN; i++)
    {
        tmu_vad& vad = fxs.vads[(startAddress/TMU_TEX_ALIGN) + i];

        if(vad.tex)
        {
            if(fxs.tex == vad.tex)
                fxs.tex = nullptr;

            tex_tmu_release(vad.tex);
            vad.tex = nullptr;
        }
    }

    // Fetch VAD for this address.
    auto& vad = fxs.vads[startAddress/TMU_TEX_ALIGN];

    vad.tex = tex_create(info->data, width, height);
}




FX_ENTRY void FX_CALL grColorCombine
(
    GrCombineFunction_t function,
    GrCombineFactor_t factor,
    GrCombineLocal_t local,
    GrCombineOther_t other,
    FxBool invert
)
{
    fxs.constant_write_mask &= 0xFF000000;

    switch(PFX_COLOR_STATE(function, factor, local, other))
    {
        case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE):
        {
        } break;

        case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
        {
            fxs.constant_write_mask |= 0x00FFFFFF;
        } break;

        default:
            ASSERT(false);
    }
}


FX_ENTRY void FX_CALL grAlphaCombine
(
    GrCombineFunction_t function,
    GrCombineFactor_t factor,
    GrCombineLocal_t local,
    GrCombineOther_t other,
    FxBool invert
)
{
    switch(PFX_COLOR_STATE(function, factor, local, other))
    {
        case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
        case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
    {
    } break;

    default:
        ASSERT(false);
    }
}


FX_ENTRY void FX_CALL grAlphaBlendFunction
(
    GrAlphaBlendFnc_t rgb_sf,
    GrAlphaBlendFnc_t rgb_df,
    GrAlphaBlendFnc_t alpha_sf,
    GrAlphaBlendFnc_t alpha_df
)
{
    fxs.constant_write_mask &= 0x00FFFFFF;

    switch(PFX_BLEND_STATE(rgb_sf, rgb_df, alpha_sf, alpha_df))
    {
        case PFX_BLEND_STATE(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            fxs.blend = GFX_BLEND_OPAQUE;
        } break;

        case PFX_BLEND_STATE(GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            fxs.blend = GFX_BLEND_ADDITIVE;
        } break;

        case PFX_BLEND_STATE(GR_BLEND_ZERO, GR_BLEND_SRC_COLOR, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            fxs.blend = GFX_BLEND_MODULATE;
        } break;

        case PFX_BLEND_STATE(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            fxs.blend = GFX_BLEND_ALPHA;
            fxs.constant_write_mask = 0xFF000000;
            break;
        }

    default:
        DebugBreak();
    }
}


FX_ENTRY void FX_CALL grConstantColorValue(GrColor_t value)
{
    fxs.constant_color = (value >> 8) | (value << 24);
}


FX_ENTRY void FX_CALL grChromakeyMode(GrChromakeyMode_t mode)
{
  
}

FX_ENTRY void FX_CALL grChromakeyValue(GrColor_t value)
{

}


//
// Device access
//
FX_ENTRY void FX_CALL grTexDownloadTable(GrTexTable_t type, void* data)
{
    DxStateSetPalette(data);
    memcpy(fxs.color_palette, data, sizeof(fxs.color_palette));
}


FX_ENTRY void FX_CALL grLoadGammaTable
(
    FxU32 nentries,
    FxU32* red,
    FxU32* green,
    FxU32* blue
)
{

}



//
// Frame-buffer access
//
FX_ENTRY FxBool FX_CALL grLfbLock
(
    GrLock_t type,
    GrBuffer_t buffer,
    GrLfbWriteMode_t writeMode,
    GrOriginLocation_t origin,
    FxBool pixelPipeline,
    GrLfbInfo_t* info
)
{
    return true;
}

FX_ENTRY FxBool FX_CALL grLfbUnlock
(
    GrLock_t type,
    GrBuffer_t buffer
)
{
    return true;
}



//
// Device caps
//
FX_ENTRY FxU32 FX_CALL grTexMinAddress(GrChipID_t tmu)
{
    return 0;
}

FX_ENTRY FxU32 FX_CALL grTexMaxAddress(GrChipID_t tmu)
{
    return TMU_BANK_SZ - (2 * TMU_TEX_ALIGN);
}

FX_ENTRY FxU32 FX_CALL grGet
(
    FxU32 pname,
    FxU32 plength,
    FxI32 *params
)
{
    LOG("grGet: %u (len:%u)", pname, plength);

    switch(pname)
    {
        case GR_NUM_BOARDS: *params = 1; 
            return 4;
        case GR_MEMORY_UMA: *params = 0; 
            return 4;
        case GR_NUM_FB: *params = 1; 
            return 4;
        case GR_NUM_TMU: *params = 1; 
            return 4;
        case GR_TEXTURE_ALIGN: *params = TMU_TEX_ALIGN;
            return 4;
        case GR_MAX_TEXTURE_SIZE: *params = 256;
            return 4;
        case GR_MAX_TEXTURE_ASPECT_RATIO: *params = 3;
            return 4;
        case GR_GAMMA_TABLE_ENTRIES: *params = 256;
            return 4;
        case GR_BITS_GAMMA: *params = 8;
            return 4;
    }

    ASSERT(false);
}



extern "C" void FX_CALL grNoImpl() { DebugBreak(); }
extern "C" void FX_CALL grUnused0() { }
extern "C" void FX_CALL grUnused4(int) { }
extern "C" void FX_CALL grUnused8(int, int) { }
extern "C" void FX_CALL grUnused12(int, int, int) {  }
extern "C" void FX_CALL grUnused16(int, int, int, int) { }
extern "C" void FX_CALL grUnused28(int, int, int, int, int, int, int) { }



void GetTexDim(const GrTexInfo* info, FxU32* w, FxU32* h)
{
    FxU32 ww = 1 << info->largeLodLog2;
    switch (info->aspectRatioLog2)
    {
    case GR_ASPECT_LOG2_1x1:
        *w = ww;
        *h = ww;
        break;
    case GR_ASPECT_LOG2_1x2:
        *w = ww / 2;
        *h = ww;
        break;
    case GR_ASPECT_LOG2_2x1:
        *w = ww;
        *h = ww / 2;
        break;
    case GR_ASPECT_LOG2_1x4:
        *w = ww / 4;
        *h = ww;
        break;
    case GR_ASPECT_LOG2_4x1:
        *w = ww;
        *h = ww / 4;
        break;
    case GR_ASPECT_LOG2_1x8:
        *w = ww / 8;
        *h = ww;
        break;
    case GR_ASPECT_LOG2_8x1:
        *w = ww;
        *h = ww / 8;
        break;
    default:
        *w = 0;
        *h = 0;
        break;
    }
}

