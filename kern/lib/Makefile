# Copyright (c) Piotr Durlej
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

include ../../mk/kern.mk

all: libk.a

CC = ../../hostcmd/gcc-xenus -Os -D_KERN_ -I../../include

LIBK_O = ../../lib/string/strncat.o	\
	 ../../lib/string/strncmp.o	\
	 ../../lib/string/strncpy.o	\
	 ../../lib/string/strpbrk.o	\
	 ../../lib/string/strrchr.o	\
	 ../../lib/string/strspn.o	\
	 ../../lib/string/memchr.o	\
	 ../../lib/string/memcmp.o	\
	 ../../lib/string/memmove.o	\
	 ../../lib/string/strcat.o	\
	 ../../lib/string/strchr.o	\
	 ../../lib/string/strcmp.o	\
	 ../../lib/string/strcpy.o	\
	 ../../lib/string/strlen.o	\
	 ../../lib/string/memset.o	\
	 ../../lib/string/memcpy.o	\
	 ../../lib/setjmp.o		\
	 ../../lib/io.o

LIB_O = printf.o minit.o malloc.o free.o $(LIBK_O)

libk.a: $(LIB_O)
	ar cr libk.a $(LIB_O)
clean:
	rm -f $(LIB_O)
