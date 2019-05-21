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

#include <signal.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	char done[argc];
	char buf[80];
	FILE *f, *of;
	char *prog;
	char *p;
	int dis = 0;
	int t = 0;
	int i;
	
	umask(022);
	
	prog = strrchr(argv[0], '/');
	if (prog)
		prog++;
	else
		prog = argv[0];
	
	if (*prog == 'd')
		dis = 1;
	
	if (argc > 1 && *argv[1] == '-')
	{
		t = argv[1][1];
		argv++;
		argc--;
	}
	
	if (argc < 2)
	{
		if (dis)
			fprintf(stderr, "%s tty1...\n", prog);
		else
			fprintf(stderr, "%s [-TYPE] tty1...\n", prog);
		return 1;
	}
	
	f = fopen("/etc/ttys", "r");
	if (!f)
	{
		perror("/etc/ttys");
		return 1;
	}
	
	of = fopen("/etc/ttys.tmp", "w");
	if (!of)
	{
		perror("/etc/ttys.tmp");
		return 1;
	}
	
	memset(done, 0, sizeof done);
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (strlen(buf) < 3)
			continue;
		
		for (i = 1; i < argc; i++)
			if (!strcmp(buf + 2, argv[i]))
			{
				*buf = dis ? '0' : '1';
				
				if (t)
					buf[1] = t;
				
				done[i] = 1;
				break;
			}
		fprintf(of, "%s\n", buf);
	}
	
	if (dis)
	{
		for (i = 1; i < argc; i++)
			if (!done[i])
				fprintf(stderr, "%s not found\n", argv[i]);
	}
	else
	{
		if (!t)
			t = 'f';
		
		for (i = 1; i < argc; i++)
			if (!done[i])
				fprintf(of, "1%c%s\n", t, argv[i]);
	}
	
	if (ferror(of) || fclose(of))
	{
		perror("/etc/ttys.tmp");
		unlink("/etc/ttys.tmp");
		return 1;
	}
	if (rename("/etc/ttys.tmp", "/etc/ttys"))
	{
		perror("/etc/ttys.tmp -> /etc/ttys");
		unlink("/etc/ttys.tmp");
		return 1;
	}
	kill(1, 1);
	return 0;
}
