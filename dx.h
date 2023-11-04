/*
 * dx.h
 *
 */
#pragma once
#include <pce/pce.h>


struct DxVertex
{
	s16 x;
	s16 y;
	u8 u;
	u8 v;
	u8 ck;
	u32 rgba;
};


bool DxInitialize(void* hWindow);

void DxCreateTexture(u32 Width, u32 Height, const void* pData, void** ppTexture, void** ppSrv);
void DxDestroyTexture(void* pTexture, void* pSrv);
void DxStateSetTexture(void* pTextureSrv);
void DxStateSetTexture(void* pTextureSrv);
void DxStateSetPalette(void* pPaletteData);
void DxDraw(DxVertex* pVertices, u32 Count);
void DxBegin();
void DxEndAndPresent();


#define DX_BLEND_OPAQUE    0
#define DX_BLEND_ADDITIVE  1
#define DX_BLEND_MODULATE  2
#define DX_BLEND_ALPHA     3

void DxSetBlend(u8 Blend);