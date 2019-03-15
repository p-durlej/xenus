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

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

char *	 buf;
int	 bs = 512;
char *	 infile;
char *	 outfile;
unsigned count = -1;
int	 ifd = STDIN_FILENO;
int	 ofd = STDOUT_FILENO;
long	 seek = 0;
long	 skip = 0;

int do_dd(void)
{
	int ic	= 0;
	int pic	= 0;
	int oc	= 0;
	int poc	= 0;
	int x	= 0;
	int cnt;
	
	if (seek && lseek(ofd, bs * seek, SEEK_SET) < 0)
	{
		perror(outfile);
		return 1;
	}
	
	errno = 0;
	while (count)
	{
		cnt = read(ifd, buf, bs);
		if (cnt < 0)
		{
			perror(infile);
			x = 1;
			break;
		}
		if (!cnt)
			break;
		if (cnt == bs)
			ic++;
		else
			pic++;
		if (skip)
		{
			skip--;
			continue;
		}
		if (write(ofd, buf, cnt) != cnt)
		{
			perror(outfile);
			x = 1;
			break;
		}
		if (cnt == bs)
			oc++;
		else
			poc++;
		if (count != -1)
			count--;
	}
	
	fprintf(stderr, "%i+%i blocks in\n",  ic, pic);
	fprintf(stderr, "%i+%i blocks out\n", oc, poc);
	return x;
}

int main(int argc, char **argv)
{
	size_t c;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (!strncmp("if=", argv[i], 3))
		{
			infile = argv[i] + 3;
			continue;
		}
		
		if (!strncmp("of=", argv[i], 3))
		{
			outfile = argv[i] + 3;
			continue;
		}
		
		if (!strncmp("bs=", argv[i], 3))
		{
			bs = atol(argv[i] + 3);
			continue;
		}
		
		if (!strncmp("count=", argv[i], 6))
		{
			count = atol(argv[i] + 6);
			continue;
		}
		
		if (!strncmp("seek=", argv[i], 5))
		{
			seek = atol(argv[i] + 5);
			continue;
		}
		
		if (!strncmp("skip=", argv[i], 5))
		{
			skip = atol(argv[i] + 5);
			continue;
		}
		
		fprintf(stderr, "dd: invalid argument: %s\n", argv[i]);
		return 1;
	}
	
	buf = malloc(bs);
	if (!buf)
	{
		perror("dd: can't allocate buffer");
		return errno;
	}
	
	if (infile)
	{
		ifd = open(infile, O_RDONLY);
		if (ifd < 0)
		{
			perror(infile);
			return errno;
		}
	}
	else
		infile = "stdin";
	
	if (outfile)
	{
		ofd = open(outfile, O_WRONLY | O_CREAT, 0666);
		if (ofd < 0)
		{
			perror(outfile);
			return errno;
		}
	}
	else
		outfile = "stdout";
	
	return do_dd();
}
