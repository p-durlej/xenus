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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

static int rflag;
static int x;

static void do_rm(char *path);

static void recurse(char *path)
{
	struct dirent *de;
	DIR *d;
	
	if (chdir(path))
	{
		x = 1;
		return;
	}
	d = opendir(".");
	if (!d)
	{
		x = 1;
		goto fini;
	}
	while (de = readdir(d), de)
	{
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		do_rm(de->d_name);
	}
	closedir(d);
fini:
	if (chdir(".."))
		exit(1);
	rmdir(path);
}

static void do_rm(char *path)
{
	if (!unlink(path))
		return;
	
	if (errno == EISDIR && rflag)
	{
		recurse(path);
		return;
	}
	
	perror(path);
	x = 1;
}

int main(int argc,char **argv)
{
	int i;
	
	if (argc > 1 && !strcmp(argv[1], "-r"))
	{
		rflag = 1;
		argc--;
		argv++;
	}
	
	if (argc < 2)
	{
		fputs("rm [-r] name1...\n", stderr);
		return 1;
	}
	for (i = 1; i < argc; i++)
		do_rm(argv[i]);
	return x;
}
