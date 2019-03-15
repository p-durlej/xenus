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
#include <xenus/config.h>
#include <xenus/printf.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <limits.h>
#include <unistd.h>
#include <stddef.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>

int do_open(char *path, int flags, mode_t mode, int *fdp)
{
	struct inode *dir;
	struct inode *ino;
	struct file *file;
	int creat = 0;
	int err;
	int fd;
	
	mode &= 0777;
	
	err = dir_traverse(path, 0, &ino);
	if (!err && (flags & O_EXCL))
	{
		inode_put(ino);
		return EEXIST;
	}
	if (err == ENOENT && (flags & O_CREAT))
	{
		err = dir_traverse(path, 1, &dir);
		if (err)
			return err;
		
		err = alloc_inode(dir->sb, &ino);
		if (err)
		{
			inode_put(dir);
			return err;
		}
		
		err = dir_creat(dir, basename(path), ino->ino);
		if (err)
		{
			inode_put(dir);
			inode_put(ino);
			return err;
		}
		
		ino->d.mode	= S_IFREG | (mode & ~curr->umask);
		ino->d.nlink	= 1;
		ino->dirty	= 2;
		
		inode_put(dir);
		creat = 1;
	}
	if (err)
		return err;
	if (S_ISDIR(ino->d.mode) && (flags & 7) != O_RDONLY)
	{
		inode_put(ino);
		return EISDIR;
	}
	if ((flags & O_TRUNC) && S_ISREG(ino->d.mode))
	{
		err = inode_chkperm(ino, W_OK);
		if (err)
		{
			inode_put(ino);
			return err;
		}
		err = trunc(ino, 1);
		if (err)
		{
			inode_put(ino);
			return err;
		}
	}
	if (!creat)
		switch (flags & 7)
		{
		case O_RDONLY:
			err = inode_chkperm(ino, R_OK);
			break;
		case O_WRONLY:
			err = inode_chkperm(ino, W_OK);
			break;
		case O_RDWR:
			err = inode_chkperm(ino, R_OK | W_OK);
			break;
		default:
			err = EINVAL;
		}
	if (err)
	{
		inode_put(ino);
		return err;
	}
	err = file_get(&file);
	if (err)
	{
		inode_put(ino);
		return err;
	}
	err = fd_get(&fd, 0);
	if (err)
	{
		inode_put(ino);
		file_put(file);
		return err;
	}
	switch (ino->d.mode & S_IFMT)
	{
	case S_IFREG:
		file->read  = reg_read;
		file->write = reg_write;
		file->ioctl = NULL;
		break;
	case S_IFDIR:
		file->read  = reg_read;
		file->write = NULL;
		file->ioctl = NULL;
		break;
	case S_IFBLK:
		file->read  = blk_lread;
		file->write = blk_lwrite;
		file->ioctl = blk_ioctl;
		break;
	case S_IFCHR:
		err = chr_open(ino, flags & O_NDELAY);
		if (err)
		{
			inode_put(ino);
			file_put(file);
			return err;
		}
		file->read  = chr_read;
		file->write = chr_write;
		file->ioctl = chr_ioctl;
		break;
	case S_IFIFO:
		file->read  = pipe_read;
		file->write = pipe_write;
		file->ioctl = NULL;
		break;
	default:
		printf("open %s bad inode\n", path);
		inode_put(ino);
		file_put(file);
		return EINVAL;
	}
	if (S_ISFIFO(ino->d.mode))
	{
		err = pipe_open(ino, flags);
		if (err)
		{
			inode_put(ino);
			file_put(file);
			return err;
		}
	}
	
	curr->fd[fd].file = file;
	file->flags = flags;
	file->ino   = ino;
	*fdp = fd;
	return 0;
}

int sys_open(char *path, int flags, mode_t mode)
{
	char lpath[PATH_MAX];
	int err;
	int fd;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = do_open(lpath, flags, mode, &fd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return fd;
}

int sys__ctty(char *path)
{
	char lpath[PATH_MAX];
	struct inode *ino;
	int err;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	if (!path)
	{
		inode_put(curr->tty);
		curr->tty = NULL;
		return 0;
	}
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = dir_traverse(lpath, 0, &ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	if (!S_ISCHR(ino->d.mode))
	{
		inode_put(ino);
		uerr(ENOTTY);
		return -1;
	}
	err = chr_ttyname(ino, lpath);
	if (err)
	{
		inode_put(ino);
		uerr(err);
		return -1;
	}
	inode_put(curr->tty);
	curr->tty = ino;
	return 0;
}

int sys_close(int fd)
{
	struct inode *ino;
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	ino = curr->fd[fd].file->ino;
	
	if (S_ISCHR(ino->d.mode))
		err = chr_close(ino);
	
	fd_put(fd);
	return err;
}

static void inode_stat(struct inode *ino, struct stat *st)
{
	st->st_ino	= ino->ino;
	st->st_dev	= ino->sb->dev;
	st->st_rdev	= ino->d.rdev;
	st->st_mode	= ino->d.mode;
	st->st_nlink	= ino->d.nlink;
	st->st_uid	= ino->d.uid;
	st->st_gid	= ino->d.gid;
	st->st_size	= ino->d.size;
	st->st_blksize	= BLK_SIZE;
	st->st_blocks	= ino->d.blocks;
	st->st_atime	= ino->d.atime;
	st->st_mtime	= ino->d.mtime;
	st->st_ctime	= ino->d.ctime;
}

int inode_chmod(struct inode *ino, mode_t mode)
{
	if ((ino->d.mode & 07777) == mode)
		return 0;
	if (curr->euid && ino->d.uid != curr->euid)
		return EPERM;
	if (ino->sb->ro)
		return EROFS;
	
	ino->d.mode &=	     ~07777;
	ino->d.mode |= mode & 07777;
	ino->d.ctime = time.time;
	ino->dirty   = 2;
	return 0;
}

int inode_chown(struct inode *ino, uid_t uid, gid_t gid)
{
	if (ino->d.uid == uid && ino->d.gid == gid)
		return 0;
	if (curr->euid)
		return EPERM;
	if (ino->sb->ro)
		return EROFS;
	
	ino->d.ctime = time.time;
	ino->d.uid = uid;
	ino->d.gid = gid;
	ino->dirty = 2;
	return 0;
}

int sys_fstat(int fd, struct stat *buf)
{
	struct inode *ino;
	struct stat st;
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	ino = curr->fd[fd].file->ino;
	inode_stat(ino, &st);
	
	err = tucpy(buf, &st, sizeof(st));
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_fchmod(int fd, mode_t mode)
{
	struct inode *ino;
	struct stat st;
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
		goto fail;
	
	ino = curr->fd[fd].file->ino;
	err = inode_chmod(ino, mode);
	if (err)
		goto fail;
	
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_fchown(int fd, uid_t uid, gid_t gid)
{
	struct inode *ino;
	struct stat st;
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
		goto fail;
	
	ino = curr->fd[fd].file->ino;
	err = inode_chown(ino, uid, gid);
	if (err)
		goto fail;
	
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_stat(char *path, struct stat *buf)
{
	struct inode *ino = NULL;
	struct stat st;
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
		goto fail;
	
	inode_stat(ino, &st);
	
	err = tucpy(buf, &st, sizeof(st));
	if (err)
		goto fail;
	
	inode_put(ino);
	return 0;
fail:
	inode_put(ino);
	uerr(err);
	return -1;
}

int sys_chmod(char *path, mode_t mode)
{
	struct inode *ino;
	struct stat st;
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
		goto fail;
	
	err = inode_chmod(ino, mode);
	if (err)
		goto fail;
	
	inode_put(ino);
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_chown(char *path, uid_t uid, gid_t gid)
{
	struct inode *ino;
	struct stat st;
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
		goto fail;
	
	err = inode_chown(ino, uid, gid);
	if (err)
		goto fail;
	
	inode_put(ino);
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_access(char *name, int mode)
{
	char lname[PATH_MAX];
	struct inode *ino;
	int err;
	
	err = fustr(lname, name, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = dir_traverse(lname, 0, &ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = inode_chkperm_rid(ino, mode);
	inode_put(ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_chdir(char *path)
{
	char lpath[PATH_MAX];
	struct inode *ino;
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = inode_chkperm(ino, X_OK);
	if (err)
	{
		inode_put(ino);
		uerr(err);
		return -1;
	}
	
	if (!S_ISDIR(ino->d.mode))
	{
		inode_put(ino);
		uerr(ENOTDIR);
		return -1;
	}
	
	
	inode_put(curr->cwd);
	curr->cwd = ino;
	return 0;
}

int sys_chroot(char *path)
{
	char lpath[PATH_MAX];
	struct inode *ino;
	int err;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (!S_ISDIR(ino->d.mode))
	{
		inode_put(ino);
		uerr(ENOTDIR);
		return -1;
	}
	
	inode_put(curr->root);
	curr->root = ino;
	return 0;
}

int sys_fcntl(int fd, int cmd, long arg)
{
	int err;
	int nfd;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	switch (cmd)
	{
	case F_DUPFD:
		err = fd_get(&nfd, arg);
		if (err)
		{
			uerr(err);
			return -1;
		}
		curr->fd[nfd] = curr->fd[fd];
		curr->fd[nfd].file->refcnt++;
		return nfd;
	case F_GETFD:
		return curr->fd[fd].cloexec;
	case F_SETFD:
		curr->fd[fd].cloexec = arg;
		return 0;
	case F_GETFL:
		return curr->fd[fd].file->flags;
	case F_SETFL:
		curr->fd[fd].file->flags &= 	 ~(O_APPEND|O_NDELAY);
		curr->fd[fd].file->flags |= arg & (O_APPEND|O_NDELAY);
		return 0;
	default:
		uerr(EINVAL);
		return -1;
	}
}

int sys_dup(int fd)
{
	int err;
	int nfd;
	
	err = fd_chk(fd, 0);
	if (err)
		goto fail;
	err = fd_get(&nfd, 0);
	if (err)
		goto fail;
	curr->fd[nfd] = curr->fd[fd];
	curr->fd[nfd].file->refcnt++;
	return nfd;
fail:
	uerr(err);
	return -1;
}

int sys_dup2(int ofd, int nfd)
{
	int err;
	
	err = fd_chk(ofd, 0);
	if (err)
		goto fail;
	
	if (nfd >= OPEN_MAX)
	{
		uerr(EBADF);
		return -1;
	}
	if (ofd == nfd)
		return 0;
	
	fd_put(nfd);
	
	curr->fd[nfd] = curr->fd[ofd];
	curr->fd[nfd].file->refcnt++;
	return nfd;
fail:
	uerr(err);
	return -1;
}

int sys__killf(char *path, int sig)
{
	char lpath[PATH_MAX];
	struct inode *ino;
	int err;
	int i;
	int n;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	for (i = 0; i < npact; i++)
	{
		if (pact[i] == curr)
			continue;
		if (pact[i]->cwd == ino || pact[i]->root == ino || pact[i]->tty == ino)
		{
			sendsig(pact[i], sig);
			continue;
		}
		for (n = 0; n < OPEN_MAX; n++)
			if (pact[i]->fd[n].file && pact[i]->fd[n].file->ino == ino)
			{
				sendsig(pact[i], sig);
				continue;
			}
	}
	
	inode_put(ino);
	return 0;
}

int sys_utime(char *path, struct utimbuf *tp)
{
	struct inode *ino = NULL;
	char lpath[PATH_MAX];
	struct utimbuf ltp;
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = fucpy(&ltp, tp, sizeof ltp);
	if (err)
		goto fail;
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
		goto fail;
	
	if (curr->euid && ino->d.uid != curr->euid)
		return EPERM;
	if (ino->sb->ro)
		return EROFS;
	
	ino->d.mtime = ltp.modtime;
	ino->d.atime = ltp.actime;
	ino->d.ctime = time.time;
	ino->dirty   = 2;
fail:
	inode_put(ino);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}
