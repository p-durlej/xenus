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

#include <xenus/process.h>
#include <xenus/syscall.h>
#include <xenus/printf.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define MAXFSZ	((BMAP0_SIZE + BMAP1_SIZE * 128 + BMAP2_SIZE * 128 * 128) * 512)
#define MAXOFF	0x80000000

ssize_t sys_read(int fd, void *buf, size_t count)
{
	struct file *file;
	struct rwreq rw;
	int err;
	
	err = fd_chk(fd, R_OK);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = uwchk(buf, count);
	if (err)
	{
		uerr(err);
		return -1;
	}
	file = curr->fd[fd].file;
	
	if (file->pos < 0)
	{
		uerr(EINVAL);
		return -1;
	}
	if (file->pos + count > MAXOFF)
	{
		uerr(EFBIG);
		return -1;
	}
	
	if (!file->ino->sb->ro)
	{
		file->ino->d.atime = time.time;
		if (!file->ino->dirty)
			file->ino->dirty = 1;
	}
	
	rw.ino		= file->ino;
	rw.buf		= buf;
	rw.start	= file->pos;
	rw.count	= count;
	rw.nodelay	= file->flags & O_NDELAY;
	
	if (file->read)
		err = file->read(&rw);
	else
		err = ENOSYS;
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	file->pos = rw.start;
	return rw.count;
}

ssize_t sys_write(int fd, void *buf, size_t count)
{
	struct file *file;
	struct rwreq rw;
	int err;
	
	err = fd_chk(fd, W_OK);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = urchk(buf, count);
	if (err)
	{
		uerr(err);
		return -1;
	}
	file = curr->fd[fd].file;
	
	if (file->pos < 0)
	{
		uerr(EINVAL);
		return -1;
	}
	if (file->flags & O_APPEND)
		file->pos = file->ino->d.size;
	if (file->pos + count > MAXOFF)
	{
		uerr(EFBIG);
		return -1;
	}
	
	rw.ino		= file->ino;
	rw.buf		= buf;
	rw.start	= file->pos;
	rw.count	= count;
	rw.nodelay	= file->flags & O_NDELAY;
	
	if (file->write)
		err = file->write(&rw);
	else
		err = ENOSYS;
	
	if (!file->ino->sb->ro)
	{
		file->ino->d.mtime = time.time;
		if (!file->ino->dirty)
			file->ino->dirty = 1;
	}
	
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	file->pos = rw.start;
	return rw.count;
}

off_t sys_lseek(int fd, off_t off, int whence)
{
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (S_ISFIFO(curr->fd[fd].file->ino->d.mode))
	{
		uerr(ESPIPE);
		return -1;
	}
	
	switch (whence)
	{
	case SEEK_SET:
		curr->fd[fd].file->pos = off;
		break;
	case SEEK_CUR:
		curr->fd[fd].file->pos += off;
		break;
	case SEEK_END:
		curr->fd[fd].file->pos = curr->fd[fd].file->ino->d.size + off;
		break;
	default:
		uerr(EINVAL);
		return -1;
	}
	return curr->fd[fd].file->pos;
}

int reg_read(struct rwreq *req)
{
	struct inode *ino = req->ino;
	char *buf	  = req->buf;
	size_t count	  = req->count;
	off_t pos	  = req->start;
	
	if (pos > ino->d.size)
	{
		req->count = 0;
		return 0;
	}
	if (count + pos > ino->d.size)
	{
		count = ino->d.size - pos;
		req->count = count;
	}
	
	while (count)
	{
		blk_t blk = pos / BLK_SIZE;
		int err;
		int l;
		
		l = BLK_SIZE - pos % BLK_SIZE;
		if (l > count)
			l = count;
		err = bmap(ino, &blk);
		if (err)
			return err;
		
		if (blk)
		{
			err = blk_upread(ino->sb->dev, blk, pos % BLK_SIZE, l, buf);
			if (err)
				return err;
		}
		else
			err = uset(buf, 0, l);
		
		buf	+= l;
		count	-= l;
		pos	+= l;
	}
	req->start = pos;
	return 0;
}

int reg_write(struct rwreq *req)
{
	struct inode *ino = req->ino;
	char *buf	  = req->buf;
	size_t count	  = req->count;
	off_t pos	  = req->start;
	
	if (count + pos > curr->fslimit * BLK_SIZE)
		return EFBIG;
	if (count + pos > MAXFSZ)
		return EFBIG;
	
	if (count + pos > ino->d.size)
	{
		ino->d.size = count + pos;
		ino->dirty  = 2;
	}
	while (count)
	{
		blk_t blk = pos / BLK_SIZE;
		int err;
		int l;
		
		l = BLK_SIZE - pos % BLK_SIZE;
		if (l > count)
			l = count;
		
		err = bmap_alloc(ino, &blk);
		if (err)
			return err;
		
		if (blk)
		{
			err = blk_upwrite(ino->sb->dev, blk, pos % BLK_SIZE, l, buf);
			if (err)
				return err;
		}
		else
			err = uset(buf, 0, l);
		
		buf	+= l;
		count	-= l;
		pos	+= l;
	}
	req->start = pos;
	return 0;
}

int blk_lread(struct rwreq *req)
{
	struct inode *ino = req->ino;
	char *buf	  = req->buf;
	size_t count	  = req->count;
	off_t pos	  = req->start;
	
	while (count)
	{
		blk_t blk = pos / BLK_SIZE;
		int err;
		int l;
		
		l = BLK_SIZE - pos % BLK_SIZE;
		if (l > count)
			l = count;
		
		err = blk_upread(ino->d.rdev, blk, pos % BLK_SIZE, l, buf);
		if (err)
			return err;
		
		buf	+= l;
		count	-= l;
		pos	+= l;
	}
	req->start = pos;
	return 0;
}

int blk_lwrite(struct rwreq *req)
{
	struct inode *ino = req->ino;
	char *buf	  = req->buf;
	size_t count	  = req->count;
	off_t pos	  = req->start;
	
	while (count)
	{
		blk_t blk = pos / BLK_SIZE;
		int err;
		int l;
		
		l = BLK_SIZE - pos % BLK_SIZE;
		if (l > count)
			l = count;
		
		err = blk_upwrite(ino->d.rdev, blk, pos % BLK_SIZE, l, buf);
		if (err)
			return err;
		
		buf	+= l;
		count	-= l;
		pos	+= l;
	}
	req->start = pos;
	return 0;
}

int blk_ioctl(struct inode *ino, int cmd, void *ptr)
{
	dev_t dev = ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXBDEVS)
	{
		printf("ioctl blk bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!blk_driver[i].ioctl)
		return ENOTTY;
	
	return blk_driver[i].ioctl(dev, cmd, ptr);
}

int sys_ioctl(int fd, int cmd, void *ptr)
{
	struct file *file;
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return err;
	}
	file = curr->fd[fd].file;
	if (file->ioctl)
		err = file->ioctl(file->ino, cmd, ptr);
	else
		err = ENOTTY;
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_ttyname3(int fd, char *buf, size_t len)
{
	char name[PATH_MAX];
	int err;
	
	err = fd_chk(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	if (!S_ISCHR(curr->fd[fd].file->ino->d.mode))
	{
		uerr(ENOTTY);
		return -1;
	}
	err = chr_ttyname(curr->fd[fd].file->ino, name);
	if (err)
	{
		uerr(err);
		return -1;
	}
	if (strlen(name) >= len)
	{
		uerr(ERANGE);
		return -1;
	}
	err = tucpy(buf, name, strlen(name));
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}
