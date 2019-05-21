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
#ifndef CROSS
#include <sys/ioctl.h>
#include <sys/dioc.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define BLK_SIZE	512

#define BMAP_SIZE	64
#define IBMAP_SIZE	57

#define XENUS_S_IFMT	0170000
#define XENUS_S_IFIFO	0010000
#define XENUS_S_IFCHR	0020000
#define XENUS_S_IFDIR	0040000
#define XENUS_S_IFBLK	0060000
#define XENUS_S_IFREG	0100000

#define xenus_makedev(major, minor)	((major) * 0x0100 + (minor))
#define xenus_major(rdev)		((rdev) >> 8)
#define xenus_minor(rdev)		((rdev) & 255)

#define MAXLNK		64

typedef unsigned int	U32;
typedef unsigned short	U16;
typedef unsigned char	U8;

struct xenus_dirent
{
	U32  d_ino;
	char d_name[28];
};

struct super
{
	U32 mounted;
	U32 ro;
	U32 dev;
	U32 bitmap;
	U32 dblock;
	U32 nblocks;
	U32 freeblk;
	U32 nfree;
	U32 root;
	U32 time;
};

struct disk_inode
{
	U32	bmap[BMAP_SIZE];
	U32	ibmap[IBMAP_SIZE];
	U32	atime;
	U32	mtime;
	U32	ctime;
	U32	size;
	U16	blocks;
	U16	uid;
	U16	gid;
	U16	rdev;
	U16	mode;
	U16	nlink;
};

struct file
{
	U32 nlink;
	U32 mode;
	U32 blocks;
	U32 size;
	U32 bmap[BMAP_SIZE];
	U32 *ibmap[IBMAP_SIZE];
	U32 inode;
	U32 uid;
	U32 gid;
	U32 rdev;
	
	struct xenus_dirent *dents;
	int ndents;
};

struct link
{
	dev_t	sdev;
	ino_t	sino;
	U32	nino;
	U16	nlink;
} links[MAXLNK];
int nlinks;

struct super	sb;
int		pflag;
int		vflag;
int		sflag;
char *		device;
int		fd;

void	wsuper();
void	initmap();
void	seekblock(U32 blk);
void	writeblock(U32 blk, void *buf, U32 size);
U32	allocblk();
U32	translate_mode(mode_t mode);
void	makeroot(int copy);
void	file_init(struct file *file);
void	file_addblock(struct file *file, char *buf, U32 size);
void	file_close(struct file *file);
void	file_flushent(struct file *dir);
void	file_addent(struct file *dir, U32 inode, char *name);
void	file_add(struct file *dir, char *src);
void	file_mkdirhead(struct file *dir, struct file *parent);
void	file_copydir(struct file *dir, struct file *parent, char *src);
void	file_copy(struct file *file, int fd, char *name);

void wsuper()
{
	writeblock(1, &sb, sizeof(sb));
}

void initmap()
{
	char buf[512];
	int nb, mb, nbits;
	U32 l1;
	U32 i;
	
	seekblock(sb.bitmap);
	mb = (sb.nblocks + 4095) / 4096;
	nb =  sb.freeblk;
	
	for (i = 0; i < mb; i++)
	{
		if (nb >= 4096)
		{
			nbits = 0;
			nb -= 4096;
			l1  = 512;
		}
		else if (nb)
		{
			nbits = nb & 7;
			l1 = nb / 8;
			nb = 0;
		}
		else
		{
			nbits = 0;
			l1 = 0;
		}
		
		memset(buf, 255, l1);
		memset(buf + l1, 0, sizeof(buf) - l1);
		
		if (nbits)
			buf[l1] = 255 >> (8 - nbits);
		
		if (write(fd, buf, 512) != 512)
		{
			perror(device);
			exit(1);
		}
	}
}

void seekblock(U32 blk)
{
	if (lseek(fd, blk * 512L, SEEK_SET) < 0)
	{
		perror(device);
		exit(1);
	}
}

void writeblock(U32 blk, void *buf, U32 size)
{
	char b[BLK_SIZE];
	
	seekblock(blk);
	memcpy(b, buf, size);
	memset(b + size, 0, sizeof(b) - size);
	if (write(fd, b, sizeof(b)) != sizeof(b))
	{
		perror(device);
		exit(1);
	}
}

U32 allocblk()
{
	if (sb.nblocks == sb.freeblk)
	{
		fprintf(stderr, "mkfs: no space left on filesystem being"
				" created\n");
		exit(1);
	}
	sb.freeblk++;
	return sb.freeblk - 1;
}

U32 translate_mode(mode_t mode)
{
	U32 ret = mode & 07777;
	
	switch (mode & S_IFMT)
	{
	case S_IFREG:
		ret |= XENUS_S_IFREG;
		break;
	case S_IFIFO:
		ret |= XENUS_S_IFIFO;
		break;
	case S_IFCHR:
		ret |= XENUS_S_IFCHR;
		break;
	case S_IFBLK:
		ret |= XENUS_S_IFBLK;
		break;
	case S_IFDIR:
		ret |= XENUS_S_IFDIR;
		break;
	}
	return ret;
}

void patchlinks(void)
{
#define NLOFF ((unsigned)&((struct disk_inode *)0)->nlink)
	int i;
	
	for (i = 0; i < nlinks; i++)
	{
		lseek(fd, links[i].nino * 512 + NLOFF, SEEK_SET);
		write(fd, &links[i].nlink, sizeof links[i].nlink);
	}
}

void makeroot(int copy)
{
	struct file root;
	
	file_init(&root);
	root.mode = XENUS_S_IFDIR | 0755;
	if (copy)
	{
		file_copydir(&root, &root, ".");
		patchlinks();
	}
	else
		file_mkdirhead(&root, &root);
	file_close(&root);
	sb.root = root.inode;
}

void file_init(struct file *file)
{
	memset(file, 0, sizeof(*file));
	file->inode = allocblk();
	file->nlink = 1;
}

void file_addblock(struct file *file, char *buf, U32 size)
{
	U32 blk;
	
	if (!size)
		return;
	
	blk = allocblk();
	writeblock(blk, buf, size);
	if (file->blocks >= BMAP_SIZE)
	{
		U32 map;
		U32 idx;
		
		map = (file->blocks - BMAP_SIZE) / (BLK_SIZE / 4);
		idx = (file->blocks - BMAP_SIZE) % (BLK_SIZE / 4);
		
		if (map >= IBMAP_SIZE)
		{
			fprintf(stderr, "mkfs: file to add is too big\n");
			exit(1);
		}
		
		if (!file->ibmap[map])
		{
			file->ibmap[map] = malloc(BLK_SIZE);
			if (!file->ibmap[map])
			{
				perror("mkfs: malloc");
				exit(errno);
			}
			memset(file->ibmap[map], 0, BLK_SIZE);
		}
		file->ibmap[map][idx] = blk;
	}
	else
		file->bmap[file->blocks] = blk;
	file->size = file->blocks * BLK_SIZE + size;
	file->blocks++;
}

void file_close(struct file *file)
{
	struct disk_inode di;
	time_t t;
	int i;
	
	file_flushent(file);
	
	time(&t);
	
	memset(&di, 0, sizeof(di));
	di.uid		= file->uid;
	di.gid		= file->gid;
	di.rdev		= file->rdev;
	di.size		= file->size;
	di.mode		= file->mode;
	di.blocks	= file->blocks;
	di.nlink	= file->nlink;
	di.atime	= t;
	di.mtime	= t;
	di.ctime	= t;
	memcpy(di.bmap, file->bmap, sizeof(di.bmap));
	for (i = 0; i < IBMAP_SIZE; i++)
		if (file->ibmap[i])
		{
			di.ibmap[i] = allocblk();
			di.blocks++;
			writeblock(di.ibmap[i], file->ibmap[i], BLK_SIZE);
		}
	writeblock(file->inode, (char *)&di, sizeof(di));
}

void file_flushent(struct file *dir)
{
	if (!dir->dents)
		return;
	
	file_addblock(dir, (char *)dir->dents, dir->ndents * sizeof *dir->dents);
	dir->ndents = 0;
}

void file_addent(struct file *dir, U32 ino, char *name)
{
	struct xenus_dirent *de;
	
	if (dir->ndents >= 512 / sizeof *de)
		file_flushent(dir);
	
	if (!dir->dents)
	{
		dir->dents = malloc(512);
		if (!dir->dents)
		{
			perror(NULL);
			exit(1);
		}
	}
	
	de = &dir->dents[dir->ndents];
	memset(de, 0, sizeof *de);
	
	strncpy(de->d_name, name, sizeof(de->d_name));
	de->d_ino = ino;
	
	dir->ndents++;
}

void file_add(struct file *dir, char *src)
{
	struct file file;
	struct stat st;
	int fd;
	int i;
	
	if (stat(src, &st))
	{
		perror(src);
		exit(1);
	}
	
	if (st.st_nlink > 1)
		for (i = 0; i < nlinks; i++)
			if (links[i].sino == st.st_ino && links[i].sdev == st.st_dev)
			{
				file_addent(dir, links[i].nino, src);
				links[i].nlink++;
				return;
			}
	
	file_init(&file);
	file.mode = translate_mode(st.st_mode);
	if (pflag)
	{
		file.uid = st.st_uid;
		file.gid = st.st_gid;
	}
	else
	{
		file.uid = 0;
		file.gid = 0;
	}
	file.rdev = xenus_makedev(xenus_major(st.st_rdev), xenus_minor(st.st_rdev));
	switch (st.st_mode & S_IFMT)
	{
	case S_IFDIR:
		file_copydir(&file, dir, src);
		break;
	case S_IFREG:
		fd = open(src, O_RDONLY);
		if (fd < 0)
		{
			perror(src);
			exit(errno);
		}
		file_copy(&file, fd, src);
		close(fd);
		break;
	}
	file_close(&file);
	file_addent(dir, file.inode, src);
	if (vflag)
		printf("%2u %6o %5u %s\n", file.nlink, file.mode, file.size, src);
	
	if (st.st_nlink > 1 && nlinks < MAXLNK)
	{
		links[nlinks].sino = st.st_ino;
		links[nlinks].sdev = st.st_dev;
		links[nlinks].nino = file.inode;
		links[nlinks].nlink = 1;
		nlinks++;
	}
}

void file_mkdirhead(struct file *dir, struct file *parent)
{
	file_addent(dir, dir->inode,	".");
	file_addent(dir, parent->inode,	"..");
	
	parent->nlink++;
}

void file_copydir(struct file *dir, struct file *parent, char *src)
{
	struct dirent *de;
	DIR *dirp;
	
	file_mkdirhead(dir, parent);
	if (chdir(src))
	{
		perror(src);
		exit(errno);
	}
	dirp = opendir(".");
	if (!dirp)
	{
		perror(src);
		exit(errno);
	}
	while (errno = 0, de = readdir(dirp))
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
			file_add(dir, de->d_name);
	if (errno)
	{
		perror(src);
		exit(errno);
	}
	closedir(dirp);
	if (chdir(".."))
	{
		perror("..");
		exit(errno);
	}
}

void file_copy(struct file *file, int fd, char *name)
{
	char buf[BLK_SIZE];
	ssize_t cnt;
	
	while (cnt = read(fd, buf, sizeof(buf)), cnt)
	{
		if (cnt < 0)
		{
			perror(name);
			exit(errno);
		}
		file_addblock(file, buf, cnt);
	}
}

int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'p':
			pflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			fprintf(stderr, "mkfs: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

int main(int argc, char **argv)
{
#ifndef CROSS
	struct dinfo di;
#endif
	U32 i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	argv += i;
	argc -= i;
	
	if (argc < 1 || argc > 4)
	{
		fputs("mkfs [-pv] [-s blocks] dev [bitmap [dir]]\n", stderr);
		return 1;
	}
	
	if (sflag)
		sb.nblocks = atoi(*argv++);
	
	device = *(argv++);
	
	fd = open(device, O_WRONLY);
	if (fd < 0)
	{
		perror(device);
		return errno;
	}
	
#ifndef CROSS
	if (!sb.nblocks)
	{
		if (ioctl(fd, DIOCGINFO, &di))
		{
			perror(device);
			return 1;
		}
		sb.nblocks = di.size;
	}
#endif
	sb.dev		 = -1;
	sb.bitmap	 = 2;
	if (*argv)
		sb.bitmap = atol(*(argv++));
	sb.root		 = -1;
	sb.dblock	 = (sb.nblocks + 4095) / 4096;
	sb.dblock	+= sb.bitmap;
	sb.freeblk	 = sb.dblock;
	sb.ro		 = 0;
	sb.mounted	 = 0;
	sb.time		 = time(NULL);
	
	if (sb.nblocks > 2000000)
	{
		fputs("blocks > 2000000\n", stderr);
		return 1;
	}
	
	if (sb.nblocks < 8)
	{
		fputs("blocks < 8\n", stderr);
		return 1;
	}
	
	if (sb.bitmap < 2)
	{
		fputs("bitmap < 2\n", stderr);
		return 1;
	}
	
	if (*argv)
	{
		if (chdir(*argv))
		{
			perror(*argv);
			return errno;
		}
		makeroot(1);
	}
	else
		makeroot(0);
	initmap();
	wsuper();
	close(fd);
#ifdef CROSS
	return 0;
#else
	if (*argv)
		execl("/bin/fsck", "mkfs", "-l", device, (void *)NULL);
	return 127;
#endif
}
