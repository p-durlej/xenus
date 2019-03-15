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
#include <string.h>
#include <stdio.h>

static void uuencode1(unsigned char *p, int len)
{
	unsigned w;
	int i;
	
	putchar(len ? ' ' + len : '`');
	len = (len + 2) / 3;
	
	for (i = 0; i < len; i++)
	{
		w  = *p++; w <<= 8;
		w |= *p++; w <<= 8;
		w |= *p++;
		
		putchar(' ' +  (w >> 18)      );
		putchar(' ' + ((w >> 12) & 63));
		putchar(' ' + ((w >>  6) & 63));
		putchar(' ' + ((w      ) & 63));
	}
	putchar('\n');
}

int main(int argc, char **argv)
{
	char *iname, *name;
	struct stat st;
	char buf[45];
	FILE *inf;
	int c;
	int i;
	
	if (argc < 2 || argc > 3)
	{
		fputs("uuencode [file] name\n", stderr);
		return 1;
	}
	
	if (argc > 2)
	{
		iname = argv[1];
		name  = argv[2];
		inf   = fopen(iname, "r");
	}
	else
	{
		iname = "stdin";
		name  = argv[1];
		inf   = stdin;
	}
	
	if (!inf)
		goto ifail;
	
	fstat(fileno(inf), &st);
	printf("begin %3o %s\n", st.st_mode & 0777, name);
	do
	{
		memset(buf, 0, sizeof buf);
		for (i = 0; i < sizeof buf; i++)
		{
			c = fgetc(inf);
			if (c == EOF)
				break;
			buf[i] = c;
		}
		uuencode1((unsigned char *)buf, i);
		if (ferror(stdout))
		{
			perror("stdout");
			return 1;
		}
	} while (i);
	puts("end");
	
	return 0;
ifail:
	perror(iname);
	return 1;
}
