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

static int limit = 10;
static int pnam;
static int xit;

static void head(FILE *ff, char *path)
{
	FILE *f = ff;
	int cnt = 0;
	int c;
	
	if (!f)
		f = fopen(path, "r");
	if (!f)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	if (pnam)
		printf("==> %s <==\n", path);
	
	while (c = fgetc(f), c != EOF)
	{
		putchar(c);
		
		if (c == '\n' && ++cnt >= limit)
			break;
	}
	
	if (!ff)
		fclose(f);
}

int main(int argc, char **argv)
{
	int i = 1;
	
	if (argc > 1 && *argv[1] == '-')
	{
		limit = atoi(argv[1] + 1);
		i++;
	}
	
	if (i >= argc)
		head(stdin, "stdin");
	if (argc - i > 1)
		pnam = 1;
	for (; i < argc; i++)
		head(NULL, argv[i]);
	return xit;
}
