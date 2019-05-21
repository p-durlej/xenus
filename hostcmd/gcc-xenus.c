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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define GCC	"cc"

#ifndef SYSTEM
#define SYSTEM	"/home/pdurlej/xenus"
#endif

void add_str(char ***l, char *s)
{
	int i = -1;
	
	while ((*l) [++i]);
	
	*l = realloc(*l, sizeof(char *) * (i + 2));
	if (!*l)
	{
		perror("realloc");
		exit(errno);
	}
	
	(*l) [i] = s;
	(*l) [i + 1] = NULL;
}

int main(int argc, char **argv)
{
	char **cc_arg;
	char *libc = SYSTEM "/lib/libc.a";
	char *crt = SYSTEM "/lib/crt0.o";
	int shared = 1;
	int eflag = 0;
	int high = 0;
	int link = 1;
	int i;
	
	cc_arg = malloc(sizeof(char *));
	*cc_arg = NULL;
	
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "-S") ||
		    !strcmp(argv[i], "-E"))
		    link = 0;
		if (!strcmp(argv[i], "-e"))
		{
			memmove(argv + i, argv + i + 1, (argc - i) * sizeof *argv);
			argc--;
			i--;
			
			eflag = 1;
			continue;
		}
		if (!strcmp(argv[i], "-R"))
		{
			memmove(argv + i, argv + i + 1, (argc - i) * sizeof *argv);
			argc--;
			i--;
			
			crt = SYSTEM "/lib/rawstart.o";
			shared = 0;
			continue;
		}
		if (!strcmp(argv[i], "-H"))
		{
			memmove(argv + i, argv + i + 1, (argc - i) * sizeof *argv);
			argc--;
			i--;
			
			high = 1;
			continue;
		}
		if (!strcmp(argv[i], "-static"))
		{
			memmove(argv + i, argv + i + 1, (argc - i) * sizeof *argv);
			argc--;
			i--;
			
			shared = 0;
			continue;
		}
		if (!strcmp(argv[i], "-shared"))
		{
			memmove(argv + i, argv + i + 1, (argc - i) * sizeof *argv);
			argc--;
			i--;
			
			shared = 1;
			continue;
		}
	}
	if (shared)
	{
		libc = SYSTEM "/cmd/shlib/libc.a";
		crt = SYSTEM "/cmd/shlib/crt0.o";
		shared = 1;
	}
	
	add_str(&cc_arg, GCC);
	add_str(&cc_arg, "-march=i386");
	add_str(&cc_arg, "-m32");
	add_str(&cc_arg, "-Wno-unused-command-line-argument");
	add_str(&cc_arg, "-fno-stack-protector");
	add_str(&cc_arg, "-fno-builtin");
	add_str(&cc_arg, "-fno-pic");
	add_str(&cc_arg, "-static");
	add_str(&cc_arg, "-nostdlib");
	add_str(&cc_arg, "-nostdinc");
	add_str(&cc_arg, "-I" SYSTEM "/include");
	if (shared)
		add_str(&cc_arg, "-D__SHARED__");
	if (link)
	{
		add_str(&cc_arg, "-L" SYSTEM "/lib");
		add_str(&cc_arg, "-nostartfiles");
		add_str(&cc_arg, crt);
		add_str(&cc_arg, "-Xlinker");
		add_str(&cc_arg, "-melf_i386");
		add_str(&cc_arg, "-Xlinker");
		add_str(&cc_arg, "-T");
		add_str(&cc_arg, "-Xlinker");
		add_str(&cc_arg, SYSTEM "/lds");
		if (high)
		{
			add_str(&cc_arg, "-Xlinker");
			add_str(&cc_arg, "-Ttext");
			add_str(&cc_arg, "-Xlinker");
			add_str(&cc_arg, "0x300000");
		}
		if (!eflag)
		{
			add_str(&cc_arg, "-Xlinker");
			add_str(&cc_arg, "--oformat");
			add_str(&cc_arg, "-Xlinker");
			add_str(&cc_arg, "binary");
		}
	}
	for (i = 1; i < argc; i++)
		add_str(&cc_arg, argv[i]);
	if (link)
		add_str(&cc_arg, libc);
#if 0
	for (i = 0; cc_arg[i]; i++)
		fprintf(stderr, "%s ", cc_arg[i]);
	fputc('\n', stdout);
#endif
	execvp(GCC, cc_arg);
	perror(GCC);
	return 1;
}
