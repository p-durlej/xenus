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
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	char *bootname, *rootname;
	struct stat st;
	int fd;
	
	if (argc != 3)
	{
		fputs("setroot bootdev rootdev\n", stderr);
		return 1;
	}
	
	bootname = argv[1];
	rootname = argv[2];
	
	if (stat(rootname, &st))
	{
		perror(rootname);
		return 1;
	}
	printf("root %i,%i\n", major(st.st_rdev), minor(st.st_rdev));
	
	fd = open(bootname, O_WRONLY);
	if (fd < 0)
		goto bderr;
	if (lseek(fd, 1024 + 16, SEEK_SET) < 0)
		goto bderr;
	errno = EINVAL;
	if (write(fd, &st.st_rdev, 4) != 4)
		goto bderr;
	if (close(fd))
		goto bderr;
	return 0;
bderr:
	perror(bootname);
	return 1;
}
