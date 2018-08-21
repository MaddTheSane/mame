/*##########################################################################

	MacInput.h

	Routines for handling keyboard, joystick and mouse input with MacMAME.

##########################################################################*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "osdepend.h"

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

void			MacMAME_InitInput (void);
void			MacMAME_ShutdownInput (void);

void 			ActivateInputDevices(Boolean fullPause);
void 			DeactivateInputDevices(Boolean fullPause);

void			UpdateLEDs(int inFlags);

void			PollInputs(void);

extern bool gAnalogInputActive;
extern bool gInBackground;
extern bool gExitToFrontend, gExitToShell;
extern bool gPauseEmulationASAP;

#ifdef __cplusplus
}
#endif
