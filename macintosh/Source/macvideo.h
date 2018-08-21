/*##########################################################################

  macvideo.c

  Mac-specific video handling.  Original code by Brad Oliver, Richard
  Bannister, John Stiles, and others.  Ripped out of mac.c and heavily
  cleaned up and tweaked by Aaron Giles.

##########################################################################*/

#pragma once

#include "videoapi.h"

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

// plugin setup
void		InitializePlugins(void);
UInt32		GetIndexedPlugin(UInt32 inIndex, StringPtr outShortName, StringPtr outLongName, Boolean *outEnabled);

// plugin getters & setters
void		SetActivePlugin(UInt32 inIdentifier);
UInt32 		GetActivePlugin(void);
const DisplayDescription *
			GetActivePluginDescription(void);

// plugin user interface
Boolean 	PluginHasConfigDialog(UInt32 inIdentifier);
void 		ConfigurePlugin(UInt32 inIdentifier);
Boolean 	PluginHasAboutBox(UInt32 inIdentifier);
void 		DisplayPluginAboutBox(UInt32 inIdentifier);

// absolute cursor showing/hiding
void 		ShowCursorAbsolute(void);
void 		HideCursorAbsolute(void);

// fix main window alignment
void		AlignMainWindow(Rect *boundsToFix);

// global menubar management
Boolean 	MenuBarVisible(void);
void 		ShowMenubar(void);
void 		HideMenubar(void);

// display state management; setting the display changed tells us to call WaitNextEvent
// for a second before hogging the machine again
void 		SetDisplayChanged(void);
void 		ComputeVectorSize(int *width, int *height);

// Carbon event handlers
void		InstallCarbonWindowEvents (WindowRef inWindow);
void		DisposeCarbonWindowEvents (void);

// screen update hook
void 		UpdateScreen(void);
void 		CopyScreenToGWorld(mame_bitmap *inBitmap, GWorldPtr inGWorld, DisplayParameters inDisplay);

// emergency shutdown hook
void 		EmergencyVideoShutdown(void);

// suspend/resume/pause hooks
void		SuspendVideo(void);
void		ResumeVideo(void);
void		PauseVideo(Boolean inPause);

// printing support
OSStatus	InitPrinting(void);
void 		PrintSetup(void);
void 		PrintScreen(void);

// copy/save snapshot support
void 		CopyToClipboard(void);
void 		SaveScreenshotAs(void);

// video state querying
Boolean 	SupportsFullScreen(Boolean inFullscreen);
Boolean 	SupportsThrottling(Boolean inThrottle);
Boolean 	SupportsAutoFrameskip(Boolean inAutoSkip);
Boolean 	SupportsFrameskip(UInt32 inFrameskip);
Boolean 	SupportsScaleAndInterlace(UInt32 inScale, Boolean inInterlace);

// video state setting
void 		SetFullScreen(Boolean inFullscreen);
void 		SetThrottling(Boolean inThrottle);
void 		SetAutoFrameskip(Boolean inAutoSkip);
void 		SetFrameskip(UInt32 inFrameskip);
void	 	SetScaleAndInterlace(UInt32 inScale, Boolean inInterlace);
void		FlushPortImmediate(GrafPtr inPort);



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// read-only access to the global display state
extern const DisplayParameters *gDisplayState;
extern int gFlipx;
extern int gFlipy;
extern int gSwapxy;
