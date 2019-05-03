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

static void xenus_output(char *s, int len)
{
	int i;
	
	for (i = 0; i < len; i++)
		putchar(s[i]);
	
	vt_x += len;
	vt_y += vt_x / cols;
	vt_x %= cols;
}

static void xenus_gotoxy(int x, int y)
{
	printf("\033p%c%c", x, y);
	
	vt_x = x;
	vt_y = y;
}

static void xenus_cleol(void)
{
	int cnt = cols;
	int i;
	
	printf("\033C");
	if (vt_y < rows - 1)
	{
		printf("\r\n");
		vt_x = 0;
		vt_y++;
	}
}

static void xenus_cleos(void)
{
	while (vt_y < rows)
		cleol();
}

static void xenus_clear(void)
{
	putchar('L' & 31);
	
	vt_x = vt_y = 0;
}

static void xenus_home(void)
{
	printf("\033h");
	
	vt_x = vt_y = 0;
}

static void xenus_scup(void)
{
	printf("\033u");
}

static void xenus_scdn(void)
{
	printf("\033d");
}

static void xenus_invert(void)
{
	printf("\033i");
}

static void xenus_reset(void)
{
	printf("\033I");
}

static int xenus_getch(void)
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
	return c;
}

void xenus_init(void)
{
	if (!cols) cols = 80;
	if (!rows) rows = 25;
	
	fast = 1;
	
	output	= xenus_output;
	gotoxy	= xenus_gotoxy;
	cleol	= xenus_cleol;
	cleos	= xenus_cleos;
	clear	= xenus_clear;
	home	= xenus_home;
	scup	= xenus_scup;
	scdn	= xenus_scdn;
	invert	= xenus_invert;
	reset	= xenus_reset;
	getch	= xenus_getch;
}
