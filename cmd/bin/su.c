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

#include <sys/termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <time.h>
#include <pwd.h>
#include <md5.h>

struct termios stio;

char passwd[MAX_CANON + 1];
char *user = "root";

void intr(int nr)
{
	tcsetattr(0, TCSANOW, &stio);
	raise(nr);
}

void incorrect()
{
	int i;
	
	for (i = 1; i < NSIG; i++)
		signal(i, SIG_IGN);
	
	sleep(3);
	printf("Sorry\n");
	sleep(2);
	exit(255);
}

int main(int argc, char **argv)
{
	struct passwd *pw;
	char *sh;
	int i;
	
	if (argc > 1)
		user = argv[1];
	
	tcgetattr(0, &stio);
	signal(SIGTERM, intr);
	signal(SIGINT, intr);
	
	for (i = 0; i < 3; i++)
		close(i);
	
	if (open("/dev/tty", O_RDONLY) != STDIN_FILENO)
		return 1;
	if (open("/dev/tty", O_WRONLY) != STDOUT_FILENO)
		return 1;
	if (open("/dev/tty", O_WRONLY) != STDERR_FILENO)
		return 1;
	
	pw = getpwnam(user);
	if (!pw)
		incorrect();
	
	if ((!pw || *pw->pw_passwd) && getuid())
	{
		write(STDOUT_FILENO, "Password: ", 10);
		getpassn(passwd, sizeof(passwd));
		write(STDOUT_FILENO, "\n", 1);
		
		if (!pw || strcmp(pw->pw_passwd, md5a(passwd)))
			incorrect();
	}
	
	sh = *pw->pw_shell ? pw->pw_shell : "/bin/sh";
	
	if (setgid(pw->pw_gid) || setuid(pw->pw_uid))
	{
		perror(NULL);
		incorrect();
	}
	endpwent();
	
	execl(sh, "-", NULL);
	perror(sh);
	incorrect();
}
