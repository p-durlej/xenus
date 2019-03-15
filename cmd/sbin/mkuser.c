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

#include <xenus/uids.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>

static char *prefix = "/udd";
static char *name;

int main(int argc, char **argv)
{
	struct passwd *pw;
	uid_t u = UBASE;
	int i, len;
	FILE *f;
	
	if (geteuid())
	{
		fputs("Not permitted\n", stderr);
		return 1;
	}
	
	if (argc != 2)
	{
		fputs("mkuser name\n", stderr);
		return 1;
	}
	name = argv[1];
	len = strlen(name);
	
	if (access(prefix, 0))
		prefix = "/usr";
	
	if (len > LOGNAME_MAX)
	{
		fputs("Name too long\n", stderr);
		return 1;
	}
	for (i = 0; i < len; i++)
		if (!isalnum(name[i]))
		{
			fputs("Bad name\n", stderr);
			return 1;
		}
	
	if (chdir(prefix))
	{
		perror(prefix);
		return 1;
	}
	
	while (pw = getpwent(), pw)
	{
		if (pw->pw_uid > 10000)
		{
			fputs("Too many uids\n", stderr);
			return 1;
		}
		if (u <= pw->pw_uid)
			u = pw->pw_uid + 1;
		
		if (!strcmp(pw->pw_name, name))
		{
			fputs("User exists\n", stderr);
			return 1;
		}
	}
	
	if (!access(name, 0))
	{
		errno = EEXIST;
		goto homef;
	}
	
	printf("uid %i\n", u);
	
	f = fopen("/etc/spwd", "a");
	if (!f)
	{
		perror("/etc/spwd");
		return 1;
	}
	fprintf(f, "%s::%i:1:%s:%s/%s:/bin/sh\n", name, u, name, prefix, name);
	fclose(f);
	
	f = fopen("/etc/passwd", "a");
	if (!f)
	{
		perror("/etc/passwd");
		return 1;
	}
	fprintf(f, "%s::%i:1:%s:%s/%s:/bin/sh\n", name, u, name, prefix, name);
	fclose(f);
	
	if (mkdir(name, 0755))
		goto homef;
	if (chown(name, u, 1))
		goto homef;
	return 0;
homef:
	fputs(prefix, stderr);
	fputc('/', stderr);
	perror(name);
	return 1;
}
