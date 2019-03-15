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
#include <stdio.h>
#include <ctype.h>

static int lflag, wflag, cflag;
static int ltot, wtot, ctot;
static int xit;

static void wc(FILE *ff, char *path)
{
	FILE *f = ff;
	int lc = 0;
	int wc = 0;
	int cc = 0;
	int wl = 0;
	int c;
	
	if (!f)
		f = fopen(path, "r");
	if (!f)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	while (c = fgetc(f), c != EOF)
	{
		if (isspace(c) && wl)
		{
			wc++;
			wl = 0;
		}
		if (!isspace(c))
			wl++;
		if (c == '\n')
			lc++;
		cc++;
	}
	
	if (!ff)
		fclose(f);
	
	if (lflag)
		printf("%7i ", lc);
	if (wflag)
		printf("%7i ", wc);
	if (cflag)
		printf("%7i ", cc);
	puts(path);
	
	ltot += lc;
	wtot += wc;
	ctot += cc;
}

int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'l':
			lflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		default:
			fprintf(stderr, "wc: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

int main(int argc, char **argv)
{
	FILE *f;
	int m = 0;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	
	if (!lflag && !wflag && !cflag)
		lflag = wflag = cflag = 1;
	
	if (argc - i > 1)
		m = 1;
	if (i >= argc)
		wc(stdin, "");
	for (; i < argc; i++)
		wc(NULL, argv[i]);
	
	if (m)
	{
		if (lflag)
			printf("%7i ", ltot);
		if (wflag)
			printf("%7i ", wtot);
		if (cflag)
			printf("%7i ", ctot);
		puts("total");
	}
	return xit;
}
