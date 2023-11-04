
#include "3dfx/glide.h"
#include "dx.h"
#include "d2fx.h"
#include <Windows.h>



// - The palette entry for the conventional transparent (color key) is index zero.
// - The color at 0,0 is zero (indexed) for all textures.



// Configuration
#define TMU_MEM_SZ     (16 * 1024 * 1024)
#define TMU_ALIGNMENT  0x100
#define TMU_MAX_VADS  0x8000


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

struct tmu_tex
{
    tmu_tex* next;
    u32 size; /* Size in bytes */
    u32 width;
    u32 height;
    GrTexInfo info;
    void* pTexture;
    void* pTextureSRV;
    bool pending_delete;
    bool in_frame_list;
};


struct tmu_vad
{
    u32 addr;
    u32 tex : 31,
        used : 1;
};

tmu_tex tmu_tex_list[TMU_MEM_SZ];
tmu_vad tmu_vad_list[TMU_MAX_VADS];
tmu_tex* tmu_tex_next_free;
tmu_tex* tmu_tex_frame_list;



struct
{
    tmu_tex* tex;
    bool rgb_src_constant;
    bool rgb_src_texture;
    u32 constant_color;
    u32 constant_write_mask;
    u32 shift;

    u32 color_palette[0x100];

    struct
    {

    };
} state;



#define PFX_COLOR_STATE(function, factor, local, other) u16(function | (factor << 5) | (local << 9) | (other << 11))
#define PFX_BLEND_STATE(rgb_sf, rgb_df, alpha_sf, alpha_df) u16(rgb_sf | (rgb_df << 4) | (alpha_sf << 8) | (alpha_df << 12))



// Prototypes
void GetTexDim(const GrTexInfo* info, FxU32* w, FxU32* h);
tmu_tex* tmu_lookup(u32 addr);
void tmu_insert(u32 addr, GrTexInfo* info);


/*

*/

void vtv(DxVertex& out, vert& in)
{
    u16 u = in.u;
    u16 v = in.v;

    u -= (u >> 8);
    v -= (v >> 8);

    out.x = (s16) in.x;
    out.y = (s16) in.y;
    out.u = u >> state.shift;
    out.v = v >> state.shift;

    out.rgba = (in.c & 0x00FFFFFF) | (state.constant_color & state.constant_write_mask);
}


void DrawSubmit(DxVertex* verts, u32 count)
{
    if(state.rgb_src_constant)
    {
        
    }

    DxDraw(verts, count);
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

    Done = true;

    SetProcessDPIAware();

    for(int i = 0; i < ARRAYSIZE(tmu_tex_list); i++)
    {
        tmu_tex_list[i].next = tmu_tex_next_free;
        tmu_tex_next_free = &tmu_tex_list[i];
    }


    if(!DxInitialize((void*) hWnd))
        DebugBreak();

    return (GrContext_t) 1;
}



//
// Command submission
//
FX_ENTRY void FX_CALL grBufferSwap(FxU32 swap_interval)
{
    LOG("Present");

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
    const auto tris = (Count - 2);
    DxVertex* vert_buf = (DxVertex*) alloca(tris * 3 * sizeof(DxVertex));
    DxVertex* dst_verts = vert_buf;
    vert** src_verts = (vert**) pointers;

    //LOG("DrawVertexArray: mode:%u", mode);

    //for(int i = 0; i < Count; i++)
    //{
    //    LOG("%f,%f (%f,%f)", src_verts[i]->x, src_verts[i]->y, src_verts[i]->u, src_verts[i]->v);
    //}

    switch(mode)
    {
        case GR_TRIANGLE_FAN:
        {
            for(int i = 2; i < Count; i++)
            {
                vtv(*dst_verts++, *src_verts[0]);
                vtv(*dst_verts++, *src_verts[i-1]);
                vtv(*dst_verts++, *src_verts[i]);
            }
        } break;

        case GR_TRIANGLE_STRIP:
        {
            for(int i = 0; i < Count - 2; i++)
            {
                if(i % 2)
                {
                    vtv(*dst_verts++, *src_verts[i+1]);
                    vtv(*dst_verts++, *src_verts[i]);
                    vtv(*dst_verts++, *src_verts[i+2]);
                }

                else
                {
                    vtv(*dst_verts++, *src_verts[i]);
                    vtv(*dst_verts++, *src_verts[i+1]);
                    vtv(*dst_verts++, *src_verts[i+2]);
                }
            }
        } break;

        default:
            DebugBreak();
    }

    DrawSubmit(vert_buf, tris * 3);
}


FX_ENTRY void FX_CALL grDrawVertexArrayContiguous
(
    FxU32 mode,
    FxU32 Count,
    void* pointers,
    FxU32 stride
)
{
    const auto tris = (Count - 2);
    DxVertex* vert_buf = (DxVertex*) alloca(tris * 3 * sizeof(DxVertex));
    DxVertex* dst_verts = vert_buf;
    vert* src_verts = (vert*) pointers;


    //LOG("DrawVertexArrayContiguous: mode:%u", mode);

    //for(int i = 0; i < Count; i++)
    //{
    //    LOG("%f,%f (%f,%f)", src_verts[i].x, src_verts[i].y, src_verts[i].u, src_verts[i].v);
    //}


    switch(mode)
    {
    case GR_TRIANGLE_FAN:
    {
        for(int i = 2; i < Count; i++)
        {
            vtv(*dst_verts++, src_verts[0]);
            vtv(*dst_verts++, src_verts[i-1]);
            vtv(*dst_verts++, src_verts[i]);
        }
    } break;

    case GR_TRIANGLE_STRIP:
    {
        for(int i = 0; i < Count - 2; i++)
        {
            if(i % 2)
            {
                vtv(*dst_verts++, src_verts[i+1]);
                vtv(*dst_verts++, src_verts[i]);
                vtv(*dst_verts++, src_verts[i+2]);
            }

            else
            {
                vtv(*dst_verts++, src_verts[i]);
                vtv(*dst_verts++, src_verts[i+1]);
                vtv(*dst_verts++, src_verts[i+2]);
            }
        }
    } break;

    default:
        DebugBreak();
    }

    DrawSubmit(vert_buf, tris * 3);
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
    tmu_tex* tex = tmu_lookup(startAddress);

    tex->next = tmu_tex_frame_list;
    tmu_tex_frame_list = tex;

    state.tex = tex;

    DWORD Idx;
    _BitScanReverse(&Idx, max(tex->width, tex->height));
    state.shift = 8 - Idx;

    DxStateSetTexture(state.tex->pTextureSRV);
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
    state.constant_write_mask &= 0xFF000000;

    switch (PFX_COLOR_STATE(function, factor, local, other))
    {
    case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE):
    {
        state.rgb_src_texture = true;
    } break;

    case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
    {
        state.rgb_src_constant = true;

        state.constant_write_mask |= 0x00FFFFFF;
    } break;

    default:
        DebugBreak();
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
    switch (PFX_COLOR_STATE(function, factor, local, other))
    {
    case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
    case PFX_COLOR_STATE(GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_CONSTANT):
    {
    } break;

    default:
        DebugBreak();
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
    state.constant_write_mask &= 0x00FFFFFF;

    switch (PFX_BLEND_STATE(rgb_sf, rgb_df, alpha_sf, alpha_df))
    {
        case PFX_BLEND_STATE(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            LOG("Set blend: OPAQUE");
            DxSetBlend(DX_BLEND_OPAQUE);
        } break;

        case PFX_BLEND_STATE(GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            LOG("Set blend: ADDITIVE");
            DxSetBlend(DX_BLEND_ADDITIVE);
        } break;

        case PFX_BLEND_STATE(GR_BLEND_ZERO, GR_BLEND_SRC_COLOR, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            LOG("Set blend: MODULATE");
            DxSetBlend(DX_BLEND_MODULATE);
        } break;

        case PFX_BLEND_STATE(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO):
        {
            state.constant_write_mask = 0xFF000000;
            
            DxSetBlend(DX_BLEND_ALPHA);

            LOG("Set blend: SRC_ALPHA");
            break;
        }

    default:
        DebugBreak();
    }
}


FX_ENTRY void FX_CALL grConstantColorValue(GrColor_t value)
{
    state.constant_color = (value >> 8) | (value << 24);
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
FX_ENTRY void FX_CALL grTexDownloadMipMap
(
    GrChipID_t tmu,
    FxU32 startAddress,
    FxU32 evenOdd,
    GrTexInfo* info
)
{
    LOG("TMU download: 0x%X", startAddress);

    tmu_insert(startAddress, info);
}


FX_ENTRY void FX_CALL grTexDownloadTable(GrTexTable_t type, void* data)
{
    DxStateSetPalette(data);
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
    return TMU_MEM_SZ - (2 * TMU_ALIGNMENT);
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
    case GR_MAX_TEXTURE_SIZE:
        *params = 256;
        return 4;
    case GR_MAX_TEXTURE_ASPECT_RATIO:
        *params = 3;
        return 4;
    case GR_NUM_BOARDS:
        *params = 1;
        return 4;
    case GR_NUM_FB:
        *params = 1;
        return 4;
    case GR_NUM_TMU:
        *params = 1;
        return 4;
    case GR_TEXTURE_ALIGN:
        *params = TMU_ALIGNMENT;
        return 4;
    case GR_MEMORY_UMA:
        *params = 0;
        return 4;
    case GR_GAMMA_TABLE_ENTRIES:
        *params = 256;
        return 4;
    case GR_BITS_GAMMA:
        *params = 8;
        return 4;
    default:
        return 0;
    }
}



extern "C" void FX_CALL grNoImpl() { DebugBreak(); }
extern "C" void FX_CALL grUnused0() { }
extern "C" void FX_CALL grUnused4(int) { }
extern "C" void FX_CALL grUnused8(int, int) { }
extern "C" void FX_CALL grUnused12(int, int, int) {  }
extern "C" void FX_CALL grUnused16(int, int, int, int) { }
extern "C" void FX_CALL grUnused28(int, int, int, int, int, int, int) { }




tmu_tex* tmu_lookup(u32 addr)
{
    for(int i = 0; i < ARRAYSIZE(tmu_vad_list); i++)
    {
        tmu_vad& vad = tmu_vad_list[i];

        if(vad.used && vad.addr == addr)
        {
            if (!vad.tex)
                break;

            return &tmu_tex_list[vad.tex];
        }
    }

    return NULL;
}


void tmu_free_tex(tmu_tex* tex)
{
    if(tex->in_frame_list)
    {
        tex->pending_delete = true;
        return;
    }

    DxDestroyTexture(tex->pTexture, tex->pTextureSRV);

    tex->next = tmu_tex_next_free;
    tmu_tex_next_free = tex;
}


void tmu_insert(u32 addr, GrTexInfo* info)
{
    tmu_vad* new_vad = NULL;


    for(int i = 0; i < ARRAYSIZE(tmu_vad_list); i++)
    {
        tmu_vad& vad = tmu_vad_list[i];
        tmu_tex& tex = tmu_tex_list[vad.tex];

        if (vad.used)
        {
            if (addr >= vad.addr && addr < vad.addr + tex.size)
            {
                LOG("TMU free VAD: 0x%X", vad.addr);
                tmu_free_tex(&tex);
                vad.used = false;
                new_vad = &vad;
            }
        }

        else
            new_vad = &vad;
    }

    if (!new_vad)
        DebugBreak();

    tmu_tex* new_tex = tmu_tex_next_free;
    tmu_tex_next_free = new_tex->next;

    new_vad->addr = addr;
    new_vad->tex = new_tex - tmu_tex_list;
    new_vad->used = true;

    memzero(new_tex, sizeof(new_tex));
    new_tex->info = *info;
    GetTexDim(&new_tex->info, &new_tex->width, &new_tex->height);
    new_tex->size = new_tex->width * new_tex->height;

    DxCreateTexture(new_tex->width, new_tex->height, new_tex->info.data, &new_tex->pTexture, &new_tex->pTextureSRV);
}


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

