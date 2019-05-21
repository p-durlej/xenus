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

#include <sys/types.h>

#define NULL	0

void *	memccpy(void *dest, void *src, int c, size_t n);
void *	memchr(void *s, int c, size_t n);
int	memcmp(void *s1, void *s2, size_t n);
void *	memcpy(void *dest, void *src, size_t n);
void *	memmove(void *dest, void *src, size_t n);
void *	memset(void *s, int c, size_t n);

char *	strcat(char *dest, char *src);
char *	strchr(char *s, int c);
int	strcmp(char *s1, char *s2);
char *	strcpy(char *dest, char *src);
size_t	strcspn(char *s, char *reject);
int	strlen(char *s);
char *	strncat(char *dest, char *src, size_t n);
int	strncmp(char *s1, char *s2, size_t n);
char *	strncpy(char *dest, char *src, size_t n);
char *	strpbrk(char *s, char *accept);
char *	strrchr(char *s, int c);
size_t	strspn(char *s, char *accept);
char *	strtok(char *s, char *delim);
char *	strdup(char *s);
char *	strerror(int err);

char *	strsignal(int nr);
