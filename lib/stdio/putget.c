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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void __libc_panic(char *msg);

int fputc(int c, FILE *f)
{
	if (!(f->mode & __LIBC_FMODE_W))
	{
		errno = EBADF;
		f->err = 1;
		return EOF;
	}
	
	switch (f->buf_mode)
	{
	case _IONBF:
		if (write(f->fd, &c, 1) == 1)
			return c & 255;
		f->err = 1;
		return EOF;
	case _IOFBF:
	case _IOLBF:
		f->buf_write  = 1;
		f->buf_insize = 0;
		if (!f->buf)
		{
			f->buf = malloc(BUFSIZ);
			if (!f->buf)
			{
				f->err = 1;
				return EOF;
			}
			f->buf_size   = BUFSIZ;
			f->buf_malloc = 1;
		}
		if (f->buf_pos == f->buf_size && fflush(f))
			return EOF;
		f->buf[f->buf_pos] = c;
		f->buf_pos++;
		if (f->buf_mode == _IOLBF && c == '\n')
			return fflush(f);
		return c & 255;
	case __LIBC_IOBF_STROUT:
		f->buf_pos++;
		if (f->buf_pos <= f->buf_size || f->buf_size == -1)
			f->buf[f->buf_pos - 1] = c;
		return c & 255;
	}
	__libc_panic("fputc buf mode");
	return EOF;
}

int fgetc(FILE *f)
{
	if (!(f->mode & __LIBC_FMODE_R))
	{
		errno = EBADF;
		f->err = 1;
		return EOF;
	}
	
	if (f->ungotc != EOF)
	{
		int c = f->ungotc;
		
		f->ungotc = EOF;
		return c;
	}
	
	switch (f->buf_mode)
	{
	case _IONBF:
	{
		unsigned char c;
		int cnt;
		
		cnt = read(f->fd, &c, 1);
		if (cnt == 1)
			return c & 255;
		if (cnt < 0)
			f->err = 1;
		return EOF;
	}
	case _IOFBF:
	case _IOLBF:
		if (!f->buf)
		{
			f->buf = malloc(BUFSIZ);
			if (!f->buf)
			{
				f->err = 1;
				return EOF;
			}
			f->buf_size   = BUFSIZ;
			f->buf_malloc = 1;
		}
		if (f->buf_pos == f->buf_insize)
		{
			f->buf_pos    = 0;
			f->buf_insize = read(f->fd, f->buf, f->buf_size);
			if (f->buf_insize < 0)
			{
				f->buf_insize = 0;
				f->err = 1;
				return EOF;
			}
			if (!f->buf_insize)
			{
				f->eof = 1;
				return EOF;
			}
		}
		return f->buf[f->buf_pos++] & 255;
	}
	__libc_panic("fgetc buf mode");
	return EOF;
}

int fputs(char *s, FILE *f)
{
	while (*s)
		if (fputc(*s++, f) == EOF)
			return EOF;
	return 0;
}

int puts(char *s)
{
	if (fputs(s, stdout))
		return EOF;
	if (fputc('\n', stdout) == EOF)
		return EOF;
	return 0;
}

int ungetc(int c, FILE *f)
{
	if (f->ungotc == EOF)
	{
		f->ungotc = c & 255;
		return c & 255;
	}
	f->err = 1;
	return EOF;
}
