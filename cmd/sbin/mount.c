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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int _readwrite(void);

static int mtfd = -1;
static int r;

static int xmount(char *dev, char *dir, int ro)
{
	struct mtab mt;
	ssize_t sz;
	
	if (mtfd < 0)
		mtfd = open("/etc/mtab", O_CREAT | O_RDWR, 0666);
	if (mtfd < 0 && errno != EROFS)
		perror("/etc/mtab");
	
	if (mount(dev, dir, ro))
		return -1;
	
	if (mtfd < 0)
		return 0;
	
	lseek(mtfd, 0, SEEK_SET);
	while (sz = read(mtfd, &mt, sizeof mt), sz == sizeof mt)
		if (!*mt.path)
			break;
	
	memset(&mt, 0, sizeof mt);
	strncpy(mt.path, dir, sizeof mt.path);
	strncpy(mt.spec, dev, sizeof mt.spec);
	
	lseek(mtfd, -sz, SEEK_CUR);
	write(mtfd, &mt, sizeof mt);
	
	return 0;
}

static int mountall(void)
{
	char *dev, *dir, *flg;
	char buf[256];
	char *p;
	FILE *f;
	int x = 0;
	int r1;
	
	f = fopen("/etc/fstab", "r");
	if (!f)
	{
		perror("/etc/fstab");
		return 1;
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		dev = strtok(buf,  " \t");
		dir = strtok(NULL, " \t");
		flg = strtok(NULL, " \t");
		
		r1 = 0;
		for (p = flg; *p; p++)
			if (*p == 'r')
				r1 = 1;
		
		if (xmount(dev, dir, r | r1) && errno != EBUSY)
		{
			perror(dir);
			x = 1;
		}
	}
	fclose(f);
	return x;
}

static int showall(void)
{
	struct mtab mt;
	ssize_t cnt;
	int fd;
	
	fd = open("/etc/mtab", O_RDONLY);
	if (fd < 0)
	{
		perror("/etc/mtab");
		return 1;
	}
	
	while (cnt = read(fd, &mt, sizeof mt), cnt == sizeof mt)
	{
		mt.spec[sizeof mt.spec - 1] = 0;
		mt.path[sizeof mt.path - 1] = 0;
		if (*mt.path)
			printf("%s on %s\n", mt.spec, mt.path);
	}
	if (cnt < 0)
	{
		perror("/etc/mtab");
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

int main(int argc,char **argv)
{
	if (argc == 2)
	{
		if (!strcmp(argv[1], "-a"))
			return mountall();
		if (!strcmp(argv[1], "-w"))
		{
			if (_readwrite())
			{
				perror(NULL);
				return 1;
			}
			return 0;
		}
	}
	
	if (argc < 2)
		return showall();
	
	if (argc > 1 && !strcmp(argv[1], "-r"))
	{
		argv++;
		argc--;
		r = 1;
	}
	
	if (argc != 3)
	{
		fputs("mount [-r] dev dir\n", stderr);
		fputs("mount -a\n", stderr);
		return 1;
	}
	if (xmount(argv[1], argv[2], r))
	{
		perror(NULL);
		return 1;
	}
	return 0;
}
