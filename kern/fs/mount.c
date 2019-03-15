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

#include <xenus/syscall.h>
#include <xenus/process.h>
#include <xenus/printf.h>
#include <xenus/malloc.h>
#include <xenus/panic.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define V 0

struct super super[MAXMOUNTS];
char *freedirt[MAXMOUNTS];
char *freemap[MAXMOUNTS];
volatile int dosync;

int read_super(dev_t dev, int ro, struct super **sb)
{
	struct blk_buffer *b;
	int err;
	int i;
	
	for (i = 0; i < MAXMOUNTS; i++)
		if (super[i].mounted && super[i].dev == dev)
			return EBUSY;
	
	for (i = 0; i < MAXMOUNTS; i++)
		if (!super[i].mounted)
		{
			err = blk_read(dev, 1, &b);
			if (err)
				return err;
			memcpy(&super[i], b->data, sizeof(super[i]));
			blk_put(b);
			super[i].mounted = 1;
			super[i].dev	 = dev;
			super[i].freeblk = super[i].dblock;
			super[i].nfree	 = -1;
			if (ro)
				super[i].ro = ro;
			*sb = &super[i];
			return 0;
		}
	
	return ENOMEM;
}

int write_super(struct super *sb)
{
	struct blk_buffer *b;
	int err;
	
	if (!sb->ro)
	{
		err = blk_get(sb->dev, 1, &b);
		if (err)
			return err;
		memcpy(b->data, sb, sizeof(*sb));
		b->valid = 1;
		b->dirty = 1;
	}
	
	return 0;
}

static int load_freemap(struct super *sb)
{
	struct blk_buffer *b;
	int i = sb - super;
	blk_t bn, eb;
	size_t ds;
	size_t sz;
	char *p;
	int err;
	
	if (freemap[i] || freedirt[i])
		panic("have freemap");
	
	sz = (sb->nblocks + 7) / 8;
	sz = (sz + 511) & ~511;
	ds = sz / 512;
	freemap[i] = p = malloc(sz);
	freedirt[i] = malloc(ds);
	if (!freemap[i] || !freedirt[i])
	{
		free(freedirt[i]);
		free(freemap[i]);
		freedirt[i] = NULL;
		freemap[i] = NULL;
		return ENOMEM;
	}
	memset(freedirt[i], 0, ds);
	
	eb = sb->bitmap + (sb->nblocks + 4095) / 4096;
	for (bn = sb->bitmap; bn < eb; bn++, p += 512)
	{
		err = blk_read(sb->dev, bn, &b);
		if (err)
		{
			free(freedirt[i]);
			free(freemap[i]);
			freedirt[i] = NULL;
			freemap[i] = NULL;
			return err;
		}
		memcpy(p, b->data, 512);
		blk_put(b);
	}
	return 0;
}

static int save_freemap(struct super *sb)
{
	struct blk_buffer *b;
	int i = sb - super;
	blk_t cnt;
	blk_t bn;
	char *p;
	int err;
	
	if (sb->ro)
		return 0;
	
	p = freemap[i];
	if (!p)
		panic("no freemap");
	
	cnt = (sb->nblocks + 4095) / 4096;
	
	for (bn = 0; bn < cnt; bn++, p += 512)
	{
		if (!freedirt[i][bn])
			continue;
		
		err = blk_get(sb->dev, bn + sb->bitmap, &b);
		if (err)
			return err;
		memcpy(b->data, p, 512);
		b->valid = 1;
		b->dirty = 1;
		blk_put(b);
		
		freedirt[i][bn] = 0;
	}
	return 0;
}

int mountroot(dev_t dev, int ro)
{
	struct inode *ino;
	struct super *sb;
	int err;
	
	err = read_super(dev, ro, &sb);
	if (err)
	{
		printf("root sb %i\n", err);
		panic("bad root");
	}
	
	err = inode_get(sb, sb->root, &ino);
	if (err)
	{
		printf("root ino %i\n", err);
		panic("bad root");
	}
	
	if (!ro)
	{
		err = load_freemap(sb);
		if (err)
			panic("root freemap");
	}
	
	if (!ino->d.mode)
	{
		if (ro)
		{
			printf("rw first\n");
			sb->ro = 0;
			ro = 0;
		}
		
		ino->d.mode	= S_IFDIR | S_IRWXU;
		ino->d.blocks	= 1;
		ino->d.nlink	= 1;
		ino->dirty	= 2;
	}

#if V
	if (ro)
		printf("mounted root (%i,%i) read-only\n", major(dev), minor(dev));
	else
		printf("mounted root (%i,%i) read-write\n", major(dev), minor(dev));
#endif
	
	curr->root   = ino;
	curr->cwd    = ino;
	ino->refcnt += 2;
	return 0;
}

int do_mount(char *dev, char *mnt, int ro)
{
	struct inode *root;
	struct inode *ino;
	struct super *sb;
	dev_t devn;
	int err;
	
	err = dir_traverse(dev, 0, &ino);
	if (err)
		return err;
	if (!S_ISBLK(ino->d.mode))
	{
		inode_put(ino);
		return EPERM;
	}
	devn = ino->d.rdev;
	inode_put(ino);
	
	err = dir_traverse(mnt, 0, &ino);
	if (err)
		return err;
	
	if (ino->mnt_dir || ino->mnt_root)
	{
		inode_put(ino);
		return EBUSY;
	}
	
	err = read_super(devn, ro, &sb);
	if (err)
	{
		inode_put(ino);
		return err;
	}
	
	if (!ro)
	{
		err = load_freemap(sb);
		if (err)
		{
			sb->mounted = 0;
			inode_put(ino);
			return err;
		}
	}
	
	err = inode_get(sb, sb->root, &root);
	if (err)
	{
		sb->mounted = 0;
		inode_put(ino);
		return err;
	}
	
	root->mnt_dir = ino;
	ino->mnt_root = root;
	
	if (!root->d.mode)
	{
		if (ro)
		{
			printf("rw first\n");
			sb->ro = 0;
			ro = 0;
		}
		
		root->d.mode	= S_IFDIR | S_IRWXU;
		root->d.blocks	= 1;
		root->d.nlink	= 1;
		root->dirty	= 2;
	}
	
#if V
	if (ro)
		printf("mounted %s (%s) read-only\n", mnt, dev);
	else
		printf("mounted %s (%s) read-write\n", mnt, dev);
#endif
	
	return 0;
}

int do_umount(char *mnt)
{
	struct super *sb;
	struct inode *ino;
	int err;
	int i;
	
	err = dir_traverse(mnt, 0, &ino);
	if (err)
		return err;
	if (!ino->mnt_dir)
	{
		inode_put(ino);
		return EINVAL;
	}
	
	if (ino->refcnt != 2)
	{
		inode_put(ino);
		return EBUSY;
	}
	
	for (i = 0; i < MAXINODES; i++)
	{
		if (&inode[i] == ino)
			continue;
		
		if (inode[i].refcnt && inode[i].sb == ino->sb)
		{
			inode_put(ino);
			return EBUSY;
		}
	}
	
	sb = ino->sb;
	inode_put(ino->mnt_dir);
	ino->refcnt--;
	inode_put(ino);
	inode_sync(1);
	save_freemap(sb);
	sb->mounted = 0;
	blk_sync(1, 0);
	free(freedirt[sb - super]);
	free(freemap[sb - super]);
	freedirt[sb - super] = freemap[sb - super] = NULL;
#if V
	printf("unmounted %s\n", mnt);
#endif
	return 0;
}

int sys_mount(char *dev, char *mnt, int ro)
{
	char ldev[PATH_MAX];
	char lmnt[PATH_MAX];
	int err;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	err = fustr(ldev, dev, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = fustr(lmnt, mnt, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = do_mount(ldev, lmnt, ro);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_umount(char *mnt)
{
	char lmnt[PATH_MAX];
	int err;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	err = fustr(lmnt, mnt, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = do_umount(lmnt);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

void psync(void)
{
	int i;
	
	for (i = 0; i < MAXMOUNTS; i++)
		if (super[i].mounted)
			save_freemap(&super[i]);
	inode_sync(2);
	blk_sync(0, HZ / 10);
}

int sys_sync()
{
	int i;
	
	for (i = 0; i < MAXMOUNTS; i++)
		if (super[i].mounted)
			save_freemap(&super[i]);
	inode_sync(1);
	blk_sync(0, 0);
	return 0;
}
