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

#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"

struct mhead mhead =
{
	.size = sizeof(struct mhead),
};

void *malloc(size_t size)
{
	struct mhead *mh = &mhead;
	struct mhead *last;
	
	size += sizeof(struct mhead);
	size +=  15;
	size &= ~15;
	
	while (mh)
	{
		if (mh->free && mh->next && mh->next->free && (char *)mh + mh->size == (char *)mh->next)
		{
			mh->next->magic = 0;
			mh->size += mh->next->size;
			mh->next  = mh->next->next;
			continue;
		}
		
		if (mh->free && mh->size >= sizeof(struct mhead) + size)
		{
			struct mhead *fmh;
			
			fmh = (struct mhead *)((char *)mh + size);
			fmh->magic = MMAG;
			fmh->free = 1;
			fmh->size = mh->size - size;
			fmh->next = mh->next;
			mh->free  = 0;
			mh->size  = size;
			mh->next  = fmh;
			return (void *)(mh + 1);
		}
		
		if (mh->free && mh->size >= size)
		{
			mh->free = 0;
			return (void *)(mh + 1);
		}
		
		last = mh;
		mh = mh->next;
	}
	
	mh = sbrk(size);
	if ((int)mh == -1)
		return NULL;
	
	last->next = mh;
	mh->magic = MMAG;
	mh->size = size;
	mh->next = NULL;
	mh->free = 0;
	return (void *)(mh + 1);
}
