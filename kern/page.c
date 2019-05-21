/* Copyright (c) Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <xenus/config.h>
#include <xenus/printf.h>
#include <xenus/panic.h>
#include <xenus/page.h>
#include <string.h>
#include <errno.h>

#define FIRSTPG	(256 + MAXPG / 1024)
#define MAXPG	16384

#define K 3

unsigned pg_flist[MAXPG];
unsigned pg_fcount;

void pg_asminit(void);

int pg_alloc(unsigned *pg)
{
	if (!pg_fcount)
		return ENOMEM;
	
	*pg = pg_flist[--pg_fcount];
	return 0;
}

void pg_free(unsigned pg)
{
	if (pg)
	{
#if FREEPOISON
		memset((void *)(pg << 12), 0x55, 4096);
#endif
		
		if (pg_fcount >= MAXPG)
			panic("pg_free");
		pg_flist[pg_fcount++] = pg;
	}
}

void *zpalloc(void)
{
	void *p;
	
	p = palloc();
	if (p)
		memset(p, 0, 4096);
	return p;
}

void *palloc(void)
{
	unsigned pg;
	
	if (pg_alloc(&pg))
		return NULL;
	return (void *)(pg << 12);
}

void pfree(void *p)
{
	pg_free((unsigned)p >> 12);
}

void *sbrk(int incr)
{
	static unsigned bp = HEAP_BASE;
	unsigned nbp = bp + incr;
	unsigned i, lpg;
	
	if (nbp > HEAP_BASE + HEAP_SIZE || incr > HEAP_SIZE)
		return (void *)-1;
	if (incr > HEAP_SIZE)
		return (void *)-1;
	if (incr < 0)
		panic("sbrk");
	if (!incr)
		return (void *)bp;
	
	for (i = (bp >> 12); i <= (nbp >> 12); i++)
	{
		if (!pg_dir[i >> 10])
		{
			void *pg;
			int err;
			
			pg = zpalloc();
			if (!pg)
				goto fail;
			
			pg_dir[i >> 10] = (unsigned)pg | K;
			pg_update();
		}
		
		if (!pg_tab[i])
		{
			void *pg;
			int err;
			
			pg = palloc();
			if (!pg)
				goto fail;
			
			pg_tab[i] = (unsigned)pg | K;
			pg_update();
		}
	}
	bp = nbp;
	
	return (void *)(bp - incr);
fail:
	printf("heap full\n");
	return (void *)-1;
}

void pg_init(unsigned memsize)
{
	unsigned i, lpg = memsize / 4096 + 256;
	unsigned *pt = (unsigned *)ITAB_BASE;
	void *p;
	
#if FILLMEM
	memset((void *)0x100000, 0xaa, memsize);
#endif
	if (memsize < 524288)
		panic("phys mem size");
	
	if (lpg >= MAXPG)
	{
		printf("pg_init lpg %u maxpg %u\n", lpg, MAXPG);
		lpg = MAXPG - 1;
	}
	
	memset(pg_dir, 0, 4096);
	for (i = 0; i < MAXPG / 1024; i++)
		pg_dir[i] = ITAB_BASE + (i << 12) | K;
	pg_dir[PTAB_BASE >> 22] = PDIR_BASE | K;
	
	for (i = 1; i < MAXPG; i++)
		pt[i] = (i << 12) | K;
	pt[0] = 0;
	
	for (i = lpg - 1; i >= FIRSTPG; i--)
		pg_flist[pg_fcount++] = i;
	
	pg_asminit();
	
	p = zpalloc();
	if (!p)
		panic("pg_init heap tab");
}

void pg_add(unsigned base, unsigned size)
{
	unsigned b = (base + 4095) >> 12;
	unsigned e = (base + size) >> 12;
	unsigned i;
	
#if FILLMEM
	memset(base, 0xaa, size);
#endif
	
	for (i = b; i < e; i++)
		pg_flist[pg_fcount++] = i;
}
