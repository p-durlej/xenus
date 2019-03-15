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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <pwd.h>

int main(int argc, char **argv)
{
	struct stat st;
	char *user;
	uid_t uid;
	int x = 0;
	int i;
	
	if (argc < 3)
	{
		fputs("chown user file1...\n", stderr);
		fputs("chown uid file1...\n", stderr);
		return 1;
	}
	
	user = argv[1];
	if (isdigit(*user))
		uid = atoi(user);
	else
	{
		struct passwd *pw;
		
		pw = getpwnam(user);
		if (!pw)
		{
			fprintf(stderr, "User %s not found\n", user);
			return 1;
		}
		uid = pw->pw_uid;
	}
	
	for (i = 2; i < argc; i++)
	{
		if (stat(argv[i], &st))
		{
			perror(argv[i]);
			x = 1;
			continue;
		}
		
		if (chown(argv[i], uid, st.st_gid))
		{
			perror(argv[i]);
			x = 1;
		}
	}
	return x;
}
