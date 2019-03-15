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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

static int cmemb(struct group *gr, char *name)
{
	int i;
	
	if (gr->gr_gid == GOTHER)
		return 1;
	
	for (i = 0; gr->gr_mem[i]; i++)
		if (!strcmp(gr->gr_mem[i], name))
			return 1;
	return 0;
}

int main(int argc, char **argv)
{
	struct passwd *pw;
	struct group *gr;
	char *sh;
	
	sh = getenv("SHELL");
	if (!sh)
	{
		pw = getpwuid(getuid());
		if (pw && *pw->pw_shell)
			sh = pw->pw_shell;
	}
	if (!sh)
		sh = "/bin/sh";
	
	if (argc < 2)
	{
		setgid(pw->pw_gid);
		setuid(getuid());
		goto fini;
	}
	
	if (argc != 2)
	{
		fputs("newgrp [group]\n", stderr);
		goto fail;
	}
	
	gr = getgrnam(argv[1]);
	if (!gr)
	{
		fputs("Group not found\n", stderr);
		goto fail;
	}
	
	if (getuid() && pw->pw_gid != gr->gr_gid && !cmemb(gr, pw->pw_name))
	{
		fputs("Not a member\n", stderr);
		goto fail;
	}
	
	if (setgid(gr->gr_gid) || setuid(getuid()))
	{
		perror(NULL);
		return 1;
	}
fini:
	execl(sh, "-", (void *)NULL);
	perror(sh);
	return 1;
fail:
	setgid(getgid());
	setuid(getuid());
	goto fini;
}
