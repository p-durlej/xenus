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

all: ../lib/libc.a
	cd shlib&& $(MAKE)
	cd sh	&& $(MAKE)
	cd bin	&& $(MAKE)
	cd sbin	&& $(MAKE)
	cd dbg	&& $(MAKE)
	cd comp	&& $(MAKE)
	cd comp7&& $(MAKE)
	cd vi	&& $(MAKE)

clean:
	cd shlib&& $(MAKE) clean
	cd sh	&& $(MAKE) clean
	cd bin	&& $(MAKE) clean
	cd sbin	&& $(MAKE) clean
	cd dbg	&& $(MAKE) clean
	cd comp	&& $(MAKE) clean
	cd comp7&& $(MAKE) clean
	cd vi	&& $(MAKE) clean
