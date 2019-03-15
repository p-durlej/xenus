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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

static char buf[512];
static int namlen;
static int x;

static void prnam(char *name)
{
	int len;
	
	printf("%s: ", name);
	len = namlen - strlen(name);
	while (len-- > 0)
		putchar(' ');
}

static void guess(char *path, ssize_t cnt)
{
	int i;
	
	prnam(path);
	
	if (!cnt)
	{
		puts("empty");
		return;
	}
	
	if (cnt >= 8 && !strncmp(buf, "!<arch>\n", 8))
	{
		puts("ar archive");
		return;
	}
	
	if (cnt > 8 && !strncmp(buf, "XENUS386", 8))
	{
		puts("executable");
		return;
	}
	
	for (i = 0; i < cnt; i++)
		if (!isprint(buf[i]) && buf[i] != '\n' && buf[i] != '\r' &&
		    buf[i] != '\t')
			break;
	if (i >= cnt)
	{
		puts("ascii text");
		return;
	}
	
	puts("data");
}

static void do_file(char *path)
{
	struct stat st;
	ssize_t cnt;
	int fd = -1;
	
	if (stat(path, &st))
		goto fail;
	
	switch (st.st_mode & S_IFMT)
	{
	case S_IFCHR:
		prnam(path);
		printf("character special (%i/%i)\n",
			major(st.st_rdev), minor(st.st_rdev));
		break;
	case S_IFBLK:
		prnam(path);
		printf("block special (%i/%i)\n",
			major(st.st_rdev), minor(st.st_rdev));
		break;
	case S_IFIFO:
		prnam(path);
		printf("fifo (named pipe)\n");
		break;
	case S_IFDIR:
		prnam(path);
		printf("directory\n");
		break;
	case S_IFREG:
		fd = open(path, O_RDONLY);
		if (fd < 0)
			goto fail;
		cnt = read(fd, buf, sizeof buf);
		if (cnt < 0)
			goto fail;
		close(fd);
		fd = -1;
		guess(path, cnt);
		break;
	default:
		prnam(path);
		printf("bad inode type %o", st.st_mode);
		x = 1;
		break;
	}
	
	return;
fail:
	perror(path);
	x = 1;
}

int main(int argc, char **argv)
{
	int len;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		len = strlen(argv[i]);
		if (namlen < len)
			namlen = len;
	}
	if (namlen > 28)
		namlen = 28;
	for (i = 1; i < argc; i++)
		do_file(argv[i]);
	return x;
}
