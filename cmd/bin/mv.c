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
#include <limits.h>
#include <unistd.h>
#include <string.h>
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

void do_mv(char *src, char *dst)
{
	static char buf[512];
	
	int fd1 = -1, fd2 = -1;
	ssize_t cnt1, cnt2;
	struct stat st;
	
	if (rename(src, dst))
	{
		if (errno == EXDEV)
		{
			fd1 = open(src, O_RDONLY | O_NDELAY);
			if (fd1 < 0)
				goto fail1;
			fstat(fd1, &st);
			
			if (!S_ISREG(st.st_mode))
			{
				fprintf(stderr, "%s: Not a regular file\n", src);
				goto fail;
			}
			
			fd2 = open(dst, O_CREAT | O_TRUNC | O_WRONLY, st.st_mode);
			if (fd2 < 0)
				goto fail2;
			
			while (cnt1 = read(fd1, buf, sizeof buf), cnt1 > 0)
			{
				cnt2 = write(fd2, buf, cnt1);
				if (cnt2 != cnt1)
				{
					fprintf(stderr, "%s: Short write\n", dst);
					goto fail;
				}
				if (cnt2 < 0)
					goto fail2;
			}
			if (cnt1 < 0)
				goto fail1;
			
			if (unlink(src))
				goto fail1;
			close(fd1);
			close(fd2);
			return;
		}
		
		xit = 1;
		perror(src);
	}
	return;
fail1:
	perror(src);
	goto fail;
fail2:
	perror(dst);
fail:
	if (fd1 >= 0)
		close(fd1);
	if (fd2 >= 0)
		close(fd2);
	xit = 1;
	return;
}

int main(int argc, char **argv)
{
	int target_exists = 1;
	struct stat st;
	char *target;
	int i;
	
	if (argc < 3)
	{
		fputs("mv name1... dir\n", stderr);
		fputs("mv name1 name2\n", stderr);
		return 1;
	}
	
	target = argv[argc - 1];
	if (stat(target, &st))
	{
		if (errno != ENOENT)
		{
			perror(target);
			return 1;
		}
		target_exists = 0;
	}
	
	for (i = 1; i < argc - 1; i++)
	{
		char buf[PATH_MAX];
		
		if (target_exists && S_ISDIR(st.st_mode))
		{
			strcpy(buf, target);
			strcat(buf, "/");
			strcat(buf, basename(argv[i]));
		}
		else
			strcpy(buf, target);
		
		do_mv(argv[i], buf);
	}
	return xit;
}
