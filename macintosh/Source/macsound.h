/*##########################################################################

	macsound.h

	Routines for interfacing between MAME and AsgardESS.

##########################################################################*/

#pragma once


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

Boolean 	HasSoundManager3(void);

void		InitializeSound(void);
void		TearDownSound(void);

void 		PauseSound(Boolean inPause);
void 		SyncSound(void);


/*##########################################################################
	INLINE FUNCTIONS
##########################################################################*/

static inline UInt32 GetMicroseconds(void)
{
#ifndef MAC_XCODE
	extern ComponentInstance 		gSoundClock;
	extern TimeRecord				gSoundTime;
	extern wide						gSoundClockConversion;
	
	// use the sound clock if we can
	if (gSoundClock != NULL)
	{
		UInt32 result;
		ClockGetTime(gSoundClock, &gSoundTime);
		
		// value is 64-bit (VH-VL); conversion is 32.32 (CH.CL)
		// multiply as follows:
		//
		//          VH     VL
		//      x   CH  .  CL
		//      ---------------
		//	      VH*CL   VL*CL
		// VH*CH  VL*CH
		// --------------------
		// VH*CH  (VH*CL+VL*CH) . VL*CL
		//
		// we're only interested in the middle 32-bits, so:
		result = __mulhwu(gSoundClockConversion.lo, gSoundTime.value.lo);
		result += (UInt32)gSoundClockConversion.lo * (UInt32)gSoundTime.value.hi;
		result += (UInt32)gSoundClockConversion.hi * (UInt32)gSoundTime.value.lo;
		return result;
	}
	
	// otherwise, fall back on microseconds
	else
#endif
	{
		UnsignedWide usecs;
		Microseconds(&usecs);
		return usecs.lo;
	}
}