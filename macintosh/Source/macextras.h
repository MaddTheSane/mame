/*##########################################################################

	macextras.c

	Miscellanous Mac-only features.

##########################################################################*/

#pragma once


/*##########################################################################
	PROTOTYPES
##########################################################################*/

void 		DisplayWarningDialog(void);
OSErr 		ShowSplashMovie(void);

void 		PushPopRGB(Boolean inPop);
void 		PushPopPenState(Boolean inPop);


/*##########################################################################
	MACROS
##########################################################################*/
#define PushRGB()			PushPopRGB(false)
#define PopRGB()			PushPopRGB(true)
#define PushPenState()		PushPopPenState(false)
#define PopPenState()		PushPopPenState(true)
