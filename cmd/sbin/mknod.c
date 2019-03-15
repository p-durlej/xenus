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

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define MODE	0600

int main(int argc, char **argv)
{
	int err;
	
	if (argc < 3)
		goto usage;
	
	if (argv[2][1])
		goto badtype;
	
	if ((*argv[2] == 'b' || *argv[2] == 'c') && argc < 5)
		goto usage;
	
	switch (*argv[2])
	{
	case 'p':
		err = mknod(argv[1], MODE | S_IFIFO, 0);
		break;
	case 'b':
		err = mknod(argv[1], MODE | S_IFBLK, makedev(atoi(argv[3]), atoi(argv[4])));
		break;
	case 'c':
		err = mknod(argv[1], MODE | S_IFCHR, makedev(atoi(argv[3]), atoi(argv[4])));
		break;
	default:
		goto badtype;
	}
	
	if (err)
	{
		perror(NULL);
		return 1;
	}
	return 0;
badtype:
	fputs("Invalid inode type\n", stderr);
	return 1;
usage:
	fputs("mknod name bc major minor\n", stderr);
	fputs("mknod name p\n", stderr);
	return 1;
}
