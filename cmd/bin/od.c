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
#include <stdio.h>
#include <ctype.h>

static char *fmt = " %06o";
static int cflag;
static int bytes;
static int x;

static void printc(unsigned c)
{
	if (isprint(c))
	{
		printf("   %c", c);
		return;
	}
	switch (c)
	{
	case '\n':
		printf("  \\n");
		break;
	case '\r':
		printf("  \\r");
		break;
	case '\b':
		printf("  \\b");
		break;
	case '\f':
		printf("  \\f");
		break;
	case '\t':
		printf("  \\t");
		break;
	case 0:
		printf("  \\0");
		break;
	default:
		printf(" %03o", c & 255);
	}
}

static void dump(FILE *f, char *path)
{
	off_t off = 0;
	int cnt = bytes ? 16 : 8;
	int c1, c2;
	int i;
	
	for (;;)
	{
		printf("%07lo", (long)off);
		for (i = 0; i < cnt; i++)
		{
			c1 = fgetc(f);
			if (c1 == EOF)
			{
				putchar('\n');
				off += i;
				goto fini;
			}
			if (bytes)
				c2 = 0;
			else
			{
				c2 = fgetc(f);
				if (c2 == EOF)
					c2 = 0;
			}
			
			if (cflag)
				printc(c1);
			else
				printf(fmt, (c1 & 255) | ((c2 & 255) << 8));
		}
		putchar('\n');
		off += i;
	}
fini:
	if (i)
		printf("%07lo\n", (long)off);
}

int main(int argc, char **argv)
{
	int i = 1;
	char *p;
	FILE *f;
	
	if (*argv[i] == '-')
	{
		for (p = argv[i] + 1; *p; p++)
			switch (*p)
			{
			case 'b':
				fmt = " %03o";
				bytes = 1;
				cflag = 0;
				break;
			case 'c':
				fmt = " %c";
				bytes = 1;
				cflag = 1;
				break;
			case 'd':
				fmt = " %05u";
				bytes = 0;
				cflag = 0;
				break;
			case 'o':
				fmt = " %07o";
				bytes = 0;
				cflag = 0;
				break;
			case 'x':
				fmt = " %04x";
				bytes = 0;
				cflag = 0;
				break;
			}
		i++;
	}
	
	if (i >= argc)
		dump(stdin, "stdin");
	
	for (; i < argc; i++)
	{
		f = fopen(argv[i], "r");
		if (f == NULL)
		{
			perror(argv[i]);
			x = 1;
			continue;
		}
		dump(f, argv[i]);
		fclose(f);
	}
	
	return x;
}
