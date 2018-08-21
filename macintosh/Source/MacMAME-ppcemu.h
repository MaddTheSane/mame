//
//  MacMAME-ppcemu.h
//  MacMAME
//
//  Created by C.W. Betts on 8/19/18.
//

#ifndef MacMAME_ppcemu_h
#define MacMAME_ppcemu_h

// Code taken from https://opensource.apple.com/source/xnu/xnu-792.21.3/iokit/Kernel/IOHibernateRestoreKernel.c
static __inline__ unsigned int __cntlzw(unsigned int num)
{
	unsigned int result;
	__asm__ volatile(	"bsrl   %1, %0\n\t"
					 "cmovel %2, %0"
					 : "=r" (result)
					 : "rm" (num), "r" (63));
	return 31 ^ result;
}

// Code taken from https://gist.github.com/rygorous/1440600

static inline unsigned int ppcmask(unsigned int mb, unsigned int me)
{
	unsigned maskmb = ~0u >> mb;
	unsigned maskme = ~0u << (31 - me);
	return (mb <= me) ? maskmb & maskme : maskmb | maskme;
}

static inline unsigned int rotl32(unsigned int x, unsigned int amount)
{
	return (x << amount) | (x >> ((32 - amount) & 31));
}

static inline unsigned int __rlwinm(unsigned int rs, unsigned int sh, unsigned int mb, unsigned int me)
{
	return rotl32(rs, sh) & ppcmask(mb, me);
}

static inline unsigned int __rlwimi(unsigned int ra, unsigned int rs, unsigned int sh, unsigned int mb, unsigned int me)
{
	unsigned int mask = ppcmask(mb, me);
	return (ra & ~mask) | (rotl32(rs, sh) & mask);
}

#endif /* MacMAME_ppcemu_h */
