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

#include <sys/mount.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "v7compat.h"

char *ubin;

struct v7_dirent
{
	unsigned short ino;
	char name[14];
};

char *xbin(char *path)
{
	static char bufs[2][PATH_MAX];
	static int bufi;
	
	char *buf = bufs[bufi];
	
	if (!ubin)
		return path;
	
	if (!strncmp(path, "/bin/", 5))
	{
		bufi ^= 1;
		
		strcpy(buf, ubin);
		strcat(buf, path + 4);
		return buf;
	}
	
	if (!strcmp(path, "/bin"))
		return ubin;
	
	return path;
}

static int v7_read_dir(int fd, void *p, size_t cnt)
{
	struct v7_dirent *de7 = p;
	struct dirent de;
	size_t resid = cnt;
	ssize_t rcnt;
	
#if V7DEBUG
//	mprintf("%s fd %i buf %p cnt %u\n", __func__, fd, p, cnt);
#endif
	while (resid >= sizeof *de7)
	{
		rcnt = read(fd, &de, sizeof de);
		if (rcnt != sizeof de)
		{
			if (rcnt > 0)
				errno = EINVAL;
			if (!rcnt)
				break;
			return -1;
		}
		
		memset(de7, 0, sizeof *de7);
		memcpy(de7->name, de.d_name, sizeof de7->name);
		de7->ino = de.d_ino;
		
		resid -= sizeof *de7;
		de7++;
	}
#if V7DEBUG
//	mprintf("%s ret %i\n", __func__, cnt - resid);
#endif
	return cnt - resid;
}

int v7_read(int fd, void *buf, int len)
{
	struct stat st;
	
	if (fstat(fd, &st))
		return -1;
	
	if (S_ISDIR(st.st_mode))
		return v7_read_dir(fd, buf, len);
	
	return read(fd, buf, len);
}

struct v7_stat
{
	short		dev;
	unsigned short	ino;
	unsigned short	mode;
	short		nlink;
	short		uid;
	short		gid;
	short		rdev;
	unsigned	size;
	long		atime;
	long		mtime;
	long		ctime;
};

static void convstat(struct v7_stat *st7, struct stat *st)
{
	memset(st7, 0, sizeof *st7);
	st7->dev   = st->st_dev;
	st7->ino   = st->st_ino;
	st7->mode  = st->st_mode;
	st7->nlink = st->st_nlink;
	st7->uid   = st->st_uid;
	st7->gid   = st->st_gid;
	st7->rdev  = st->st_rdev;
	st7->size  = st->st_size;
	st7->atime = st->st_atime;
	st7->mtime = st->st_mtime;
	st7->ctime = st->st_ctime;
	
	if (S_ISDIR(st->st_mode))
		st7->size /= 2;
}

int v7_stat(char *path, void *st7)
{
	struct stat st;
	
	if (stat(xbin(path), &st))
		return -1;
	convstat(st7, &st);
	return 0;
}

int v7_fstat(int fd, void *st7)
{
	struct stat st;
	
	if (fstat(fd, &st))
		return -1;
	convstat(st7, &st);
	return 0;
}

int v7_open(char *path, int mode)
{
	if (mode < 0 || mode > 2)
	{
		errno = EINVAL;
		return -1;
	}
	return open(xbin(path), mode);
}

int v7_dup(int ofd, int nfd)
{
	if (ofd & 0100)
		return dup2(ofd & ~0100, nfd);
	return dup(ofd);
}

int v7_chmod(char *path, mode_t mode)
{
	return chmod(xbin(path), mode);
}

int v7_creat(char *path, mode_t mode)
{
	return creat(xbin(path), mode);
}

int v7_mknod(char *path, mode_t mode, dev_t rdev)
{
	return mknod(xbin(path), mode, rdev);
}

int v7_mkfifo(char *path, mode_t mode)
{
	return mknod(xbin(path), mode);
}

int v7_mkdir(char *path, mode_t mode)
{
	return mkdir(xbin(path), mode);
}

int v7_chdir(char *path)
{
	return chdir(xbin(path));
}

int v7_chroot(char *path)
{
	return chroot(xbin(path));
}

int v7_unlink(char *name)
{
	return unlink(xbin(name));
}

int v7_link(char *name1, char *name2)
{
	return link(xbin(name1), xbin(name2));
}

int v7_access(char *name, int mode)
{
	return access(xbin(name), mode);
}

int v7_chown(char *path, int uid, int gid)
{
	return chown(xbin(path), uid, gid);
}

int v7_mount(char *dev, char *mnt, int ro)
{
	return mount(xbin(dev), xbin(mnt), ro);
}

int v7_umount(char *mnt)
{
	return umount(xbin(mnt));
}
