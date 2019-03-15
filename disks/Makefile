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

BIN_DEP	 = ../cmd/sh/sh # XXX
BOOT_DEP = ../boot/boot.bin ../boot/pcmbr.bin
KERN_DEP = ../kern/kern

all: xenus.img

clean:
	rm -Rf tree base.tar libh.tar xenus.img xenus.img0 xenus.tmp

xenus.img0: $(BIN_DEP) $(KERN_DEP)
	./mktree
	>xenus.img0
	../hostcmd/mkfs.xenus -s 2880 xenus.img0 160 tree
	dd status=none if=../boot/boot.bin	of=xenus.img0 conv=notrunc
	dd status=none if=../kern/kern		of=xenus.img0 conv=notrunc bs=1024 seek=1
	dd status=none if=/dev/null		of=xenus.img0 seek=2880

xenus.img: xenus.img0
	cp -fp xenus.img0 xenus.tmp
	qemu-system-i386 -curses -fda xenus.tmp -m 2 -boot a -no-reboot
	mv -f xenus.tmp xenus.img
