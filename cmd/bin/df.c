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
#include <sys/statfs.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

static struct dev
{
	struct dev *next;
	char *name;
	dev_t dev;
} *devs;
static struct statfs st;
static int kflag;
static int qflag;
static int vflag;

static void ldevs(void)
{
	char path[NAME_MAX + 6] = "/dev/";
	struct dirent *de;
	struct stat st;
	struct dev *nd;
	DIR *d;
	
	d = opendir("/dev");
	if (d == NULL)
		return;
	
	while (de = readdir(d), de)
	{
		strcpy(path + 5, de->d_name);
		if (stat(path, &st))
			continue;
		
		if (!S_ISBLK(st.st_mode))
			continue;
		
		nd = sbrk(sizeof *nd);
		if (!nd)
			goto fail;
		
		nd->name = strdup(de->d_name);
		nd->next = devs;
		nd->dev  = st.st_rdev;
		
		devs = nd;
	}
fail:
	closedir(d);
}

static void prdev(dev_t dev)
{
	char *fmt = "%-10s";
	struct dev *dp;
	char buf[40];
	
	for (dp = devs; dp; dp = dp->next)
		if (dp->dev == dev)
			break;
	if (dp)
	{
		if (qflag)
		{
			printf("/dev/%s", dp->name);
			return;
		}
		sprintf(buf, "/dev/%s", dp->name);
		printf(fmt, buf);
		return;
	}
	
	sprintf(buf, "%i,%i", major(dev), minor(dev));
	printf(fmt, buf);
}

static void showstat(char *path, int g)
{
	blk_t f, t, u;
	
	if (g && statfs(path, &st))
	{
		perror(path);
		return;
	}
	t = st.f_blocks;
	f = st.f_bfree;
	u = t - f;
	
	if (kflag)
	{
		t /= 2;
		f /= 2;
		u /= 2;
	}
	
	if (qflag)
	{
		prdev(st.f_dev);
		printf(" %i\n", f);
		return;
	}
	
	if (vflag)
	{
		printf("%-12s ", path);
		prdev(st.f_dev);
		printf(" %7i %7i %7i %3i%%\n", t, u, f,
			(st.f_blocks - st.f_bfree) * 100 / st.f_blocks);
		return;
	}
	
	printf("%-10s (", path);
	prdev(st.f_dev);
	printf("): %6i blocks\n", f);
}

static void showfst(void)
{
	char *dev, *dir;
	char buf[256];
	char *p;
	FILE *f;
	
	f = fopen("/etc/fstab", "r");
	if (!f)
	{
		perror("/etc/fstab");
		exit(1);
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		dev = strtok(buf,  " \t");
		dir = strtok(NULL, " \t");
		
		showstat(dir, 1);
	}
	fclose(f);
}

static void showall(void)
{
	struct mtab mt;
	ssize_t cnt;
	int fd;
	
	fd = open("/etc/mtab", O_RDONLY);
	if (fd < 0)
	{
		showfst();
		return;
	}
	
	showstat("/", 1);
	while (cnt = read(fd, &mt, sizeof mt), cnt == sizeof mt)
		if (*mt.path)
			showstat(mt.path, 1);
}

static int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'k':
			kflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case '-':
			break;
		default:
			fprintf(stderr, "df: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

int main(int argc, char **argv)
{
	dev_t rdev;
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
	
	if (vflag)
		printf("Mount point  Filesystem  Blocks    Used   Avail Use%\n");
	ldevs();
	
	if (argc)
	{
		for (i = 0; i < argc; i++)
			showstat(argv[i], 1);
		return 0;
	}
	
	showall();
	return 0;
}
