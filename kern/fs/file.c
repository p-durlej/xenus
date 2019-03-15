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
#include <xenus/printf.h>
#include <xenus/fs.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>

void pipe_close(struct inode *ino, int flags);

struct file file[MAXFILES];

int file_get(struct file **buf)
{
	int i;
	
	for (i = 0; i < MAXFILES; i++)
		if (!file[i].refcnt)
		{
			file[i].refcnt	= 1;
			file[i].ino	= NULL;
			file[i].flags	= 0;
			file[i].pos	= 0;
			*buf = &file[i];
			return 0;
		}
	
	printf("no files\n");
	return ENFILE;
}

int file_put(struct file *file)
{
	file->refcnt--;
	if (!file->refcnt && file->ino)
	{
		if (S_ISFIFO(file->ino->d.mode))
			pipe_close(file->ino, file->flags);
		return inode_put(file->ino);
	}
	return 0;
}

int fd_get(int *fd, int lowest)
{
	int i;
	
	for (i = lowest; i < MAXFDS; i++)
		if (!curr->fd[i].file)
		{
			curr->fd[i].cloexec = 0;
			*fd = i;
			return 0;
		}
	return EMFILE;
}

int fd_put(int fd)
{
	if (!curr->fd[fd].file)
		return EBADF;
	file_put(curr->fd[fd].file);
	curr->fd[fd].file	= NULL;
	curr->fd[fd].cloexec	= 0;
	return 0;
}

int fd_chk(int fd, int mode)
{
	struct file *file;
	int m;
	
	if (fd < 0 || (fd >= MAXFDS && !curr->fd[fd].file))
		return EBADF;
	
	file = curr->fd[fd].file;
	
	switch (file->flags & 3)
	{
	case O_RDONLY:
		m = R_OK;
		break;
	case O_WRONLY:
		m = W_OK;
		break;
	case O_RDWR:
		m = R_OK | W_OK;
		break;
	default:
		return EBADF;
	}
	
	if ((mode & m) != mode)
		return EBADF;
	return 0;
}

void fd_cloexec(void)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (curr->fd[i].cloexec)
			fd_put(i);
}
