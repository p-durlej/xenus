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

include ../mk/kern.mk

all: kern # kern.elf

CC = ../hostcmd/gcc-xenus -Os -D_KERN_ -I../include

LIB_O = lib/libk.a

FS_O =	fs/blk.o	\
	fs/mount.o	\
	fs/inode.o	\
	fs/bmap.o	\
	fs/dir.o	\
	fs/alloc.o	\
	fs/file.o	\
	fs/rw.o		\
	fs/open.o	\
	fs/mknod.o	\
	fs/chr.o	\
	fs/pipe.o

DRV_O =	dv/condev.o	\
	dv/procdev.o	\
	dv/tty.o	\
	dv/mem.o	\
	dv/hd.o		\
	dv/fd.o		\
	dv/rs.o

KERN_O = asmstart.o asmintr.o intr.o start.o console.o pckbd.o	  \
         clock.o process.o umem.o syscall.o exec.o fork.o uname.o \
         $(FS_O) $(BLK_O) $(DRV_O) $(LIB_O) ttyld.o sigret.o

kern: $(KERN_O)
	ld -T lds -m elf_i386 --oformat binary $(KERN_O) -o kern

kern.elf: $(KERN_O)
	ld -T lds -m elf_i386 $(KERN_O) -o kern.elf

lib/libk.a:
	cd lib && $(MAKE)

clean:
	rm -f $(KERN_O) kern kern.elf
	cd lib && $(MAKE) clean
