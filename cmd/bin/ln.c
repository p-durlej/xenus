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
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int xit;

char *basename(char *name)
{
	char *s;
	
	s = strrchr(name, '/');
	if (s)
		return s + 1;
	return name;
}

void do_ln(char *src, char *dst)
{
	if (link(src, dst))
	{
		xit = 1;
		perror(src);
	}
}

int main(int argc, char **argv)
{
	int target_exists = 1;
	struct stat st;
	char *target;
	int i;
	
	if (argc < 2)
	{
		fputs("ln name1... dir\n", stderr);
		fputs("ln name1 name2\n", stderr);
		fputs("ln name\n", stderr);
		return 255;
	}
	
	target = argv[argc - 1];
	if (stat(target, &st))
	{
		if (errno != ENOENT)
		{
			perror(target);
			return 1;
		}
		target_exists = 0;
	}
	
	if (argc < 3)
	{
		do_ln(argv[1], basename(argv[1]));
		return xit;
	}
	
	for (i = 1; i < argc - 1; i++)
	{
		char buf[PATH_MAX];
		
		if (target_exists && S_ISDIR(st.st_mode))
		{
			strcpy(buf, target);
			strcat(buf, "/");
			strcat(buf, basename(argv[i]));
		}
		else
			strcpy(buf, target);
		
		do_ln(argv[i], buf);
	}
	return xit;
}
