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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "main.h"
#include "term.h"

static int vt_x, vt_y;

static void vtxx_output(char *s, int len)
{
	int i;
	
	for (i = 0; i < len; i++)
		putchar(s[i]);
	
	vt_x += len;
	vt_y += vt_x / cols;
	vt_x %= cols;
}

static void vtxx_gotoxy(int x, int y)
{
	printf("\033[%i;%iH", y + 1, x + 1);
	
	vt_x = x;
	vt_y = y;
}

static void vtxx_cleol(void)
{
	printf("\033[K\033[%iH", vt_y + 2);
	
	vt_x = 0;
	vt_y++;
}

static void vtxx_cleos(void)
{
	while (vt_y < rows)
		cleol();
}

static void vtxx_clear(void)
{
	fputs("\033[H\033[J", stdout);
	
	vt_x = vt_y = 0;
}

static void vtxx_home(void)
{
	fputs("\033[H", stdout);
	
	vt_x = vt_y = 0;
}

static void vtxx_scup(void)
{
	printf("\033[%iH\n", rows);
}

static void vtxx_scdn(void)
{
	printf("\033M");
}

static void vtxx_invert(void)
{
	printf("\033[7m");
}

static void vtxx_reset(void)
{
	printf("\033[0m");
}

static int vtxx_getch(void)
{
	unsigned char c;
	
	fflush(stdout);
again:
	switch (read(0, &c, 1))
	{
	case -1:
		if (errno == EINTR)
			goto again;
		stop();
	case 1:
		break;
	default:
		stop();
	}
	if (c == '\r')
		return '\n';
	return c;
}

void vtxx_init(void)
{
	if (!cols) cols = 80;
	if (!rows) rows = 24;
	
	output	= vtxx_output;
	gotoxy	= vtxx_gotoxy;
	cleol	= vtxx_cleol;
	cleos	= vtxx_cleos;
	clear	= vtxx_clear;
	home	= vtxx_home;
	scup	= vtxx_scup;
	scdn	= vtxx_scdn;
	invert	= vtxx_invert;
	reset	= vtxx_reset;
	getch	= vtxx_getch;
}
