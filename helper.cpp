/*
 * helper.cpp
 *
 */
#include "helper.h"
#include <winternl.h>
#include <windows.h>




// Helpers
#define RVA(image_base, rva_addr) (((uiptr) image_base) + ((uiptr) rva_addr))




static bool NameCompare(const char* a, const char* b)
{
	while(*a && *b && (*a++ & 0xDF) == (*b++ & 0xDF));

	return (*a == 0 && *b == 0);
}


static bool SymbolCompare(void* ImageBase, DWORD ImageSymbol, const char* UserSymbol)
{
	return (ImageSymbol & IMAGE_ORDINAL_FLAG) ? IMAGE_ORDINAL((uiptr) UserSymbol) == IMAGE_ORDINAL(ImageSymbol) : (uiptr(UserSymbol) & ~0xFFFF) && NameCompare(UserSymbol, (const char*) ((IMAGE_IMPORT_BY_NAME*) RVA(ImageBase, ImageSymbol))->Name);
}


void** PsLookupImport(const void* Module, const char* Library, const char* Symbol)
{
	byte* ImageBase = (byte*) Module;


	union
	{
		byte* p;
		IMAGE_DOS_HEADER* DOS;
		IMAGE_NT_HEADERS* NT;
		IMAGE_IMPORT_DESCRIPTOR* IDT;
	};

	if(DOS->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	p += DOS->e_lfanew;

	if(NT->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	if(!NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
		return nullptr;

	p = ImageBase + NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	while(IDT->Characteristics)
	{
		if(!Library || NameCompare(Library, (const char*) RVA(ImageBase, IDT->Name)))
		{
			IMAGE_THUNK_DATA* ILT = (IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->OriginalFirstThunk);

			while(ILT->u1.Function)
			{
				if(SymbolCompare(ImageBase, ILT->u1.Ordinal, Symbol))
				{
					return (void**) &(((IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->FirstThunk))[(ILT - ((IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->OriginalFirstThunk)))].u1.AddressOfData);
				}

				ILT++;
			}
		}

		IDT++;
	}

	return nullptr;
}


u32 PsWriteImport(const void* Module, const char* Library, const char* Symbol, uiptr Data)
{
	byte* ImageBase = (byte*) Module;
	u32 EntryCount = 0;
	DWORD PrevProt;
	

	union
	{
		byte* p;
		IMAGE_DOS_HEADER* DOS;
		IMAGE_NT_HEADERS* NT;
		IMAGE_IMPORT_DESCRIPTOR* IDT;
	}; p = ImageBase;

	if(DOS->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	p += DOS->e_lfanew;

	if(NT->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	if(!NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
		return 0;

	auto DirSize = NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	auto DirRVA = NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	p = (byte*) RVA(ImageBase, DirRVA);


	while(IDT->Characteristics)
	{
		if(!Library || NameCompare(Library, (const char*) RVA(ImageBase, IDT->Name)))
		{
			IMAGE_THUNK_DATA* ILT = (IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->OriginalFirstThunk);

			while(ILT->u1.Function)
			{
				if(SymbolCompare(ImageBase, ILT->u1.Ordinal, Symbol))
				{
					auto Addr = (void**) &(((IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->FirstThunk))[(ILT - ((IMAGE_THUNK_DATA*) RVA(ImageBase, IDT->OriginalFirstThunk)))].u1.AddressOfData);

					VirtualProtect(Addr, sizeof(uiptr), PAGE_READWRITE, &PrevProt);
					*((uiptr*) Addr) = Data;
					VirtualProtect(Addr, sizeof(uiptr), PrevProt, &PrevProt);

					EntryCount++;
				}

				ILT++;
			}
		}

		IDT++;
	}

	return EntryCount;
}