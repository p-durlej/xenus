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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

struct termios tp, otp;
int nlin, ncol;
int lin, col;
int tty_fd;
int bflag;
int cflag;
int bmode;
int umode;
int qmode;
volatile int quit;

char *inv = "", *rst = "";
char *bold = "";
char *und = "";

void setmode(void)
{
	fputs(rst, stdout);
	if (bmode)
		fputs(bold, stdout);
	if (umode)
		fputs(und, stdout);
}

void pputc(unsigned char ch)
{
	if (ch == '\t')
	{
		int i;
		int t;
		
		t  = col & ~0x07;
		t += 8;
		
		for (i = col; i < t; i++)
			pputc(' ');
		return;
	}
	
	if ((ch < 0x20 || ch > 0x7f) && ch != '\n')
	{
		char hex[3];
		
		pputc('\\');
		pputc('x');
		sprintf(hex, "%02x", (unsigned int)ch);
		pputc(hex[0]);
		pputc(hex[1]);
		return;
	}
	
	fputc(ch, stdout);
	
	if (ch == '\n')
	{
		lin++;
		col = 0;
	}
	else
		col++;
	
	if (col == ncol)
	{
		col = 0;
		lin++;
	}
	
	if (lin == nlin - 1)
	{
		int cnt;
		char c;
		
		printf("%s(more)%s", inv, bmode ? bold : rst);
		fflush(stdout);
		
		do
		{
			cnt = read(tty_fd, &c, 1);
			if (cnt < 0)
			{
				if (errno == EINTR)
					continue;
				tcsetattr(tty_fd, TCSANOW, &otp);
				perror("stdin");
				exit(1);
			}
			if (c == 'q')
				quit = 1;
		} while (cnt && c != '\n' && c != '\r' && c != ' ' && !quit);
		
		printf("\r      \r");
		fflush(stdout);
		setmode();
		
		if (c == ' ')
			lin = 0;
		else
			lin--;
	}
}

void cputc(char ch)
{
	if (qmode)
	{
		qmode = 0;
		pputc(ch);
		return;
	}
	
	if (bflag)
		switch (ch)
		{
		case '\\':
			qmode = 1;
			return;
		case '_':
			umode = !umode;
			setmode();
			return;
		case '*':
			bmode = !bmode;
			setmode();
			return;
		default:
			;
		}
	
	pputc(ch);
}

void do_more(char *name)
{
	char buf[512];
	int ln = 0;
	int cnt;
	int fd;
	int i;
	
	if (name)
	{
		fd = open(name, O_RDONLY);
		if (fd < 0)
		{
			perror(name);
			return;
		}
	}
	else
		fd = STDIN_FILENO;
	
	while (cnt = read(fd, buf, sizeof buf), cnt && !quit)
	{
		if (cnt < 0)
		{
			perror(name);
			return;
		}
		
		for (i = 0;i < cnt && !quit; i++)
			cputc(buf[i]);
	}
	
	if (fd != STDIN_FILENO)
		close(fd);
}

void intr(int nr)
{
	tcsetattr(tty_fd, TCSANOW, &otp);
	if (nr != SIGINT)
		_exit(0);
	quit = 1;
}

int main(int argc, char **argv)
{
	char *tt = getenv("TERM");
	char *p;
	int dlin = 25;
	int vt = 0;
	int i;
	
	if (getenv("COLUMNS"))	ncol = atoi(getenv("COLUMNS"));
	if (getenv("LINES"))	nlin = atoi(getenv("LINES"));
	
	if (!strncmp(tt, "vt", 2))
	{
		bold = "\033[1m";
		inv = "\033[7m";
		und = "\033[4m";
		rst = "\033[0m";
		dlin = 24;
		vt = 1;
	}
	
	if (!strcmp(tt, "xenus"))
	{
		bold = "\033b";
		inv = "\033i";
		und = "\033n";
		rst = "\033I\033B\033N";
	}
	
	if (!strcmp(tt, "test"))
	{
		bold = "<B>";
		inv = "<I>";
		und = "<U>";
		rst = "<R>";
	}
	
	nlin = nlin ? nlin : dlin;
	ncol = ncol ? ncol : 80;
	
	tty_fd = open("/dev/tty", O_RDONLY);
	if (tty_fd < 0)
	{
		perror("/dev/tty");
		return 1;
	}
	tcgetattr(tty_fd, &otp);
	signal(SIGQUIT, intr);
	signal(SIGTERM, intr);
	signal(SIGINT, intr);
	tp = otp;
	tp.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(tty_fd, TCSANOW, &tp);
	
	if (*argv[1] == '-')
	{
		for (p = argv[1] + 1; *p; p++)
			switch (*p)
			{
			case 'b':
				bflag = 1;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'x':
				bold = "";
				inv = "";
				und = "";
				break;
			default:
				fprintf(stderr, "Bad option '%c'\n", *p);
				return 1;
			}
		argv++;
		argc--;
	}
	
	if (vt && cflag)
	{
		bold = "\033[1;33m";
		und = "\033[1;36m";
	}
	
	for (i = 1; i < argc; i++)
		do_more(argv[i]);
	if (argc == 1)
		do_more(NULL);
	
	tcsetattr(tty_fd, TCSANOW, &otp);
	return 0;
}
