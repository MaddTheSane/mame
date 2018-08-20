#include <Carbon/Carbon.h>

extern "C"
{
	#include "mac.h"
	#include "macvideo.h"
}
#include "macinput.h"

#include <cstdarg>
#include <cstdio>

//-----------------------------------------------------------------------------
// printf
//-----------------------------------------------------------------------------
// This custom variant of printf overrides the standard library version in
// order to route all text to a Mac dialog.

extern "C" int printf(const char *format,...)
{
	int retVal = 0;
	CFStringRef errorString = NULL;
	CFStringRef detailString = NULL;
	char *buf = NULL;
	
	va_list args;
	va_start(args,format);
	size_t length = vsnprintf (NULL, 0, format, args);
	
	buf = new char[length + 1];
	__Require (buf, cantAllocate);

	vsprintf(buf, format, args);

	__Require ( errorString = CFCopyLocalizedString(CFSTR("ErrorText"), NULL), cantGetErrorString );
	__Require ( detailString = CFStringCreateWithCString (NULL, buf, kCFStringEncodingMacRoman), cantCreateDetailString );
	
	DialogRef dialog;
	DialogItemIndex itemIndex;
	
	ShowCursorAbsolute();
	if (!gEmulationPaused)
		DeactivateInputDevices(false);
	
	CreateStandardAlert (kAlertStopAlert, errorString, detailString, NULL, &dialog);
	RunStandardAlert (dialog, NULL, &itemIndex);
	
	if (!gEmulationPaused)
		ActivateInputDevices(false);

	retVal = length;

cantAllocate:
cantGetErrorString:
cantCreateDetailString:

	va_end (args);

	if (errorString) CFRelease (errorString);
	if (detailString) CFRelease (detailString);
	if (buf) delete[] buf;
		
	return retVal;
}
