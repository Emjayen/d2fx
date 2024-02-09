/*
 * gfx.cpp
 *
 */
#include "gfx.h"
#include "dx.h"
#include "d2fx.h"
#include <algorithm>


#define MAX_CMDLIST_SZ  0x10000
#define MAX_VB_SZ       0x10000



struct gfx_cmd
{
	union
	{
		struct
		{
			u32 seq : 20,
				blend : 2;
		};

		u32 sortval;
	} sort;

	u16 voff;
	u8 blend;
	gfx_tex* tex;

	struct
	{
		u16 v0 : 4,
			v1 : 4,
			v2 : 4;
	} tri;
};


gfx_cmd cmdl[MAX_CMDLIST_SZ];
gfx_vert vertex_buffer[MAX_VB_SZ];
u16 cmdl_len;
u32 vb_len;
u32 frame_seq_num;
u32 next_cmd_seq;

bool operator < (const gfx_cmd& lhs, const gfx_cmd& rhs)
{
	return lhs.sort.sortval > rhs.sort.sortval;

	//return (&lhs - cmdl) < (&rhs - cmdl);
}


gfx_vert* gfx_vb_write_begin()
{
	return &vertex_buffer[vb_len];
}


void gfx_vb_write_end(u32 count)
{
	vb_len += count;
}


void gfx_draw_tri(gfx_tex* tex, gfx_blend blend, u8 v0, u8 v1, u8 v2)
{
	gfx_cmd& cmd = cmdl[cmdl_len++];

	cmd.voff = vb_len;
	cmd.tri.v0 = v0;
	cmd.tri.v1 = v1;
	cmd.tri.v2 = v2;

	cmd.tex = tex;
	cmd.blend = blend;

	cmd.sort.seq = ++next_cmd_seq;
	cmd.sort.blend = ~blend;

	tex_cmd_reference(cmd.tex);
}


void batch_render()
{

	for(u32 i = 0; i < cmdl_len;)
	{
		u32 start_cmd = i;
		u8 blend_state = cmdl[i].blend;

		while(i < cmdl_len && cmdl[i].blend == blend_state)
			i++;

		u32 batch_size = i - start_cmd;

		DxSetBlend(blend_state);
		DxDrawIndexed(cmdl[start_cmd].voff, batch_size * 3);
	}

}


void write_vertex(gfx_cmd* __restrict cmd, gfx_vert*  __restrict dst, gfx_vert*  __restrict src)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = cmd->sort.seq;
	dst->u = sataddu8(src->u, cmd->tex->tilex);
	dst->v = sataddu8(src->v, cmd->tex->tiley);
	dst->rgba = src->rgba;
	dst->tid = cmd->tex->tileid;
}


void gfx_flush()
{
	void* pVB;
	void* pIB;

	

	DxStartVertices(pVB, pIB);

	u16* dst_ib = (u16*) pIB;
	gfx_vert* dst_vb = (gfx_vert*) pVB;
	u16 dst_vb_len = 0;

	u16 z_value = 1;


	std::sort(&cmdl[0], &cmdl[cmdl_len]);


	for(s32 i = 0; i < cmdl_len; i++)
	//for(s32 i = cmdl_len-1; i >= 0; i--)
	{
		auto& cmd = cmdl[i];
		gfx_vert* src;
		gfx_vert* dst;


		write_vertex(&cmd, &dst_vb[dst_vb_len], &vertex_buffer[cmd.voff+cmd.tri.v0]);
		dst_ib[0] = dst_vb_len++;

		write_vertex(&cmd, &dst_vb[dst_vb_len], &vertex_buffer[cmd.voff+cmd.tri.v1]);
		dst_ib[1] = dst_vb_len++;

		write_vertex(&cmd, &dst_vb[dst_vb_len], &vertex_buffer[cmd.voff+cmd.tri.v2]);
		dst_ib[2] = dst_vb_len++;

		cmd.voff = dst_vb_len - 3;
		dst_ib += 3;
	}

	DxEndVertices();

	//DxStateSetTextureCache();

	DxStateSetTexture(DxGetTextureCacheSrv());


	if(true)
		batch_render();
	
	else
	{
		for(u32 i = 0; i < cmdl_len; i++)
			//for(s32 i = cmdl_len-1; i >= 0; i--)
		{
			auto& cmd = cmdl[i];


			DxSetBlend(cmd.blend);
			DxDrawIndexed(cmd.voff, 3);
		}
	}

	vb_len = 0;
	cmdl_len = 0;
	frame_seq_num++;
	next_cmd_seq = 0;
}


u32 gfx_frame_seq_num()
{
	return frame_seq_num;
}