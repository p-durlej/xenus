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

#include <xenus/sfilsys.h>
#include <xenus/config.h>
#include <environ.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "v7compat.h"

#if V7DEBUG
extern int trace;
#endif

static void split(char *s, char **v)
{
	char *p;
	
	for (p = s; *s; s++)
		if (*s == '\377')
		{
			*v++ = p;
			p = s + 1;
			*s = 0;
		}
	*v = NULL;
}

static int count(char *s)
{
	int cnt;
	
	for (cnt = 0; *s; s++)
		if (*s == '\377')
			cnt++;
	return cnt;
}

void start(char *arg, char *env)
{
	int ac, ec;
#if V7DEBUG
	char *p;
#endif
	
	ac = count(arg);
	ec = count(env);
	
	unsigned stack[ac + ec + 3];
	char **av = (char **)(stack + 1);
	char **ev = av + ac + 1;
	
	split(arg, av);
	split(env, av + ac + 1);
	environ = av + ac + 1;
	stack[0] = ac;
	
#if V7DEBUG
	p = getenv("V7TRACE");
	if (p && atoi(p))
		trace = 1;
#endif
	ubin = getenv("UBIN");
	
	if (sbrk(0) != (void *)ibrk)
		panic("v7 start brk");
	
	_sfilsys(SF_NAM14 | SF_OLINK);
	enter(stack);
}

int main()
{
	panic("v7 main");
}
