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
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>

int main(int argc, char **argv)
{
	struct stat st;
	int flag = 0;
	char *n;
	
	if (argc > 2)
	{
		fputs("mesg [yn]\n", stderr);
		return 1;
	}
	
	if (argc > 1)
		flag = *argv[1];
	
	n = ttyname(0);
	if (!n)
	{
		perror(NULL);
		return 1;
	}
	
	if (stat(n, &st))
	{
		perror(NULL);
		return 1;
	}
	
	if (!S_ISCHR(st.st_mode))
	{
		fputs("Not a tty\n", stderr);
		return 1;
	}
	
	if (!flag)
	{
		if (st.st_mode & 020)
			puts("is y");
		else
			puts("is n");
		return 0;
	}
	
	switch (flag)
	{
	case 'y':
		st.st_mode |=  020;
		break;
	case 'n':
		st.st_mode &= ~020;
		break;
	default:
		fprintf(stderr, "Bad flag '%c'\n", flag);
		return 1;
	}
	
	if (chmod(n, st.st_mode & 0777))
	{
		perror(NULL);
		return 1;
	}
	
	return 0;
}
