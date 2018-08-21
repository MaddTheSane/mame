#ifndef __OSINLINE__
#define __OSINLINE__

/* What goes herein depends heavily on the OS. */

#include "mame.h"	// to work around conflicting definitions of osd_cycles

#define vec_mult(a,b) __mulhw(a,b)

/* profiler callback */
extern unsigned int (*gGetCyclesFunction)(void);
#define osd_cycles (*gGetCyclesFunction)


#endif /* __OSINLINE__ */
