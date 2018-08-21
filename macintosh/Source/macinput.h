/*##########################################################################

	MacInput.h

	Routines for handling keyboard, joystick and mouse input with MacMAME.

##########################################################################*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "osdepend.h"

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

void			MacMAME_InitInput (void);
void			MacMAME_ShutdownInput (void);
void			InitializeKeyboardEvents (WindowRef inWindow);

void 			ActivateInputDevices(Boolean fullPause);
void 			DeactivateInputDevices(Boolean fullPause);

void			UpdateLEDs(int inFlags);

void			PollInputs(void);

#ifdef __cplusplus
}
#endif
