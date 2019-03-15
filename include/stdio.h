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

#ifndef _STDIO_H
#define _STDIO_H

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>

#define NULL	0

#define BUFSIZ	512

#define _IONBF	0
#define _IOLBF	1
#define _IOFBF	2
#define __LIBC_IOBF_STROUT	3

#define EOF	(-1)

#define __LIBC_FMODE_R	0x0001
#define __LIBC_FMODE_W	0x0002
#define __LIBC_FMODE_A	0x0004

typedef off_t fpos_t;

typedef struct
{
	int	fd;
	int	mode;
	
	int	eof;
	int	err;
	
	char *	buf;
	int	buf_mode;
	int	buf_size;
	int	buf_pos;
	int	buf_insize;
	int	buf_write;
	int	buf_malloc;
	
	int	ungotc;
} FILE;

extern FILE __libc_file[];

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(char *format, ...);
int fprintf(FILE *f, char *format, ...);
int sprintf(char *str, char *format, ...);
int snprintf(char *str, size_t n, char *format, ...);

int rename(char *name1, char *name2);
int remove(char *path);

FILE *fopen(char *path, char *mstr);
FILE *fdopen(int fd, char *mstr);
int fclose(FILE *f);

void clearerr(FILE *f);
int feof(FILE *f);
int ferror(FILE *f);
int fileno(FILE *f);

int fputc(int c, FILE *f);
#define putc(c,f)	fputc(c, f)
#define putchar(c)	fputc(c, stdout)
int fgetc(FILE *f);
#define getc(f)		fgetc(f)
#define getchar()	fgetc(stdin)
int fflush(FILE *f);

int fseek(FILE *f, long off, int whence);
long ftell(FILE *f);
void rewind(FILE *f);

char *fgets(char *s, int size, FILE *f);
int fputs(char *s, FILE *f);
int puts(char *s);

void setbuf(FILE *f, char *buf);
void setbuffer(FILE *f, char *buf, size_t size);
void setlinebuf(FILE *f);
int setvbuf(FILE *f, char *buf, int mode, size_t size);

void perror(char *s);

#endif
