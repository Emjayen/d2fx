/*
 * dbg.cpp
 *
 */
#include "dbg.h"
#include <Windows.h>





void DumpTexture(u32* pPalette, u8* pSurface, u32 Width, u32 Height)
{
	DWORD Result;
	HANDLE hFile;
	HANDLE hScn;
	void* pView;
	char Path[MAX_PATH];

	static u32 count;
	wsprintf(Path, "C:\\d2fxdump\\tex-%u.bmp", ++count);

	if((hFile = CreateFile(Path, GENERIC_ALL, NULL, NULL, CREATE_ALWAYS, NULL, NULL)) == INVALID_HANDLE_VALUE ||
		!(hScn = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(Width*Height*sizeof(u32)), NULL)) ||
		!(pView = MapViewOfFile(hScn, FILE_MAP_WRITE, 0, 0, 0)))
	{
		ASSERT(FALSE);
	}

	union
	{
		byte* pd;
		BITMAPFILEHEADER* hdr;
		BITMAPINFOHEADER* info;
		u32* px;
	}; pd = (byte*) pView;

	hdr->bfType = 'MB';
	hdr->bfOffBits = sizeof(*hdr) + sizeof(*info);
	hdr->bfSize = hdr->bfOffBits + (Width*Height*sizeof(u32));
	pd += sizeof(*hdr);

	info->biSize = sizeof(*info);
	info->biWidth = Width;
	info->biHeight = -Height;
	info->biPlanes = 1;
	info->biBitCount = 32;
	info->biCompression = BI_RGB;
	pd += sizeof(*info);
	
	for(u32 i = 0; i < Width*Height; i++)
	{
		*px++ = pPalette[pSurface[i]];
	}

	UnmapViewOfFile(pView);
	CloseHandle(hScn);
	CloseHandle(hFile);
}