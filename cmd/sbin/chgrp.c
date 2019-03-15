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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <grp.h>

int main(int argc, char **argv)
{
	struct stat st;
	char *group;
	gid_t gid;
	int x = 0;
	int i;
	
	if (argc < 3)
	{
		fputs("chgrp group file1...\n", stderr);
		fputs("chgrp gid file1...\n", stderr);
		return 1;
	}
	
	group = argv[1];
	if (isdigit(*group))
		gid = atoi(group);
	else
	{
		struct group *gr;
		
		gr = getgrnam(group);
		if (!gr)
		{
			fprintf(stderr, "Group %s not found\n", group);
			return 1;
		}
		gid = gr->gr_gid;
	}
	
	for (i = 2; i < argc; i++)
	{
		if (stat(argv[i], &st))
		{
			perror(argv[i]);
			x = 1;
			continue;
		}
		
		if (chown(argv[i], st.st_uid, gid))
		{
			perror(argv[i]);
			x = 1;
		}
	}
	return x;
}
