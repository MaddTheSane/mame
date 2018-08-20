/*##########################################################################

	mactext.h

	Routines for displaying text results in a dialog using WASTE.

##########################################################################*/

#pragma once


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

extern OSErr 		rPrintf(const char *s, ...);
extern OSErr 		CreateTextResults(ConstStr255Param inTitle);
extern void 		DisplayTextResults(ConstStr255Param inDefaultFileName);
extern void 		DisposeTextResults(void);
