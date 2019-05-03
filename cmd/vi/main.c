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
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "term.h"

static char	path[PATH_MAX];
static char	filebuf[65536];
static char *	lineptr = filebuf;
static char *	linebuf;
static char *	topptr = filebuf;
static int	dirty;

static int x, y, t;

static struct termios tio, otio;

static char *rline(char *p);
static void redraw(void);
static int insert(void);
static void ucurs(void);
static void utop(int r);
static void uline(void);
static void error(char *s);
static void cmd_read(void);
static void cmd_write(char *s);
static void cmd_quit(char *s);
static void cmd_name(char *s, int req);
static void command(void);
static void loop(void);

int fast;
int cols;
int rows;

static char *rline(char *p)
{
	char *e;
	
	for (e = p; *e && *e != '\n'; e++)
		;
	output(p, e - p);
	cleol();
	
	if (*e)
		e++;
	return e;
}

static void redraw(void)
{
	char *p = topptr;
	int i;
	
	home();
	for (i = 1; i < rows; i++)
		p = rline(p);
	cleol();
	gotoxy(x, y - t);
}

static void rlinebuf(void)
{
	int len = cols - 1;
	
	while (len > 0 && linebuf[len - 1] == ' ')
		len--;
	
	gotoxy(0, y - t);
	output(linebuf, len);
	cleol();
}

static int insert(void)
{
	int len, len2;
	char *p, *p2;
	int xlen;
	int c;
	
again:
	p = strchr(lineptr, '\n');
	if (!p)
		p = strchr(lineptr, 0);
	len = p - lineptr;
	
	memset(linebuf, ' ', cols);
	memcpy(linebuf, lineptr, len);
	
	for (;;)
		switch (c = getch(), c)
		{
		case '\n':
		case 27:
			goto store;
		case '\b':
		case 127:
			if (!x)
				break;
			
			memmove(linebuf + x - 1, linebuf + x, cols - x);
			lineptr[cols - 1] = ' ';
			x--;
			
			if (fast)
				rlinebuf();
			else
			{
				gotoxy(x, y);
				output(" ", 1);
			}
			
			ucurs();
			dirty = 1;
			break;
		case 'L' & 31:
			rlinebuf();
			ucurs();
			break;
		default:
			if (!isprint(c))
				break;
			if (x > cols)
				break;
			
			memmove(linebuf + x + 1, linebuf + x, cols - x + 1);
			linebuf[x] = c;
			
			if (fast)
				rlinebuf();
			else
			{
				gotoxy(x, y - t);
				output(&linebuf[x], 1);
			}
			dirty = 1;
			x++;
			ucurs();
		}
	
store:
	rlinebuf();
	ucurs();
	
	for (p2 = linebuf + cols - 1; p2 > linebuf; p2--)
		if (*p2 != ' ')
			break;
	len2 = p2 - linebuf + 1;
	
	xlen = strlen(p) + 1;
	memmove(lineptr + len2, p, xlen); // XXX
	memcpy(lineptr, linebuf, len2);
	lineptr[len2] = '\n';
	
	if (c == '\n')
	{
		memmove(lineptr + x + 1, lineptr + x, strlen(lineptr + x) + 1);
		lineptr[x] = '\n';
		if (++y >= t + rows)
		{
			t++;
			utop(0);
		}
		x = 0;
		uline();
		redraw();
		goto again;
	}
	
	return c;
}

static void ucurs(void)
{
	gotoxy(x, y - t);
}

static void utop(int r)
{
	int cnt = t;
	char *p;
	
	for (p = filebuf; *p && cnt; p++)
		if (*p == '\n' && !--cnt)
		{
			p++;
			break;
		}
	topptr = p;
	if (r)
		redraw();
}

static void uline(void)
{
	int cnt = y - t;
	char *p;
	
	for (p = topptr; *p && cnt; p++)
		if (*p == '\n' && !--cnt)
		{
			p++;
			break;
		}
	lineptr = p;
	ucurs();
}

static void error(char *s)
{
	gotoxy(0, rows - 1);
	invert();
	output(s, strlen(s));
	reset();
	getch();
	redraw();
}

static void cmd_read(void)
{
	char *p;
	int fd;
	
	if (dirty)
	{
		error("Unsaved changes");
		return;
	}
	
	if (!*path)
	{
		error("No filename");
		return;
	}
	
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		if (errno != ENOENT)
			error("Cannot open");
		return;
	}
	if (read(fd, filebuf, sizeof filebuf - 1) < 0)
		error("Cannot read");
	close(fd);
	
	for (p = filebuf; *p; p++)
		if (*p == '\t')
			*p = ' ';
	
	topptr = lineptr = filebuf;
	x = y = t = 0;
	redraw();
}

static void cmd_write(char *p)
{
	ssize_t len = strlen(filebuf);
	int fd;
	
	if (!*path)
	{
		error("No filename");
		return;
	}
	
	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (fd < 0)
	{
		error("Cannot open");
		return;
	}
	if (write(fd, filebuf, len) != len)
	{
		error("Cannot write");
		close(fd);
		return;
	}
	if (close(fd))
	{
		error("Cannot write");
		return;
	}
	
	dirty = 0;
	
	if (*p == 'q')
	{
		clear();
		tcsetattr(0, TCSANOW, &otio);
		exit(0);
	}
}

static void cmd_quit(char *s)
{
	while (isspace(*s))
		s++;
	
	if (dirty && *s != '!')
	{
		error("Unsaved changes");
		return;
	}
	
	clear();
	tcsetattr(0, TCSANOW, &otio);
	exit(0);
}

static void cmd_name(char *s, int req)
{
	if (*s++ != ' ')
		goto bad;
	
	while (isspace(*s))
		s++;
	
	if (!*s)
		goto bad;
	strcpy(path, s);
	return;
bad:
	if (req)
		error("No filename");
}

static void cmd_goto(char *s)
{
	int l = atoi(s) - 1;
	int m = 0;
	char *p;
	
	for (p = filebuf; *p; p++)
		if (*p == '\n')
			m++;
	
	l = l <  m ? l : m;
	l = l >= 0 ? l : 0;
	t = l - rows / 2;
	t = t >= 0 ? t : 0;
	y = l;
	
	uline();
	utop(1);
}

static void command(void)
{
	char buf[128];
	char *p;
	int cnt = 0;
	int c;
	
	gotoxy(0, rows - 1);
	output(":", 1);
	fflush(stdout);
	
	while (c = getch(), c != '\n')
		switch (c)
		{
		case '\b':
		case 127:
			if (!cnt)
				break;
			gotoxy(cnt--, rows - 1);
			break;
		default:
			if (cnt >= sizeof buf - 1)
				break;
			buf[cnt] = c;
			output(buf + cnt++, 1);
		}
	buf[cnt] = 0;
	
	p = buf;
	while (isspace(*p))
		p++;
	
	switch (*p++)
	{
	case 'q':
		cmd_quit(p);
		break;
	case 'n':
		cmd_name(p, 1);
		break;
	case 'r':
		cmd_name(p, 0);
		cmd_read();
		break;
	case 'w':
		cmd_name(p, 0);
		cmd_write(p);
		break;
	case '!':
		clear();
		fflush(stdout);
		tcsetattr(0, TCSANOW, &otio);
		system(p);
		tcsetattr(0, TCSANOW, &tio);
		getch();
		redraw();
		break;
	default:
		if (isdigit(p[-1]))
		{
			cmd_goto(p - 1);
			break;
		}
		error("Invalid command");
	}
	
	gotoxy(0, rows - 1);
	cleol();
	ucurs();
}

static void join(void)
{
	char *p;
	
	p = strchr(lineptr, '\n');
	if (p == NULL)
		return;
	memmove(p, p + 1, strlen(p));
	redraw();
}

static void replace(void)
{
	char *p;
	char c;
	
	p = strchr(lineptr, '\n');
	c = getch();
	if (p - lineptr > x)
	{
		putchar(c);
		putchar('\b');
		lineptr[x] = c;
	}
}

static int linelen(void)
{
	char *p;
	
	p = strchr(lineptr, '\n');
	if (!p)
		p = strchr(lineptr, 0);
	return p - lineptr;
}

static void loop(void)
{
	char *p;
	int len;
	int i;
	
	for (;;)
		switch (getch())
		{
		case '^':
			x = 0;
			ucurs();
			break;
		case '$':
			x = linelen();
			ucurs();
			break;
		case 'd':
			p = strchr(lineptr, '\n');
			if (!p)
				break;
			memmove(lineptr, p + 1, strlen(p));
			redraw();
			dirty = 1;
			break;
		case 'x':
			memmove(lineptr + x, lineptr + x + 1, strlen(lineptr + x));
			ucurs();
			rline(lineptr + x);
			ucurs();
			dirty = 1;
			break;
		case 'h':
			if (!x)
				break;
			x--;
			ucurs();
			break;
		case 'k':
			if (!y)
				break;
			if (--y < t)
			{
				t = y;
				utop(0);
				scdn();
				gotoxy(0, 0);
				rline(topptr);
				gotoxy(0, rows - 1);
				cleol();
			}
			uline();
			break;
		case 'j':
			if (!strchr(lineptr, '\n'))
				break;
			if (++y >= t + rows - 1)
			{
				t++;
				utop(0);
				scup();
				gotoxy(0, rows - 2);
				for (p = topptr, i = 2; i < rows; i++, p++)
					if (p = strchr(p, '\n'), !p)
						break;
				if (p)
					rline(p);
				else
					cleol();
			}
			uline();
			break;
		case 'l':
			if (x >= linelen())
				break;
			gotoxy(++x, y - t);
			break;
		case 'i':
			if (x > linelen())
			{
				x = linelen();
				ucurs();
			}
			insert();
			break;
		case 'J':
			join();
			break;
		case 'a':
			if (++x > linelen())
				x = linelen();
			ucurs();
			insert();
			break;
		case 'A':
			p = strchr(lineptr, '\n');
			if (!p)
				p = strchr(lineptr, 0);
			x = p - lineptr;
			ucurs();
			insert();
			break;
		case 'I':
			x = 0;
			ucurs();
			insert();
			break;
		case 'O':
			len = strlen(lineptr) + 1;
			x = 0;
			
			memmove(lineptr + 1, lineptr, len);
			*lineptr = '\n';
			redraw();
			insert();
			break;
		case 'o':
			while (*lineptr != '\n' && *lineptr)
				lineptr++;
			len = strlen(lineptr) + 1;
			x = 0;
			
			memmove(lineptr + 1, lineptr, len);
			*lineptr = '\n';
			
			if (++y >= t + rows)
			{
				t++;
				utop(1);
			}
			lineptr++;
			redraw();
			insert();
			break;
		case 'r':
			replace();
			break;
		case ':':
			command();
			break;
		case 'l' & 31:
			redraw();
			break;
		default:
			break;
		}
}

void stop(void)
{
	clear();
	tcsetattr(0, TCSANOW, &otio);
	exit(1);
}

int main(int argc, char **argv)
{
	char *lp, *cp;
	
	cp = getenv("COLUMNS");
	lp = getenv("LINES");
	
	if (cp)
		cols = atoi(cp);
	if (lp)
		rows = atoi(lp);
	
	term_init();
	
	linebuf = calloc(1, cols + 1);
	
	tcgetattr(0, &tio);
	otio = tio;
	tio.c_lflag &= ~(ECHO | ICANON | ISIG);
	tio.c_oflag &= ~ONLCR;
	tcsetattr(0, TCSANOW, &tio);
	
	clear();
	redraw();
	
	if (argc > 1)
	{
		strcpy(path, argv[1]);
		cmd_read();
	}
	
	loop();
	clear();
	tcsetattr(0, TCSANOW, &otio);
	return 0;
}
