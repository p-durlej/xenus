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

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

int _killu(uid_t ruid, int sig);

int main(int argc, char **argv)
{
	struct passwd *pw;
	int err = 0;
	int sig = SIGTERM;
	int i = 1;
	
	if (argc < 2)
	{
		fputs("killu [-sig] user...\n", stderr);
		return 1;
	}
	
	if (argc > 2 && *argv[i] == '-')
	{
		sig = atoi(argv[i] + 1);
		i++;
	}
	
	
	for (; i < argc; i++)
	{
		pw = getpwnam(argv[i]);
		if (pw == NULL)
		{
			fprintf(stderr, "%s: No such user\n", argv[i]);
			err = 1;
			continue;
		}
		
		if (_killu(pw->pw_uid, sig))
		{
			perror(argv[i]);
			err = 1;
		}
	}
	return err;
}
