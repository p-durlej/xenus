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

#include <xenus/fs.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

static void cinode(ino_t dir, ino_t ino, char *name);

static char *devname;
static char *bam;
static char *lcm;
static char *rcm;
static struct super sb;
static int devd = -1;

static int vflag;
static int fflag;
static int aflag;
static int xit;

static int brange(blk_t b)
{
	if (b < sb.dblock || b >= sb.nblocks)
		return -1;
	return 0;
}

static int rdblk(void *buf, blk_t b)
{
	int cnt;
	
	if (lseek(devd, b * 512, SEEK_SET) < 0)
		goto fail;
	
	errno = EINVAL;
	
	cnt = read(devd, buf, 512);
	if (cnt != 512)
		goto fail;
	
	return 0;
fail:
	perror(devname);
	return -1;
}

static void wrblk(void *buf, blk_t b)
{
	if (lseek(devd, b * 512, SEEK_SET) < 0)
	{
		perror(devname);
		return;
	}
	
	errno = EINVAL;
	
	if (write(devd, buf, 512) != 512)
	{
		perror(devname);
		return;
	}
}

static void clink(ino_t ino)
{
	rcm[ino]++;
}

static void bused(blk_t blk)
{
	bam[blk / 8] |= 1 << (blk & 7);
}

static void cdirb(ino_t dir, blk_t blk)
{
	struct dirent *de;
	char buf[512];
	int i;
	
	rdblk(buf, blk);
	
	for (de = (void *)buf, i = 0; i < 512 / sizeof *de; i++, de++)
	{
		if (!de->d_ino)
			continue;
		
		if (vflag > 1)
			printf("%5i %s\n", de->d_ino, de->d_name);
		
		if (brange(de->d_ino))
		{
			printf("de %i ino %i\n", blk, de->d_ino);
			if (!fflag)
				return;
			
			memset(de, 0, sizeof *de);
			wrblk(buf, blk);
			return;
		}
		if (strncmp(de->d_name, ".",  NAME_MAX + 1) &&
		    strncmp(de->d_name, "..", NAME_MAX + 1))
			cinode(dir, de->d_ino, de->d_name);
	}
}

static void cdir(struct disk_inode *di, ino_t ino)
{
	struct dirent *de;
	int i;
	
	for (i = 0; i < BMAP_SIZE; i++)
		if (di->bmap[i])
			cdirb(ino, di->bmap[i]);
}

static void cinode(ino_t dir, ino_t ino, char *name)
{
	struct disk_inode di;
	blk_t map[128];
	blkcnt_t bc = 0;
	int i, n;
	
	bused(ino);
	clink(ino);
	
	if (rdblk(&di, ino))
		return;
	
	lcm[ino] = di.nlink;
	
	for (i = 0; i < BMAP_SIZE; i++)
	{
		if (!di.bmap[i])
			continue;
		bc++;
		
		if (brange(di.bmap[i]))
		{
			printf("ino %i blk %i\n", ino, di.bmap[i]);
			if (fflag)
			{
				di.bmap[i] = 0;
				wrblk(&di, ino);
			}
			continue;
		}
		
		bused(di.bmap[i]);
	}
	
	for (i = 0; i < IBMAP_SIZE; i++)
	{
		if (!di.ibmap[i])
			continue;
		
		if (brange(di.ibmap[i]))
		{
			printf("ino %i ibmap %i\n", ino, di.ibmap[i]);
			if (fflag)
			{
				di.ibmap[i] = 0;
				wrblk(&di, ino);
			}
			continue;
		}
		bused(di.ibmap[i]);
		bc++;
		
		if (rdblk(map, di.ibmap[i]))
			continue;
		
		for (n = 0; n < 128; n++)
		{
			if (!map[n])
				continue;
			
			if (brange(map[n]))
			{
				printf("ino %i indir %i %i\n",
					ino, di.ibmap[i], map[n]);
				if (fflag)
				{
					map[n] = 0;
					wrblk(map, di.ibmap[i]);
				}
				continue;
			}
			bused(map[n]);
			bc++;
		}
	}
	
	if (di.blocks != bc)
	{
		printf("ino %i %s bcount %i real %i\n",
			ino, name, di.blocks, bc);
		if (fflag)
		{
			di.blocks = bc;
			wrblk(&di, ino);
		}
	}
	
	switch (di.mode & S_IFMT)
	{
	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
	case S_IFREG:
		break;
	case S_IFDIR:
		rcm[dir]++;
		cdir(&di, ino);
		break;
	default:
		printf("ino %i type %o\n", ino, di.mode & S_IFMT);
		if (fflag)
		{
			di.mode &= ~S_IFMT;
			di.mode |=  S_IFREG;
			wrblk(&di, ino);
		}
	}
}

static void cnlink(void)
{
	struct disk_inode di;
	blk_t i;
	
	for (i = 0; i < sb.nblocks; i++)
		if (lcm[i] != rcm[i])
		{
			printf("ino %i nlink %i real %i\n", i, lcm[i], rcm[i]);
			if (fflag)
			{
				if (rdblk(&di, i))
					continue;
				di.nlink = rcm[i];
				wrblk(&di, i);
			}
		}
}

static void cbam(void)
{
	blk_t cnt = (sb.nblocks + 4095) / 4096;
	blk_t i;
	char buf[512];
	
	for (i = 0; i < cnt; i++)
	{
		if ((i & 511) == 0)
		{
			if (rdblk(buf, i / 512 + sb.bitmap))
			{
				i += 511;
				continue;
			}
		}
		
		if (buf[i & 511] != bam[i])
		{
			printf("bam %i is %02x real %02x\n",
				i, buf[i & 511] & 255, bam[i] & 255);
			if (fflag)
			{
				buf[i & 511] = bam[i];
				wrblk(buf, i / 512 + sb.bitmap);
			}
		}
	}
}

static void fsck1(char *path)
{
	blk_t i;
	
	devname = path;
	devd = open(path, O_RDWR);
	if (devd < 0)
		goto fail;
	
	if (lseek(devd, 512, SEEK_SET) < 0)
		goto fail;
	
	errno = EINVAL;
	if (read(devd, &sb, sizeof sb) != sizeof sb)
		goto fail;
	
	if (vflag)
	{
		printf("filesys %s\n", path);
		printf("nblocks %u\n", sb.nblocks);
		printf("bitmap  %u\n", sb.bitmap);
		printf("dblock  %u\n", sb.dblock);
	}
	
	bam = calloc(3, sb.nblocks);
	if (!bam)
	{
		if (!vflag)
			printf("nblocks %u\n", sb.nblocks);
		goto fail;
	}
	lcm = bam + sb.nblocks;
	rcm = lcm + sb.nblocks;
	
	for (i = 0; i < sb.bitmap + (sb.nblocks + 4095) / 4096; i++)
		bused(i);
	
	cinode(sb.root, sb.root, "/");
	cnlink();
	cbam();
	
	close(devd);
	free(bam);
	
	bam  = NULL;
	devd = -1;
	return;
fail:
	perror(devname);
	close(devd);
	xit = 1;
}

static void checkall(void)
{
	char *dev, *dir;
	char buf[256];
	char *p;
	FILE *f;
	
	f = fopen("/etc/fstab", "r");
	if (!f)
	{
		perror("/etc/fstab");
		xit = 1;
		return;
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		dev = strtok(buf,  " \t");
		dir = strtok(NULL, " \t");
		
		fsck1(dev);
	}
	fclose(f);
}

int main(int argc, char **argv)
{
	char *p;
	
	argv++;
	argc--;
	
	if (argc > 0 && **argv == '-')
	{
		for (p = *argv + 1; *p; p++)
			switch (*p)
			{
			case 'a':
				aflag = 1;
				break;
			case 'f':
				fflag = 1;
				break;
			case 'v':
				vflag++;
				break;
			default:
				fprintf(stderr, "fsck: bad option '%c'\n", *p);
				return 1;
			}
		argv++;
		argc--;
	}
	
	if (argc <= 0 && !aflag)
	{
		fputs("fsck [-fv] dev1...\n", stderr);
		fputs("fsck [-fv] -a\n", stderr);
		return 0;
	}
	
	sync();
	
	if (aflag)
		checkall();
	
	while (argc)
	{
		fsck1(*argv);
		
		argv++;
		argc--;
	}
	
	return xit;
}
