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
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>

static struct passwd pwent;
static char pwbuf[256];
static FILE *pwfile;

struct passwd *getpwent()
{
	if (!pwfile)
	{
		if (geteuid())
			pwfile = fopen("/etc/passwd", "r");
		else
			pwfile = fopen("/etc/spwd", "r");
		if (!pwfile)
			return NULL;
	}
	
	fcntl(pwfile->fd, F_SETFD, 1);
	return fgetpwent(pwfile);
}

void setpwent()
{
	if (pwfile)
		rewind(pwfile);
}

void endpwent()
{
	if (pwfile)
	{
		fclose(pwfile);
		pwfile = NULL;
	}
}

struct passwd *getpwnam(char *name)
{
	setpwent();
	
	while (getpwent())
		if (!strcmp(pwent.pw_name, name))
			return &pwent;
	return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
	setpwent();
	
	while (getpwent())
		if (pwent.pw_uid == uid)
			return &pwent;
	return NULL;
}

static void badpasswd(void)
{
	fputs("fgetpwent: inconsistent passwd database file\n", stderr);
	errno = EINVAL;
}

struct passwd *fgetpwent(FILE *f)
{
	char *p = pwbuf;
	
	if (!fgets(pwbuf, sizeof(pwbuf), f))
		return NULL;
	
	pwent.pw_name = p;
	p = strchr(p, ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	*p = 0;
	
	pwent.pw_passwd = p + 1;
	p = strchr(p + 1, ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	*p = 0;
	
	pwent.pw_uid = atoi(p + 1);
	p = strchr(p + 1, ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	
	pwent.pw_gid = atoi(p + 1);
	p = strchr(p + 1, ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	
	pwent.pw_gecos = p + 1;
	p = strchr(p + 1 , ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	*p = 0;
	
	pwent.pw_dir = p + 1;
	p = strchr(p + 1, ':');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	*p = 0;
	
	pwent.pw_shell = p + 1;
	p = strchr(p + 1, '\n');
	if (!p)
	{
		badpasswd();
		return NULL;
	}
	*p = 0;
	
	return &pwent;
}

int putpwent(struct passwd *pw, FILE *f)
{
	clearerr(f);
	fprintf(f, "%s:%s:%i:%i:%s:%s:%s\n", pw->pw_name, pw->pw_passwd,
	        (int)pw->pw_uid, (int)pw->pw_gid, pw->pw_gecos, pw->pw_dir,
		pw->pw_shell);
	if (ferror(f))
		return -1;
	return 0;
}
