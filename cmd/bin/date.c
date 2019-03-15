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
#include <stdio.h>
#include <time.h>

#define dd2int(s)	(((s)[0] - '0') * 10 + (s)[1] - '0')
#define dddd2int(s)	(dd2int(s) * 100 + dd2int((s) + 2))

int main(int argc,char **argv)
{
	time_t t = time(NULL);
	char *s = argv[1];
	struct tm *tm;
	int l;
	
	if (argc > 2)
		goto usage;
	
	if (argc > 1)
	{
		l = strlen(s);
		if (l != 10 && l != 14)
			goto usage;
		if (strspn(s, "0123456789") != l)
			goto usage;
		
		tm = localtime(&t);
		tm->tm_mon	= dd2int(s) - 1;
		tm->tm_mday	= dd2int(s + 2);
		tm->tm_hour	= dd2int(s + 4);
		tm->tm_min	= dd2int(s + 6);
		tm->tm_sec	= dd2int(s + 8);
		if (l > 10)
			tm->tm_year = dddd2int(s + 10) - 1900;
		t = mktime(tm);
	}
	printf("%s", ctime(&t));
	if (argc == 2 && stime(&t))
	{
		perror("stime");
		return 1;
	}
	return 0;
usage:
	fputs("date [MMDDhhmmss[CCYY]]\n", stderr);
	return 1;
}
