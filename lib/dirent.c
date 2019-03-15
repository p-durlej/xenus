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

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

typedef struct DIR
{
	int		fd;
	struct dirent	de;
	char		nul;
} DIR;

DIR *opendir(char *path)
{
	struct stat st;
	DIR *dir;
	
	dir = calloc(sizeof(DIR), 1);
	if (!dir)
		return NULL;
	
	dir->fd = open(path, O_RDONLY);
	if (dir->fd < 0)
	{
		free(dir);
		return NULL;
	}
	
	if (fstat(dir->fd, &st))
	{
		closedir(dir);
		return NULL;
	}
	
	if (!S_ISDIR(st.st_mode))
	{
		closedir(dir);
		errno = ENOTDIR;
		return NULL;
	}
	
	return dir;
}

int closedir(DIR *dir)
{
	if (!dir)
	{
		errno = EBADF;
		return -1;
	}
	close(dir->fd);
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	do
	{
		if (read(dir->fd, &dir->de, sizeof(dir->de)) != sizeof(dir->de))
			return NULL;
	} while(!dir->de.d_ino);
	
	return &dir->de;
}

void rewinddir(DIR *dir)
{
	lseek(dir->fd, (off_t)0, SEEK_SET);
}

void seekdir(DIR *dir, off_t off)
{
	lseek(dir->fd, off, SEEK_SET);
}

off_t telldir(DIR *dir)
{
	return lseek(dir->fd, (off_t)0, SEEK_CUR);
}
