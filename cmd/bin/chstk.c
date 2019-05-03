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

#include <xenus/exec.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	struct exehdr hdr;
	unsigned sz = 0;
	int fd;
	int x = 0;
	int i = 1;
	
	if (argc < 2)
	{
		fprintf(stderr,"chstk [-newsize] binfile1...\n");
		return 1;
	}
	
	if (*argv[i] == '-')
		sz = atol(argv[i++] + 1);
	else
		puts("FILE           BASE    START      END    STACK");
	
	for (; i < argc; i++)
	{
		fd = open(argv[i], sz ? O_RDWR : O_RDONLY);
		if (fd < 0)
		{
			perror(argv[1]);
			x = 1;
			continue;
		}
		
		memset(&hdr, 0, sizeof hdr);
		
		if (read(fd, &hdr, sizeof(hdr)) < 0)
		{
			perror(argv[1]);
			close(fd);
			x = 1;
			continue;
		}
		
		if (memcmp(hdr.magic, "XENUS386", 8))
		{
			fprintf(stderr, "%s: bad magic\n", argv[i]);
			x = 1;
			goto next;
		}
		
		if (!sz)
			printf("%-10s %8u %8u %8u %8u\n", argv[i], hdr.base, hdr.start, hdr.end, hdr.stack);
		
		if (sz)
		{
			hdr.stack = sz;
			
			if (lseek(fd, (off_t)0, SEEK_SET))
			{
				perror(argv[i]);
				x = 1;
				goto next;
			}
			
			if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
			{
				perror(argv[i]);
				x = 1;
				goto next;
			}
		}
next:
		close(fd);
	}
	return x;
}
