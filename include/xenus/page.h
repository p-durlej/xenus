/* Copyright (c) Piotr Durlej
 * All rights reserved
 */

#define PDIR_BASE 0x00001000
#define KSTK_BASE 0x00008000
#define ITAB_BASE 0x00100000
#define USER_BASE 0x04000000
#define USER_SIZE 0x00400000
#define STXT_OFF  0x00200000
#define PTAB_BASE 0x04800000
#define HEAP_BASE 0x04c00000
#define HEAP_SIZE 0x00400000

#ifndef __ASSEMBLER__

#define pg_dir ((unsigned *)PDIR_BASE)
#define pg_tab ((unsigned *)PTAB_BASE)

extern unsigned pg_flist[];
extern unsigned pg_fcount;

void pg_update(void);
unsigned cr2(void);

int pg_alloc(unsigned *pg);
void pg_free(unsigned pg);

void *zpalloc(void);
void *palloc(void);
void pfree(void *p);
int pccopy(unsigned *ntab);

#endif
