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
#include <sys/reboot.h>
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
static short *lcm;
static short *rcm;
static struct super sb;
static int devd = -1;
static dev_t rootdev = -1;

static int vflag;
static int fflag;
static int aflag;
static int lflag;
static int rflag;
static int xit;

static int modroot;
static int modf;

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

static int cdirb(ino_t dir, blk_t blk)
{
	struct dirent *de;
	char buf[512];
	int size = 0;
	int i;
	
	rdblk(buf, blk);
	
	for (de = (void *)buf, i = 0; i < 512 / sizeof *de; i++, de++)
	{
		if (!de->d_ino)
			continue;
		size = (i + 1) * sizeof *de;
		
		if (vflag > 1)
			printf("%5i %s\n", de->d_ino, de->d_name);
		
		if (brange(de->d_ino))
		{
			printf("de %i ino %i\n", blk, de->d_ino);
			if (!fflag)
				return size;
			
			memset(de, 0, sizeof *de);
			wrblk(buf, blk);
			modf = 1;
			return size;
		}
		
		if (strncmp(de->d_name, ".",  NAME_MAX + 1) &&
		    strncmp(de->d_name, "..", NAME_MAX + 1))
			cinode(dir, de->d_ino, de->d_name);
	}
	return size;
}

static void cdir(struct disk_inode *di, ino_t ino)
{
	struct dirent *de;
	int size = 0;
	int i;
	
	for (i = 0; i < BMAP0_SIZE; i++)
		if (di->bmap0[i])
			size = i * BLK_SIZE + cdirb(ino, di->bmap0[i]);
	
	if (di->size < size)
	{
		printf("dir %i size %i real %i\n", ino, di->size, size);
		if (fflag)
		{
			di->size = size;
			wrblk(di, ino);
			modf = 1;
		}
	}
}

static void cinode(ino_t dir, ino_t ino, char *name)
{
	static blk_t map2[128];
	static blk_t map[128];
	
	struct disk_inode di;
	blkcnt_t bc = 0;
	int i, n, m;
	
	bused(ino);
	clink(ino);
	
	if (rcm[ino] > 1)
		return;
	
	if (rdblk(&di, ino))
		return;
	
	lcm[ino] = di.nlink;
	
	for (i = 0; i < BMAP0_SIZE; i++)
	{
		if (!di.bmap0[i])
			continue;
		bc++;
		
		if (brange(di.bmap0[i]))
		{
			printf("ino %i blk %i\n", ino, di.bmap0[i]);
			if (fflag)
			{
				di.bmap0[i] = 0;
				wrblk(&di, ino);
				modf = 1;
			}
			continue;
		}
		
		bused(di.bmap0[i]);
	}
	
	for (i = 0; i < BMAP1_SIZE; i++)
	{
		if (!di.bmap1[i])
			continue;
		
		if (brange(di.bmap1[i]))
		{
			printf("ino %i ibmap %i\n", ino, di.bmap1[i]);
			if (fflag)
			{
				di.bmap1[i] = 0;
				wrblk(&di, ino);
				modf = 1;
			}
			continue;
		}
		bused(di.bmap1[i]);
		bc++;
		
		if (rdblk(map, di.bmap1[i]))
			continue;
		
		for (n = 0; n < 128; n++)
		{
			if (!map[n])
				continue;
			
			if (brange(map[n]))
			{
				printf("ino %i indir %i %i\n",
					ino, di.bmap1[i], map[n]);
				if (fflag)
				{
					map[n] = 0;
					wrblk(map, di.bmap1[i]);
					modf = 1;
				}
				continue;
			}
			bused(map[n]);
			bc++;
		}
	}
	
	for (i = 0; i < BMAP2_SIZE; i++)
	{
		if (!di.bmap2[i])
			continue;
		
		if (brange(di.bmap2[i]))
		{
			printf("ino %i ibmap %i\n", ino, di.bmap2[i]);
			if (fflag)
			{
				di.bmap2[i] = 0;
				wrblk(&di, ino);
				modf = 1;
			}
			continue;
		}
		bused(di.bmap2[i]);
		bc++;
		
		if (rdblk(map, di.bmap2[i]))
			continue;
		
		for (n = 0; n < 128; n++)
		{
			if (!map[n])
				continue;
			
			if (brange(map[n]))
			{
				printf("ino %i indir %i %i\n",
					ino, di.bmap2[i], map[n]);
				if (fflag)
				{
					map[n] = 0;
					wrblk(map, di.bmap2[i]);
					modf = 1;
				}
				continue;
			}
			
			if (rdblk(map2, map[n]))
				continue;
			
			for (m = 0; m < 128; m++)
			{
				if (!map2[m])
					continue;
				
				if (brange(map2[m]))
				{
					printf("ino %i indir %i %i %i\n", ino, di.bmap2[i], map[n], map2[m]);
					if (fflag)
					{
						map2[m] = 0;
						wrblk(map2, map[n]);
						modf = 1;
					}
				}
				
				bused(map2[m]);
				bc++;
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
			modf = 1;
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
		rcm[ino]++;
		cdir(&di, ino);
		break;
	default:
		printf("ino %i type %o\n", ino, di.mode & S_IFMT);
		if (fflag)
		{
			di.mode &= ~S_IFMT;
			di.mode |=  S_IFREG;
			wrblk(&di, ino);
			modf = 1;
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
			if (!lflag)
				printf("ino %i nlink %i real %i\n", i, lcm[i], rcm[i]);
			if (fflag || lflag)
			{
				if (rdblk(&di, i))
					continue;
				di.nlink = rcm[i];
				wrblk(&di, i);
				modf = 1;
			}
		}
}

static void cbam(void)
{
	blk_t cnt = (sb.nblocks + 4095) / 4096;
	blk_t i, n;
	char buf[512];
	int bad;
	
	for (i = 0; i < cnt; i++)
	{
		if (rdblk(buf, i + sb.bitmap))
			continue;
		
		bad = 0;
		for (n = 0; n < 512; n++)
			if (buf[n] != bam[i * 512 + n])
			{
				printf("bam %i,%i is %02x real %02x\n",
					i, n, buf[n] & 255, bam[i * 512 + n] & 255);
				bad = 1;
			}
		if (fflag && bad)
		{
			memcpy(buf, &bam[i * 512], 512);
			wrblk(buf, i + sb.bitmap);
			modf = 1;
		}
	}
}

static void fsck1(char *path)
{
	struct stat st;
	blk_t i;
	
	modf = 0;
	
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
	
	bam = calloc(sb.nblocks, sizeof *bam);
	lcm = calloc(sb.nblocks, sizeof *lcm);
	rcm = calloc(sb.nblocks, sizeof *rcm);
	
	if (!bam || !lcm || !rcm)
	{
		if (!vflag)
			printf("nblocks %u\n", sb.nblocks);
		goto fail;
	}
	
	for (i = 0; i < sb.dblock; i++)
		bused(i);
	
	cinode(sb.root, sb.root, "/");
	rcm[sb.root]--;
	cnlink();
	cbam();
	
	if (modf)
	{
		fstat(devd, &st);
		if (st.st_rdev == rootdev)
			modroot = 1;
	}
	
	close(devd);
	free(bam);
	
	bam  = NULL;
	devd = -1;
	modf = 0;
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
	struct stat st;
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
			case 'l':
				lflag = 1;
				break;
			case 'r':
				rflag = 1;
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
	
	if (rflag)
	{
		if (stat("/", &st))
			perror("/: stat");
		rootdev = st.st_dev;
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
	
	if (rflag && modroot)
	{
		fputs("fsck: rebooting\n", stderr);
		sleep(1);
		reboot(RB_AUTOBOOT | RB_NOSYNC);
	}
	
	return xit;
}
