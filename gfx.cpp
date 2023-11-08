/*
 * gfx.cpp
 *
 */
#include "gfx.h"
#include "dx.h"
#include "d2fx.h"


#define MAX_CMDLIST_SZ  0x10000
#define MAX_VB_SZ       0x10000



gfx_cmd cmdl[MAX_CMDLIST_SZ];
gfx_vert vertex_buffer[MAX_VB_SZ];
u16 cmdl_len;
u32 vb_len;




gfx_vert* gfx_vb_write_begin()
{
	return &vertex_buffer[vb_len];
}


void gfx_vb_write_end(u32 count)
{
	vb_len += count;
}


void gfx_draw_tri(void* tex_srv, gfx_blend blend, u8 v0, u8 v1, u8 v2)
{
	gfx_cmd& cmd = cmdl[cmdl_len++];

	cmd.voff = vb_len;
	cmd.tri.v0 = v0;
	cmd.tri.v1 = v1;
	cmd.tri.v2 = v2;

	cmd.tex_srv = tex_srv;
	cmd.blend = blend;
}


void gfx_flush()
{
	void* pVB;
	void* pIB;

	
	LOG("Flushing: %u vertices; %u commands", vb_len, cmdl_len);

	DxStartVertices(pVB, pIB);

	u16* dst_ib = (u16*) pIB;
	gfx_vert* dst_vb = (gfx_vert*) pVB;
	u16 dst_vb_len = 0;

	u16 z_value = 1;

	//for(s32 i = 0; i < cmdl_len; i++)
	for(s32 i = cmdl_len-1; i >= 0; i--)
	{
		auto& cmd = cmdl[i];
		gfx_vert* src;
		gfx_vert* dst;

		src = &vertex_buffer[cmd.voff+cmd.tri.v0];
		dst = &dst_vb[dst_vb_len];

		*dst = *src;
		*dst_ib++ = dst_vb_len++;
		dst->z = z_value;

		src = &vertex_buffer[cmd.voff+cmd.tri.v1];
		dst = &dst_vb[dst_vb_len];

		*dst = *src;
		*dst_ib++ = dst_vb_len++;
		dst->z = z_value;

		src = &vertex_buffer[cmd.voff+cmd.tri.v2];
		dst = &dst_vb[dst_vb_len];

		*dst = *src;
		*dst_ib++ = dst_vb_len++;
		dst->z = z_value;

		cmd.voff = dst_vb_len - 3;

		z_value++;
	}

	DxEndVertices();

	//for(u32 i = 0; i < cmdl_len; i++)
	for(s32 i = cmdl_len-1; i >= 0; i--)
	{
		auto& cmd = cmdl[i];

		DxStateSetTexture(cmd.tex_srv);
		DxSetBlend(cmd.blend);
		DxDrawIndexed(cmd.voff, 3);
	}

	vb_len = 0;
	cmdl_len = 0;
}