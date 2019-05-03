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

#include "mnxcompat.h"

struct mnx_dirent
{
	unsigned short ino;
	char name[30];
};

static int mnx_read_dir(int fd, void *p, size_t cnt)
{
	struct mnx_dirent *mde = p;
	struct dirent de;
	size_t resid = cnt;
	ssize_t rcnt;
	
	while (resid >= sizeof *mde)
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
		
		memset(mde, 0, sizeof *mde);
		memcpy(mde->name, de.d_name, sizeof de.d_name);
		mde->ino = de.d_ino;
		
		resid -= sizeof *mde;
		mde++;
	}
	return cnt - resid;
}

int mnx_read(struct message *msg)
{
	struct stat st;
	
	if (fstat(msg->m1.i1, &st))
		return -1;
	
	if (S_ISDIR(st.st_mode))
		return mnx_read_dir(msg->m1.i1, msg->m1.p1, msg->m1.i2);
	
	return read(msg->m1.i1, msg->m1.p1, msg->m1.i2);
}

int mnx_write(struct message *msg)
{
	return write(msg->m1.i1, msg->m1.p1, msg->m1.i2);
}

static int fd_m2x(int mf)
{
	int fl = mf & 3;
	
	if (mf & 00100) fl |= O_CREAT;
	if (mf & 00200) fl |= O_EXCL;
	// if (mf & 00400) fl |= O_NOCTTY;
	if (mf & 01000) fl |= O_TRUNC;
	if (mf & 02000) fl |= O_APPEND;
	if (mf & 04000) fl |= O_NONBLOCK;
	
	return fl;
}

static int fd_x2m(int fl)
{
	int mf = fl & 3;
	
	if (fl & O_CREAT)    mf |= 00100;
	if (fl & O_EXCL)     mf |= 00200;
	// if (fl & O_NOCTTY)   mf |= 00400;
	if (fl & O_TRUNC)    mf |= 01000;
	if (fl & O_APPEND)   mf |= 02000;
	if (fl & O_NONBLOCK) mf |= 04000;
	
	return mf;
}

int mnx_open(struct message *msg)
{
	int mf = msg->m1.i2;
	int fl = fd_m2x(mf);
	
	if (fl & O_CREAT)
		return open(msg->m1.p1, fl, msg->m1.i3);
	else
		return open(msg->m3.p1, fl);
}

int mnx_creat(struct message *msg)
{
	return creat(msg->m3.p1, msg->m3.i2);
}

int mnx_close(struct message *msg)
{
	return close(msg->m1.i1);
}

int mnx_lseek(struct message *msg)
{
	return lseek(msg->m2.i1, msg->m2.l1, msg->m2.i2);
}

int mnx_mkdir(struct message *msg)
{
	return mkdir(msg->m1.p1, msg->m1.i2);
}

int mnx_rmdir(struct message *msg)
{
	return rmdir(msg->m3.p1);
}

int mnx_unlink(struct message *msg)
{
	return unlink(msg->m3.p1);
}

int mnx_chdir(struct message *msg)
{
	return chdir(msg->m3.p1);
}

int mnx_chroot(struct message *msg)
{
	return chroot(msg->m3.p1);
}

int mnx_access(struct message *msg)
{
	return access(msg->m3.p1, msg->m3.i2);
}

int mnx_link(struct message *msg)
{
	return link(msg->m1.p1, msg->m1.p2);
}

int mnx_dup(struct message *msg)
{
	int ofd = msg->m1.i1;
	int nfd = msg->m1.i2;
	
	if (ofd & 0100)
		return dup2(ofd & ~0100, nfd);
	return dup(ofd);
}

struct mnx_flock
{
	short	 type;
	short	 whence;
	unsigned start;
	unsigned len;
	int	 pid;
};

int mnx_fcntl(struct message *msg)
{
	struct mnx_flock *lk = msg->m1.p1;
	unsigned cmd = msg->m1.i2;
	int fd = msg->m1.i1;
	int fl;
	
	switch (cmd & 0xffff)
	{
	case 0:
		return fcntl(fd, F_DUPFD, msg->m1.i3);
	case 1:
		fl = fcntl(fd, F_GETFD);
		if (fl < 0)
			return -1;
		return fd_x2m(fl);
	case 2:
		return fcntl(fd, F_SETFD, fd_m2x(msg->m1.i3));
	case 3:
		return fcntl(fd, F_GETFL) & 1;
	case 4:
		return fcntl(fd, F_SETFL, msg->m1.i3 & 1);
	case 5:
		memset(lk, 0, sizeof *lk);
		lk->type = 3;
		return 0;
	case 6:
	case 7:
		errno = ENOSYS;
		return -1;
	default:
#if MNXDEBUG
		mprintf("mnx_fcntl 0x%x\n", cmd);
#endif
		errno = EINVAL;
		return -1;
	}
	return 0;
}

struct mnx_stat
{
	short		dev;
	unsigned short	ino;
	unsigned short	mode;
	short		nlink;
	short		uid;
	char		gid;
	short		rdev;
	unsigned	size;
	long		atime;
	long		mtime;
	long		ctime;
};

static void convstat(struct mnx_stat *mst, struct stat *st)
{
	memset(mst, 0, sizeof *mst);
	mst->dev   = st->st_dev;
	mst->ino   = st->st_ino;
	mst->mode  = st->st_mode;
	mst->nlink = st->st_nlink;
	mst->uid   = st->st_uid;
	mst->gid   = st->st_gid;
	mst->rdev  = st->st_rdev;
	mst->size  = st->st_size;
	mst->atime = st->st_atime;
	mst->mtime = st->st_mtime;
	mst->ctime = st->st_ctime;
}

int mnx_stat(struct message *msg)
{
	struct mnx_stat *mst = msg->m1.p2;
	struct stat st;
	
	if (stat(msg->m1.p1, &st))
		return -1;
	convstat(mst, &st);
	return 0;
}

int mnx_fstat(struct message *msg)
{
	struct mnx_stat *mst = msg->m1.p1;
	struct stat st;
	
	if (fstat(msg->m1.i1, &st))
		return -1;
	convstat(mst, &st);
	return 0;
}

int mnx_mknod(struct message *msg)
{
	return mknod(msg->m1.p1, msg->m1.i2, msg->m1.i3);
}

int mnx_chmod(struct message *msg)
{
	return chmod(msg->m3.p1, msg->m3.i2);
}

int mnx_chown(struct message *msg)
{
	return chown(msg->m1.p1, msg->m1.i2, msg->m1.i3);
}

int mnx_pipe(struct message *msg)
{
	int fd[2];
	
	if (pipe(fd))
		return -1;
	
	msg->m1.i1 = fd[0];
	msg->m1.i2 = fd[1];
	return 0;
}

int mnx_utime(struct message *msg)
{
	struct utimbuf tb;
	
	if (msg->m2.i1)
	{
		tb.actime  = msg->m2.l1;
		tb.modtime = msg->m2.l2;
	}
	else
		tb.modtime = tb.actime = time(NULL);
	
	return utime(msg->m2.p1, &tb);
}

int mnx_mount(struct message *msg)
{
	return mount(msg->m1.p1, msg->m1.p2, msg->m1.i3);
}

int mnx_umount(struct message *msg)
{
	return umount(msg->m3.p1); // XXX Minix allows either device or dir
}
