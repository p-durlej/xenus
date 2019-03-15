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

#include <sys/utsname.h>
#include <stdlib.h>
#include <stdio.h>

int sysname;
int nodename;
int release;
int version;
int machine;

void procopt(char *s)
{
	while (*s)
		switch (*(s++))
		{
		case '-':
			break;
		case 'a':
			sysname	 = 1;
			nodename = 1;
			release	 = 1;
			version	 = 1;
			machine	 = 1;
			break;
		case 's':
			sysname = 1;
			break;
		case 'n':
			nodename = 1;
			break;
		case 'r':
			release = 1;
			break;
		case 'v':
			version = 1;
			break;
		case 'm':
			machine = 1;
			break;
		default:
			fprintf(stderr, "uname: bad option '%c'\n", s[-1]);
			exit(1);
		}
}

int main(int argc, char **argv)
{
	struct utsname u;
	char *s = "";
	
	if (uname(&u))
	{
		perror(NULL);
		return 1;
	}
	if (argc < 2)
		sysname = 1;
	else
		while (*(++argv))
			procopt(*argv);
	if (sysname)
	{
		printf("%s", u.sysname);
		s = " ";
	}
	if (nodename)
	{
		printf("%s%s", s, u.nodename);
		s = " ";
	}
	if (release)
	{
		printf("%s%s", s, u.release);
		s = " ";
	}
	if (version)
	{
		printf("%s%s", s, u.version);
		s = " ";
	}
	if (machine)
	{
		printf("%s%s", s, u.machine);
		s = " ";
	}
	putc('\n', stdout);
	return 0;
}
