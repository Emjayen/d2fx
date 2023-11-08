/*
 * tcache.h
 *
 */
#pragma once
#include "d2fx.h"




typedef u16 htex;


struct gfx_tex
{
	union
	{
		slink tilelnk;
		slink freelnk;
	};

	dlink lrulnk;
	

	u32 chash;
	u16 width;
	u16 height;
	u16 tileid;
	u8 tilex;
	u8 tiley;
};

