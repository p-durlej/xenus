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

#ifndef _XENUS_FS_H
#define _XENUS_FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#define MAXMOUNTS	8
#define MAXINODES	200
#define MAXFILES	150
#define MAXFDS		OPEN_MAX

#define BLK_SIZE	512
#define MAXBDEVS	16
#define MAXCDEVS	16

struct blk_buffer
{
	int	refcnt;
	int	valid;
	int	dirty;
	dev_t	dev;
	blk_t	blk;
	char	data[BLK_SIZE];
};

struct super
{
	int	mounted;
	int	ro;
	dev_t	dev;
	blk_t	bitmap;
	blk_t	dblock;
	blk_t	nblocks;
	blk_t	freeblk;
	blk_t	nfree;
	ino_t	root;
	time_t	time;
};

#define BMAP_SIZE	64
#define IBMAP_SIZE	55

struct disk_inode
{
	uid_t	uid;
	gid_t	gid;
	dev_t	rdev;
	off_t	size;
	mode_t	mode;
	blk_t	blocks;
	time_t	atime;
	time_t	mtime;
	time_t	ctime;
	nlink_t	nlink;
	blk_t	bmap[BMAP_SIZE];
	blk_t	ibmap[IBMAP_SIZE];
};

struct inode
{
	ino_t			ino;
	int			dirty;
	int			refcnt;
	struct super *		sb;
	struct inode *		mnt_dir;
	struct inode *		mnt_root;
	struct disk_inode	d;
	
	int			pipe_wrp;
	int			pipe_rdp;
	char *			pipe_buf;
	int			pipe_datlen;
	int			pipe_readers;
	int			pipe_writers;
};

struct blk_driver
{
	int (*read)(dev_t dev, blk_t blk, void *buf);
	int (*write)(dev_t dev, blk_t blk, void *buf);
	int (*ioctl)(dev_t dev, int cmd, void *ptr);
};

extern struct blk_driver blk_driver[];

extern struct super super[MAXMOUNTS];
extern char *freedirt[MAXMOUNTS];
extern char *freemap[MAXMOUNTS];

void	blk_init(void);
int	blk_get(dev_t dev, blk_t blk, struct blk_buffer **buf);
int	blk_read(dev_t dev, blk_t blk, struct blk_buffer **buf);
int	blk_write(struct blk_buffer *buf);
int	blk_upread(dev_t dev, blk_t blk, off_t start, int len, char *buf);
int	blk_upwrite(dev_t dev, blk_t blk, off_t start, int len, char *buf);
int	blk_put(struct blk_buffer *buf);
void	blk_sync(int invl, int timeout);

int	mountroot(dev_t dev, int ro);

extern struct inode inode[MAXINODES];

int	inode_get(struct super *sb, ino_t ino, struct inode **buf);
int	inode_put(struct inode *ino);
int	inode_chkperm(struct inode *ino, int mode);
int	inode_chkperm_rid(struct inode *ino, int mode);
void	inode_sync(int level);

int	bmap(struct inode *ino, blk_t *blk);
int	bmap_alloc(struct inode *ino, blk_t *blk);
int	bmget(struct inode *ino, blk_t blk, struct blk_buffer **buf);
int	bmget_alloc(struct inode *ino, blk_t blk, struct blk_buffer **buf);
int	trunc(struct inode *ino, int umap);

int	alloc_blk(struct super *sb, blk_t *blk);
int	free_blk(struct super *sb, blk_t blk);
int	alloc_inode(struct super *sb, struct inode **ino);
int	free_inode(struct inode *ino);
int	count_free(struct super *sb);

struct dirpos
{
	struct blk_buffer *	buf;
	struct dirent *		de;
};

#define dirpos_put(dp)	blk_put(dp.buf)

int	dir_search(struct inode *dir, char *name, struct dirpos *dp);
int	dir_search_i(struct inode *dir, char *name, struct inode **ino);
int	dir_creat(struct inode *dir, char *name, ino_t ino);
int	dir_traverse(char *path, int parent, struct inode **ino);
int	dir_isempty(struct inode *dir);
int     dir_reparent(struct inode *dir, ino_t ino);

struct rwreq
{
	char *		buf;
	off_t		start;
	size_t		count;
	int		nodelay;
	struct inode *	ino;
};

struct chr_driver
{
	int (*ttyname)(struct inode *ino, char *buf);
	int (*open)(struct inode *ino, int nodelay);
	int (*close)(struct inode *ino);
	int (*read)(struct rwreq *req);
	int (*write)(struct rwreq *req);
	int (*ioctl)(struct inode *inode, int cmd, void *ptr);
};

extern struct chr_driver chr_driver[];

void	chr_init();
int	chr_open(struct inode *ino, int nodelay);
int	chr_close(struct inode *ino);
int	chr_read(struct rwreq *req);
int	chr_write(struct rwreq *req);
int	chr_ioctl(struct inode *ino, int cmd, void *ptr);
int	chr_ttyname(struct inode *ino, char *buf);
int	reg_read(struct rwreq *req);
int	reg_write(struct rwreq *req);
int	blk_lread(struct rwreq *req);
int	blk_lwrite(struct rwreq *req);
int	blk_ioctl(struct inode *ino, int cmd, void *ptr);

int	pipe_open(struct inode *ino, int flags);
int	pipe_read(struct rwreq *req);
int	pipe_write(struct rwreq *req);

struct file
{
	int (*read)(struct rwreq *rw);
	int (*write)(struct rwreq *rw);
	int (*ioctl)(struct inode *inode, int cmd, void *ptr);
	
	struct inode *	ino;
	int		refcnt;
	int		flags;
	off_t		pos;
};

struct fd
{
	struct file *file;
	int cloexec;
};

int	file_get(struct file **buf);
int	file_put(struct file *file);
int	file_chkperm(struct file *file, int mode);
int	fd_get(int *fd, int lowest);
int	fd_put(int fd);
int	fd_chk(int fd, int mode);
void	fd_cloexec();

char *	basename(char *path);

extern volatile int dosync;

void	psync(void);

#endif
