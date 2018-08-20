/*##########################################################################

	mac.c

	Primary Mac-specific system file for MacMAME/MacMESS.
	Written by Brad Oliver, John Butler, and Aaron Giles.

##########################################################################*/

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <mach/mach_time.h>

#include "driver.h"
#include "info.h"

#include "mac.h"
#include "macextras.h"
#include "macinput.h"
#include "macvideo.h"
#include "macstrings.h"
#include "macfrontend.h"
#include "macreports.h"
#include "macsound.h"
#include "macutils.h"
#ifndef NEW_DEBUGGER
#include "macdebug.h"	// R. Nabet 001212
#else
#include "macdebugwin.h"
#endif

#ifdef MESS
// R. Nabet 000601
#include "macmessbase.h"
#include "macmesscassette.h"
#include "macmessfrontend.h"
#include "mac_imgtool.h"

#endif

#ifdef __MWERKS__
#pragma require_prototypes on
#endif


/*##########################################################################
	MACROS
##########################################################################*/

#define SetMenuItemEnable(m,i,e) ((e) ? EnableMenuItem(m,i) : DisableMenuItem(m,i))


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// application states
Boolean			gInBackground;			// true if we're in the background
Boolean			gDragAndDropMode;		// true if we were started via drag & drop
Boolean			gEmulationPaused;		// true if the emulator is paused
Boolean			gAnalogInputActive;		// true if analog input is being read
MenuState		gMenuState;				// state of the menubar
EventRecord		gLastEvent;				// last event we processed
#ifdef MESS
Boolean			gUseCoreCommandMenu;	// use the core command menu (modeless mode for MESS computers)
#endif

// application flow control
Boolean			gExitToShell;			// true if we should exit on next event loop
Boolean			gExitToFrontend;		// true if we should exit to the frontend
Boolean			gPauseEmulationASAP;	// true if we should pause as soon as we can
#ifdef MESS
Boolean			gOpenNewGame;			// true if we should open a new game
#endif

// gestalt-type information
Boolean			gHasAltivec;			// true if the CPU supports the Altivec instruction set

// windows and such
CursHandle		gWatchCursor;			// handle to the watch cursor
ModalFilterUPP	gGenericModalFilter;	// global generic filter

// statically initialized options
CFURLRef		gRecordingFileURL;		// CFURL of the recording file, if any
RecordingState 	gRecordingState;		// state of the recording
CFURLRef		gLangURL;				// CFURL of the language file, if any
CFURLRef		gCheatURL;				// CFURL of the cheat file, if any

// profiler
unsigned int 	(*gGetCyclesFunction)(void);// callback

// current ROM set or system
ROMSetData		gActiveRomset;			// the active ROMset

SInt32			gBIOSSelection;

// local variables
static Boolean	sPausedOnSuspend;		// true if we paused as a result of a suspend event
static Boolean	sShutdownCleanly;		// true if we shut down the application cleanly
static FILE *	sErrorlogFile;			// errorlog file

// real-time clock
static cycles_t	sCyclesPerSecond;

#if MAC_XCODE && !MAME_DEBUG
// Needed for some sloppy disassemblers, plus the fact that the deployment
// xcode target doesn't dead-strip code yet.
unsigned char *Buffer;
#endif

/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

// Startup/Shutdown Routines
static void 		InitializeToolbox(void);
static Boolean 		VerifySystem(void);
static void 		InitializeSystem(void);
static void 		InitializeGlobals(void);
static void 		InitializeOptions(const game_driver *inDriver);
static void 		ShutdownSystem(void);

// Profiler routines
static void 		InitCycleTimer(void);

// OS Routines
static int 			ChooseRom(int *outGame);
static void	 		HandleMouseDown(const EventRecord *theEvent);
static void 		HandleOSEvent(EventRecord *inEvent);

// Menu Handling Routines
static void			InitializeMenuBar (void);
void				ProcessGameCommand (HICommand inCommand);
void				UpdateGameCommand (HICommand inCommand);

// Misc UI Routines
void 	         	PauseMacMAME (Boolean pause);
static void 		DisplayAboutBox(void);
pascal              Boolean GenericModalFilter(DialogRef inDialog, EventRecord *inEvent, short *inItemHit);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark е Startup/Shutdown Routines

#pragma CALL_ON_UNLOAD EmergencyShutdown

//===============================================================================
//	main
//
//	The main entry point for the application.
//===============================================================================

int main (void)
{
	// initialize the Mac toolbox first
	InitializeToolbox();
	
	// verify that we can run first
	if (!VerifySystem())
		exit(1);
	
	// initialize the system
	InitializeSystem();
	
	// The front-end needs the cpuintrf structure set up to query hardware info
	cpuintrf_init();
	sndintrf_init();

	// display a startup movie if found
	(void)ShowSplashMovie ();
	
	// display the warning dialog before entering the main loop
	DisplayWarningDialog();

	// loop until it's time to go
	while (!gExitToShell)
	{
		int game;
	
		// go to the modal menu state
		gMenuState = kMenuStateModal;
//		UpdateMenus();

		// Turn off all keyboard LEDs
		UpdateLEDs (0);
		
		// pick a ROM
		if (!ChooseRom(&game))
		{
#ifdef MESS
			// always enable core command menu for now
			gUseCoreCommandMenu = true/*(drivers[game]->flags & GAME_COMPUTER) != 0*/;
#endif
			// fill in the global options record
			InitializeOptions(drivers[game]);
			
			// set the cursor to a watch; osd_create_display sets it back
			if (gWatchCursor)
				SetCursor(*gWatchCursor);

#if __MWERKS__
extern Boolean usingAsmCores;
extern Boolean using6800AsmCores;
			if (
				// еее LBO 11/14/04. A nasty hack to revert to the C 68k cores.
				// The callbacks that the System16 driver uses aren't yet working
				// with the PPC cores.
				(strcmp (drivers[game]->source_file, "segas16a.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segas16b.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segas18.c") == 0) ||
				(strcmp (drivers[game]->source_file, "system16.c") == 0) ||
				(strcmp (drivers[game]->source_file, "system18.c") == 0) ||
				(strcmp (drivers[game]->source_file, "system24.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segaxbd.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segaybd.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segahang.c") == 0) ||
				(strcmp (drivers[game]->source_file, "segaorun.c") == 0) ||
				
				// LBO 5/1/05. The arcadia (Amiga) driver won't get out of
				// the BIOS tests unless we use the C core.
				(strcmp (drivers[game]->source_file, "arcadia.c") == 0) ||

				// LBO 4/30/05. Chimera Beast (prototype) also does not work with
				// the PPC cores.
				(strcmp (drivers[game]->source_file, "megasys1.c") == 0)			
			   )
			{
				usingAsmCores = false;
			}
			else
				usingAsmCores = true;

			if (
				// LBO 5/8/05. Qix lost sound with the move to Mach-O. Investigate.
				(strcmp (drivers[game]->source_file, "qix.c") == 0) ||
				// LBO 12/30/05. Baraduke has no sound and Metro Cross doesn't work after coin-up.
				(strcmp (drivers[game]->source_file, "baraduke.c") == 0) ||
				// Pac-Mania has funky sound with the PPC core.
				(strcmp (drivers[game]->source_file, "namcos1.c") == 0)			
			   )
			{
				using6800AsmCores = false;
			}
			else
				using6800AsmCores = true;
#endif

			// if run_game() returns nonzero, something failed to startup
			// in this case, we'll pick a new ROM
			if (run_game(game))
				gExitToFrontend = true;

			// close any recording or playback files
			if (options.record)
				mame_fclose(options.record);
			if (options.playback)
				mame_fclose(options.playback);
		}
	}
	
	// bring it all to a screeching halt
	ShutdownSystem();
	
	return 0;
}

//===============================================================================
//	InitializeToolbox
//
//	Initializes the Mac toolbox.
//===============================================================================

void InitializeToolbox(void)
{
	MoreMasterPointers(64UL * 11);

	// initialize the toolbox
	InitCursor();
	InitializeAppleEvents();
	
	// create any global UPPs
	gGenericModalFilter = NewModalFilterUPP(GenericModalFilter);
	
	// select the profiler callback
	InitCycleTimer();
	
	RenameOldMacMAMEFolder ();
}

	
//===============================================================================
//	VerifySystem
//
//	Checks lowest-level requirements before anything else starts up.
//===============================================================================

Boolean VerifySystem(void)
{
	long result;
	OSErr err;
	
	// We require 10.2 or greater
	err = Gestalt(gestaltSystemVersion, &result);
	if (result < 0x1020)
	{
		ErrorAlert (CFSTR("MinOSError"), false);
		return false;
	}

	gHasAltivec = TestGestaltBit(gestaltPowerPCProcessorFeatures, gestaltPowerPCHasVectorInstructions);
	
	// we passed!
	return true;
}


//===============================================================================
//	InitializeSystem
//
//	Loads the preferences and initializes the rest of the system.
//===============================================================================

void InitializeSystem(void)
{
	UInt32 timeoutTicks;
	
	// load the preferences
	LoadPrefs();

	// initialize the globals
	InitializeGlobals();

	// bring up the sound system, keyboard, and other input devices
	InitializeSound();
	MacMAME_InitInput();

	// scan for and initialize plugins
	InitializePlugins();

	// fetch the menubar and add the Apple menu items
	InitializeMenuBar ();
	
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	debugwin_init_windows();
#endif

	// wait for the first AppleEvent to come through, but time out after 1/2 second
	timeoutTicks = TickCount() + 30;
	while (!WaitNextEvent(highLevelEventMask, &gLastEvent, 0x7fffffff, NULL) && TickCount() < timeoutTicks) ;
	AEProcessAppleEvent(&gLastEvent);

#ifdef MESS
	MESSInitSystem();
#endif
}


//===============================================================================
//	InitializeGlobals
//
//	Initializes the global variables.
//===============================================================================

void InitializeGlobals(void)
{
	// application states
	gInBackground = false;
	gDragAndDropMode = false;
	gEmulationPaused = true;
	gAnalogInputActive = false;
#ifdef MESS
	gMenuState = kMenuStateStopped;
#else
	gMenuState = kMenuStateModal;
#endif

	// application flow control
	gExitToShell = false;
	gExitToFrontend = false;
	gPauseEmulationASAP = false;

	// windows and such
	gWatchCursor = GetCursor(watchCursor);
	if (gWatchCursor)
		HLockHi((Handle)gWatchCursor);

	// statically initialized options
	gRecordingState = kRecordingNone;
	gLangURL = NULL;
	gCheatURL = NULL;

	// current ROM set or system
	gActiveRomset.driver = NULL;

	// local variables
	sPausedOnSuspend = false;
	sShutdownCleanly = false;
}


//===============================================================================
//	InitializeOptions
//
//	Fills in the global options record with the current state of the preferences.
//===============================================================================

void InitializeOptions(const game_driver *inDriver)
{
	machine_config machine;
	Boolean success;
	char fileName[kMacMaxPath];
	extern const char *cheatfile; // Defined in cheat.c

	expand_machine_driver(inDriver->drv, &machine);

	// reset all the options
	memset(&options, 0, sizeof(options));

	// handle generation of a new recording
	if (gRecordingState == kRecordingOutput)
	{
		inp_header inp_header;

		memset(&inp_header, '\0', sizeof(inp_header));
		strcpy(inp_header.name, inDriver->name);

		success = CFURLGetFileSystemRepresentation (gRecordingFileURL, true, (UInt8 *)fileName, sizeof(fileName));
		options.record = mame_fopen(inDriver->name, fileName, FILETYPE_INPUTLOG, 1);

		mame_fwrite(options.record, &inp_header, sizeof(inp_header));
	}

	// handle reading of an existing recording
	if (gRecordingState == kRecordingInput)
	{
		inp_header inp_header;

		// open the file; we've ensured that it exists
		success = CFURLGetFileSystemRepresentation (gRecordingFileURL, true, (UInt8 *)fileName, sizeof(fileName));
		options.playback = mame_fopen(inDriver->name, fileName, FILETYPE_INPUTLOG, 0);

		// read playback header
		mame_fread(options.playback, &inp_header, sizeof(inp_header));

		// determine which version of the header we have and back up if there is no header
		if (!isalnum(inp_header.name[0]))
    		mame_fseek(options.playback, 0, SEEK_SET);
    	else
    	{
    		// TODO: add warning if inp gamename doesn't match chosen game
    	}
	}

	if (gLangURL)
	{
		success = CFURLGetFileSystemRepresentation (gLangURL, true, (UInt8 *)fileName, sizeof(fileName));
	}
	else
		strcpy (fileName, "default.lng");
	
	options.language_file = mame_fopen(NULL, fileName, FILETYPE_LANGUAGE, 0);

	if (gCheatURL)
	{
		static char cheatFileName[kMacMaxPath];
		success = CFURLGetFileSystemRepresentation (gCheatURL, true, (UInt8 *)cheatFileName, sizeof(cheatFileName));
		cheatfile = cheatFileName;
	}
	else
		cheatfile = nil;				// cheat.c resets this to "cheat.dat" if nil
	
#ifdef MESS
		InitializeMacMESSOptions(inDriver);
#endif

	// set the debug flag
#ifdef MAME_DEBUG
	options.mame_debug 	= true;
//	options.mame_debug 	= false;
	options.debug_width = 640;
	options.debug_height = 480;
	options.debug_depth = 0;
#else
	options.mame_debug 	= false;
#endif
	options.cheat 			= gPrefs.cheat;
	options.skip_gameinfo	= gPrefs.noGameInfo;
	options.skip_disclaimer	= gPrefs.noDisclaimer;
	options.gui_host 		= true;

	// set the sound options
	options.samplerate 		= gPrefs.playSound ? gPrefs.sampleRate : 0;
	options.use_samples 	= true;

	// set the video options
	options.brightness		= 1.0;
	options.pause_bright	= .7;
	options.gamma			= 1.0;
	
	{
		// first start with the game's built in orientation
		int orientation = inDriver->flags & ORIENTATION_MASK;
		options.ui_orientation = orientation;

		if (options.ui_orientation & ORIENTATION_SWAP_XY)
		{
			// if only one of the components is inverted, switch them
			if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
					(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
				options.ui_orientation ^= ROT180;
		}

		if (! gPrefs.standardRotation)
		{
			// override if no rotation requested
			if (gPrefs.noRotation)
				orientation = options.ui_orientation = ROT0;
			
			// rotate right
			if (gPrefs.rotateRight)
			{
				// if only one of the components is inverted, switch them
				/*if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
						(orientation & ROT180) == ORIENTATION_FLIP_Y)
					orientation ^= ROT180;*/

				orientation ^= ROT90;
			}

			/* rotate left */
			if (gPrefs.rotateLeft)
			{
				// if only one of the components is inverted, switch them
				if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
						(orientation & ROT180) == ORIENTATION_FLIP_Y)
					orientation ^= ROT180;

				orientation ^= ROT270;
			}
			
			/* flip X/Y */
			if (gPrefs.flipX)
				orientation ^= ORIENTATION_FLIP_X;
			if (gPrefs.flipY)
				orientation ^= ORIENTATION_FLIP_Y;
		}

		gFlipx = ((orientation & ORIENTATION_FLIP_X) != 0);
		gFlipy = ((orientation & ORIENTATION_FLIP_Y) != 0);
		gSwapxy = ((orientation & ORIENTATION_SWAP_XY) != 0);
	}

	if (machine.video_attributes & VIDEO_TYPE_VECTOR)
		ComputeVectorSize(&options.vector_width, &options.vector_height);


	options.beam 			= gPrefs.beamWidth << 8; // convert from 8.8 fixed point to 16.16 fixed point
	options.vector_flicker 	= gPrefs.flicker;
	options.vector_intensity= 1.0;//gPrefs.intensity;
	options.translucency 	= gPrefs.translucency;
	options.antialias 		= gPrefs.antialias;
	options.use_artwork		= ARTWORK_USE_NONE;
	if (gPrefs.artworkAll)
		options.use_artwork 	= ARTWORK_USE_ALL;
	else
	{
		if (gPrefs.artworkBackdrop)
			options.use_artwork 	= ARTWORK_USE_BACKDROPS;
		if (gPrefs.artworkOverlay)
			options.use_artwork 	|= ARTWORK_USE_OVERLAYS;
		if (gPrefs.artworkBezel)
			options.use_artwork 	|= ARTWORK_USE_BEZELS;
	}

//options.artwork_res = 2;
//options.artwork_crop = 1;

	if (inDriver->bios && (gBIOSSelection > 0))
	{
		static char biosString[4];
		
		sprintf (biosString, "%ld", gBIOSSelection - 1);
		options.bios = biosString;
	}
	else
		options.bios = NULL;
}


//===============================================================================
//	ShutdownSystem
//
//	Tears down the system and saves the preferences.
//===============================================================================

void ShutdownSystem(void)
{
	// tear down the input devices, keyboard, and sound
	MacMAME_ShutdownInput();
	TearDownSound();
	#ifdef MESS
		MESSShutdownSystem(false);
	#endif
	sShutdownCleanly = true;

	// write the preferences out
	SavePrefs();
}


//===============================================================================
//	EmergencyShutdown
//
//	This routine is always called, even in the event of a crash.
//===============================================================================

extern void EmergencyShutdown(void)
{
	if (!sShutdownCleanly)
	{
		// make sure the video system is restored
		EmergencyVideoShutdown();
	
		// tear down the input devices, keyboard, and sound
		MacMAME_ShutdownInput();
		TearDownSound();
		#ifdef MESS
			MESSShutdownSystem(true);
		#endif
		sShutdownCleanly = true;
	}
}


#pragma mark -
#pragma mark е Profiler Routines

//===============================================================================
//	InitCycleTimer
//
//	Sets up our cycle timer for use later on.
//===============================================================================

static void InitCycleTimer(void)
{
	// Determine the number of cycles in a nanosecond
	mach_timebase_info_data_t timebase; 
	mach_timebase_info (&timebase);

	// Convert "cycles per nanosecond" to "seconds per cycle"
	sCyclesPerSecond = 1.0 / ((double) timebase.numer / (double) timebase.denom * 0.000000001); 
}

//============================================================
//	osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return mach_absolute_time();
}

//============================================================
//	osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return sCyclesPerSecond;
}

//============================================================
//	osd_profiling_ticks
//============================================================

cycles_t osd_profiling_ticks(void)
{
	return mach_absolute_time();
}



#pragma mark -
#pragma mark е MAME System Routines

//===============================================================================
//	osd_init
//
//	Called early on in the startup process.
//===============================================================================

int osd_init(void)
{
	// reset the globals
	gExitToFrontend = false;
	gEmulationPaused = false;
	gAnalogInputActive = false;

	return 0;
}


//===============================================================================
//	osd_pause
//
//	Called when the pause key is toggled.
//===============================================================================

void osd_pause(int inPaused)
{
	PauseMacMAME(inPaused);
}


//===============================================================================
//	osd_exit
//
//	Called after everything else is shutdown.
//===============================================================================

void osd_exit(void)
{
	// turn off the sprockets
	DeactivateInputDevices(true);
	{
		Cursor arrow;
		SetCursor(GetQDGlobalsArrow(&arrow));
	}

	// make sure the menubar and cursor are visible again
	ShowMenubar();
	ShowCursorAbsolute();
	FlushEvents(keyDown | keyUp | autoKey | mouseDown | mouseUp, 0);

	// reset the globals
	gExitToFrontend = false;
	gEmulationPaused = true;
	gAnalogInputActive = false;
}

//===============================================================================
//	osd_die
//
//	Something bad has happened, and we're terminating abnormally.
//===============================================================================

void osd_die(const char *format,...)
{
	char outTxt[2048];
	va_list args;

	va_start (args, format);
	vsprintf (outTxt, format, args);
	va_end (args);

	printf ("%s", outTxt);
	exit(-1);
}

//============================================================
//  osd_is_bad_read_ptr
//============================================================

int osd_is_bad_read_ptr(const void *ptr, size_t size)
{
	return 0;
}


//===============================================================================
//	logerror
//
//	Error logging.
//===============================================================================

void CLIB_DECL logerror(const char *text,...)
{
#if MAME_DEBUG
	va_list arg;
	va_start(arg,text);
	if (gPrefs.errorLog)
	{
		if (!sErrorlogFile)
		{
			char path[kMacMaxPath];
			
			GetCStringForMAMEFolder (MAC_FILETYPE_REPORTS, path);
			strlcat (path, "/errorlog.txt", sizeof(path));
			
			sErrorlogFile = fopen(path, "w");
		}
		if (sErrorlogFile)
			vfprintf(sErrorlogFile, text, arg);
	}

	va_end(arg);
#endif
}


#pragma mark -
#pragma mark е OS Routines

//===============================================================================
//	ChooseRom
//
//	Determines which ROMset the user wishes to run.
//	This is the MAME version of this routine
// Raphael Nabet 000120 : merged the 2 variants, since only 2 lines are MESS-specific
//===============================================================================

int ChooseRom(int *outGame)
{
	// if we're supposed to quit *all the way* out, bail right here
	if (gExitToShell)
		return 1;
	
	// clear the "exit to frontend" flag; this is it!
	gExitToFrontend = false;

	// if we were a drop target, assume everything was setup and clear the flag
	if (gDragAndDropMode)
	{
		gDragAndDropMode = false;
	}

#if 1 // FRONTEND ееееее
	// otherwise, display the fancy-schmancy frontend dialog
	else
	{
#ifdef MESS
		// trick : use MESS "SYSINFO.DAT"
		extern char *history_filename;

		history_filename = "SYSINFO.DAT";
#endif

		// set up the menus
		gMenuState = kMenuStateModal;
//		UpdateMenus();
	
		// bail if we've been canned
		if (!ChooseGame(&gActiveRomset))
		{
#ifndef MESS
			gExitToShell = true;
#endif
			return 1;
		}
	}
	
	// the index of the chosen ROMset is the game
	*outGame = GetDriverIndex(gActiveRomset.driver);
#endif
	
	// if it's bad, though, display an alert
	if (*outGame == -1)
	{
		printf("game \"%s\" not supported\n", gActiveRomset.driver->description);
		return 1;
	}

	return 0;
}


//===============================================================================
//	ProcessEvents
//
//	This is the main event handler for the system.
//===============================================================================

void ProcessEvents(Boolean inCallWaitNextEvent)
{
#if 0
#elif 0
EventTargetRef eventTarget = GetEventDispatcherTarget();
EventRef eventRef;
OSStatus err = ReceiveNextEvent (0, NULL, kEventDurationNoWait, true, &eventRef);
if (err == noErr)
{
	SendEventToEventTarget (eventRef, eventTarget);
	ReleaseEvent (eventRef);
}
#else
	Boolean gotEvent;
	
	// loop at least once; continue looping if we're in the background
	do
	{
		Boolean paused = gEmulationPaused || gInBackground || gDebuggerFocus;	// R. Nabet 001212
		Boolean callWNE = paused || inCallWaitNextEvent;

		// make sure the cursor is in the state we want it
		if (!paused && (gAnalogInputActive || gDisplayState->fullscreen))
			HideCursorAbsolute ();
		else
			ShowCursorAbsolute ();

		// if we can hog the CPU, do it
		if (!callWNE)
		{
			gotEvent = GetNextEvent(everyEvent, &gLastEvent);
			if (!gotEvent)
				CheckUpdate(&gLastEvent);
		}
		
		// otherwise, call WaitNextEvent and pay the consequences
		else
			gotEvent = WaitNextEvent(everyEvent, &gLastEvent, 1, NULL);

		// switch off the event
		switch (gLastEvent.what)
		{
			// mouse down: only process if the mouse is visible
			case mouseDown:
				if (paused || !gAnalogInputActive)
					HandleMouseDown(&gLastEvent);
				break;

			// update events: make sure the window handles its update
			case updateEvt:
				HandleUpdate((WindowRef)gLastEvent.message, & gLastEvent);
				break;
				
#ifdef MESS
			// activate events : MESS allows to open multiple windows
			case activateEvt:
				if (IsMacMESSWindow((WindowRef)gLastEvent.message))
					ActivateMacMESSWindow((WindowRef)gLastEvent.message, &gLastEvent);
				break;
#endif

			// OS events: generally suspend/resume
			case osEvt:
				HandleOSEvent(&gLastEvent);
				break;
			
			// high level events; let the AppleEvent Manager do the work
			case kHighLevelEvent:
				AEProcessAppleEvent(&gLastEvent);
				break;
		}
	}
	while (gInBackground || gotEvent);
#endif
}


//===============================================================================
//	HandleMouseDown
//
//	Processes mouse down events.
//===============================================================================

void HandleMouseDown(const EventRecord *inEvent)
{
	WindowRef window;
	short part = FindWindow(inEvent->where, &window);

	// quick exits if we're actively running the emulator
	if ((!gEmulationPaused) && (!gDebuggerFocus))	// R. Nabet 001212
		if (gAnalogInputActive || (part != inMenuBar && part != inDrag))
			return;

	// pause sound while we track the mouse down	
	PauseSound(true);
	switch (part)
	{
		// menu bar click
		case inMenuBar:
			MenuSelect (inEvent->where);
			break;

		// drag region click
		case inDrag:
		{
			Rect bounds;
			Rect finalBounds;
			OSStatus err;

			// let the system track the drag
			DragWindow(window, inEvent->where, GetRegionBounds(GetGrayRgn(), &bounds));


			if ( (window == gDisplayState->window)
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
				|| (window == gDebugDisplay.window)
#endif
			   )
			{
				// afterwards, align the window on a long-word boundary
				// KLUDGE : we get a null rect if the window is collapsed, so we un-collapse it
				// temporarily
				int collapsed;

				collapsed = IsWindowCollapsed(window);

				if (collapsed)
					CollapseWindow(window, false);

				err = GetWindowBounds(window, kWindowContentRgn, &finalBounds);
				if ((noErr == err) && finalBounds.top && finalBounds.left && finalBounds.bottom && finalBounds.right)
				{
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
					if (window == gDebugDisplay.window)
						finalBounds.left &= ~7;
			     	else
#endif
						AlignMainWindow(&finalBounds);
					MoveWindow(window, finalBounds.left, finalBounds.top, true);
				}

				if (collapsed)
					CollapseWindow(window, true);
			}

			// some extra work for moving the full window
			if (window == gDisplayState->window)
			{
				Rect		bounds;

				// force a full update
				InvalWindowRect(gDisplayState->window, GetWindowPortBounds(gDisplayState->window, &bounds));

				// set the new window location
				gPrefs.windowLocation.h = finalBounds.left;
				gPrefs.windowLocation.v = finalBounds.top;
			}
			
			// give other apps time to update after a move
			SetDisplayChanged();
			break;

#ifdef MESS
		// close box click
		case inGoAway:
			if (TrackGoAway(window, inEvent->where))
				if (IsMacMESSWindow(window))
				{
					CloseMacMESSWindow(window);
				}
			break;

		// window contents click
		case inContent:
			if (window != FrontWindow())
			{
				SelectWindow(window);
			}
			else if (IsMacMESSWindow(window))
			{
				HandleMacMESSWindowClick(window, inEvent);
			}
			break;

		// window size box click
		case inGrow:
			if (IsMacMESSWindow(window))
			{
				const Rect limitRect = { 64, 64, 32767, 32767 };
				long newSize;

				if ((newSize = GrowWindow(window, inEvent->where, &limitRect)) != 0)
				{
					ResizeMacMESSWindow(window, LoWord(newSize), HiWord(newSize));
				}
			}
			break;
#endif
		}
	}
	PauseSound(false);
}


//===============================================================================
//	HandleUpdate
//
//	Processes update events for windows.
//===============================================================================

void HandleUpdate(WindowRef inWindow, const EventRecord *inEvent)
{
#ifdef MESS
	if (IsMacMESSWindow(inWindow))
		// BeginUpdate causes problems with DialogSelect
		UpdateMacMESSWindow(inWindow, inEvent);
	else
#endif
	{
		BeginUpdate(inWindow);

		// if we're the main window, we know what to do
		if (inWindow == gDisplayState->window)
			UpdateScreen();
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
		else if (inWindow == gDebugDisplay.window)
			UpdateDebugger();	// R. Nabet 001212
#endif

		// Tell the core to redraw everything next frame. This is so any
		// renderers invalidate their dirty rects
		schedule_full_refresh ();

		EndUpdate(inWindow);
	}
}

//===============================================================================
//	HandleOSEvent
//
//	Processes non-command key events.
//===============================================================================

void HandleOSEvent(EventRecord *inEvent)
{
	//static int autoPaused = false;
	
	// only care about suspend/resume events
	if (((inEvent->message >> 24) & 0x0ff) == suspendResumeMessage)
	{
		// resume case:
		if (inEvent->message & resumeFlag)
		{
			// if we were previously in the background, resume the video
			if (gInBackground && (gMenuState == kMenuStateRunning))
				ResumeVideo();
		
			// no longer in thebackground
			gInBackground = false;

			// if we paused automatically, unpause now
			if (sPausedOnSuspend)
				PauseMacMAME(false);
			sPausedOnSuspend = false;
	
			if (! gEmulationPaused)
				// reactivate InputSprocket devices
				ActivateInputDevices(true);
		}
		
		// suspend case:
		else
		{
			// if we weren't previously in the background, suspend the video
			if ((!gInBackground) && (gMenuState == kMenuStateRunning))
				SuspendVideo();
		
			// in the background now
			gInBackground = true;

			// if we're not already paused, go into autopause mode
			sPausedOnSuspend = false;
			if (!gEmulationPaused)
			{
				sPausedOnSuspend = true;
				PauseMacMAME(true);
			}

			if (sPausedOnSuspend)
				// deactivate InputSprocket devices
				DeactivateInputDevices(true);
		}

#ifdef MESS
		if (IsMacMESSWindow(FrontWindow()))
			SuspendResumeMacMESSWindow(FrontWindow(), &gLastEvent);
#endif
	}
}

#pragma mark -
#pragma mark е Menu Handling Routines

void ProcessGameCommand (HICommand inCommand)
{
	Boolean interlace = 0;
	UInt32 scale = 0x100;
	double gamma_correction;

	HiliteMenu(0);

	switch (inCommand.commandID)
	{
		case kHICommandAbout:
			DisplayAboutBox();
			break;

		case kHICommandOpen:
		case kHICommandClose:
			gExitToFrontend = true;
			gEmulationPaused = false;
			break;
		case kMenuGame_Screenshot:
			SaveScreenshotAs();
			break;
		case kMenuGame_Reset:
			machine_reset();
			break;
		case kMenuGame_Log:
			gPrefs.errorLog = !gPrefs.errorLog;
			
			// close the existing debug log if we just toggled off
			if (!gPrefs.errorLog)
			{
				if (sErrorlogFile)
					fclose(sErrorlogFile);
				sErrorlogFile = NULL;
			}
			break;
		case kMenuGame_Pause:
			gPauseEmulationASAP = true;
			break;
		case kHICommandPrint:
			PrintScreen();
			break;
		case kHICommandPageSetup:
			PrintSetup();
			break;
		case kHICommandQuit:
			gExitToShell = true;
			gExitToFrontend = true;
			gEmulationPaused = false;
			break;

		case kHICommandCopy:
			CopyToClipboard();
			break;
#ifdef MESS
		case kHICommandPaste:
			MESSPasteText();
			break;
#endif
		case kMenuPerf_IncSkip:
			SetFrameskip(gPrefs.frameSkip + 1);
			break;
		case kMenuPerf_DecSkip:
			SetFrameskip(gPrefs.frameSkip - 1);
			break;
		case kMenuPerf_AutoSkip:
			SetAutoFrameskip(!gPrefs.autoSkip);
			break;
		case kMenuPerf_Throttle:
			SetThrottling(!gPrefs.speedThrottle);
			break;
		case kMenuPerf_ResetAvg:
			// setting the throttling to the same value will effectively reset
			// the frame count
			SetThrottling(gPrefs.speedThrottle);
			break;

		case kMenuVideo_1x1:
			scale = 0x100, interlace = 0;
			break;
		case kMenuVideo_2x2:
			scale = 0x200, interlace = 0;
			break;
		case kMenuVideo_2x2s:
			scale = 0x200, interlace = 1;
			break;
		case kMenuVideo_3x3:
			scale = 0x300, interlace = 0;
			break;
		case kMenuVideo_3x3s:
			scale = 0x300, interlace = 1;
			break;
		case kMenuVideo_IncGamma:
			gamma_correction = palette_get_global_gamma();

			gamma_correction += 0.05;
			if (gamma_correction > 2.0) gamma_correction = 2.0;

			palette_set_global_gamma(gamma_correction);
			break;
		case kMenuVideo_DecGamma:
			gamma_correction = palette_get_global_gamma();

			gamma_correction -= 0.05;
			if (gamma_correction < 0.5) gamma_correction = 0.5;

			palette_set_global_gamma(gamma_correction);
			break;
		case kMenuVideo_HideDesk:
			SetFullScreen(!gPrefs.hideDesktop);
			break;

		default:
			break;
	}

	if ((inCommand.commandID == kMenuVideo_1x1) ||
		(inCommand.commandID == kMenuVideo_2x2) ||
		(inCommand.commandID == kMenuVideo_2x2s) ||
		(inCommand.commandID == kMenuVideo_3x3) ||
		(inCommand.commandID == kMenuVideo_3x3s))
	{
		// set it if we can support it
		SetScaleAndInterlace(scale, interlace);
				
		// force an update
		if (gEmulationPaused)
			UpdateScreen();
	}
}

void UpdateGameCommand (HICommand inCommand)
{
	MenuRef menuRef = inCommand.menu.menuRef;
	MenuItemIndex menuIndex = inCommand.menu.menuItemIndex;
	
	switch (inCommand.commandID)
	{
		case kHICommandAbout:
			break;

		case kHICommandOpen:
		case kHICommandClose:
			break;
		case kMenuGame_Screenshot:
			break;
		case kMenuGame_Reset:
			break;
		case kMenuGame_Log:
			CheckMenuItem (menuRef, menuIndex, gPrefs.errorLog);
			break;
		case kMenuGame_Pause:
			CheckMenuItem (menuRef, menuIndex, gEmulationPaused);
			break;
		case kHICommandPrint:
			break;
		case kHICommandPageSetup:
			break;
		case kHICommandQuit:
			break;
#ifdef MESS
		case kHICommandPaste:
			SetMenuItemEnable(menuRef, menuIndex, true);
			break;
#endif
		case kMenuPerf_IncSkip:
			SetMenuItemEnable(menuRef, menuIndex, SupportsFrameskip(gPrefs.frameSkip + 1));
			break;
		case kMenuPerf_DecSkip:
			SetMenuItemEnable(menuRef, menuIndex, SupportsFrameskip(gPrefs.frameSkip - 1));
			break;
		case kMenuPerf_AutoSkip:
			SetMenuItemEnable(menuRef, menuIndex, SupportsAutoFrameskip(!gPrefs.autoSkip));
			CheckMenuItem (menuRef, menuIndex, gPrefs.autoSkip);
			break;
		case kMenuPerf_Throttle:
			SetMenuItemEnable(menuRef, menuIndex, SupportsThrottling(!gPrefs.speedThrottle));
			CheckMenuItem (menuRef, menuIndex, gPrefs.speedThrottle);
			break;
		case kMenuPerf_ResetAvg:
			break;

		case kMenuVideo_1x1:
			SetMenuItemEnable(menuRef, menuIndex, SupportsScaleAndInterlace(0x100, false));
			CheckMenuItem (menuRef, menuIndex, gDisplayState->scale == 0x100);
			break;
		case kMenuVideo_2x2:
			SetMenuItemEnable(menuRef, menuIndex, SupportsScaleAndInterlace(0x200, false));
			CheckMenuItem (menuRef, menuIndex, gDisplayState->scale == 0x200 && !gPrefs.interlace);
			break;
		case kMenuVideo_2x2s:
			SetMenuItemEnable(menuRef, menuIndex, SupportsScaleAndInterlace(0x200, true));
			CheckMenuItem (menuRef, menuIndex, gDisplayState->scale == 0x200 &&  gPrefs.interlace);
			break;
		case kMenuVideo_3x3:
			SetMenuItemEnable(menuRef, menuIndex, SupportsScaleAndInterlace(0x300, false));
			CheckMenuItem (menuRef, menuIndex, gDisplayState->scale == 0x300 && !gPrefs.interlace);
			break;
		case kMenuVideo_3x3s:
			SetMenuItemEnable(menuRef, menuIndex, SupportsScaleAndInterlace(0x300, true));
			CheckMenuItem (menuRef, menuIndex, gDisplayState->scale == 0x300 &&  gPrefs.interlace);
			break;
		case kMenuVideo_IncGamma:
			break;
		case kMenuVideo_DecGamma:
			break;
		case kMenuVideo_HideDesk:
			SetMenuItemEnable(menuRef, menuIndex, SupportsFullScreen(!gPrefs.hideDesktop));
			CheckMenuItem (menuRef, menuIndex, gPrefs.hideDesktop);
			break;

		default:
			break;
	}
}

static pascal OSStatus gameMenuHandler (EventHandlerCallRef inCallRef, EventRef inEvent, void *inUserData)
{
	OSStatus  err = eventNotHandledErr;
	HICommand cmd;
 
	// make sure that we're processing a command event
	if ( GetEventClass( inEvent ) != kEventClassCommand )
		return err;
		
	(void)GetEventParameter (inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(cmd), NULL, &cmd);

	switch ( GetEventKind( inEvent ) )
	{
		// see if this is a process event
		case kEventCommandProcess:
			ProcessGameCommand (cmd);
			break;
		case kEventCommandUpdateStatus:
			UpdateGameCommand (cmd);
			break;
	}
 
	return err;
}

static void InitializeMenuBar (void)
{
	OSStatus err;
	IBNibRef menuBarNib;
	
	static const EventTypeSpec menuEvents[] = 
	{
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassCommand, kEventCommandUpdateStatus }
	};
	
	err = CreateNibReference (CFSTR("MenuBar"), &menuBarNib);
	if (err) goto bail;
	
	err = SetMenuBarFromNib (menuBarNib, CFSTR("MenuBar"));
	if (err) goto bail;

	DisposeNibReference (menuBarNib);

	InstallApplicationEventHandler(NewEventHandlerUPP(gameMenuHandler), GetEventTypeCount(menuEvents), menuEvents, 0, NULL);
	return;
	
bail:
	printf ("Failed to set up menu bar nib");
	return;
}

#pragma mark -
#pragma mark е Misc UI Routines

//===============================================================================
//	PauseMacMAME
//
//	Pauses or unpauses the video system.
//===============================================================================

void PauseMacMAME(Boolean inPause)
{
	// if the state isn't changing, bail
	if (gEmulationPaused == inPause)
		return;

	// set the new paused state and update the music		
	gEmulationPaused = inPause;
	
	// deactivate/reactive input devices
	if (inPause)
		DeactivateInputDevices(false);
	else
		ActivateInputDevices(false);
	
	// Update the video state and let the plugins take any action needed
	PauseSound(inPause);
	PauseVideo(inPause);
}


//===============================================================================
//	DisplayAboutBox
//
//	Displays the about box dialog.
//===============================================================================

void DisplayAboutBox(void)
{
	PaletteHandle oldPalette = GetPalette(FrontWindow());
	MenuState oldState = gMenuState;
	PaletteHandle sysPalette;
	short itemHit = cancel;
	DialogRef aboutDialog;
	GDHandle savedDevice;
	GWorldPtr savedPort;

	// go back to the system palette
	sysPalette = GetNewPalette(0);
	SetPalette(FrontWindow(), sysPalette, false);
	
	// set up the menus
	gMenuState = kMenuStateModal;
//	UpdateMenus();

	// get the dialog box and point to it
	aboutDialog = GetNewDialog(rAboutDialog, NULL, kWindowToFront);
	GetGWorld(&savedPort, &savedDevice);
	SetPortDialogPort(aboutDialog);

#if 0
	// get the short version string and set it
	versionResource = GetResource('vers', 1); // еее
	if (versionResource)
	{
		DialogItemType itemType;
		Handle itemHandle;
		Rect itemRect;
	
		HLockHi(versionResource);
		GetDialogItem(aboutDialog, rAboutVersionItem, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, (ConstStr255Param)(*versionResource + 6));
		ReleaseResource(versionResource);
	}
#endif

	// set the default item and show the window
	SetDialogDefaultItem(aboutDialog, ok);
	ShowWindow(GetDialogWindow(aboutDialog));
	
	// wait for the OK button to be hit
	while (itemHit != ok)
		ModalDialog(gGenericModalFilter, &itemHit);
	
	// tear it down
	DisposeDialog(aboutDialog);
	SetGWorld(savedPort, savedDevice);

	// reset the palette
	if (oldPalette != NULL)
		SetPalette(FrontWindow(), oldPalette, false);
	DisposePalette(sysPalette);

	// restore the menus
	gMenuState = oldState;
//	UpdateMenus();
}


//===============================================================================
//	ErrorAlert
//
//	Throws up an error dialog, can optionally quit the app if needed
//===============================================================================

void ErrorAlert (CFStringRef inString, Boolean inQuit)
{
	CFStringRef errorString = NULL;
	CFStringRef detailString = NULL;

	require ( errorString = CFCopyLocalizedString(CFSTR("ErrorText"), NULL), cantGetErrorString );
	require ( detailString = CFCopyLocalizedString(inString, NULL), cantCreateDetailString );
	
	DialogRef dialog;
	DialogItemIndex itemIndex;
	
	CreateStandardAlert (kAlertStopAlert, errorString, detailString, NULL, &dialog);
	RunStandardAlert (dialog, NULL, &itemIndex);
	
cantGetErrorString:
cantCreateDetailString:

	if (errorString) CFRelease (errorString);
	if (detailString) CFRelease (detailString);

	// ExitToShell now if we need to quit
	if (inQuit)
		ExitToShell();
}


//===============================================================================
//	GenericModalFilter
//
//	Handles basic events during a modal dialog.
//===============================================================================

pascal Boolean GenericModalFilter(DialogRef inDialog, EventRecord *inEvent, short *inItemHit)
{
	ModalFilterUPP standardProc;
	OSErr err;
	
	// look only for certain types of events
	switch (inEvent->what)
	{
		// mouse downs: we need to handle dragging and menubar access
		case mouseDown:
		{
			WindowRef window;
			short part = FindWindow(inEvent->where, &window);
			
			// only handle dragging and menus
			if (part != inDrag && part != inMenuBar)
				break;
			
			HandleMouseDown(inEvent);
			return true;
		}
		
		// update events: handle anything we can
		case updateEvt:
			if (!IsDialogEvent(inEvent))
			{
				HandleUpdate((WindowRef)inEvent->message, inEvent);
				return true;
			}
			break;
			
		// high level events; let the AppleEvent Manager do the work
		case kHighLevelEvent:
			AEProcessAppleEvent(inEvent);
			break;
	}

	// call through to the standard proc	
	err = GetStdFilterProc(&standardProc);
	if (err == noErr)
		return InvokeModalFilterUPP(inDialog, inEvent, inItemHit, standardProc);
	else
		return false;
}
