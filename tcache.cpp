/*
 * tcache.cpp
 *
 */
#include "tcache.h"
#include "gfx.h"
#include "xxh/xxhash.h"
#include <algorithm>




 // 
 // #define GR_LOD_LOG2_256         0x8
 // #define GR_LOD_LOG2_128         0x7
 // #define GR_LOD_LOG2_64          0x6
 // #define GR_LOD_LOG2_32          0x5
 // #define GR_LOD_LOG2_16          0x4
 // #define GR_LOD_LOG2_8           0x3
 // #define GR_LOD_LOG2_4           0x2
 // #define GR_LOD_LOG2_2           0x1
 // #define GR_LOD_LOG2_1           0x0
 // 
 // #define GR_ASPECT_LOG2_8x1        3       /* 8W x 1H */
 // #define GR_ASPECT_LOG2_4x1        2       /* 4W x 1H */
 // #define GR_ASPECT_LOG2_2x1        1       /* 2W x 1H */
 // #define GR_ASPECT_LOG2_1x1        0       /* 1W x 1H */
 // #define GR_ASPECT_LOG2_1x2       -1       /* 1W x 2H */
 // #define GR_ASPECT_LOG2_1x4       -2       /* 1W x 4H */
 // #define GR_ASPECT_LOG2_1x8       -3       /* 1W x 8H */
 // 



#define TEX_HTBL_SZ  0x40000
#define MAX_TEXTURES 0x10000
#define MAX_TILES    0x800
#define TILE_CX      256
#define TILE_CY      256

// Number of size classes, per dimension.
#define TEX_SIZE_BINS    6

// Size base, log2
#define TEX_SIZE_BASE  3 /* 8x8 */


struct tile
{
	union
	{
		slink texs;
		slink freelnk;
	};

	dlink binlnk;
};


struct tbin
{
	dlink tiles;
	dlink lru;
	u16 tile_next_idx;
	u16 tile_capacity;
};

tbin bins[TEX_SIZE_BINS][TEX_SIZE_BINS];
tile tiles[MAX_TILES];
gfx_tex textures[MAX_TEXTURES];
slink texfl;
slink tilefl;


union hte
{
	struct
	{
		u32 chash;
		htex th;
		u16 valid;
	};

	u64 data;
} htbl[TEX_HTBL_SZ];


void expand_bin(tbin* bin)
{
	list_insert_head(&bin->tiles, &slist_pop(&tilefl, tile, freelnk)->binlnk);
}


void tcache_init()
{
	slist_init(&texfl);
	slist_init(&tilefl);


	for(int i = ARRAYSIZE(tiles)-1; i >= 0; i--)
		slist_insert_head(&tilefl, &tiles[i].freelnk);

	for(int i = ARRAYSIZE(textures)-1; i >= 0; i--)
		slist_insert_head(&texfl, &textures[i].freelnk);

	for(int y = 0; y < TEX_SIZE_BINS; y++)
		for(int x = 0; x < TEX_SIZE_BINS; x++)
		{
			bins[y][x].tile_capacity = (TILE_CX / (1<<TEX_SIZE_BASE+x)) * (TILE_CY / (1<<TEX_SIZE_BASE+y));
			bins[y][x].tile_next_idx = bins[y][x].tile_capacity;
		}


}


u32 hmask(u32 h)
{
	return h & (TEX_HTBL_SZ-1);
}

u32 query_psl(u32 idx)
{
	return hmask(idx - htbl[idx].chash);
}


gfx_tex* tcache_lookup(u32 chash)
{
	u32 idx = hmask(chash);
	u32 psl = 0;


	while(htbl[idx].valid)
	{
		if(psl > query_psl(idx))
			return nullptr;

		if(htbl[idx].chash == chash)
			return &textures[htbl[idx].th];

		idx = hmask(idx+1);
		psl++;
	}

	return nullptr;
}


void tcache_insert(gfx_tex* tex)
{
	hte entry = { tex->chash, (htex) (tex - textures), true };
	u32 idx = hmask(entry.chash);
	u32 psl = 0;
	u32 dst_psl;

	
	while(htbl[idx].valid)
	{
		if((dst_psl = query_psl(idx)) < psl)
		{
			std::swap(htbl[idx], entry);
			psl = dst_psl;
		}

		idx = hmask(idx+1);
		psl++;
	}

	htbl[idx].data = entry.data;
}


void tcache_delete(gfx_tex* tex)
{
	u32 chash = tex->chash;
	u32 idx = hmask(chash);
	u32 psl;


	while(htbl[idx].chash != chash)
		idx = hmask(idx+1);

	while(htbl[hmask(++idx)].valid && query_psl(idx) != 0)
		htbl[idx-1] = htbl[idx];

	htbl[idx-1].valid = false;
}


u32 texhash(const void* data, u16 cx, u16 cy)
{
	return XXH32(data, cx*cy, 0xDEADBEEF);
}


tbin& get_tex_bin(u16 cx, u16 cy)
{
	return bins[(_bsr(cy) >> TEX_SIZE_BASE)*TEX_SIZE_BINS][_bsr(cx) >> TEX_SIZE_BASE];
}

void tile_idx_to_xy(u16 idx, u16 cx, u16 cy, u8& x, u8& y)
{
	y = idx / (TILE_CX / cx);
	x = idx % (TILE_CX / cx);

	x *= cx;
	y *= cy;
}

u16 get_tile_id(void* t)
{
	return (u16) (container_of(t, tile, binlnk) - tiles);
}

gfx_tex* tex_fetch(const void* data, u16 cx, u16 cy)
{
	gfx_tex* tex;


	if(!(tex = tcache_lookup(texhash(data, cx, cy))))
	{
		auto& bin = get_tex_bin(cx, cy);

		if(bin.tile_next_idx != bin.tile_capacity || !list_is_empty(&tilefl))
		{
			if(bin.tile_next_idx == bin.tile_capacity)
				expand_bin(&bin);

			tex = slist_pop(&texfl, gfx_tex, freelnk);

			tex->width = cx;
			tex->height = cy;
			tex->tileid = get_tile_id(bin.tiles.next);

			tile_idx_to_xy(bin.tile_next_idx, cx, cy, tex->tilex, tex->tiley);
			
			list_insert_head(&bin.lru, &tex->lrulnk);

			bin.tile_next_idx++;
		}

		else
		{
			tex = list_pop(&bin.lru, gfx_tex, lrulnk);
		}


	}

	return tex;
}


