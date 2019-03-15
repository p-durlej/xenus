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
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

static struct termios tp, otp;
static int ttyfd = -1;
static int rflag;
static int xit;

static int otoi(char *nptr)
{
	int n = 0;
	
	while (*nptr >= '0' && *nptr <= '7')
	{
		n <<= 3;
		n  += *nptr++ - '0';
	}
	return n;
}

static int uudecode1(unsigned char *p, FILE *f)
{
	int len = *p++ - ' ';
	unsigned w;
	int i;
	
	if (len < 0 || len > 64)
		return 1;
	if (len == 64)
		len = 0;
	
	while (len)
	{
		w  = (*p++ - ' ') & 63; w <<= 6;
		w |= (*p++ - ' ') & 63; w <<= 6;
		w |= (*p++ - ' ') & 63; w <<= 6;
		w |= (*p++ - ' ') & 63;
		
		for (i = 0; i < 3 && len; i++, len--)
		{
			fputc(w >> 16, f);
			w <<= 8;
		}
	}
	
	return 0;
}

static void uudecode(char *path)
{
	FILE *f = NULL, *inf = stdin;
	char *p, *n, *ms;
	char buf[256];
	mode_t m;
	int fd = -1;
	
	if (strcmp(path, "-"))
		inf = fopen(path, "r");
	if (!inf)
		goto ifail;
	
retry:
	if (!fgets(buf, sizeof buf, inf))
		goto ifail;
	
	p = strchr(buf, '\n');
	if (p)
		*p = 0;
	
	p  = strtok(buf,  " ");
	ms = strtok(NULL, " ");
	n  = strtok(NULL, " ");
	
	if (!p || !n || !ms || strcmp(p, "begin"))
		goto retry;
	m = otoi(ms);
	
	fd = open(n, O_CREAT | O_TRUNC | O_WRONLY, m);
	if (fd < 0)
		goto ofail;
	f = fdopen(fd, "w");
	if (!f)
		goto ofail;
	
	for (;;)
	{
		if (!fgets(buf, sizeof buf, inf))
			goto ifail;
		
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (!strcmp(buf, "end"))
			break;
		
		if (uudecode1((unsigned char *)buf, f))
			goto badfmt;
		if (ferror(f))
			goto ofail;
		if (rflag && ttyfd >= 0)
			write(ttyfd, ".", 1);
	}
	if (rflag && ttyfd >= 0)
		write(ttyfd, "\n", 1);
	
clean:
	if (inf != stdin)
		fclose(inf);
	if (f && fclose(f))
		goto ofail;
	if (fd >= 0 && !f)
		close(fd);
	return;
badfmt:
	fprintf(stderr, "%s: Invalid format\n", path);
	xit = 1;
	goto clean;
ofail:
	perror(n);
	xit = 1;
	goto clean;
ifail:
	if (ferror(inf))
		perror(path);
	else
		fprintf(stderr, "%s: EOF\n", path);
	xit = 1;
	goto clean;
}

static void intr(int nr)
{
	tcsetattr(0, TCSANOW, &otp);
	raise(nr);
}

int main(int argc, char **argv)
{
	char *p;
	int i;
	
	p = strrchr(*argv, '/') + 1;
	if (p == (char *)1)
		p = *argv;
	if (!strcmp(p, "uureceive") || !strcmp(p, "uur"))
	{
		ttyfd = open("/dev/tty", O_WRONLY);
		tcgetattr(0, &otp);
		signal(SIGINT, intr);
		tp = otp;
		tp.c_lflag &= ~ECHO;
		tcsetattr(0, TCSANOW, &tp);
		rflag = 1;
	}
	
	for (i = 1; i < argc; i++)
		uudecode(argv[i]);
	if (argc < 2)
		uudecode("-");
	
	if (rflag)
		tcsetattr(0, TCSANOW, &otp);
	return xit;
}
