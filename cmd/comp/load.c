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

#include <xenus/selector.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "mnxcompat.h"
#include "aout.h"

int _newregion(void *base, size_t len);

static void loadsep(struct aout *hdr, int fd)
{
	if (lseek(fd, hdr->hlen, SEEK_SET) < 0)
		panic("mnx load seek");
	
	if (_newregion((void *)STXT, hdr->text))
		panic("mnx load text alloc");
	
	if (read(fd, (void *)STXT, hdr->text) != hdr->text)
		panic("mnx load text");
	
	if (brk((void *)(hdr->data + hdr->bss)))
		panic("mnx load brk");
	
	if (read(fd, 0, hdr->data) != hdr->data)
		panic("mnx load data");
	
	ibrk = hdr->data + hdr->bss;
	entry = hdr->entry;
	cs = USER_SEP_CS;
}

static void loadcom(struct aout *hdr, int fd)
{
	if (brk((void *)(hdr->text + hdr->data + hdr->bss)))
		panic("mnx load brk");
	
	if (lseek(fd, hdr->hlen, SEEK_SET) < 0)
		panic("mnx load seek");
	
	if (read(fd, NULL, hdr->text + hdr->data) != hdr->text + hdr->data)
		panic("mnx load common");
	
	ibrk = hdr->text + hdr->data + hdr->bss;
	entry = hdr->entry;
	cs = USER_CS;
}

void load(char *path, int fd)
{
	struct aout hdr;
	
	if (read(fd, &hdr, sizeof hdr) != sizeof hdr)
		panic("mnx hdr");
	
	if (hdr.flags & SEPID)
		loadsep(&hdr, fd);
	else
		loadcom(&hdr, fd);
	
	close(fd);
}
