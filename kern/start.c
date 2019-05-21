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

#include <xenus/process.h>
#include <xenus/console.h>
#include <xenus/config.h>
#include <xenus/printf.h>
#include <xenus/clock.h>
#include <xenus/panic.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <xenus/fs.h>

extern dev_t root;
extern int rootro;
extern int fpu;

int errno = 0;

void __libc_malloc_init();
void pg_add(unsigned base, unsigned size);
void pg_init(size_t memsize);
void pckbd_init(void);
void con_init(void);
void rs_init(void);
void proc_init(void);
void banner(void);

void __libc_panic(char *msg)
{
	panic(msg);
}

static void ena20(void)
{
	while (inb(0x64) & 2);
	outb(0x64, 0xd1);
	while (inb(0x64) & 2);
	outb(0x60, 0xdf);
	while (inb(0x64) & 2);
}

static void hw_init(void)
{
	extern int mem, low;
	extern int _end;
	
	int lowb = (u32_t)&_end;
	
	lowb +=  15;
	lowb &= ~15;
	
	mem *= 1024;
	low *= 1024;
	low -= lowb;
	
	ena20();
	intr_init();
	clock_init();
	pckbd_init();
	con_init();
	pg_init(mem);
	pg_add(lowb, low);
	pg_add(0x2000, 0x5000);
	rs_init();
	banner();
	
	printf("mem = %i\n", mem + low);
	printf("fpu = %i\n", fpu);
}

int main(void)
{
	hw_init();
	blk_init();
	chr_init();
	
#if 1
	if (sizeof(struct disk_inode) != BLK_SIZE)
	{
		printf("dino %i\n", sizeof(struct disk_inode));
		panic("dino size");
	}
#endif
	
	printf("root = %i,%i\n", major(root), minor(root));
	mountroot(root, rootro);
	time.ticks = 0;
	time.time = curr->root->sb->time;
	proc_init();
}

void panic(char *s)
{
	con_puts(s);
	con_putc('\n');
	/* asm("cli; wbinvd"); */
	for (;;);
}
