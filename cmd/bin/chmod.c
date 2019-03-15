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
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

static int otoi(char *nptr)
{
	int n = 0;
	
	while (*nptr >= '0' && *nptr <= '7')
	{
		n <<= 3;
		n  += *nptr++ - '0';
	}
	return n;
}

static int uspec(int c)
{
	return c == 'a' || c == 'u' || c == 'g' || c == 'o';
}

int main(int argc, char **argv)
{
	struct stat st;
	mode_t mode = 0;
	mode_t r = 0;
	mode_t w = 0;
	mode_t x = 0;
	mode_t s = 0;
	int exact = 0;
	int xit = 0;
	int clr;
	int i;
	
	if (argc < 3)
	{
		fputs("chmod augo+-rwxst file1...\n", stderr);
		fputs("chmod mode file1...\n", stderr);
		return 1;
	}
	
	if (isdigit(*argv[1]))
	{
		mode  = otoi(argv[1]);
		exact = 1;
	}
	else
	{
		char *mstr = argv[1];
		
		if (!uspec(*mstr))
		{
			mode_t um;
			
			um = umask(0);
			umask(um);
			
			r = 0444 & ~um;
			w = 0222 & ~um;
			x = 0111 & ~um;
			s = 0;
		}
		
		while (uspec(*mstr))
			switch (*mstr++)
			{
			case 'a':
				r = S_IRUSR | S_IRGRP | S_IROTH;
				w = S_IWUSR | S_IWGRP | S_IWOTH;
				x = S_IXUSR | S_IXGRP | S_IXOTH;
				s = S_ISUID | S_ISGID;
				break;
			case 'u':
				r |= S_IRUSR;
				w |= S_IWUSR;
				x |= S_IXUSR;
				s |= S_ISUID;
				break;
			case 'g':
				r |= S_IRGRP;
				w |= S_IWGRP;
				x |= S_IXGRP;
				s |= S_ISGID;
				break;
			case 'o':
				r |= S_IROTH;
				w |= S_IWOTH;
				x |= S_IXOTH;
				break;
			}
		
		switch (*mstr)
		{
		case '+':
			clr = 0;
			break;
		case '-':
			clr = 1;
			break;
		default:
			fprintf(stderr, "chmod: %s: bad mode string\n", argv[1]);
			return 1;
		}
		
		mstr++;
		
		while (*mstr)
			switch(*mstr++)
			{
			case 'r':
				mode |= r;
				break;
			case 'w':
				mode |= w;
				break;
			case 'x':
				mode |= x;
				break;
			case 's':
				mode |= s;
				break;
			case 't':
				mode |= S_ISVTX;
				break;
			default:
				fprintf(stderr, "chmod: %s: bad mode string\n", argv[1]);
				return 1;
			}
	}
	
	for (i = 2; i < argc; i++)
	{
		if (stat(argv[i], &st))
		{
			perror(argv[i]);
			xit = 1;
			continue;
		}
		
		if (exact)
			st.st_mode = mode;
		else
		{
			if (clr)
				st.st_mode &= ~mode;
			else
				st.st_mode |= mode;
		}
		
		if (chmod(argv[i], st.st_mode))
		{
			perror(argv[i]);
			xit = 1;
		}
	}
	return xit;
}
