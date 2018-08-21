#ifndef MacMAME_CW_h
#define MacMAME_CW_h

#pragma showmessagenumber on

// Disable warning about using variables before initializing them.
// Dangerous, I know, but MAME's address map and input port macros
// use a syntax style that explicitly disables this warning on gcc
// but doesn't work in CW. Allowing CW to generate these warnings
// will result in 20,000 or so and will slow down compile times
// dramatically.
#pragma warning off (10185)

#if __ide_target("Debug") || __ide_target("Debug (New)")
	#include "MacMAME_Debug.mch"
#elif __ide_target("Release")
	#include "MacMAME_Release.mch"
#else
	#pragma error Unknown target!
#endif

// We have to fix up a few #defines for everything to work right with the BSD headers and
// the MAME core.
#include "mach_fixup.h"

#endif