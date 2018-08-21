/*##########################################################################

	macdebug.c

	Support module for integrated MAME debugger. Simulates in a window a 
	character-mapped debugger screen.
	Written by Brad Oliver, John Butler, and Aaron Giles.

##########################################################################*/

#ifdef MAME_DEBUG

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

#include <Carbon/Carbon.h>

#include "driver.h"
#include "osdepend.h"

#include "mac.h"
#include "macvideo.h"
#include "macdebug.h"
#include "macinput.h"

/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// display parameters
DisplayParameters		gDebugDisplay;		// current state of the debugger display
GWorldPtr				gDebugGWorld;

static int 				sWidth, sHeight;	// resolution of our text screen, usually 80x25
static int				sDepth;

static GrafPtr			sSavedPort;			// GrafPtr to restore
static int				sOldMenuState = -1;	// MenuState to restore to gMenuState

static Point			sWindowPosition = { 50, 50 };	// current position of debug window
static UInt8			sRawPalette[65538*3]; // a copy of the current palette as-is

Boolean					gDebuggerFocus;		// true if the debugger is active
											// we should make the mouse available, and all UI features we can

static const int debugWinWidth = 640;
static const int debugWinHeight = 480;

/*##########################################################################
	MACROS
##########################################################################*/



/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static void				EnableDebugger (void);
static void				DisableDebugger (void);
//static void				AdjustDebugWindowPosition(void);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/


//===============================================================================
//	DisableDebugger
//
//	In DOS, flips from text-mode debugger screen to the bitmapped emulation
//	screen. Here, hide debugger window if present and force emulation screen to
//	redraw.
//===============================================================================

void DisableDebugger(void)
{
	if (gDebugDisplay.window && IsWindowVisible(gDebugDisplay.window))
	{
		// don't dispose of everything, because mamedbg.c expects our
		// screen to still be valid -- just hide the window instead
		HideWindow(gDebugDisplay.window);
		if (sSavedPort)
			SetPort(sSavedPort);
		sSavedPort = NULL;
	}
	
	// reset the menu state
	if (sOldMenuState != -1)
	{
		gMenuState = (MenuState)sOldMenuState;
//		UpdateMenus();
		sOldMenuState = -1;
	}
	
	// allow plugin renderers to unpause and refresh the emulation screen
	PauseVideo(false);
}


//===============================================================================
//	osd_screen_update
//
//	Updates the debugger screen if necessary. Returns true if the debugger is
//  dominant.
//===============================================================================

#define kOffsetHack 50

static int BlitDebugger(void)
{
	PixMapHandle spixmap;
	PixMapHandle dpixmap;
	Rect sbounds, dbounds;
	
	// bail if window or debugger bitmap aren't available
	if (gDebugDisplay.window == NULL || gDebugGWorld == NULL) return 0;

	spixmap = GetGWorldPixMap(gDebugGWorld);
	dpixmap = GetPortPixMap(GetWindowPort(gDebugDisplay.window));

	// compute the source rectangle
	sbounds.left = 0 + kOffsetHack;
	sbounds.right = debugWinWidth + kOffsetHack;
	sbounds.top = 0 + kOffsetHack;
	sbounds.bottom = debugWinHeight + kOffsetHack;

	dbounds.left = 0;
	dbounds.right = debugWinWidth;
	dbounds.top = 0;
	dbounds.bottom = debugWinHeight;

	// make sure CopyBits doesn't do any needless remapping
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	// copy it, locking the GWorld while we do it
	LockPixels(spixmap);
	CopyBits((BitMapPtr)*spixmap, (BitMapPtr)*dpixmap, &sbounds, &dbounds, srcCopy, NULL);
	UnlockPixels(spixmap);

	return 1;
}

void UpdateDebugger()
{
	GDHandle oldDevice;
	GWorldPtr oldPort;

	// remember where we were, then point to the window
	GetGWorld(&oldPort, &oldDevice);
	SetGWorld(GetWindowPort(gDebugDisplay.window), NULL);

	// Now do draw window
	BlitDebugger();

	// point back to the old port
	SetGWorld(oldPort, oldDevice);
}

int DrawDebugger (mame_bitmap *debug_bitmap)
{
	if (debug_bitmap && input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
	{
		mac_set_debugger_focus(gDebuggerFocus ^ 1);
	}

	if (gDebuggerFocus == 0) return 0;

	// bail if window or debugger bitmap aren't available
	if (gDebugDisplay.window == NULL || debug_bitmap == NULL || gDebugGWorld == NULL) return 0;

	// copy the current screen into the GWorld
	CopyScreenToGWorld (debug_bitmap, gDebugGWorld, gDebugDisplay);

	return BlitDebugger();
}

void mac_set_debugger_focus (int hasfocus)
{
	if (hasfocus)
	{
		EnableDebugger ();
		gDebuggerFocus = 1;
	}
	else
	{
		DisableDebugger ();
		gDebuggerFocus = 0;
	}
}


#pragma mark -


//===============================================================================
//	CreateDebugScreen
//
//	Create a debugger screen with the specified width and height and initialize 
//	associated structures.
//===============================================================================

static void EnableDebugger(void)
{
	int inWidth = debugWinWidth;
	int inHeight = debugWinHeight;
	OSStatus err;

	if (inWidth != sWidth || inHeight != sHeight)
	{
		// screen width or height changed -- force screen to be recreated
		DisposeDebugScreen();
	}
		
	if (gDebugDisplay.window == NULL)
	{
		Rect	windowBounds;

		// remember the width and height
		sWidth = inWidth;
		sHeight = inHeight;
		
		SetRect(&windowBounds, 0, 0, inWidth, inHeight);
		OffsetRect (&windowBounds, sWindowPosition.h, sWindowPosition.v);
		
		// create the window
		err = CreateNewWindow (kDocumentWindowClass, kWindowCloseBoxAttribute, &windowBounds, &gDebugDisplay.window);

		if (err)
		{
			printf("Error allocating debugger window");
			return;
		}

		SetWTitle (gDebugDisplay.window, "\pMAME Debugger");

		// Attach the keyboard handler to the window
		InitializeKeyboardEvents(gDebugDisplay.window);

		ShowWindow (gDebugDisplay.window);
		
		{
			Rect	r;
			
			// activate the window and make it the current port
			SelectWindow(gDebugDisplay.window);
			GetPort(&sSavedPort);
			SetPortWindowPort(gDebugDisplay.window);
			
			// erase the window to black
			ForeColor(blackColor);
			PaintRect(GetWindowPortBounds(gDebugDisplay.window, &r));
			ValidWindowRect(gDebugDisplay.window, &r);
			
			gDebugDisplay.width = debugWinWidth;
			gDebugDisplay.height = debugWinHeight;

			gDebugDisplay.visxoffset = gDebugDisplay.visyoffset = 0;
			gDebugDisplay.viswidth = gDebugDisplay.width;
			gDebugDisplay.visheight = gDebugDisplay.height;

#if 1
			gDebugDisplay.depth = sDepth;
#else
			if (gDebugDisplay.depth != gDisplayState->depth)
			{
				gDebugDisplay.depth = gDisplayState->depth;
				UpdatePalette16();
			}
#endif
			gDebugDisplay.device = gDisplayState->device;
		}

		if (gDebugGWorld == NULL)
		{
			// allocate a GWorld
			err = NewGWorld(&gDebugGWorld, gDebugDisplay.depth, &windowBounds, NULL, NULL, 0);
			if (err || !gDebugGWorld)
			{
				printf("Error allocating debugger bitmap");
				return;
			}
		}
	}
	else
	{
		GrafPtr	curPort;
		Rect	r;

		// screen already exists -- just unhide it
		if (!IsWindowVisible(gDebugDisplay.window))
			ShowWindow(gDebugDisplay.window);
		SelectWindow(gDebugDisplay.window);
		GetPort(&curPort);
		if (curPort != GetWindowPort(gDebugDisplay.window))
			sSavedPort = curPort;
		SetPortWindowPort(gDebugDisplay.window); // <-- necessary!

		// erase it to black
//		RGBForeColor(&sPalette[BLACK]);
		PaintRect(GetWindowPortBounds(gDebugDisplay.window, &r));
		ValidWindowRect(gDebugDisplay.window, &r);

	}
	
	// pause plugin renderers
	PauseVideo(true);
	
	// adjust the menubar
    if (sOldMenuState == -1)
    {
    	sOldMenuState = (int)gMenuState;
    	gMenuState = kMenuStateModal;
//    	UpdateMenus();
    }
}


//===============================================================================
//	DisposeDebugScreen
//
//	Remove the debugger screen and free memory and structures associated with it.
//===============================================================================

void DisposeDebugScreen (void)
{
	if (sSavedPort)
		SetPort(sSavedPort);
	sSavedPort = NULL;
	
	if (gDebugDisplay.window)
		DisposeWindow(gDebugDisplay.window);
	gDebugDisplay.window = NULL;
	
	if (gDebugGWorld)
		DisposeGWorld (gDebugGWorld);
	gDebugGWorld = NULL;
	
	if (sOldMenuState != -1)
	{
		gMenuState = (MenuState)sOldMenuState;
//		UpdateMenus();
	}
	sOldMenuState = -1;
}


Boolean CreateDebugPalette16(void)
{
	// allocate the 16-bit lookup table
	gDebugDisplay.lookup16 = malloc(sizeof(UInt16) * 2 * DEBUGGER_TOTAL_COLORS);
	if (!gDebugDisplay.lookup16)
		return false;
	
	sDepth = 16;
	return true;
}

Boolean CreateDebugPalette32(void)
{
	// allocate the 32-bit lookup table
	gDebugDisplay.lookup32 = malloc(sizeof(UInt32) * 2 * DEBUGGER_TOTAL_COLORS);
	if (!gDebugDisplay.lookup32)
		return false;

	sDepth = 32;
	return true;
}

void update_debug_palette(const rgb_t *palette)
{
	UInt32 *lookup;
	int i;

	lookup = (sDepth == 16) ? gDebugDisplay.lookup16 : gDebugDisplay.lookup32;

	// set up the main palette
	for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
	{
		// for 16-bit palettes, the pens are just indices
		UInt32 pen		= i;
		UInt32 red 		= (*palette >> 16) & 0xff;
		UInt32 green	= (*palette >> 8) & 0xff;
		UInt32 blue 	= *palette++ & 0xff;
		UInt32 value;

		// copy the data into the raw palette
		sRawPalette[pen * 3 + 0] = red;
		sRawPalette[pen * 3 + 1] = green;
		sRawPalette[pen * 3 + 2] = blue;

		if (sDepth == 16)
		{
			// Set the 16-bit lookup table
			value 	= ((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3);
			lookup[pen] = (value << 16) | value;
		}
		else
		{
			// Set the 32-bit lookup table
			value 	= (red << 16) | (green << 8) | blue;
			lookup[2*pen+1] = lookup[2*pen] = value;
		}
	}
}

//===============================================================================
//	AdjustDebugWindowPosition
//
//	Move the window to an optimal position for blitting.
//===============================================================================

/*static void AdjustDebugWindowPosition(void)
{
	Rect	r;
	OSErr	err;

	// contRgn won't be setup if window is not visible	
	if (!gDebugDisplay.window || !IsWindowVisible(gDebugDisplay.window)) return;

	// Align the window on a long-word boundary
	err = GetWindowBounds(gDebugDisplay.window, kWindowContentRgn, &r);
	if (noErr == err)
		MoveWindow(gDebugDisplay.window, r.left & 0xfff8, r.top, false);
}*/


#endif // MAME_DEBUG
