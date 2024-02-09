/*
 * tcache.cpp
 *
 */
#include "tcache.h"
#include "gfx.h"
#include "xxh/xxhash.h"
#include <algorithm>





// Prototypes
void tex_cache_release(gfx_tex* tex);


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

#define TILE_CX      256
#define TILE_CY      256

// Number of size classes, per dimension.
#define TEX_SIZE_BINS    6

// Size base, log2
#define TEX_SIZE_BASE  3 /* 8x8 */


struct tile
{
	slink freelnk;
	dlink binlnk;

	bool dbg_used;
};


struct tbin
{
	dlist<tile> tiles;
	dlist<gfx_tex> lru;

	u16 tile_next_idx;
	u16 tile_capacity;
	u16 tiles_allocated;
};

tbin bins[TEX_SIZE_BINS][TEX_SIZE_BINS];
tile tiles[MAX_TILES];
gfx_tex textures[MAX_TEXTURES];
slink texfl;
slink tilefl;

// Textures that are pending to become resident.
dlist<gfx_tex> paging_queue;



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
			list_init(&bins[y][x].tiles);
			list_init(&bins[y][x].lru);

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
	auto y = _bsf(cy) - TEX_SIZE_BASE;
	auto x = _bsf(cx) - TEX_SIZE_BASE;

	ASSERT(x < TEX_SIZE_BINS && y < TEX_SIZE_BINS);

	return bins[y][x];
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


void tcache_flush_paging_queue()
{
	list_iterate(&paging_queue, lrulnk, gfx_tex)
	{

	}
}


void validate_list(dlink& list)
{
	u32 count = 0;

	for(dlink* p = list.next; p != &list; p = p->next)
	{
		ASSERT(p->next->prev == p);

		count++;
	}

	for(dlink* p = list.prev; p != &list; p = p->prev)
	{
		ASSERT(p->prev->next == p);
		count--;
	}

	ASSERT(count == 0);
}


void check_bins()
{
	for(int i = 0; i < ARRAYSIZE(tiles); i++)
		tiles[i].dbg_used = false;

	for(int i = 0; i < ARRAYSIZE(textures); i++)
		textures[i].dbg_used = false;

	for(int y = 0; y < TEX_SIZE_BINS; y++)
		for(int x = 0; x < TEX_SIZE_BINS; x++)
		{
			tbin& bin = bins[y][x];

			list_iterate(&bin.tiles, binlnk, tile)
			{
				ASSERT(p->dbg_used == false);
				p->dbg_used = true;
			}

			list_iterate(&bin.lru, lrulnk, gfx_tex)
			{
				ASSERT(p->dbg_used == false);
				p->dbg_used = true;
			}
		}
}

void check_lists()
{
	for(int y = 0; y < TEX_SIZE_BINS; y++)
		for(int x = 0; x < TEX_SIZE_BINS; x++)
		{
			//////validate_list(bins[y][x].lru);
		}
}


void verify_unoccupied(gfx_tex* src, u16 tileid, u16 x, u16 y, u16 cx, u16 cy)
{
	for(u32 i = 0; i < ARRAYSIZE(textures); i++)
	{
		gfx_tex& tex = textures[i];

		if(!tex.width || !tex.vid_resident || &tex == src || tex.tileid != tileid)
			continue;

		if((x >= tex.tilex && x < tex.tilex+tex.width) && (y >= tex.tiley && y < tex.tiley+tex.height))
			__asm int 3
	}
}



gfx_tex* lookup_tex(u32 chash)
{
	for(int i = 0; i < ARRAYSIZE(textures); i++)
	{
		if(textures[i].chash == chash)
			return &textures[i];
	}

	return nullptr;
}



void tex_make_resident(gfx_tex* tex)
{
	LOG("Make resident [%u, %X] size:%u,%u tileid:%u tilexy:%u,%u", tex - textures, tex->chash, tex->width, tex->height, tex->tileid, tex->tilex, tex->tiley);


	auto& bin = get_tex_bin(tex->width, tex->height);


	// If the current tile of our pool isn't exhausted (or we are capable of allocating a further tile)
	if(bin.tile_next_idx != bin.tile_capacity || !list_is_empty(&tilefl))
	{
		if(bin.tile_next_idx == bin.tile_capacity)
		{
			list_insert_head(&bin.tiles, &slist_pop(&tilefl, tile, freelnk)->binlnk);
			check_lists();

			bin.tile_next_idx = 0;
			bin.tiles_allocated++;
		}

		// Allocate at next available location within the tile.
		tex->tile_idx = bin.tile_next_idx++;
		tex->tileid = get_tile_id(bin.tiles.next);
	}

	// We have to evict an existing.
	else
	{
		gfx_tex* old_tex = nullptr;


		if(list_is_empty(&bin.lru))
			__asm int 3

		list_iterate_back(&bin.lru, lrulnk, gfx_tex)
		{
			if(p->frame_reference_stamp == gfx_frame_seq_num())
			{
				LOG("Couldn't evict LRU; in use by frame; skip");
				continue;
			}

			old_tex = p;
			break;
		}

		LOG("Evict tex[%u, %X]", old_tex - textures, old_tex->chash);

		// Inherit old address.
		tex->tile_idx = old_tex->tile_idx;
		tex->tileid = old_tex->tileid;

		// Release the existing texture.
		list_remove(&old_tex->lrulnk);
		tex_cache_release(old_tex);

		check_lists();
	}

	// Assign new address.
	tile_idx_to_xy(tex->tile_idx, tex->width, tex->height, tex->tilex, tex->tiley);

	//verify_unoccupied(tex, tex->tileid, tex->tilex, tex->tiley, tex->width, tex->height);

	tex->vid_resident = true;

	ASSERT(tex->lrulnk.next == nullptr);

	list_insert_head(&bin.lru, &tex->lrulnk);
	check_lists();

	DxTextureCacheUpload(tex->tileid, tex->tilex, tex->tiley, tex->width, tex->height, tex->sysmem);
}




void tex_cmd_reference(gfx_tex* tex)
{
	tex->frame_reference_stamp = gfx_frame_seq_num();
	tex->stats.cmd_refs++;

	// Make resident if not already.
	if(!tex->vid_resident)
	{
		tex_make_resident(tex);
	}

	else
	{
		// Update LRU
		list_remove(&tex->lrulnk);
		list_insert_head(&get_tex_bin(tex->width, tex->height).lru, &tex->lrulnk);
	}

	check_lists();
}



gfx_tex* tex_create(const void* data, u16 width, u16 height)
{
	gfx_tex* tex;


	// Compute content hash over the texture data.
	const auto chash = texhash(data, width, height);

	// Create-if-not-exists.
	if(!(tex = tcache_lookup(chash)))
	{
		// Allocate a new texture descriptor & setup.
		tex = slist_pop(&texfl, gfx_tex, freelnk);
		tex->width = width;
		tex->height = height;
		tex->chash = chash;
		tex->texture_srv = DxGetTextureCacheSrv();

		// Capture the surface data.
		memcpy((tex->sysmem = HeapAlloc(GetProcessHeap(), 0, width*height)), data, width*height);

		// Insert into the content lookup hash-table.
		tcache_insert(tex);

		LOG("Create tex [%u, %X]", tex-textures, tex->chash);
	}

	tex->tmu_refs++;

	return tex;
}


void tex_try_release(gfx_tex* tex)
{
	if(tex->tmu_refs == 0 && tex->vid_resident == false)
	{
		LOG("Released texture [%u]; lifetime refs: %u", tex - textures, tex->stats.cmd_refs);


		tcache_delete(tex);

		HeapFree(GetProcessHeap(), 0, tex->sysmem);

		ASSERT(tex->lrulnk.next == nullptr && tex->lrulnk.prev == nullptr && tex->freelnk.next == nullptr);

		memzero(tex, sizeof(*tex));
		slist_insert_head(&texfl, &tex->freelnk);

		
	}
}

void tex_tmu_release(gfx_tex* tex)
{
	ASSERT(tex->tmu_refs > 0);

	tex->tmu_refs--;

	tex_try_release(tex);
}


void tex_cache_release(gfx_tex* tex)
{
	tex->vid_resident = false;

	tex_try_release(tex);
}