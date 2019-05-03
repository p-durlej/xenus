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
#include <xenus/config.h>
#include <xenus/panic.h>
#include <xenus/page.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

int sys_pipe(int *buf)
{
	struct file *file1;
	struct file *file2;
	struct inode *ino;
	int fd[2];
	int err;
	
	err = fd_get(&fd[0], 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = fd_get(&fd[1], fd[0] + 1);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = file_get(&file1);
	if (err)
	{
		uerr(err);
		return -1;
	}
	err = file_get(&file2);
	if (err)
	{
		file_put(file1);
		uerr(err);
		return -1;
	}
	if (curr->root->sb->ro)
	{
		uerr(EROFS);
		return -1;
	}
	err = alloc_inode(curr->root->sb, &ino);
	if (err)
	{
		file_put(file1);
		file_put(file2);
		uerr(err);
		return -1;
	}
	ino->d.mode	= S_IFIFO | 0777;
	ino->dirty	= 1;
	ino->refcnt++;
	
	file1->ino	= ino;
	file1->flags	= O_RDONLY;
	file2->ino	= ino;
	file2->flags	= O_WRONLY;
	file1->read	= pipe_read;
	file1->write	= pipe_write;
	file2->read	= pipe_read;
	file2->write	= pipe_write;
	curr->fd[fd[0]].file = file1;
	curr->fd[fd[1]].file = file2;
	
	err = pipe_open(ino, O_RDWR);
	if (err)
	{
		fd_put(fd[0]);
		fd_put(fd[1]);
		uerr(err);
		return -1;
	}
	err = tucpy(buf, fd, sizeof(fd));
	if (err)
	{
		fd_put(fd[0]);
		fd_put(fd[1]);
		uerr(err);
		return -1;
	}
	return 0;
}

int pipe_open(struct inode *ino, int flags)
{
	if (!ino->pipe_buf)
	{
		ino->pipe_buf = palloc();
		if (!ino->pipe_buf)
			return ENOMEM;
	}
	
	switch (flags & 7)
	{
	case O_RDONLY:
		ino->pipe_readers++;
		
		while (!ino->pipe_writers && !ino->pipe_datlen)
		{
			if (flags & O_NDELAY)
				break;
			if (curr->sig)
			{
				ino->pipe_readers--;
				return EINTR;
			}
			idle();
		}
		return 0;
	case O_WRONLY:
		ino->pipe_writers++;
		
		while (!ino->pipe_readers)
		{
			if (flags & O_NDELAY)
			{
				ino->pipe_writers--;
				return ENXIO;
			}
			if (curr->sig)
			{
				ino->pipe_writers--;
				return EINTR;
			}
			idle();
		}
		return 0;
	case O_RDWR:
		ino->pipe_readers++;
		ino->pipe_writers++;
		return 0;
	default:
		return EINVAL;
	}
	return 0;
}

void pipe_close(struct inode *ino, int flags)
{
	switch (flags & 7)
	{
	case O_RDONLY:
		ino->pipe_readers--;
		break;
	case O_WRONLY:
		ino->pipe_writers--;
		break;
	}
}

static void pipe_getc(struct inode *ino, char *c)
{
	if (!ino->pipe_datlen--)
		panic("pipe getc");
	
	*c = ino->pipe_buf[ino->pipe_rdp++];
	ino->pipe_rdp %= PIPE_BUF;
}

static void pipe_putc(struct inode *ino, char c)
{
	if (ino->pipe_datlen++ == PIPE_BUF)
		panic("pipe putc");
	
	ino->pipe_buf[ino->pipe_wrp++] = c;
	ino->pipe_wrp %= PIPE_BUF;
}

int pipe_read(struct rwreq *req)
{
	struct inode *ino = req->ino;
	size_t count	  = req->count;
	int l;
	
	while (count)
	{
		char ch;
		
		if (!ino->pipe_datlen)
		{
			if (count != req->count || req->nodelay ||
			    !ino->pipe_writers)
			{
				req->count -= count;
				if (req->count)
					wakeup();
				return 0;
			}
			if (curr->sig)
				return EINTR;
			idle();
			continue;
		}
		pipe_getc(ino, &ch);
		tucpy(req->buf++, &ch, 1);
		count--;
	}
	if (req->count)
		wakeup();
	return 0;
}

int pipe_write(struct rwreq *req)
{
	struct inode *ino = req->ino;
	size_t count	  = req->count;
	int l;
	
	while (count <= PIPE_BUF && PIPE_BUF - ino->pipe_datlen < req->count)
	{
		if (req->nodelay)
		{
			req->count = 0;
			return 0;
		}
		if (curr->sig)
			return EINTR;
		idle();
	}
	
	while (count)
	{
		char ch;
		
		if (!ino->pipe_readers)
		{
			sendsig(curr, SIGPIPE);
			return EPIPE;
		}
		if (ino->pipe_datlen == PIPE_BUF)
		{
			if (curr->sig)
				return EINTR;
			if (req->nodelay)
			{
				req->count -= count;
				if (req->count)
					wakeup();
				return 0;
			}
			idle();
			continue;
		}
		if (!ino->pipe_buf && !(ino->pipe_buf = palloc()))
			return ENOMEM;
		fucpy(&ch, req->buf++, 1);
		pipe_putc(ino, ch);
		count--;
	}
	if (req->count)
		wakeup();
	return 0;
}
