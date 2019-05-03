/* Copyright (c) Piotr Durlej
 * All rights reserved
 */

#include <xenus/process.h>
#include <xenus/ualloc.h>
#include <xenus/page.h>
#include <string.h>
#include <errno.h>

int newregion(unsigned base, unsigned end)
{
	unsigned *pt = curr->ptab;
	unsigned i;
	char *p;
	
	if (base >= USER_SIZE || end >= USER_SIZE)
		return EINVAL;
	
	end--;
	base >>= 12;
	end  >>= 12;
	
	for (i = base; i <= end; i++)
		if (!pt[i])
		{
			p = zpalloc();
			if (!p)
				return ENOMEM;
			
			pt[i] = (unsigned)p | 7;
			curr->size++;
		}
	return 0;
}

int pcalloc(void)
{
	unsigned *pt = curr->ptab;
	int i;
	
	if (newregion(curr->base, curr->end))
		goto fail;
	if (newregion(0, curr->brk))
		goto fail;
	
	pg_update();
	return 0;
fail:
	for (i = 0; i < 1024; i++)
	{
		pfree((void *)pt[i]);
		pt[i] = 0;
	}
	pg_update();
	return ENOMEM;
}

void pcfree(void)
{
	int i;
	
	for (i = 0; i < 1024; i++)
		if (curr->ptab[i])
			pg_free(curr->ptab[i] >> 12);
	
	pfree(curr->ptab);
	curr->ptab = NULL;
}

int pccopy(unsigned *ntab)
{
	unsigned *stab = curr->ptab;
	void *sp, *dp;
	int i;
	
	for (i = 0; i < 1024; i++)
		if (stab[i])
		{
			sp = (unsigned *)(stab[i] & ~0xfff);
			dp = palloc();
			
			if (!dp)
				goto fail;
			
			ntab[i] = (unsigned)dp | 7;
			memcpy(dp, sp, 4096);
		}
	pg_update();
	return 0;
fail:
	for (; i >= 0; i--)
		if (ntab[i])
		{
			pg_free(ntab[i] >> 12);
			ntab[i] = 0;
		}
	pg_update();
	return ENOMEM;
}
