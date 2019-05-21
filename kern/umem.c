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

#include <xenus/process.h>
#include <xenus/printf.h>
#include <xenus/panic.h>
#include <xenus/page.h>
#include <xenus/umem.h>
#include <string.h>
#include <errno.h>

void *kaddr(void *uaddr)
{
	return (char *)uaddr + USER_BASE;
}

int fucpy(void *dst, void *src, unsigned int len)
{
	if (urchk(src, len))
		return EFAULT;
	
	memcpy(dst, kaddr(src), len);
	return 0;
}

int tucpy(void *dst, void *src, unsigned int len)
{
	if (uwchk(dst, len))
		return EFAULT;
	
	memcpy(kaddr(dst), src, len);
	return 0;
}

int uset(void *ptr, int c, unsigned int len)
{
	if (uwchk(ptr, len))
		return EFAULT;
	
	memset(kaddr(ptr), c, len);
	return 0;
}

int fustr(char *dst, char *src, unsigned int len)
{
	int err;
	
	while (len-- > 0)
	{
		err = fucpy(dst++, src++, 1);
		if (err)
			return err;
		
		if (!dst[-1])
			return 0;
	}
	return EFAULT;
}

static int growstk(unsigned a, unsigned len)
{
	unsigned pg, end;
	unsigned i;
	
	if (curr->astk > USER_SIZE)
	{
		printf("astk %p\n", (void *)curr->astk);
		panic("bad astk");
	}
	
	if (a >= curr->astk)
		return 0;
	
	end = curr->astk >> 12;
	pg  = a >> 12;
	
	for (i = pg; i < end; i++)
		if (!curr->ptab[i])
		{
			void *p;
			
			p = zpalloc();
			if (!p)
				return ENOMEM;
			
			curr->ptab[i] = (unsigned)p | 7;
			curr->size++;
		}
	curr->astk = a & ~4095;
	return 0;
}

int urchk(void *ptr, unsigned int len)
{
	unsigned a = (unsigned)ptr;
	unsigned pg, end;
	
	if (a > USER_SIZE || len > USER_SIZE)
		return EFAULT;
	
	if (a >= curr->stk)
	{
		if (a + len > USER_SIZE)
			return EFAULT;
		return growstk(a, len);
	}
	
	if (a >= curr->base && a < curr->end)
	{
		if (a + len > curr->end)
			return EFAULT;
		return 0;
	}
	
	if (a < curr->brk)
	{
		if (len > curr->brk || a + len > curr->brk)
			return EFAULT;
		return 0;
	}
	
	end = (a + len - 1) >> 12;
	for (pg = a >> 12; pg <= end; pg++)
		if (!curr->ptab[pg])
			return EFAULT;
	return 0;
}

int uwchk(void *ptr, unsigned int len)
{
	return urchk(ptr, len);
}

int ufault(unsigned a)
{
	void *p;
	
	if (a < USER_BASE)
		return EFAULT;
	a -= USER_BASE;
	
	if (a < curr->stk || a >= USER_SIZE)
		return EFAULT;
	
	p = zpalloc();
	if (!p)
		return ENOMEM;
	
	curr->ptab[a >> 12] = (unsigned)p | 7;
	pg_update();
	return 0;
}
