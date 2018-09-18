/*##########################################################################

  MacMAME.h

  Common core header file. Intended to be part of a precompiled header.

##########################################################################*/

#ifndef MacMAME_h
#define MacMAME_h

#define kCreator	'eMuL'
#define kAppName	"MacMAME"

#define kComponentSignatureString	kAppName
#define COMPONENT_SIGNATURE			kCreator

// a useful speedup
#pragma once //on

#if defined(__GNUC__)
	#define MAC_XCODE 1
	//#include <ppc_intrinsics.h>
#endif

#define TARGET_RT_MAC_MACHO		1
#define TARGET_API_MAC_CARBON	1

#if MAC_XCODE
	#define __CF_USE_FRAMEWORK_INCLUDES__ 1
#else
	// We are using the traditional CW environment (Mach-O + MSL)
	#define _MSL_USING_MW_C_HEADERS 1
	#include "MSL MacHeadersMach-O.h"
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Including these in the pre-compiled header set is a nice speed-up
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include "OldCarbHeaders.h"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

#define kIOHIDUsageTables_h	<Kernel/IOKit/hidsystem/IOHIDUsageTables.h>
#if !defined(__MSL__)
	#define strnicmp strncasecmp
#else
	int strnicmp(const char *s1, const char *s2, size_t n);
#endif

#if !defined (_STDINT_H) && defined(_MSL_STDINT_H)
	// Hack around MSL's weak C99 support
	#define _STDINT_H
#endif

/*##########################################################################
	CORE MacMAME INCLUDES
##########################################################################*/

struct _osd_file
{
	FSRef		fileRef;
	SInt16		refNum;

	Boolean		isCached;
	Boolean		eof;
	void *		cachedData;
	UInt32		length;
	UInt32		offset;
};


#if TARGET_RT_MAC_CFM
	#define _asm_get_global(x,y)	lwz		x,y(RTOC);
	#define _asm_get_global_h(x,y)	lhz		x,y(RTOC);
	#define _asm_get_global_b(x,y)	lbz		x,y(RTOC);

	#define _asm_set_global(x,y)	stw		x,y(RTOC);
	#define _asm_set_global_h(x,y)	sth		x,y(RTOC);
	#define _asm_set_global_b(x,y)	stb		x,y(RTOC);

	#define _asm_get_global_ptr _asm_get_global
#elif TARGET_RT_MAC_MACHO
#if MAC_XCODE
	// XCode doesn't support rPIC for inline assembly, so this requires
	// you to build with dynamic-no-pic. Even so, the inline assembly
	// doesn't (yet?) support macro expansion or non-auto variables so
	// it's essentially broken. You can reference non-auto variables with
	// gas-style syntax, but it requires you to break up the inline blocks.
	#define _asm_get_global(x,y) 	lis		r2,ha16(y); \
								 	lwz		x,lo16(y)(r2);
	#define _asm_get_global_h(x,y) 	lis		r2,ha16(y); \
								 	lhz		x,lo16(y)(r2);
	#define _asm_get_global_b(x,y) 	lis		r2,ha16(y); \
								 	lbz		x,lo16(y)(r2);

	#define _asm_set_global(x,y) 	lis		r2,ha16(y); \
								 	stw		x,lo16(y)(r2);
	#define _asm_set_global_h(x,y) 	lis		r2,ha16(y); \
								 	sth		x,lo16(y)(r2);
	#define _asm_set_global_b(x,y) 	lis		r2,ha16(y); \
								 	stb		x,lo16(y)(r2);

	#define _asm_get_global_ptr(x,y) 	lis		x,ha16(y); \
								 		addi	x,x,lo16(y);
#else
	// this does not compile correctly as of Xcode 1.1
	#define _asm_get_global(x,y) 	addis	r2,RPIC,ha16(y); \
								 	lwz		x,lo16(y)(r2);
	#define _asm_get_global_h(x,y) 	addis	r2,RPIC,ha16(y); \
								 	lhz		x,lo16(y)(r2);
	#define _asm_get_global_b(x,y) 	addis	r2,RPIC,ha16(y); \
								 	lbz		x,lo16(y)(r2);

	#define _asm_set_global(x,y) 	addis	r2,RPIC,ha16(y); \
								 	stw		x,lo16(y)(r2);
	#define _asm_set_global_h(x,y) 	addis	r2,RPIC,ha16(y); \
								 	sth		x,lo16(y)(r2);
	#define _asm_set_global_b(x,y) 	addis	r2,RPIC,ha16(y); \
								 	stb		x,lo16(y)(r2);

	#define _asm_get_global_ptr(x,y) 	addis	x,RPIC,ha16(y); \
								 		addi	x,x,lo16(y);
#endif
#else
	#pragma error Unknown architecture!
#endif

/*##########################################################################
	CORE MAME INCLUDES
##########################################################################*/

#ifndef INLINE
#define INLINE static inline
#endif

#ifndef M_PI
#define M_PI 			(3.14159265358979323846L)
#endif
#define PI 				M_PI

// For fileio.c:
// 1 = CR only
// 2 = LF only (the OSX default)
// 3 = CR+LF
#define CRLF 2

#if __LP64__
#define PTR64 1
#endif

#include "Source/MacCoreDefines.h"
#include "osdcomm.h"
#include "mame.h"

#ifndef __ppc__
#include "MacMAME-ppcemu.h"
#endif

#endif
