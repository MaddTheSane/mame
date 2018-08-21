#ifndef mach_fixup_h
#define mach_fixup_h

#if TARGET_RT_MAC_MACHO
// /usr/include/ctype.h has some macros that conflict with some CPU
// accessor stuff.
#undef _A
#undef _B
#undef _C
#undef _D
#undef _L
#undef _I
#undef _R
#undef _S

// /usr/include/mach/clock_types.h defines this
#undef SYSTEM_CLOCK

// ppc appears to be a compiler constant when building for Mach-O.
// We have to undefine it each time for things to work right.
#undef ppc
#endif // TARGET_RT_MAC_MACHO

#endif // mach_fixup_h