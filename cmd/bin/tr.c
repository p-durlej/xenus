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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static char *str1 = "", *str2 = "";
static char smap[256];
static char map[256];

static int cflag, sflag, dflag;
static char buf[4096];

static void mkmap(void)
{
	char *p1, *p2 = str2;
	int i;
	
	for (i = 0; i < 256; i++)
		map[i] = i;
	
	if (sflag)
		for (p1 = dflag ? str2 : str1; *p1; p1++)
			smap[*p1 & 255] = 1;
	
	if (dflag)
	{
		for (p1 = str1; *p1; p1++)
			map[*p1 & 255] = 0;
		return;
	}
	
	if (*str1 && *str2)
		for (p1 = str1; *p1; p1++)
		{
			map[*p1 & 255] = *p2++;
			
			if (!*p2)
				p2--;
		}
}

static char *pstr(char *str)
{
	return str;
}

static void usage(void)
{
	fputs("tr -ds delete squeeze\n", stderr);
	fputs("tr -d delete\n", stderr);
	fputs("tr -s squeeze\n", stderr);
	fputs("tr from to\n", stderr);
	exit(1);
}

int main(int argc, char **argv)
{
	ssize_t rcnt, wcnt;
	char *sp;
	char *p;
	int i;
	
	if (argc > 1 && *argv[1] == '-')
	{
		for (p = argv[1] + 1; *p; p++)
			switch (*p)
			{
			case 'c':
				cflag = 1;
				break;
			case 'd':
				dflag = 1;
				break;
			case 's':
				sflag = 1;
				break;
			default:
				;
			}
		argv++;
		argc--;
	}
	
	if (argc > 1)
		str1 = pstr(argv[1]);
	if (argc > 2)
		str2 = pstr(argv[2]);
	if (argc > 3)
		usage();
	
	if (!sflag && !dflag && !*str2)
		usage();
	
	if (dflag && *str2)
		usage();
	
	mkmap();
	
	while (rcnt = read(0, buf, sizeof buf), rcnt > 0)
	{
		if (dflag)
		{
			for (p = buf, sp = buf, i = 0; i < rcnt; i++, sp++)
				if (map[*sp & 255])
					*p++ = *sp;
			rcnt = p - buf;
		}
		
		if (sflag)
		{
			for (p = buf, sp = buf, i = 0; i < rcnt; i++, p++, sp++)
			{
				*p = *sp;
				
				if (smap[*sp & 255])
					while (sp[1] == *sp)
					{
						rcnt--;
						sp++;
					}
			}
			rcnt = p - buf;
		}
		
		if (!dflag)
			for (i = 0; i < rcnt; i++)
				buf[i] = map[buf[i]];
		
		wcnt = write(1, buf, rcnt);
		if (wcnt < 0)
		{
			perror("stdout");
			return 1;
		}
		if (wcnt != rcnt)
		{
			fputs("stdout: Short write\n", stderr);
			return 1;
		}
	}
	if (rcnt < 0)
		perror("stdin");
	
	return 0;
}
