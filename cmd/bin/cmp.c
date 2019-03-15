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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

static int sflag, lflag;

static int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'l':
			lflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '-':
			return 0;
		default:
			fprintf(stderr, "cmp: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

static int do_open(char *path)
{
	int fd;
	
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(1);
	}
	return fd;
}

static int do_read(int fd, void *buf, int len, char *name)
{
	int r;
	
	r = read(fd, buf, len);
	if (r < 0)
	{
		perror(name);
		exit(1);
	}
	return r;
}

int main(int argc, char **argv)
{
	unsigned char buf1[512], buf2[512];
	int cnt1, cnt2, cnt;
	int fd1, fd2;
	unsigned line = 1;
	unsigned off = 1;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	argc -= i;
	argv += i;
	
	if (argc != 2)
	{
		fputs("cmp [-ls] file1 file2\n", stderr);
		return 1;
	}
	
	fd1 = do_open(argv[0]);
	fd2 = do_open(argv[1]);
	
	for (;;)
	{
		cnt1 = do_read(fd1, buf1, sizeof buf1, argv[0]);
		cnt2 = do_read(fd2, buf2, sizeof buf2, argv[1]);
		
		if (!cnt1 && !cnt2)
			break;
		
		cnt = cnt1 < cnt2 ? cnt1 : cnt2;
		for (i = 0; i < cnt; i++, off++)
		{
			if (buf1[i] == '\n')
				line++;
			if (buf1[i] == buf2[i])
				continue;
			
			if (lflag)
			{
				printf("%6u %3o %3o\n", off, buf1[i], buf2[i]);
				continue;
			}
			
			if (!sflag)
				printf("%s %s differ: char %u, line %u\n",
					argv[0], argv[1], off, line);
			return 1;
		}
		
		if (cnt1 < cnt2)
		{
			printf("cmp: EOF on %s\n", argv[0]);
			return 1;
		}
		
		if (cnt1 > cnt2)
		{
			printf("cmp: EOF on %s\n", argv[1]);
			return 1;
		}
	}
	return 0;
}
