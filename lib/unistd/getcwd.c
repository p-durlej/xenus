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
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

char *getcwd(char *buf, int len)
{
	char cwd[PATH_MAX];
	char sub[PATH_MAX];
	struct dirent *de;
	struct stat rst;
	struct stat st;
	ino_t inum[32];
	dev_t dnum[32];
	int i = 0;
	DIR *d;
	
	if (stat("/", &rst))
		return NULL;
	
	strcpy(cwd, ".");
	do
	{
		if (i == 32)
		{
			errno = ENAMETOOLONG;
			return NULL;
		}
		
		if (stat(cwd, &st))
			return NULL;
		
		inum[i] = st.st_ino;
		dnum[i] = st.st_dev;
		
		if (strlen(cwd) + 3 >= PATH_MAX)
		{
			errno = ENAMETOOLONG;
			return NULL;
		}
		
		strcat(cwd, "/..");
		i++;
	} while (st.st_ino != rst.st_ino || st.st_dev != rst.st_dev);
	
	i--;
	*cwd = 0;
	while (i)
	{
		i--;
		
		if (*cwd)
			d = opendir(cwd);
		else
			d = opendir("/");
		
		if (!d)
			return NULL;
		
		do
		{
			errno = 0;
			de = readdir(d);
			if (!de)
			{
				if (!errno)
					errno = ENOENT;
				
				closedir(d);
				return NULL;
			}
			
			if (strlen(cwd) + strlen(de->d_name) + 1 >= PATH_MAX)
			{
				errno = ENAMETOOLONG;
				closedir(d);
				return NULL;
			}
			
			strcpy(sub, cwd);
			strcat(sub, "/");
			strcat(sub, de->d_name);
			
			if (stat(sub, &st))
			{
				closedir(d);
				return NULL;
			}
		} while (st.st_ino != inum[i] ||
			 st.st_dev != dnum[i] ||
			 !strcmp(de->d_name, ".") ||
			 !strcmp(de->d_name, ".."));
		
		if (strlen(cwd) + strlen(de->d_name) + 1 >= PATH_MAX)
		{
			errno = ERANGE;
			return NULL;
		}
		
		sprintf(strchr(cwd, 0), "/%s", de->d_name);
		closedir(d);
	}
	
	if (!*cwd)
		strcpy(cwd, "/");
	
	if (strlen(cwd) >= len)
	{
		errno = EINVAL;
		return NULL;
	}
	
	strcpy(buf, cwd);
	return buf;
}
