/*
 * dx.h
 *
 */
#pragma once
#include "pce/pce.h"
#include <d3d11.h>




bool DxInitialize(void* hWindow);

void DxCreateTexture(u32 Width, u32 Height, const void* pData, void** ppTexture, void** ppSrv);
void DxDestroyTexture(void* pTexture, void* pSrv);
void DxStateSetTexture(void* pTextureSrv);
void DxStateSetTexture(void* pTextureSrv);
void DxStateSetPalette(void* pPaletteData);
void DxBegin();
void DxEndAndPresent();

void DxStartVertices(void*& pVB, void*& pIB);
void DxEndVertices();
void DxDrawIndexed(u32 Offset, u32 Count);

void DxTextureCacheUpload(u32 ArrayIdx, u16 DstX, u16 DstY, u16 TexWidth, u16 TexHeight, const void* data);
void* DxGetTextureCacheSrv();


#define DX_BLEND_OPAQUE    0
#define DX_BLEND_ADDITIVE  1
#define DX_BLEND_MODULATE  2
#define DX_BLEND_ALPHA     3

void DxSetBlend(u8 Blend);