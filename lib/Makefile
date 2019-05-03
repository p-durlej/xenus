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

all: crt0.o rawstart.o libc.a

include ../mk/user.mk
include xenus/sys.mk

STRING_O = string/strncat.o	\
	   string/strncmp.o	\
	   string/strncpy.o	\
	   string/strpbrk.o	\
	   string/strrchr.o	\
	   string/strspn.o	\
	   string/memccpy.o	\
	   string/memchr.o	\
	   string/memcmp.o	\
	   string/memcpy.o	\
	   string/memmove.o	\
	   string/memset.o	\
	   string/strcat.o	\
	   string/strchr.o	\
	   string/strcmp.o	\
	   string/strcpy.o	\
	   string/strcspn.o	\
	   string/strlen.o	\
	   string/strdup.o	\
	   string/memcpy.o	\
	   string/memset.o	\
	   string/strtok.o

UNISTD_O = unistd/creat.o	\
	   unistd/exec.o	\
	   unistd/ttyname.o	\
	   unistd/isatty.o	\
	   unistd/getcwd.o	\
	   unistd/raise.o	\
	   unistd/mkfifo.o

STDIO_O = stdio/stdfd.o		\
	  stdio/putget.o	\
	  stdio/vprintf.o	\
	  stdio/vfprintf.o	\
	  stdio/vsprintf.o	\
	  stdio/vsnprintf.o	\
	  stdio/printf.o	\
	  stdio/fprintf.o	\
	  stdio/sprintf.o	\
	  stdio/snprintf.o	\
	  stdio/remove.o	\
	  stdio/fopen.o		\
	  stdio/fseek.o		\
	  stdio/setbuf.o	\
	  stdio/clearerr.o	\
	  stdio/feof.o		\
	  stdio/ferror.o	\
	  stdio/fileno.o	\
	  stdio/gets.o		\
	  stdio/fgets.o		\
	  stdio/fflush.o

STDLIB_O = stdlib/exit.o	\
	   stdlib/atol.o	\
	   stdlib/atoi.o	\
	   stdlib/abort.o	\
	   stdlib/system.o	\
	   stdlib/qsort.o

MALLOC_O = malloc/calloc.o	\
	   malloc/free.o	\
	   malloc/malloc.o	\
	   malloc/realloc.o

ENVIRON_O = environ.o

PASSWD_O = passwd/passwd.o	\
	   passwd/group.o	\
	   passwd/getpassn.o

CTYPE_O = ctype/isalnum.o ctype/isalpha.o ctype/iscntrl.o ctype/isdigit.o \
	  ctype/islower.o ctype/isprint.o ctype/ispunct.o ctype/isspace.o \
	  ctype/isupper.o ctype/isxdigit.o

LIB_O = libc.o		\
	io.o		\
	perror.o	\
	stdarg.o	\
	setjmp.o	\
	dirent.o	\
	time.o		\
	siglist.o	\
	termios.o	\
	errno.o		\
	md5.o		\
	$(STRING_O)	\
	$(UNISTD_O)	\
	$(STDIO_O)	\
	$(STDLIB_O)	\
	$(ENVIRON_O)	\
	$(PASSWD_O)	\
	$(CTYPE_O)	\
	$(MALLOC_O)	\
	$(SYS_O)

libc.a: $(LIB_O)
	rm -f libc.a
	$(AR) -q libc.a $(LIB_O)

crt0_kd:
	gcc-xenus-kd -c crt0_kd.c

clean:
	rm -f *.o libc.a
	rm -f $(LIB_O)
