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

#include <xenus/mtab.h>
#include <sys/mount.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

static int mtfd = -1;

static void clrm(char *dir)
{
	struct mtab mt;
	ssize_t cnt;
	
	if (mtfd < 0)
		mtfd = open("/etc/mtab", O_RDWR);
	if (mtfd < 0)
	{
		if (errno != ENOENT)
			perror("/etc/mtab");
		return;
	}
	lseek(mtfd, 0, SEEK_SET);
	
	while (cnt = read(mtfd, &mt, sizeof mt), cnt == sizeof mt)
		if (!strncmp(mt.path, dir, sizeof mt.path))
		{
			memset(&mt, 0, sizeof mt);
			lseek(mtfd, -cnt, SEEK_CUR);
			write(mtfd, &mt, sizeof mt);
			return;
		}
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fputs("umount dir\n", stderr);
		return 1;
	}
	
	if (umount(argv[1]))
	{
		perror(NULL);
		return 1;
	}
	clrm(argv[1]);
	return 0;
}
