/*##########################################################################

	software.h

	Implementation of the software-only display system.

##########################################################################*/

#pragma once


Boolean Software_GetDisplayDescription(DisplayDescription *ioDescription);

Boolean UpdateDisplayWithClip(const DisplayParameters *inParams, RgnHandle clipRgn);
