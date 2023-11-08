/*
 * pce.h
 *
 */
#pragma once
#include <intrin.h>
#include <memory.h>
#pragma warning(disable:4200) /* Zero-sized arrays */



// Various global assumptions
#define CACHE_LINE_SZ  64
#define SMP_PREFER_SZ  128 /* Streaming granularity on P4+ */
#define cache_align  __align(CACHE_LINE_SZ)
#define smp_align   __align(SMP_PREFER_SZ)

// Basic types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef u8 byte;

typedef size_t uint;
typedef size_t uiptr;

// Endian qualified types.
typedef u32 be32;
typedef u16 be16;
typedef u32 le32;
typedef u16 le16;

// Endianness
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x)  _byteswap_ulong(x)

// Alignment
#define __align(x) __declspec(align(x))
#define ispow2(x) ((x & (x - 1)) == 0)

// Rounding
#define round_up(n, m) (((((size_t) (n)) + (m) - 1) / m) * m)
#define round_down(n, m) ((((size_t) (n)) / (m)) * (m))

// Minmax
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

// Prefetching
#define prefetch(addr) _mm_prefetch((const char*) addr, _MM_HINT_T0)
#define prefetcht1(addr) _mm_prefetch((const char*) addr, _MM_HINT_T1)
#define prefetcht2(addr) _mm_prefetch((const char*) addr, _MM_HINT_T2)
#define prefetchw(addr) _m_prefetchw((const volatile void*) addr)

// Bitscan
static inline u32 _bsr(u32 mask)
{
	unsigned long idx;
	_BitScanReverse(&idx, mask);
	return (u32) idx;
}

// Memory
#define memzero(dst, len) memset(dst, 0, len)

// Pointer to container.
#define container_of(address, type, field) ((type*) ((byte*)(address) - (uiptr)(&((type *)0)->field)))

// Circular lists.
struct dlink
{
	dlink* next;
	dlink* prev;
};


struct slink
{
	slink* next;
};

#define list_is_empty(lst) (((slink*) lst)->next == ((slink*) lst))

static inline void list_insert(dlink* dst, dlink* src)
{
	src->next = dst->next;
	dst->next->prev = src;
	dst->next = src;
	src->prev = dst;
}

static inline void list_remove(dlink* entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	entry->next = nullptr;
	entry->prev = nullptr;
}

static inline void* __list_pop(dlink* lst)
{
	auto r = lst->next;
	list_remove(r);
	return r;
}

#define list_insert_head(dst, src) list_insert(dst, src)
#define list_insert_tail(dst, src) list_insert((dst)->prev, src)
#define list_pop(lst, type, link) container_of(__list_pop(lst), type, link)

static inline void slist_insert(slink* dst, slink* src)
{
	src->next = dst->next;
	dst->next = src;
}

static inline void slist_remove(slink* prior)
{
	prior->next = prior->next->next;
}

static inline void* __slist_pop(slink* lst)
{
	auto r = lst->next;
	lst->next = r->next;
	return r;
}

#define slist_insert_head(dst, src) slist_insert(dst ,src)
#define slist_pop(lst, type, link) ((type*) container_of(__slist_pop(lst), type, link))

#define list_get_head(lst, type, link) container_of((lst)->next, type, link)


static inline void list_init(dlink* lst)
{
	lst->next = lst;
	lst->prev = lst;
}

static inline void slist_init(slink* lst)
{
	lst->next = lst;
}

#define list_iterate(list, link, type) for(type* p = (type*) (list)->next, *__nxt = nullptr; p != (type*) (list) ? __nxt = (type*) ((slink*) p)->next, (p = container_of(p, type, link)) : false; p = __nxt)