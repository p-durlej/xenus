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

#include <xenus/selector.h>
#include <xenus/process.h>
#include <xenus/printf.h>
#include <xenus/panic.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <signal.h>

#define IRQ0		0x40
#define IRQ8		0x48

#define A8259A_M	0x20
#define A8259A_S	0xa0

#define CASCADE_IRQ	2

void asm_irq_0();
void asm_irq_1();
void asm_irq_2();
void asm_irq_3();
void asm_irq_4();
void asm_irq_5();
void asm_irq_6();
void asm_irq_7();
void asm_irq_8();
void asm_irq_9();
void asm_irq_10();
void asm_irq_11();
void asm_irq_12();
void asm_irq_13();
void asm_irq_14();
void asm_irq_15();

void asm_exc_0();
void asm_exc_1();
void asm_exc_2();
void asm_exc_3();
void asm_exc_4();
void asm_exc_5();
void asm_exc_6();
void asm_exc_7();
void asm_exc_8();
void asm_exc_9();
void asm_exc_10();
void asm_exc_11();
void asm_exc_12();
void asm_exc_13();
void asm_exc_14();
void asm_exc_15();
void asm_exc_16();
void asm_exc_17();
void asm_exc_18();

void asm_syscall();

void *irq_vect[16];

void intr_set8259a(int irq0, int irq8)
{
	outb(A8259A_M,	   0x11);
	outb(A8259A_M + 1, irq0);
	outb(A8259A_M + 1, 1 << CASCADE_IRQ);
	outb(A8259A_M + 1, 0x03);
	
	outb(A8259A_S,	   0x11);
	outb(A8259A_S + 1, irq8);
	outb(A8259A_S + 1, CASCADE_IRQ);
	outb(A8259A_S + 1, 0x03);

	outb(A8259A_M + 1, 0xff);
	outb(A8259A_S + 1, 0xff);
}

void intr_setvect(int nr, void *addr, int dpl)
{
	extern u16_t idt[];
	u16_t *p;
	
	p = idt + nr * 4;
	
	intr_disable();
	p[0]  = (u32_t)addr;
	p[1]  = KERN_CS;
	p[2]  = 0x8e00;
	p[2] |= dpl << 13;
	p[3]  = (u32_t)addr >> 16;
	intr_enable();
}

static void noirq(int nr)
{
	static int x[16];
	
	if (x[nr])
		return;
	
	printf("bad irq %i\n", nr);
	x[nr] = 1;
}

void intr_init()
{
	int i;
	
	for (i = 0; i < 16; i++)
		irq_vect[i] = noirq;
	
	intr_set8259a(IRQ0,IRQ8);
	irq_ena(CASCADE_IRQ);
	intr_setvect(IRQ0,	asm_irq_0,  0);
	intr_setvect(IRQ0 + 1,	asm_irq_1,  0);
	intr_setvect(IRQ0 + 2,	asm_irq_2,  0);
	intr_setvect(IRQ0 + 3,	asm_irq_3,  0);
	intr_setvect(IRQ0 + 4,	asm_irq_4,  0);
	intr_setvect(IRQ0 + 5,	asm_irq_5,  0);
	intr_setvect(IRQ0 + 6,	asm_irq_6,  0);
	intr_setvect(IRQ0 + 7,	asm_irq_7,  0);
	intr_setvect(IRQ8,	asm_irq_8,  0);
	intr_setvect(IRQ8 + 1,	asm_irq_9,  0);
	intr_setvect(IRQ8 + 2,	asm_irq_10, 0);
	intr_setvect(IRQ8 + 3,	asm_irq_11, 0);
	intr_setvect(IRQ8 + 4,	asm_irq_12, 0);
	intr_setvect(IRQ8 + 5,	asm_irq_13, 0);
	intr_setvect(IRQ8 + 6,	asm_irq_14, 0);
	intr_setvect(IRQ8 + 7,	asm_irq_15, 0);
	intr_enable();
	
	intr_setvect(0,	 asm_exc_0,  0);
	intr_setvect(1,	 asm_exc_1,  0);
	intr_setvect(2,	 asm_exc_2,  0);
	intr_setvect(3,	 asm_exc_3,  0);
	intr_setvect(4,	 asm_exc_4,  0);
	intr_setvect(5,	 asm_exc_5,  0);
	intr_setvect(6,	 asm_exc_6,  0);
	intr_setvect(7,	 asm_exc_7,  0);
	intr_setvect(8,	 asm_exc_8,  0);
	intr_setvect(9,	 asm_exc_9,  0);
	intr_setvect(10, asm_exc_10, 0);
	intr_setvect(11, asm_exc_11, 0);
	intr_setvect(12, asm_exc_12, 0);
	intr_setvect(13, asm_exc_13, 0);
	intr_setvect(14, asm_exc_14, 0);
	intr_setvect(15, asm_exc_15, 0);
	intr_setvect(16, asm_exc_16, 0);
	intr_setvect(17, asm_exc_17, 0);
	intr_setvect(18, asm_exc_18, 0);
	
	intr_setvect(0x80, asm_syscall, 3);
}

void irq_ena(int nr)
{
	unsigned int addr;
	unsigned char m;
	
	if (nr < 8)
		addr = A8259A_M + 1;
	else
	{
		addr = A8259A_S + 1;
		nr -= 8;
		if (nr > 7)
			panic("irq_ena: invalid irq number");
	}
	
	intr_disable();
	m  = inb(addr);
	m &= ~(1 << nr);
	outb(addr, m);
	intr_enable();
}

void irq_dis(int nr)
{
	unsigned int addr;
	unsigned char m;
	
	if (nr < 8)
		addr = A8259A_M + 1;
	else
	{
		addr = A8259A_S + 1;
		nr -= 8;
		if (nr > 7)
			panic("irq_dis: invalid irq number");
	}
	
	intr_disable();
	m  = inb(addr);
	m |= (1 << nr);
	outb(addr, m);
	intr_enable();
}

void irq_set(int nr, void *p)
{
	if (nr > 15)
		panic("irq_set: invalid irq number");
	irq_vect[nr] = p;
}

void intr_irq(struct intr_regs *r, int nr)
{
	void (*p)(int nr);
	
	p = irq_vect[nr];
	p(nr);
	wakeup();
}

void dumpregs(struct intr_regs *r)
{
	printf("EAX    0x%x\n", r->eax);
	printf("EBX    0x%x\n", r->ebx);
	printf("ECX    0x%x\n", r->ecx);
	printf("EDX    0x%x\n", r->edx);
	printf("ESI    0x%x\n", r->esi);
	printf("EDI    0x%x\n", r->edi);
	printf("EBP    0x%x\n", r->ebp);
	printf("DS     0x%x\n", r->ds);
	printf("ES     0x%x\n", r->es);
	printf("FS     0x%x\n", r->fs);
	printf("GS     0x%x\n", r->gs);
	printf("EIP    0x%x\n", r->eip);
	printf("CS     0x%x\n", r->cs);
	printf("EFLAGS 0x%x\n", r->eflags);
	printf("ESP    0x%x\n", r->esp);
	printf("SS     0x%x\n", r->ss);
}

void exception(struct intr_regs *r, int nr)
{
	if (!(r->cs & 0x03))
	{
		printf("pid %i comm %s trap %i kern 0x%x\n", curr->pid, curr->comm, nr, r->eip);
		dumpregs(r);
		panic("exception");
	}
	
	printf("pid %i comm %s trap %i user 0x%x\n", curr->pid, curr->comm, nr, r->eip);
	dumpregs(r);
	
	switch (nr)
	{
	case 0:
		sendksig(curr, SIGFPE);
		break;
	case 6:
		sendksig(curr, SIGILL);
		break;
	case 1:
	case 3:
	case 4:
	case 5:
	case 17:
		sendksig(curr, SIGTRAP);
		break;
	case 7:
	case 8:
	case 13:
		sendksig(curr, SIGBUS);
		break;
	case 9:
	case 11:
	case 12:
	case 14:
		sendksig(curr, SIGSEGV);
		break;
	case 16:
		sendksig(curr, SIGFPE);
		break;
	case 2:
		panic("nmi");
	case 10:
		panic("invalid tss");
	case 15:
		panic("int 15");
	case 18:
		panic("mce");
	}
}

void intr_return(struct intr_regs *r)
{
restart:
	if (r->cs & 0x03)
	{
		if (curr->sig)
			pushsig(r);
		if (dosync)
		{
			dosync = 0;
			psync();
		}
		if (resched)
		{
			sched();
			goto restart;
		}
	}
}
