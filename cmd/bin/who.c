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

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <utmp.h>
#include <pwd.h>

int main(int argc, char **argv)
{
	char user[LOGNAME_MAX + 1];
	char ttyn[NAME_MAX + 1];
	char buf[32];
	struct utmp ut;
	
	char *fuser = NULL;
	char *ftty = NULL;
	struct passwd *pw;
	uid_t uid;
	int cnt;
	int fd;
	int i;
	
	if (argc >= 3)
	{
		ftty = ttyname(0);
		uid  = getuid();
		pw   = getpwuid(uid);
		
		if (ftty)
			ftty = strrchr(ftty, '/') + 1;
		if (pw)
			fuser = pw->pw_name;
	}
	
	fd = open("/etc/utmp", O_RDONLY);
	if (fd < 0)
	{
		perror("/etc/utmp");
		return 1;
	}
	
	while (cnt = read(fd, &ut, sizeof ut), cnt)
	{
		if (cnt < 0)
		{
			perror("/etc/utmp");
			return 1;
		}
		if (cnt != sizeof ut)
		{
			fputs("/etc/utmp: partial read\n", stderr);
			return 1;
		}
		if (!*ut.ut_name)
			continue;
		
		strncpy(user, ut.ut_name, sizeof user);
		strncpy(ttyn, ut.ut_line, sizeof ttyn);
		user[sizeof user - 1] = 0;
		ttyn[sizeof ttyn - 1] = 0;
		
		if (fuser && strcmp(user, user))
			continue;
		if (ftty && strcmp(ftty, ttyn))
			continue;
		
		strcpy(buf, ctime(&ut.ut_time) + 4);
		buf[12] = 0;
		
		printf("%-8s %-8s %s\n", user, ttyn, buf);
	}
	
	return 0;
}
