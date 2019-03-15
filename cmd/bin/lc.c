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

#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAXNAM	1024

static char *names[MAXNAM];
static int namecnt;
static int cols;
static int xit;

static int ncmp(void *p1, void *p2)
{
	char *n1 = *(char **)p1;
	char *n2 = *(char **)p2;
	
	return strcmp(n1, n2);
}

static void list(char *path)
{
	struct dirent *de;
	int l, w = 0;
	int i, c;
	DIR *d;
	
	d = opendir(path);
	if (!d)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	while (de = readdir(d), de != NULL)
	{
		if (*de->d_name == '.')
			continue;
		
		if (namecnt >= MAXNAM)
		{
			fprintf(stderr, "%s: Too many names\n", path);
			xit = 1;
			break;
		}
		names[namecnt++] = strdup(de->d_name);
		
		l = strlen(de->d_name);
		if (w < l)
			w = l;
	}
	w++;
	
	closedir(d);
	
	qsort(names, namecnt, sizeof *names, ncmp);
	
	for (i = c = 0; i < namecnt; i++)
	{
		fputs(names[i], stdout);
		l = w - strlen(names[i]);
		while (l--)
			putchar(' ');
		
		if (++c >= cols / w)
		{
			putchar('\n');
			c = 0;
		}
		free(names[i]);
	}
	if (c)
		putchar('\n');
	namecnt = 0;
}

int main(int argc, char **argv)
{
	char *p;
	int i;
	
	p = getenv("COLUMNS");
	if (p)
		cols = atoi(p);
	if (!cols)
		cols = 80;
	
	if (argc < 2)
	{
		list(".");
		return 0;
	}
	
	for (i = 1; i < argc; i++)
	{
		if (argc > 2)
			printf("%s:\n", argv[i]);
		list(argv[i]);
	}
	return 0;
}
