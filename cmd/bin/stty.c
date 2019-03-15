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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static struct
{
	char *name;
	int flag;
	int word;
} flags[] =
{
	{ "isig",	ISIG,	2 },
	{ "icanon",	ICANON,	2 },
	{ "echo",	ECHO,	2 },
	{ "echoe",	ECHOE,	2 },
	{ "xhup",	XHUP,	2 },
	{ "crtbs",	CRTBS,	2 },
	
	{ "opost",	OPOST,	3 },
	{ "onlcr",	ONLCR,	3 },
};

static cc_t defchars[] =
{
	VBOTH,
	'U'  & 31,
	'C'  & 31,
	'\\' & 31,
	'D'  & 31,
};

static char *chars[] =
{
	"erase",
	"kill",
	"intr",
	"quit",
	"eof",
};

static int aflag;

static char *vchar(unsigned char ch)
{
	static char buf[8];
	
	if (ch == (unsigned char)VDISABLE)
		return "UNDEF";
	
	if (ch == (unsigned char)VBOTH)
		return "BOTH";
	
	if (ch == (unsigned char)127)
		return "DEL";
	
	if (ch == (unsigned char)8)
		return "BS";
	
	if (ch >= 128)
	{
		sprintf(buf, "'\\%03o'", ch);
		return buf;
	}
	
	if (ch < 32)
	{
		sprintf(buf, "'^%c'", '@' + ch);
		return buf;
	}
	
	sprintf(buf, "'%c'", ch);
	return buf;
}

static char pchar(char *s)
{
	if (!strcmp(s, "undef") || !strcmp(s, "UNDEF"))
		return VDISABLE;
	
	if (!strcmp(s, "both") || !strcmp(s, "BOTH"))
		return VBOTH;
	
	if (!strcmp(s, "del") || !strcmp(s, "DEL"))
		return '\177';
	
	if (!strcmp(s, "bs") || !strcmp(s, "BS"))
		return 8;
	
	if (*s == '^')
	{
		if (s[1] == '?')
			return 127;
		return s[1] & 31;
	}
	return *s;
}

int main(int argc, char **argv)
{
	struct termios tio;
	tcflag_t *wp;
	int i, n;
	int f;
	int r;
	int x = 0;
	int o = 0;
	char *p;
	
	if (tcgetattr(0, &tio))
	{
		perror(NULL);
		return 1;
	}
	
	if (argc == 2 && !strcmp(argv[1], "-a"))
	{
		aflag = 1;
		argc--;
		argv++;
	}
	
	if (argc < 2)
	{
		printf("speed %i baud; ", tio.c_ispeed);
		for (i = 0; i < sizeof flags / sizeof *flags; i++)
		{
			wp = &tio.c_cflag + flags[i].word;
			if (!(*wp & flags[i].flag))
				putchar('-');
			printf("%s ", flags[i].name);
		}
		putchar('\n');
		
		for (i = 0; i < sizeof chars / sizeof *chars; i++)
		{
			if (!aflag && (tio.c_cc[i] == defchars[i]))
				continue;
			
			if (o)
				fputs("; ", stdout);
			
			printf("%s = %s", chars[i], vchar(tio.c_cc[i]));
			o = 1;
		}
		if (o)
			putchar('\n');
		return 0;
	}
	
	for (i = 1; i < argc; i++)
	{
		p = argv[i];
		r = 0;
		
		if (isdigit(*p))
		{
			int speed = atoi(p);
			
			cfsetispeed(&tio, speed);
			cfsetospeed(&tio, speed);
			continue;
		}
		
		if (*p == '-')
		{
			r = 1;
			p++;
		}
		
		if (!strcmp(p, "hard"))
		{
			tio.c_lflag &= ~(ECHOE | CRTBS);
			tio.c_lflag |= ECHO;
			tio.c_cc[VERASE]  = '#';
			tio.c_cc[VKILL]   = '@';
			tio.c_cc[VINTR]   = '\177';
			tio.c_cc[VQUIT]   = '\\' & 31;
			tio.c_cc[VEOF]    = 'D'  & 31;
			continue;
		}
		
		if (!strcmp(p, "crt"))
		{
			tio.c_lflag |= ECHO | ECHOE | CRTBS;
			tio.c_cc[VERASE]  = VBOTH;
			tio.c_cc[VKILL]   = 'U'  & 31;
			tio.c_cc[VINTR]   = 'C'  & 31;
			tio.c_cc[VQUIT]   = '\\' & 31;
			tio.c_cc[VEOF]    = 'D'  & 31;
			continue;
		}
		
		for (n = 0; n < sizeof chars / sizeof *chars; n++)
			if (!strcmp(chars[n], p))
			{
				if (!argv[i + 1])
				{
					fprintf(stderr, "stty: %s: missing operand\n", p);
					x = 1;
					break;
				}
				tio.c_cc[n] = pchar(argv[++i]);
				break;
			}
		if (n < sizeof chars / sizeof *chars)
			continue;
		
		for (n = 0; n < sizeof flags / sizeof *flags; n++)
			if (!strcmp(flags[n].name, p))
			{
				wp = &tio.c_cflag + flags[n].word;
				f = flags[n].flag;
				if (r)
					*wp &= ~f;
				else
					*wp |= f;
				break;
			}
		if (n >= sizeof flags / sizeof *flags)
		{
			fprintf(stderr, "stty: bad flag '%s'\n", p);
			x = 1;
		}
	}
	
	if (tcsetattr(0, TCSANOW, &tio))
	{
		perror(NULL);
		return 1;
	}
	return x;
}
