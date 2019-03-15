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
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

int all	  = 0;
int kbyte = 0;
int psub  = 1;
int xdev  = 1;

int procopt(char *str)
{
	str++;
	
	while (*str)
	{
		switch (*str)
		{
		case 'a':
			all = 1;
			break;
		case 'k':
			kbyte = 1;
			break;
		case 's':
			psub = 0;
			break;
		case 'x':
			xdev = 0;
			break;
		case '-':
			return 0;
		default:
			fprintf(stderr, "du: bad option '%c'\n", *str);
			exit(1);
		}
		
		str++;
	}
	return 1;
}

blkcnt_t do_du(char *path, dev_t dev, int sub)
{
	blkcnt_t cnt = 0;
	struct stat st;
	int s;
	
	s = (path[strlen(path) - 1] != '/');
	
	if (stat(path, &st))
	{
		perror(path);
		return 0;
	}
	
	if (st.st_dev != dev && dev != -1)
		return 0;
	if (!xdev)
		dev = st.st_dev;
	
	cnt += st.st_blocks;
	
	if (S_ISDIR(st.st_mode))
	{
		char sub[PATH_MAX];
		struct dirent *de;
		DIR *dir;
		
		dir = opendir(path);
		if (!dir)
			perror(path);
		else
		{
			errno = 0;
			while (de = readdir(dir), de)
			{
				if (!strcmp(de->d_name, "."))
					continue;
				if (!strcmp(de->d_name, ".."))
					continue;
				if (strlen(de->d_name) + strlen(path) + 2 >
				    PATH_MAX)
				{
					fprintf(stderr,"%s/%s: name too long\n",
					        path, de->d_name);
					break;
				}
				
				strcpy(sub, path);
				if (s)
					strcat(sub, "/");
				strcat(sub, de->d_name);
				cnt += do_du(sub, dev, 1);
				errno = 0;
			}
			if (errno)
				perror(path);
			closedir(dir);
		}
	}
	
	if ((S_ISDIR(st.st_mode) && psub) || !sub || all)
	{
		if (!kbyte)
			printf("%-10li %s\n", (long)cnt, path);
		else
			printf("%-10li %s\n", (long)cnt / 2, path);
	}
	return cnt;
}

int main(int argc, char **argv)
{
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	
	if (i == argc)
	{
		do_du(".", -1, 0);
		return 0;
	}
	for (; i < argc; i++)
		do_du(argv[i], -1, 0);
	return 0;
}
