#ifndef __OSINLINE__
#define __OSINLINE__

/* What goes herein depends heavily on the OS. */

#include "mame.h"	// to work around conflicting definitions of osd_cycles

#ifdef __ppc__

#define vec_mult(a,b) __mulhw(a,b)

#elif defined(__i386__)

#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	int result;
	__asm__ (
			 "movl  %1    , %0    ; "
			 "imull %2            ; "    /* do the multiply */
			 "movl  %%edx , %%eax ; "
			 :  "=&a" (result)           /* the result has to go in eax */
			 :  "mr" (x),                /* x and y can be regs or mem */
			 "mr" (y)
			 :  "%edx", "cc"            /* clobbers edx and flags */
			 );
	return result;
}

#elif defined(__x86_64__)
#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	int result;
	__asm__ (
			 "movl  %1    , %0    ; "
			 "imull %2            ; "    /* do the multiply */
			 "movl  %%edx , %%eax ; "
			 :  "=&a" (result)           /* the result has to go in eax */
			 :  "mr" (x),                /* x and y can be regs or mem */
			 "mr" (y)
			 :  "%edx", "cc"            /* clobbers edx and flags */
			 );
	return result;
}

#else
#error unknown arch!
#endif

/* profiler callback */
extern unsigned int (*gGetCyclesFunction)(void);
#define osd_cycles (*gGetCyclesFunction)


#endif /* __OSINLINE__ */
