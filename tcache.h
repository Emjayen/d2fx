/*
 * tcache.h
 *
 */
#pragma once
#include "d2fx.h"


#define MAX_TILES    1024 /*0x800*/

typedef u16 htex;


struct gfx_tex
{
	slink freelnk;
	dlink lrulnk;
	dlink paginglnk;

	u8 vid_resident : 1; /* The texture has been promoted to the vidmem cache. */
	   

	u16 tmu_refs;

	struct
	{
		u32 cmd_refs; /* Count of times referenced in a graphics command-list. */
	} stats;

	u32 tile_idx;
	u32 frame_reference_stamp;
	u32 chash;
	u16 width;
	u16 height;
	u16 tileid;
	u8 tilex;
	u8 tiley;
	void* sysmem;

	void* texture_srv;

	bool dbg_used;
};


void tex_cmd_reference(gfx_tex* tex);


gfx_tex* tex_create(const void* data, u16 width, u16 height);
void tex_tmu_release(gfx_tex* tex);