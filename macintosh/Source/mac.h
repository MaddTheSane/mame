/*##########################################################################

	mac.h

	Primary Mac-specific header for MacMAME/MacMESS.
	Written by Brad Oliver, John Butler, and Aaron Giles.

##########################################################################*/

#pragma once

#include "MacAppleEvents.h"
#include "MacMenus.h"
#include "MacPrefs.h"
#include "MacFiles.h"
#include "maclists.h"


/*##########################################################################
	MACROS
##########################################################################*/

/*##########################################################################
	CONSTANTS
##########################################################################*/

// dialog defines
enum
{
#ifdef MESS
	rAboutDialog			= 512,
#else
	rAboutDialog			= 128,
#endif
		rAboutVersionItem	= 3,
	rNewChooseDialog		= 132,
	rOptionsDialog			= 133,
};


/*##########################################################################
	MACROS
##########################################################################*/

// these macros prevent us from going direct to the OS, so we can track the cursor state
#define ShowCursor (Use ShowCursorAbsolute!)
#define HideCursor (Use HideCursorAbsolute!)

#define kWindowToFront ((WindowRef)-1L)


/*##########################################################################
	TYPEDEFS
##########################################################################*/

// these represent the states of the recording file
typedef enum
{
	kRecordingNone,
	kRecordingInput,
	kRecordingOutput
} RecordingState;


// these represent the state of the window scale variable
typedef enum
{
	kWindowScaleNormal,
	kWindowScaleAdjusted,
	kWindowScaleVector
} WindowScaleState;


// these represent the state of the menus
typedef enum
{
	kMenuStateRunning,
	kMenuStateModal
#ifdef MESS
	,
	kMenuStateStopped
#endif
} MenuState;


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

// Startup/Shutdown Routines
void			EmergencyShutdown(void);

// OS Routines
void 			ProcessEvents(Boolean inCallWaitNextEvent);
void 			HandleUpdate(WindowRef inWindow, const EventRecord *inEvent);

// Misc UI Routines
void 			ErrorAlert(CFStringRef inString, Boolean inQuit);

// Misc MacOS Utility Routines
char *			GetIndCString(short inResource, short inIndex, const char *inDefault);
StringPtr 		GetIndPString(short inResource, short inIndex, const unsigned char *inDefault);
void         	PauseMacMAME (Boolean pause);

// Generic Modal Filter
pascal Boolean GenericModalFilter(DialogRef inDialog, EventRecord *inEvent, short *inItemHit);



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// application states
extern Boolean			gInBackground;			// true if we're in the background
extern Boolean			gDragAndDropMode;		// true if we were started via drag & drop
extern Boolean			gEmulationPaused;		// true if the emulator is paused
extern Boolean			gAnalogInputActive;		// true if analog input is being read
extern MenuState		gMenuState;				// state of the menubar
extern EventRecord		gLastEvent;				// last event we processed
#ifdef MESS
extern Boolean			gUseCoreCommandMenu;	// use the core command menu (modeless mode for MESS computers)
#endif

// application flow control
extern Boolean			gExitToShell;			// true if we should exit on next event loop
extern Boolean			gExitToFrontend;		// true if we should exit to the frontend
extern Boolean			gPauseEmulationASAP;	// true if we should pause as soon as we can

// gestalt-type information
extern Boolean			gHasAltivec;			// true if the CPU supports the Altivec instruction set

// windows and such
extern CursHandle		gWatchCursor;			// handle to the watch cursor
extern ModalFilterUPP	gGenericModalFilter;	// global generic filter

// statically initialized options
extern CFURLRef			gRecordingFileURL;		// CFURL of the recording file, if any
extern RecordingState 	gRecordingState;		// state of the recording
extern CFURLRef			gLangURL;				// CFURL of the language file, if any
extern CFURLRef			gCheatURL;				// CFURL of the cheat file, if any

// current ROM set or system
extern ROMSetData		gActiveRomset;			// the active ROMset


extern SInt32			gBIOSSelection;		// TODO: consider adding to prefs structure
