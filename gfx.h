/*
 * gfx.h
 *
 */
#pragma once
#include "pce\pce.h"
#include "dx.h"



enum gfx_blend
{
	GFX_BLEND_OPAQUE    = 0,
	GFX_BLEND_ADDITIVE  = 1,
	GFX_BLEND_MODULATE  = 2,
	GFX_BLEND_ALPHA     = 3,
};


union gfx_vert
{
	struct
	{
		s16 x;
		s16 y;
		s16 z;
		u8 u;
		u8 v;
		u32 rgba;
		u16 tid;
	};

	struct
	{
		u32 done;
		u16 vidx;
	};
};


struct gfx_cmd
{
	u16 voff;
	u8 blend;
	void* tex_srv;


	struct
	{
		u16 v0 : 4,
			v1 : 4,
			v2 : 4;
	} tri;
};




gfx_vert* gfx_vb_write_begin();
void gfx_vb_write_end(u32 count);

void gfx_draw_tri(void* tex_srv, gfx_blend blend, u8 v0, u8 v1, u8 v2);

void gfx_flush();