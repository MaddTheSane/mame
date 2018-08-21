/*##########################################################################

	macextras.c

	Miscellanous Mac-only features.

##########################################################################*/

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include "driver.h"

#include "mac.h"
#include "macstrings.h"
#include "macextras.h"

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	CONSTANTS
##########################################################################*/

#define kRGBStackSize		20
#define kPenStateStackSize	20


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/
#pragma mark е Utility functions


//===============================================================================
//	PushPopRGB
//
//	Utility function for saving or restoring the foreground and background 
//	RGBColor of the current port. Use PushRGB and PopRGB macros for convenience.
//
//	Can be nested up to kRGBStackSize.
//===============================================================================

void PushPopRGB(Boolean inPop)
{
	typedef struct {
		RGBColor	fore;
		RGBColor	back;
	} tRGBPair;

	static tRGBPair	sStack[kRGBStackSize];	// the stack
	static int		sStackPtr;				// index into the stack

	if (inPop)
	{
		// pop the saved RGB colors off the stack
		--sStackPtr;
		if (sStackPtr < kRGBStackSize)
		{
			RGBForeColor(&sStack[sStackPtr].fore);
			RGBBackColor(&sStack[sStackPtr].back);
		}
	}
	else
	{
		// push the current RGB colors onto the stack
		if (sStackPtr < kRGBStackSize)
		{
			GetForeColor(&sStack[sStackPtr].fore);
			GetBackColor(&sStack[sStackPtr].back );
		}
		sStackPtr++;
	}
}


//===============================================================================
//	PushPopPenState
//
//	Utility function for saving or restoring the PenState of the current port.
//	Use PushPenState and PopPenState macros for convenience.
//
//	Can be nested up to kPenStateStackSize.
//===============================================================================

void PushPopPenState(Boolean inPop)
{
	static PenState	sStack[kPenStateStackSize];	// the stack
	static int		sStackPtr;					// index into the stack

	if (inPop)
	{
		// pop the saved PenState off the stack
		--sStackPtr;
		if (sStackPtr < kRGBStackSize)
		{
			SetPenState(&sStack[sStackPtr]);
		}
	}
	else
	{
		// push the current PenState onto the stack
		if (sStackPtr < kRGBStackSize)
		{
			GetPenState(&sStack[sStackPtr]);
		}
		sStackPtr++;
	}
}


#pragma mark -
#pragma mark е Misc Mame add-ons


//===============================================================================
//	DisplayWarningDialog
//
//	Puts up the ROM warning dialog when MacMAME is first started up.
//===============================================================================

static pascal OSStatus dialogEventHandler (EventHandlerCallRef inCallRef, EventRef inEvent, void *inUserData)
{
	OSStatus  err = eventNotHandledErr;
	WindowRef windowRef;
	HICommandExtended cmd;
	Boolean value;
	
	windowRef = inUserData;
 
	if ( GetEventClass( inEvent ) != kEventClassCommand )
		return err;
		
	(void)GetEventParameter (inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(cmd), NULL, &cmd);

	switch ( GetEventKind( inEvent ) )
	{
		// see if this is a process event
		case kEventCommandProcess:
			switch (cmd.commandID)
			{
				case kHICommandOK:
					QuitAppModalLoopForWindow(windowRef);
					err = noErr;
					break;
				case 'Mwrn':
					value = GetControl32BitValue (cmd.source.control);
					SetPrefID_App ();
					SetPrefAsBoolean (value, CFSTR("noRomWarning"));
					err = noErr;
					break;
			}
			break;
	}
 
	return err;
}

void DisplayWarningDialog(void)
{
	Boolean dontShowIt;
		
	SetPrefID_App ();
	GetPrefAsBoolean (&dontShowIt, CFSTR("noRomWarning"), false);

	if (dontShowIt)
	{
#if 1 // FRONTEND ееееее
		(void)GetFirstROMSet();
#endif
	}
	else
	{
		WindowRef myDialog;
		OSStatus err;
		IBNibRef dialogNib;
		EventTypeSpec      dialogEvents[] = {{ kEventClassCommand, kEventCommandProcess } };

		err = CreateNibReference (CFSTR("Warning"), &dialogNib);
		if (err) return;
	
		err = CreateWindowFromNib (dialogNib, CFSTR("Warning"), &myDialog); 
		if (err) return;
	
		InstallWindowEventHandler(myDialog, NewEventHandlerUPP(dialogEventHandler), GetEventTypeCount(dialogEvents), dialogEvents, myDialog, NULL); 

		// draw the dialog and find the ROMsets before going into modal land
		ShowWindow (myDialog);
#if 1 // FRONTEND ееееее
		(void)GetFirstROMSet();
#endif
		RunAppModalLoopForWindow (myDialog);

		DisposeWindow (myDialog);
		DisposeNibReference (dialogNib);
	}
}


#pragma mark -
#pragma mark е Movies


//===============================================================================
//	ShowSplashMovie
//
//	Automatically load and play any QuickTime movie named "macmame.mov" and
//	located in the Misc Support FIles folder.
//===============================================================================

OSErr ShowSplashMovie(void)
{
	FSRef		folderRef, fileRef;
	FSSpec		movieSpec;
	short		movieRefNum;
	Movie		theMovie;
	Ptr			movieRestoreState;
	WindowRef	movieWindow;
	Boolean		done;
	OSStatus	err = fnfErr;
	HFSUniStr255 name;

	// look for a file named macmame.mov in Screenshots folder

	__Require_noErr( err = GetFSRefForMAMEFolder (MAC_FILETYPE_SUPPORT, &folderRef), cantGetMAMEFolder );

	name.length = ConvertCStringToUnicode ("MacMAME.mov", name.unicode);
	if (name.length == 0) goto bail;
	__Require_noErr_Quiet( err = FSMakeFSRefUnicode (&folderRef, name.length, name.unicode, kTextEncodingUnknown, &fileRef), cantMakeFSRef );

	err = FSGetCatalogInfo (&fileRef, kFSCatInfoNone, NULL, NULL, &movieSpec, NULL );
	if (err) return err;

	// initialize movie toolbox
	err = EnterMovies();
	if (err) return err;
	
	// open the movie file
	err = OpenMovieFile(&movieSpec, &movieRefNum, fsRdPerm);
	
	// create a movie from the file
	if (noErr == err)
	{
		err = NewMovieFromFile(&theMovie,
								movieRefNum,
								NULL,			// &movieResID,
								NULL,			// movieName
								newMovieActive,
								NULL);			// &wasChanged
		
		CloseMovieFile(movieRefNum);
	}
	if (err) return err;
	
	// start the movie playing fullscreen
	{
		Rect	movieBox;
		short	desiredWidth = 0;	// don't change
		short	desiredHeight = 0;	// don't change

		// take over the main screen		
		err = BeginFullScreen(	&movieRestoreState,
								NULL, 			// whichGD -- use main screen
								&desiredWidth,	// don't change
								&desiredHeight,	// don't change
								&movieWindow,	// create for me
								NULL,			// eraseColor -- use black
								/*fullScreenDontChangeMenuBar |*/
								fullScreenHideCursor |
								fullScreenAllowEvents); // flags

		if (noErr == err)
		{
			SetPortWindowPort(movieWindow);
			
			// resize the movie to fill the screen
			movieBox = (**GetMainDevice()).gdRect;
			SetMovieBox(theMovie, &movieBox);
			
			// init movie stuff and start the movie
			SetMovieGWorld(theMovie, NULL /*port*/, NULL /*gdh*/);
			SetMovieActiveSegment(theMovie, -1 /*TimeValue startTime*/,0/*TimeValue duration*/);
			GoToBeginningOfMovie(theMovie);
			//MoviesTask(theMovie, 0); 	// <-- workaround for very strange bug with MPEG movies
			SetMoviePreferredRate(theMovie, 0x00010000);
			StartMovie(theMovie);
			err = GetMoviesError();
			
			if (err)
			{
				DisposeMovie(theMovie);
				(void)EndFullScreen(movieRestoreState, 0 /*flags*/);
				return err;
			}
		}
		else
		{
			DisposeMovie(theMovie);
			return err;
		}
	}
	
	// loop until movie is done or user cancels with keystroke or mouse click
	done = false;
	while ((done || IsMovieDone(theMovie)) == false)
	{
		EventRecord	evt;
		
		(void)WaitNextEvent(keyDownMask+mDownMask+updateMask+activMask, &evt, 0, NULL);
		switch (evt.what)
		{
			case keyDown:
			case mouseDown:
				done = true;
				break;
			case updateEvt:
			{
				WindowRef wp = (WindowRef)evt.message;
				BeginUpdate(wp);
				if (wp == movieWindow)
				{
					UpdateMovie(theMovie);
					//SetPort (movieWindow);
					//PaintRect (&movieWindow->portRect);
				}
				EndUpdate(wp);
				break;
			}
			case activateEvt:
				break;
		}

		// continue playing the movie
		MoviesTask(theMovie, 10000L /*maxMillSecToUse*/);
		
		if (noErr != GetMovieStatus(theMovie, NULL /*Track *firstProblemTrack*/))
		{
			done = true;
		}
	}
	
	// show's over, folks...
	DisposeMovie(theMovie);
	(void)EndFullScreen(movieRestoreState, 0 /*flags*/);
	FlushEvents(keyDownMask+autoKeyMask+mDownMask+mUpMask, 0);

cantGetMAMEFolder:
cantMakeFSRef:
bail:
	return err;
}


