/*
 * gfx.h
 *
 */
#pragma once
#include "pce\pce.h"
#include "dx.h"
#include "tcache.h"



enum gfx_blend
{
	GFX_BLEND_OPAQUE    = 0,
	GFX_BLEND_ADDITIVE  = 1,
	GFX_BLEND_MODULATE  = 2,
	GFX_BLEND_ALPHA     = 3,
	GFX_BLEND_UNDEFINED = 4,
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


union cmd_pso
{
	struct
	{
		u32 ta_x : 8,
			ta_y : 8,
			ta_t : 11,
			blend : 2;
	};
};





gfx_vert* gfx_vb_write_begin();
void gfx_vb_write_end(u32 count);

void gfx_draw_tri(gfx_tex* tex, gfx_blend blend, u8 v0, u8 v1, u8 v2);

void gfx_flush();
u32 gfx_frame_seq_num();