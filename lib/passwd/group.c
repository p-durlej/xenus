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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <grp.h>

#define MAXMEM 16

static struct group grent;
static char grbuf[256];
static FILE *grfile;
static char *grmem[MAXMEM + 1];

struct group *getgrent()
{
	if (!grfile)
	{
#if GSHADOW
		if (geteuid())
			grfile = fopen("/etc/group", "r");
		else
			grfile = fopen("/etc/sgrp", "r");
#else
		grfile = fopen("/etc/group", "r");
#endif
		if (!grfile)
			return NULL;
	}
	
	fcntl(fileno(grfile), F_SETFD, 1);
	return fgetgrent(grfile);
}

void setgrent()
{
	if (grfile)
		rewind(grfile);
}

void endgrent()
{
	if (grfile)
	{
		fclose(grfile);
		grfile = NULL;
	}
}

struct group *getgrnam(char *name)
{
	setgrent();
	
	while (getgrent())
		if (!strcmp(name, grent.gr_name))
			return &grent;
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	setgrent();
	
	while (getgrent())
		if (gid == grent.gr_gid)
			return &grent;
	return NULL;
}

static void badgroup(void)
{
	fputs("fgetpwent: inconsistent group database file\n", stderr);
	errno = EINVAL;
}

struct group *fgetgrent(FILE *f)
{
	char *p;
	int i;
	
	if (!fgets(grbuf, sizeof(grbuf), f))
		return NULL;
	
	p = strchr(grbuf, '\n');
	if (p)
		*p = 0;
	
	grent.gr_name = grbuf;
	p = strchr(grbuf, ':');
	if (!p)
	{
		badgroup();
		return NULL;
	}
	*p = 0;
	
	grent.gr_passwd = p + 1;
	p = strchr(p + 1, ':');
	if (!p)
	{
		badgroup();
		return NULL;
	}
	*p = 0;
	
	grent.gr_gid = atoi(p + 1);
	p = strchr(p + 1, ':');
	if (!p)
	{
		badgroup();
		return NULL;
	}
	*p++ = 0;
	
	for (i = 0; p && *p && i < MAXMEM; i++)
	{
		grmem[i] = p;
		
		p = strchr(p, ',');
		if (p)
			*p++ = 0;
	}
	grmem[i] = NULL;
	grent.gr_mem = grmem;
	
	return &grent;
}
