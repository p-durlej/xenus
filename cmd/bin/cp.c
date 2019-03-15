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
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

int xit;

char *basename(char *name)
{
	char *s;
	
	s = strrchr(name, '/');
	if (s)
		return s + 1;
	return name;
}

void do_cp(char *src, char *dst)
{
	struct stat st, st2;
	char buf[512];
	int srcfd;
	int dstfd;
	int cnt;
	
	srcfd = open(src, O_RDONLY);
	if (srcfd < 0)
	{
		perror(src);
		xit = 1;
		return;
	}
	
	if (fstat(srcfd, &st))
	{
		perror(src);
		xit = 1;
		close(srcfd);
		return;
	}
	
	if (!stat(dst, &st2) && st.st_dev == st2.st_dev && st.st_ino == st2.st_ino)
	{
		fprintf(stderr, "cp: %s and %s: same file\n", src, dst);
		xit = 1;
		return;
	}
	
	dstfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 07777);
	if (dstfd < 0)
	{
		perror(dst);
		xit = 1;
		close(srcfd);
		return;
	}
	
	while (cnt = read(srcfd, buf, sizeof(buf)), cnt)
	{
		if (cnt < 0)
		{
			perror(src);
			close(srcfd);
			close(dstfd);
			xit = 1;
			return;
		}
		
		if (write(dstfd, buf, cnt) != cnt)
		{
			perror(dst);
			close(srcfd);
			close(dstfd);
			xit = 1;
			return;
		}
	}
	
	close(srcfd);
	close(dstfd);
}

int main(int argc, char **argv)
{
	int dest_dir = 0;
	struct stat st;
	char *dest;
	int i;
	
	if (argc < 3)
	{
		fputs("cp file1... dir\n", stderr);
		fputs("cp file1 file2\n", stderr);
		return 1;
	}
	
	dest = argv[argc - 1];
	
	if (stat(dest, &st))
	{
		if (errno != ENOENT)
		{
			perror(dest);
			return 1;
		}
	}
	else if (S_ISDIR(st.st_mode))
		dest_dir = 1;
	
	for (i = 1; i < argc - 1; i++)
	{
		char buf[PATH_MAX];
		
		if (dest_dir)
		{
			strcpy(buf, dest);
			strcat(buf, "/");
			strcat(buf, basename(argv[i]));
		}
		else
			strcpy(buf, dest);
		
		do_cp(argv[i], buf);
	}
	return xit;
}
