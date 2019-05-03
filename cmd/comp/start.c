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

#include <xenus/config.h>
#include <unistd.h>
#include <string.h>

#include "mnxcompat.h"

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
	int i;
	
	ac = count(arg);
	ec = count(env);
	
	char *av[ac + 1], *ev[ec + 1];
	
	split(arg, av);
	split(env, ev);
	environ = ev;
	
	unsigned stack[ac + ec + 3];
	
	stack[0] = entry;
	stack[1] = cs;
	stack[2] = ac;
	memcpy(stack + 3,      av, (ac + 1) * 4);
	memcpy(stack + 4 + ac, ev, (ec + 1) * 4);
	
	if (sbrk(0) != (void *)ibrk)
		panic("mnx start brk");
	
#if MNXDEBUG
	for (i = 0; i < ec; i++)
		if (!strcmp(ev[i], "MNXTRON=1"))
			trace = 1;
#endif
	
	enter(stack);
}

int main()
{
	panic("mnx main");
}
