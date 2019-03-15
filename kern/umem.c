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
#include <xenus/malloc.h>
#include <xenus/umem.h>
#include <string.h>
#include <errno.h>

int fucpy(void *dst, void *src, unsigned int len)
{
	if (urchk(src, len))
		return EFAULT;
	memcpy(dst, (char *)src + curr->base, len);
	return 0;
}

int tucpy(void *dst, void *src, unsigned int len)
{
	if (uwchk(dst, len))
		return EFAULT;
	memcpy((char *)dst + curr->base, src, len);
	return 0;
}

int uset(void *ptr, int c, unsigned int len)
{
	if (uwchk(ptr, len))
		return EFAULT;
	memset((char *)ptr + curr->base, c, len);
	return 0;
}

int fustr(char *dst, char *src, unsigned int len)
{
	while (len--)
	{
		if ((unsigned)src > curr->size)
			return EFAULT;
		*dst = src[curr->base];
		if (!*dst)
			return 0;
		dst++;
		src++;
	}
	
	return EFAULT;
}

int usdup(char **dst, char *src, unsigned len)
{
	unsigned l;
	char *p;
	
	for (p = src + curr->base, l = 1; l < len; l++, p++)
		if (!*p)
			break;
	if (l >= len)
		return EFAULT;
	
	p = malloc(l);
	if (!p)
		return ENOMEM;
	
	memcpy(p, src + curr->base, l);
	*dst = p;
	return 0;
}

int urchk(void *ptr, unsigned int len)
{
	if ((unsigned int)ptr > curr->size || len > curr->size)
		return EFAULT;
	if ((unsigned int)ptr + len > curr->size)
		return EFAULT;
	return 0;
}

int uwchk(void *ptr, unsigned int len)
{
	return urchk(ptr, len);
}
