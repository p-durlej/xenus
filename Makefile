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

all:
	cd hostcmd	&& $(MAKE)
	cd lib		&& $(MAKE)
	cd cmd		&& $(MAKE)
	cd boot		&& $(MAKE)
	cd kern		&& $(MAKE)
	cd disks	&& $(MAKE)

clean:
	cd hostcmd	&& $(MAKE) clean
	cd lib		&& $(MAKE) clean
	cd cmd		&& $(MAKE) clean
	cd boot		&& $(MAKE) clean
	cd kern		&& $(MAKE) clean
	cd disks	&& $(MAKE) clean

accept:
	cp disks/xenus.img www/
	gzip < www/xenus.img > www/xenus.img.gz
	gzip < www/mnx.img > www/mnx.img.gz
	gzip < www/sdk.img > www/sdk.img.gz
	git clone . www/xenus
	rm -rf www/xenus/www/xenus.tgz
	rm -rf www/xenus/.git
	tar zcf www/xenus.tgz -C www xenus
	rm -rf www/xenus
	git add .
	git commit -m "Import a new build"

disks/xenus.img: all

disks/disk.img:
	dd if=/dev/null of=disks/disk.img seek=82000

disks/flp2.img:
	dd if=/dev/null of=disks/flp2.img seek=2880

run-fd: disks/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -curses -fda disks/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 2 -boot a -serial telnet:0.0.0.0:8000,server,nowait

run-hd: disks/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -curses -fda disks/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 8 -boot c -serial telnet:0.0.0.0:8000,server,nowait

run-hd-sdk: disks/xenus.img disks/disk.img
	qemu-system-i386 -curses -fda disks/xenus.img -fdb ~/xmnx/sdk.img -hda disks/disk.img -m 8 -boot c -serial telnet:0.0.0.0:8000,server,nowait

run-sdl-fd: disks/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -display sdl -fda disks/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 2 -boot a

run-sdl-hd: disks/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -display sdl -fda disks/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 8 -boot c

daemon-test: disks/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -display none -daemonize -fda disks/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 8 -boot c -serial telnet:0.0.0.0:8000,server,nowait

upgrade: www/xenus.img www/mnx.img www/sdk.img disks/disk.img
	qemu-system-i386 -curses -fda www/xenus.img -fdb www/sdk.img -hda disks/disk.img -m 8 -boot a -serial telnet:0.0.0.0:8000,server,nowait

daemon: www/xenus.img disks/disk.img disks/flp2.img
	qemu-system-i386 -display none -daemonize -fda www/xenus.img -fdb disks/flp2.img -hda disks/disk.img -m 8 -boot c -serial telnet:0.0.0.0:8000,server,nowait
