/*##########################################################################

  macvideo.c

  Mac-specific video handling.  Original code by Brad Oliver, Richard
  Bannister, John Stiles, and others.  Ripped out of mac.c and heavily
  cleaned up and tweaked by Aaron Giles.

##########################################################################*/

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <math.h>

#if __MSL__
int	 usleep(unsigned int);
#else
#include <unistd.h>
#endif

// Metrowerks includes
#if defined(__profile__) && __profile__
	#include <Profiler.h>
#endif

// MAME includes
#include "driver.h"
#include "mame.h"
#include "vidhrdw/vector.h"

// our includes
#include "mac.h"
#include "macblitters.h"
#ifdef MAME_DEBUG
#ifndef NEW_DEBUGGER
#include "macdebug.h"
#endif
#endif
#include "macinput.h"
#include "macsound.h"
#include "macstrings.h"
#include "macutils.h"
#include "macvideo.h"
#include "software.h"
#include "OpenGLPluginX.h"

#undef hideDesktop

#ifdef __MWERKS__
#pragma require_prototypes on
#endif

/*##########################################################################
	COMPILE-TIME CONSTANTS
##########################################################################*/

#define FRAMESKIP_LOG			0


/*##########################################################################
	CONSTANTS
##########################################################################*/

enum
{
	kMaxPlugins					= 16,				// number of plugins, maximum

	kMaxPaletteSize				= 65536,			// maximum number of colors
	
	kBitmapSlop					= 16,				// pixels of slop on allocated bitmaps
	
	kSettleFramesBeforeTiming 	= 20,				// number of frames to ignore before timing
	kDisplaySettleTicks			= 60,				// time for other applications to settle down, in ticks
	
	kFrameskipLevels			= 12,				// number of frameskip levels
	kFPSAverageCount			= 4					// average the FPS over the last 2 seconds
};


/*##########################################################################
	TYPE DEFINITIONS
##########################################################################*/

typedef struct
{
	FSSpec					spec;						// filespec of the plugin
	CFragConnectionID		connection;					// CFM connection ID
	Boolean					enabled;					// true if enabled
	DisplayDescription		desc;						// description returned from main
} PluginData;


/*##########################################################################
	EXTERNALLY-VISIBLE GLOBAL VARIABLES
##########################################################################*/

// display parameters
DisplayParameters			gDisplay;					// current state of the display
const DisplayParameters *	gDisplayState = &gDisplay;	// for external use

int							frameskip;					// these two are tweaked by the skanky cheat code
int							autoframeskip;
UInt32						gCursorHidden;

int							gFlipx;
int							gFlipy;
int							gSwapxy;


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

// display descriptions
static UInt32				sPluginCount;				// number of plugins found
static PluginData 			sPluginList[kMaxPlugins];	// list of available plugins
static const PluginData *	sPlugin;					// pointer to the currently active plugin
static const PluginData *	sSoftwarePlugin;			// pointer to the software plugin

// global screen information
static Boolean				sVideoSuspended;			// true if the video system has been suspended
static Rect					sEffectiveScreenRect;		// boundaries of the screen, minus menubar & such
static Rect					sFullScreenRect;			// boundaries of the screen

// game screen information
static UInt32				sScreenAttributes;			// the current screen attributes
static GWorldPtr			sScreenGWorld;				// GWorld for offscreen drawing
static EventHandlerUPP		sWindowEventHandlerUPP;		// carbon window constrain event handler
static EventHandlerRef		sEventHandlerRef;			// event handler reference

// dirty buffer information
static UInt32				sDirtyBufferSize;			// size of each buffer, in bytes
static UInt8 *				sDirtyBuffer;				// pointers to two dirty buffers

// global display information
static mame_bitmap *		sCurrentDisplayBitmap;		// pointer to the current display bitmap

// frameskipping information
static Boolean				sFastForwarding;			// true if we're fast-forwarding
static UInt32				sSkipFrameCount;			// our current position in the frameskipping array

// autoframeskipping information
static UInt32				sValidBlitTimes[kFrameskipLevels];// amount of time for a blit at the given frameskip
static UInt32				sValidSkippedTime;			// amount of time for a skipped frame: last valid value
static UInt32				sValidDrawnTime;			// amount of time for a drawn frame: last valid value
static UInt32				sIdleTimeMakeup;			// amount of idle time to make up (fudge factor)

// autoframeskipping timing information
static Boolean				sSkipTimesValid;			// true if these times are valid
static UInt32				sDrawnFrames;				// number of frames we actually drew in this time period
static UInt32				sSkippedFrames;				// number of frames we skipped in this time period
static UInt32				sDrawnFramesTime;			// total time to process and draw the displayed frames
static UInt32				sSkippedFramesTime;			// total time to process and draw the skipped frames
static UInt32				sIdleTime;					// total time spent idling
static UInt32				sBlitTime;					// total time spent blitting
static UInt32				sLastTweakTime;				// time when we last did frameskip tweaking
static UInt32				sStartFrameTime;			// time at the start of the current frame

// frame timing information
static UInt32				sMicrosecondsPerFrame;		// number of microseconds per frame, rounded to an int
static UInt32				sFPSElapsedFrames[kFPSAverageCount];// total frames (displayed or not) since last FPS update
static UInt32				sFPSDeltaTime[kFPSAverageCount];// delta time during which these frames elapsed
static UInt64				sFPSTotalElapsedTime;		// time of all the frames, used for the average calculation
static UInt32 				sFPSLastUpdateTime;			// time of the last FPS update
static UInt32				sPreviousTargetTime;		// target time of the last displayed frame (always within a fraction of second of sPreviousDisplayTime)
static UInt32				sPreviousDisplayTime;		// actual time of the last displayed frame
static UInt32				sFramesDisplayed;			// total number of frames displayed
static UInt64				sAvgTotalFrames = 0;
static UInt64				sAvgTotalTime = 0;
static UInt32				sAvgFPS;

// previous frame's display information
static Boolean				sLastInterlace;				// were we interlaced last frame?
static UInt32				sLastScale;					// were we scaled last frame?
static Boolean				sLastHideDesktop;			// was the desktop hidden last frame?
static Rect					sLastDisplayBounds;			// gdRect of monitor, for res switches
static Boolean				sLastBlitWasSlow;			// true if the previous blit was a slow one

// palette information
static PaletteHandle		sPalette;					// handle to the palette we're using
static UInt8				sRawPalette[kMaxPaletteSize*3];// a copy of the current palette as-is

// display change information
static UInt32				sLastDisplayChangeTicks;	// tick count of last display change

// printing information
static Handle				sPageFormat = NULL;
static Handle				sPrintSettings = NULL;

// menu hiding information
static Boolean				sMenubarHidden;				// true if the menubar is currently hidden

// frameskipping tables
static const UInt8 sSkipTable[kFrameskipLevels][kFrameskipLevels] =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
};

static const UInt8 sWaitTable[kFrameskipLevels][kFrameskipLevels] =
{
	{ 1,1,1,1,1,1,1,1,1,1,1,1 },
	{ 2,1,1,1,1,1,1,1,1,1,1,0 },
	{ 2,1,1,1,1,0,2,1,1,1,1,0 },
	{ 2,1,1,0,2,1,1,0,2,1,1,0 },
	{ 2,1,0,2,1,0,2,1,0,2,1,0 },
	{ 2,0,2,1,0,2,0,2,1,0,2,0 },
	{ 2,0,2,0,2,0,2,0,2,0,2,0 },
	{ 2,0,2,0,0,3,0,2,0,0,3,0 },
	{ 3,0,0,3,0,0,3,0,0,3,0,0 },
	{ 4,0,0,0,4,0,0,0,4,0,0,0 },
	{ 6,0,0,0,0,0,6,0,0,0,0,0 },
	{12,0,0,0,0,0,0,0,0,0,0,0 }
};


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static void mac_set_visible_area(int min_x, int max_x, int min_y, int max_y);
static int mac_allocate_colors(UINT32 *rgb_components);
mame_bitmap * mac_screenshot_create_flipped (mame_bitmap *bitmap, const rectangle *bounds, rectangle *newbounds);

//static void					PreparePlugin(const FSSpec *inFilespec);

static Boolean				CreateGWorldBitmap(UInt32 width, UInt32 height, UInt32 depth, CTabHandle colorTable);
static int					CreateGameWindow(void);
static void					AdjustWindowSize(mame_bitmap *inBitmap);
static void 				PositionWindowOnscreen(UInt32 inWidth, UInt32 inHeight);
static void					UpdateBitmap (mame_bitmap *inBitmap);
static pascal OSStatus		appWindowEventHandler (EventHandlerCallRef myHandler, EventRef event, void* userData);

static void 				UpdateDisplayState(mame_bitmap *inBitmap);
static void 				ComputeScalingFactor(void);

static UInt32 				SpeedThrottle(UInt32 inCurrentFrameTime);
static UInt32				TweakFrameskip(UInt32 inCurrentFrameTime);
static UInt32				RedrawVideo(UInt32 inCurrentFrameTime, mame_bitmap *inBitmap, int game_palette_entries);
static Boolean 				CanBlitFast(const Rect *inBounds);
static void 				DoBlitSlow(const Rect *inDest);
static void					ResetDirtyRects(Boolean clear);
//static void					UpdateFPS(mame_bitmap *inBitmap, UInt32 inCurrentFrameTime);

static void 				AdjustPalette(int game_palette_entries);
static Boolean 				CreatePalette15(UINT32 *rgb_components);
static Boolean 				CreatePalette16(UINT32 *rgb_components);
static Boolean 				CreatePalette32(UINT32 *rgb_components);
static Boolean 				CreatePalette32d(UINT32 *rgb_components);

static PicHandle			CopyScreenToPICT(mame_bitmap *bitmap);

static void 				ResetGlobals(void);
static void					ResetFrameCount(void);
static void 				FreeAllDisplayData(void);


/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark -
#pragma mark ¥ Display Management

//===============================================================================
//	osd_create_display
//
//	Allocates memory for the display at the given resolution; this function
//	returns 0 if everything succeeded.
//===============================================================================

int osd_create_display(const osd_create_params *params, UINT32 *rgb_components)
{
	int depth = params->depth;

	// make sure we have plugins before we proceed	
	if (!sPlugin)
	{
		printf("No plugins installed!");
		return NULL;
	}

	logerror ("osd_create_display, width: %d, height: %d, depth: %d, fps: %f, attr: %d\n",
				params->width, params->height, params->depth, params->fps, params->video_attributes);

	// reset the globals
	ResetGlobals();

	// reset the cursor state
	InitCursor();

	if (depth == 15) depth = 16;

	// set initial display parameters
	gDisplay.flipx = gFlipx;
	gDisplay.flipy = gFlipy;
	gDisplay.swapxy = gSwapxy;

	gDisplay.visxoffset = gDisplay.visyoffset = 0;
	gDisplay.width = gDisplay.viswidth = params->width;
	gDisplay.height = gDisplay.visheight = params->height;
	gDisplay.depth = depth;
	gDisplay.vector = ((params->video_attributes & VIDEO_TYPE_VECTOR) != 0);
	gDisplay.direct = ((params->video_attributes & VIDEO_RGB_DIRECT) != 0);
	
	// set the global attributes
	sScreenAttributes = params->video_attributes;
	
	// determine the aspect ratio
	gDisplay.aspect = (double) params->aspect_x / params->aspect_y;
	/*gDisplay.aspect = 4.0 / 3.0;
	if (attributes & VIDEO_DUAL_MONITOR)
	{
		if (width > height)
			gDisplay.aspect *= 2.0;
		else
			gDisplay.aspect *= 0.5;
	}
	if (orientation & ORIENTATION_SWAP_XY)
		gDisplay.aspect = 1.0 / gDisplay.aspect;*/
	
	// attempt to allocate the display
	if (!(*sPlugin->desc.allocateDisplay)(&gDisplay))
	{
		// if we failed for software, we're hosed
		if (sPlugin == sSoftwarePlugin)
			goto AllocateDisplayFailed;
			
		// otherwise, try software
		sPlugin = sSoftwarePlugin;
		return osd_create_display(params, rgb_components);
	}

	// allocate the offscreen bitmap
	if (!CreateGWorldBitmap(gDisplay.swapxy ? gDisplay.height : gDisplay.width,
								gDisplay.swapxy ? gDisplay.width : gDisplay.height,
								gDisplay.depth, NULL))
		goto CreateGWorldFailed;
	
	// hide the desktop if the user specifies, or if it's required
	gDisplay.fullscreen = gPrefs.hideDesktop || sPlugin->desc.fullscreen;
	
	// create the game window
	if (!CreateGameWindow())
		goto CreateWindowFailed;

	// set the initial scale
	ComputeScalingFactor();
	gDisplay.interlace = sPlugin->desc.interlace ? gPrefs.interlace : false;
	
	// reset the dirty rects and set the globals
	ResetDirtyRects(false);
	// reset the full blit flag
	gDisplay.full = false;

	// start profiling now
	#if defined(__profile__) && __profile__
		ProfilerInit(collectDetailed, bestTimeBase, 1000, 20);
	#endif

	// turn on the sprockets
	ActivateInputDevices(true);
	
	// Let the plug-in do any last-minute stuff
	(*sPlugin->desc.prepareDisplay)(&gDisplay);

	// back to an arrow
	{
		Cursor arrow;
		SetCursor(GetQDGlobalsArrow(&arrow));
	}

	// go to the modal menu state
	gMenuState = kMenuStateRunning;
//	UpdateMenus();

	/* init colors */
	mac_allocate_colors(rgb_components);

	// set the UI area
	ui_set_visible_area(gDisplay.visxoffset, gDisplay.visyoffset, gDisplay.visxoffset+gDisplay.viswidth, gDisplay.visyoffset+gDisplay.visheight);

	return 0;

CreateWindowFailed:
CreateGWorldFailed:
//CreateBitmapFailed:
	(*sPlugin->desc.closeDisplay)(&gDisplay);
AllocateDisplayFailed:
	logerror ("osd_create_display failed\n");

	FreeAllDisplayData();
	{
		Cursor arrow;
		SetCursor(GetQDGlobalsArrow(&arrow));
	}
	return 1;
}

//===============================================================================
//	osd_set_visible_area
//
//	Sets the visible area of the game
//===============================================================================
static void mac_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	// start with the basics
	gDisplay.visxoffset = min_x;
	gDisplay.visyoffset = min_y;
	gDisplay.viswidth = max_x - min_x + 1;
	gDisplay.visheight = max_y - min_y + 1;

	// set the UI area
	ui_set_visible_area(min_x, min_y, max_x, max_y);
	
	// let the plugin know
	UpdateDisplayState(NULL);
	if (sPlugin->desc.updateVisibleArea != NULL)
		(*sPlugin->desc.updateVisibleArea)(&gDisplay);
		
	// tweak the last scale value so the next call to AdjustWindowSize will
	// properly resize the window
	// ...but only do this for the software renderer; the GL renderer can strech
	// the bitmap to fit any size, so the window size should never change from
	// the initial dimensions (just like a real physical monitor!)
	if (sPlugin->desc.identifier != 'OGL ')
		sLastScale = -1;
	else
		SetDisplayChanged();			// keep the framecount correct for GL plugin
}

//===============================================================================
//	osd_close_display
//
//	Frees up memory and such for the display
//===============================================================================

void osd_close_display(void)
{
	// stop profiling now
	#if defined(__profile__) && __profile__
		ProfilerDump("\pMAME Profile");
		ProfilerTerm();
	#endif

	// important: redisplay the menubar before closing the display
	// if the plugin changes resolutions on close, we will restore the incorrect grayRgn
	ShowMenubar();
	
	// tear down the display
	UpdateDisplayState(NULL);
	(*sPlugin->desc.closeDisplay)(&gDisplay);

	DisposeCarbonWindowEvents ();

	// free the window and show the cursor
	if (gDisplay.window != NULL)
		DisposeWindow(gDisplay.window);
	gDisplay.window = NULL;
	ShowCursorAbsolute();
	
	// free all allocated memory
	FreeAllDisplayData();

	// restore the originally selected plugin
	SetActivePlugin(gPrefs.plugin);
	
	// reset the current display
	sCurrentDisplayBitmap = NULL;
}


const char *osd_get_fps_text(const performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;

	// display the FPS, frameskip, percent, fps and target fps
	dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
			autoframeskip ? "auto" : "fskp", frameskip,
			(int)(performance->game_speed_percent + 0.5),
			(int)(performance->frames_per_second + 0.5),
			(int)(Machine->drv->frames_per_second + 0.5));

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
}

#pragma mark -
#pragma mark ¥ Palette Management

//===============================================================================
//	osd_allocate_colors
//
//	Sets up our color usage, given the total number of colors, and an 
//	array of pens
//===============================================================================

static int mac_allocate_colors(UINT32 *rgb_components)
{
	Boolean result;

	// no colortable and no lookup
	gDisplay.obsolete = NULL;
	gDisplay.lookup16 = NULL;
	gDisplay.lookup32 = NULL;

	if (gDisplay.depth == 16)
	{
		if (sScreenAttributes & VIDEO_RGB_DIRECT)
		{
			result = CreatePalette15(rgb_components);
		}
		else
		{
			result = CreatePalette16(rgb_components);
		}
#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
		result |= CreateDebugPalette16();
#endif
	}
	else
	{
		if (sScreenAttributes & VIDEO_RGB_DIRECT)
		{
			result = CreatePalette32d(rgb_components);
		}
		else
		{
			result = CreatePalette32(rgb_components);
		}
#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
		result |= CreateDebugPalette32();
#endif
	}
	
	// if we didn't succeed, return an error
	if (!result)
		return 1;
	
	// of course, at this time we consider the palette to be dirty
	gDisplay.paletteDirty = true;
	
	// initialize the plugin's palette
	UpdateDisplayState(NULL);
	(*sPlugin->desc.initializePalette)(&gDisplay);

	return 0;
}

static void update_palette(mame_display *display)
{
	UINT32 i;

	for (i=0; i<display->game_palette_entries; i++)
	{
		if (display->game_palette_dirty[i>>5] & (1 << (i & 31)))
		{
			UInt8 *rawdata = &sRawPalette[i * 3];

			// modify the raw palette
			*rawdata++	= display->game_palette[i] >> 16;
			*rawdata++	= display->game_palette[i] >> 8;
			*rawdata++	= display->game_palette[i];

			display->game_palette_dirty[i>>5] &= ~(1 << (i & 31));	/* right??? */

			gDisplay.paletteDirty = true;
		}
	}
}


//===============================================================================
//	AdjustPalette
//
//	Update a dirty palette
//===============================================================================

static void AdjustPalette(int game_palette_entries)
{
	UInt8 *rawdata = sRawPalette;
	int i;

	// If it's direct RGB, we don't do any palette stuff
	if (sScreenAttributes & VIDEO_RGB_DIRECT) return;

	// note: as of 0.87 the old mac-specific gamma table is no longer used; the mac gamma items
	// now go directly to the core. This means that gamma is not adjustable in vector games.
	// (in 0.77u2a vector gamma was adjustable by the mac-specific menus, but not by the ~ menu.
	// To fix this, palette_set_global_gamma in the core could be patched to call vector_set_gamma.)

	// update the palette for 16-bit
	if (gDisplay.depth == 16)
	{
		UInt32 *lookup16 = gDisplay.lookup16;
		for (i = 0; i < game_palette_entries; i++)
		{
			UInt32 red 		= *rawdata++;
			UInt32 green	= *rawdata++;
			UInt32 blue 	= *rawdata++;
			UInt32 value 	= ((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3);
			*lookup16++ = (value << 16) | value;
		}
	}
	// update the palette for 32-bit
	else if ((gDisplay.depth == 32) && !(sScreenAttributes & VIDEO_RGB_DIRECT))
	{
		UInt32 *lookup32 = gDisplay.lookup32;
		for (i = 0; i < game_palette_entries; i++)
		{
			UInt32 red 		= *rawdata++;
			UInt32 green	= *rawdata++;
			UInt32 blue 	= *rawdata++;
			*lookup32++ = (red << 16) | (green << 8) | blue;
			*lookup32++ = (red << 16) | (green << 8) | blue;
		}
	}
}


//===============================================================================
//	CreatePalette15
//
//	Allocate the machine's initial palette and set the colors
//===============================================================================

static Boolean CreatePalette15(UINT32 *rgb_components)
{
	// Tell the core we're using 555
	rgb_components[0] = 0x7c00;
	rgb_components[1] = 0x03e0;
	rgb_components[2] = 0x001f;

	return true;
}

//===============================================================================
//	CreatePalette16
//
//	Allocate the machine's initial palette and set the colors
//===============================================================================

static Boolean CreatePalette16(UINT32 *rgb_components)
{
	int i;
	
	// allocate the 16-bit lookup table
	gDisplay.lookup16 = malloc(sizeof(UInt32) /** 2*/ * kMaxPaletteSize);
	if (!gDisplay.lookup16)
		return false;
	
	// Clear the palette to black
	for (i = 0; i < kMaxPaletteSize; i ++)
		sRawPalette[3*i+0] = sRawPalette[3*i+1] = sRawPalette[3*i+2] = 0;

	return true;
}
	
//===============================================================================
//	CreatePalette32
//
//	Allocate the machine's initial palette and set the colors
//===============================================================================

static Boolean CreatePalette32(UINT32 *rgb_components)
{
	int i;
	
	// allocate the 32-bit lookup table
	gDisplay.lookup32 = malloc(sizeof(UInt32) * 2 * kMaxPaletteSize);
	if (!gDisplay.lookup32)
		return false;
	
	// Clear the palette to black
	for (i = 0; i < kMaxPaletteSize; i ++)
		sRawPalette[3*i+0] = sRawPalette[3*i+1] = sRawPalette[3*i+2] = 0;

	return true;
}
	
//===============================================================================
//	CreatePalette32d
//
//	Allocate the machine's initial palette and set the colors
//===============================================================================

static Boolean CreatePalette32d(UINT32 *rgb_components)
{
	// Tell the core we're using 888
	rgb_components[0] = 0x00ff0000;
	rgb_components[1] = 0x0000ff00;
	rgb_components[2] = 0x000000ff;

	return true;
}
	

#pragma mark -
#pragma mark ¥ Dirty Handling

//===============================================================================
//	osd_mark_dirty
//
//	Adds a dirty rectangle to the dirty rect system
//===============================================================================

static void mac_mark_dirty(int x, int y)
{
	int ymask;
	// don't bother if we're not a vector game or we don't support dirties
	/*if (!(sScreenAttributes & VIDEO_TYPE_VECTOR))
		return;*/

	// adjust relative to the visible area
	x -= gDisplay.visxoffset;
	y -= gDisplay.visyoffset;

	// clip the rect to the screen boundaries
	if ((x < 0) || (x > gDisplay.viswidth))
		return;
	if ((y < 0) || (y > gDisplay.visheight))
		return;

	// manage rotation
	if (gDisplay.swapxy)
	{
		int temp;

		temp = x;
		x = y;
		y = temp;
	}

	if (gDisplay.flipx)
	{
		if (gDisplay.swapxy)
			x = gDisplay.visheight - x;
		else
			x = gDisplay.viswidth - x;
	}

	if (gDisplay.flipy)
	{
		if (gDisplay.swapxy)
			y = gDisplay.viswidth - y;
		else
			y = gDisplay.visheight - y;
	}

	// get down to the nitty-gritty
	x >>= DIRTY_X_SHIFT;
	ymask = (0x01 << (y & DIRTY_Y_MASK)) & 0xff;
	y >>= DIRTY_Y_SHIFT;

	// dirty one point
	gDisplay.curdirty[(y << gDisplay.dirtyrowshift) + x] |= ymask;
}


//============================================================
//  check_inputs
//============================================================

static void check_inputs(void)
{
	// increment frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		// if autoframeskip, disable auto and go to 0
		if (autoframeskip)
		{
			SetAutoFrameskip (false);
			SetFrameskip (0);
		}

		// wrap from maximum to auto
		else if (frameskip == kFrameskipLevels - 1)
		{
			SetFrameskip (0);
			SetAutoFrameskip (true);
		}

		// else just increment
		else
			frameskip++;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (autoframeskip)
		{
			SetAutoFrameskip (false);
			frameskip = kFrameskipLevels-1;
		}

		// wrap from 0 to auto
		else if (frameskip == 0)
			SetAutoFrameskip (true);

		// else just decrement
		else
			frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		SetThrottling (!gPrefs.speedThrottle);
	}
	
	// Update the mouse, joystick and keyboard states.
	PollInputs();
}

//===============================================================================
//	osd_skip_this_frame
//
//	Returns true if we want to skip rendering this frame
//===============================================================================

int osd_skip_this_frame(void)
{
	if (gEmulationPaused) return 0;

	if (sFastForwarding)
		return (sSkipFrameCount != 0);
	else
		return sSkipTable[frameskip][sSkipFrameCount];
}


//===============================================================================
//	osd_update_video_and_audio
//
//	Update the display and handle any sound updates
//===============================================================================

void osd_update_video_and_audio(mame_display *display)
{
	Boolean skipping = osd_skip_this_frame();
	UInt32 currentFrameTime;
	Boolean newFastForward;

#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
	// if the debugger focus changed, update it
	if (display->changed_flags & DEBUG_FOCUS_CHANGED)
		mac_set_debugger_focus(display->debug_focus);
#endif

	// if the game palette has changed, update it
	if (display->changed_flags & GAME_PALETTE_CHANGED)
	{
		//assert(display->game_palette_entries <= kMaxPaletteSize);
		update_palette(display);
	}


	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
		mac_set_visible_area(display->game_visible_area.min_x, display->game_visible_area.max_x, display->game_visible_area.min_y, display->game_visible_area.max_y);

	if ((sScreenAttributes & VIDEO_TYPE_VECTOR) && (sPlugin->desc.identifier != 'OGL '))
	{
		gDisplay.full = false;
		if (display->changed_flags & VECTOR_PIXELS_CHANGED)
		{
			vector_pixel_t *list;

			memset(sDirtyBuffer, 0, sDirtyBufferSize);

			for (list = display->vector_dirty_pixels; (*list) != VECTOR_PIXEL_END; list++)
			{
				mac_mark_dirty(VECTOR_PIXEL_X(*list), VECTOR_PIXEL_Y(*list));
			}
		}
		else
		{
			gDisplay.full = true;
		}
	}


	if (display->changed_flags & GAME_BITMAP_CHANGED)
		UpdateBitmap(display->game_bitmap);
	
	// handle events, calling WaitNextEvent if we're still settling
	ProcessEvents(TickCount() < sLastDisplayChangeTicks + kDisplaySettleTicks);

#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
	if (display->changed_flags & DEBUG_PALETTE_CHANGED)
		update_debug_palette(display->debug_palette);

	// If the debugger handled the update, bail now
	if (display->changed_flags & DEBUG_BITMAP_CHANGED)
		/*(void) DrawDebugger (display->debug_bitmap);*/
		if (DrawDebugger (display->debug_bitmap)) return;
#endif

	// if the LEDs have changed, update them
	if (display->changed_flags & LED_STATE_CHANGED)
		UpdateLEDs (display->led_state);

	// check for inputs
	check_inputs();

	// keep track of total elapsed frames, displayed or not
	sFPSElapsedFrames[0]++;
	
	// redraw the screen if this isn't a frame to skip
	currentFrameTime = GetMicroseconds();
	if ((!skipping) && (display->changed_flags & GAME_BITMAP_CHANGED))
	{
		// keep track of how much time it took to process this displayed frame
		sDrawnFrames++;
		sDrawnFramesTime += currentFrameTime - sStartFrameTime;

		// blit the current video buffer to the screen
		if (display->changed_flags & GAME_BITMAP_CHANGED)
		{
			UInt32 beforeBlitTime = currentFrameTime;
			currentFrameTime = RedrawVideo(currentFrameTime, display->game_bitmap, display->game_palette_entries);
			sBlitTime += currentFrameTime - beforeBlitTime;
		}

		// throttle now if appropriate
		if (gPrefs.speedThrottle && !sFastForwarding)
		{
			UInt32 beforeIdleTime = currentFrameTime;
			currentFrameTime = SpeedThrottle(currentFrameTime);
			sIdleTime += currentFrameTime - beforeIdleTime;

			// if we've gone through an entire sequence of skipping, time to update the auto frameskipping
			if (autoframeskip && sSkipFrameCount == 0)
				currentFrameTime = TweakFrameskip(currentFrameTime);
		}

		// update the total accumulated time
		sFPSTotalElapsedTime += currentFrameTime - sPreviousDisplayTime;
		
		// remember the last time we blitted, for speed throttling
		sPreviousDisplayTime = currentFrameTime;
	}
	else
	{
		// keep track of how much time it took to process this skipped frame
		sSkippedFrames++;
		sSkippedFramesTime += currentFrameTime - sStartFrameTime;
	}

	// update the frameskip counters
	if (++sSkipFrameCount >= kFrameskipLevels)
		sSkipFrameCount = 0;
	
	// if we've skipped the first few garbage frames, set the start time for the average calculations
	if (++sFramesDisplayed == kSettleFramesBeforeTiming)
		sFPSTotalElapsedTime = 0;

	// the next frame effectively starts now
	sStartFrameTime = currentFrameTime;
	
	// update the fast-forwarding state
	newFastForward = (gLastEvent.modifiers & (optionKey | cmdKey)) == (optionKey | cmdKey);
	if (newFastForward != sFastForwarding)
	{
		sFastForwarding = newFastForward;
		ResetFrameCount();
		SyncSound();
	}
}
		

mame_bitmap * mac_screenshot_create_flipped (mame_bitmap *bitmap, const rectangle *bounds, rectangle *newbounds)
{
	mame_bitmap *copy;
	int x, y, w, h, t;

	// allocate a copy
	w = gSwapxy ? bitmap->height : bitmap->width;
	h = gSwapxy ? bitmap->width : bitmap->height;
	copy = bitmap_alloc_depth(w, h, bitmap->depth);
	if (!copy)
		return NULL;

	// populate the copy
	for (y = bounds->min_y; y <= bounds->max_y; y++)
		for (x = bounds->min_x; x <= bounds->max_x; x++)
		{
			int tx = x, ty = y;

			// apply the rotation/flipping
			if (gSwapxy)
			{
				t = tx; tx = ty; ty = t;
			}
			if (gFlipx)
				tx = copy->width - tx - 1;
			if (gFlipy)
				ty = copy->height - ty - 1;

			// read the old pixel and copy to the new location
			switch (copy->depth)
			{
				case 15:
				case 16:
					*((UINT16 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT16 *)bitmap->base + y * bitmap->rowpixels + x);
					break;

				case 32:
					*((UINT32 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT32 *)bitmap->base + y * bitmap->rowpixels + x);
					break;
			}
		}

	// compute the oriented bounds
	*newbounds = *bounds;

	// apply X/Y swap first
	if (gSwapxy)
	{
		t = newbounds->min_x; newbounds->min_x = newbounds->min_y; newbounds->min_y = t;
		t = newbounds->max_x; newbounds->max_x = newbounds->max_y; newbounds->max_y = t;
	}

	// apply X flip
	if (gFlipx)
	{
		t = copy->width - newbounds->min_x - 1;
		newbounds->min_x = copy->width - newbounds->max_x - 1;
		newbounds->max_x = t;
	}

	// apply Y flip
	if (gFlipy)
	{
		t = copy->height - newbounds->min_y - 1;
		newbounds->min_y = copy->height - newbounds->max_y - 1;
		newbounds->max_y = t;
	}

	return copy;
}

mame_bitmap *osd_override_snapshot(mame_bitmap *bitmap, rectangle *bounds)
{
	mame_bitmap *copy;

	// if we can send it in raw, do it
	if (!gSwapxy && !gFlipx && !gFlipy)
	{
		return NULL;
	}

	copy = mac_screenshot_create_flipped (bitmap, bounds, bounds);
	return copy;
}

#pragma mark -
#pragma mark ¥ Plug-In Management

//===============================================================================
//	InitializePlugins
//
//	Searches for and initializes all the plugins.
//===============================================================================

void InitializePlugins(void)
{
	PluginData *plugin;

	// reset the plugin count	
	sPluginCount = 0;

	// first create and add the software plugin
	plugin = &sPluginList[sPluginCount++];
	memset(&plugin->spec, 0, sizeof(plugin->spec));
	plugin->connection = 0;
	plugin->desc.version = VIDEOAPI_VERSION;
	plugin->enabled = Software_GetDisplayDescription(&plugin->desc);

	// make this the default
	sPlugin = sSoftwarePlugin = plugin;
	
	// Now add the OpenGL plugin
	plugin = &sPluginList[sPluginCount++];
	memset(&plugin->spec, 0, sizeof(plugin->spec));
	plugin->connection = 0;
	plugin->desc.version = VIDEOAPI_VERSION;
	plugin->enabled = OpenGL_GetDisplayDescription(&plugin->desc);

	// set the active plugin based on the preferences
	SetActivePlugin(gPrefs.plugin);
}


// obsoleted, video plugins are now integrated. dyld is dumb about stripping unused code, so...
#if 0
//===============================================================================
//	PreparePlugin
//
//	Loads the plugin and prepares it for execution.
//===============================================================================

static void PreparePlugin(const FSSpec *inFilespec)
{
	CFragConnectionID connectionID;
	DisplayDescription description;
	GetDescriptionProc getDesc;
	PluginData *data;
	Str255 errorName;
	Boolean result;
	OSErr err;
	
	// attempt to load the fragment
	err = GetDiskFragment(inFilespec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, 
							&connectionID, (Ptr *)&getDesc, errorName);
	if (err != noErr)
	{
		char pluginName[256], errorFragment[256];
		p2cstrcpy(pluginName, inFilespec->name);
		p2cstrcpy(errorFragment, errorName);

		// If we're running Carbon MacMAME, don't warn about InterfaceLib plugins
		if (!strstr (errorFragment, "InterfaceLib"))
			printf("Unable to load the plug-in Ò%sÓ because an error occurred (#%d) attempting to load the fragment %s.", pluginName, err, errorFragment);
		return;
	}
	
	// now call through the main address
	memset(&description, 0, sizeof(description));
	description.version = VIDEOAPI_VERSION;
	description.sixteenbit = true;
	result = (*getDesc)(&description);
	
	// fill in this new entry
	data = &sPluginList[sPluginCount++];
	data->spec = *inFilespec;
	data->connection = connectionID;
	data->desc = description;
	data->enabled = true;

	// determine if this plugin should be enabled
	if (!result || description.version > VIDEOAPI_VERSION)
	{
		data->enabled = false;
		
		// copy the short filename
		if (data->desc.shortName == NULL)
			data->desc.shortName = PLstrcpy(malloc(inFilespec->name[0] + 1), inFilespec->name);
		else
			data->desc.shortName = PLstrcpy(malloc(data->desc.shortName[0] + 1), data->desc.shortName);

		// copy the long filename
		if (data->desc.longName == NULL)
			data->desc.longName = PLstrcpy(malloc(inFilespec->name[0] + 1), inFilespec->name);
		else
			data->desc.longName = PLstrcpy(malloc(data->desc.longName[0] + 1), data->desc.longName);

		// close our connection to the lib
		CloseConnection(&connectionID);
	}
}
#endif


//===============================================================================
//	GetIndexedPlugin
//
//	Index is zero-based. Returns the identifier, short and long names of the
//	plug-in.
//===============================================================================

UInt32 GetIndexedPlugin(UInt32 inIndex, StringPtr outShortName, StringPtr outLongName, Boolean *outEnabled)
{
	const PluginData *data;
	
	// first verify that the plugins go up this high
	if (inIndex >= sPluginCount)
		return 0;
	data = &sPluginList[inIndex];
	
	// copy the data requested
	if (outShortName != NULL)
		PLstrcpy(outShortName, data->desc.shortName);
	if (outLongName != NULL)
		PLstrcpy(outShortName, data->desc.longName);
	if (outEnabled != NULL)
		*outEnabled = data->enabled;

	return data->desc.identifier;
}


//===============================================================================
//	SetActivePlugin
//
//	Sets the plugin based on the identifier.
//===============================================================================

void SetActivePlugin(UInt32 inIdentifier)
{
	const PluginData *data = sPluginList;
	int i;
	
	// find the plugin with this identifier
	for (i = 0; i < sPluginCount; i++, data++)
		if (data->desc.identifier == inIdentifier)
			break;
	
	// bail if we didn't find it, or if it's not enabled
	if (i == sPluginCount || !data->enabled)
		return;

	// set the active plugin
	sPlugin = data;
	gPrefs.plugin = inIdentifier;
}


//===============================================================================
//	GetActivePlugin
//
//	Returns the identifier of the currently active plugin.
//===============================================================================

UInt32 GetActivePlugin(void)
{
	return sPlugin->desc.identifier;
}


//===============================================================================
//	GetActivePluginDescription
//
//	Returns a read-only copy of the active plugin's description.
//===============================================================================

const DisplayDescription *GetActivePluginDescription(void)
{
	return &sPlugin->desc;
}


//===============================================================================
//	PluginHasConfigDialog
//
//	Returns true if the plugin supports a configuration dialog.
//===============================================================================

Boolean PluginHasConfigDialog(UInt32 inIdentifier)
{
	const PluginData *data = sPluginList;
	int i;
	
	// find the plugin with this identifier
	for (i = 0; i < sPluginCount; i++, data++)
		if (data->desc.identifier == inIdentifier)
			break;
	
	// bail if we didn't find it, or if it's not enabled
	if (i == sPluginCount || !data->enabled)
		return false;

	// bail if the plugin doesn't have a config proc
	return (data->desc.configure != NULL);
}


//===============================================================================
//	ConfigurePlugin
//
//	Display a configuration dialog for the specified plugin.
//===============================================================================

void ConfigurePlugin(UInt32 inIdentifier)
{
	const PluginData *data = sPluginList;
	SInt16 resourceRef;
	int i;
	
	// find the plugin with this identifier
	for (i = 0; i < sPluginCount; i++, data++)
		if (data->desc.identifier == inIdentifier)
			break;
	
	// bail if we didn't find it, or if it's not enabled
	if (i == sPluginCount || !data->enabled)
		return;

	// bail if the plugin doesn't have a config proc
	if (data->desc.configure == NULL)
		return;
	
	// change the resource
	resourceRef = FSpOpenResFile(&data->spec, fsRdPerm);
	(*data->desc.configure)();
	if (resourceRef != -1)
		CloseResFile(resourceRef);
}



//===============================================================================
//	PluginHasAboutBox
//
//	Returns true if the plugin supports an about box.
//===============================================================================

Boolean PluginHasAboutBox(UInt32 inIdentifier)
{
	const PluginData *data = sPluginList;
	int i;
	
	// find the plugin with this identifier
	for (i = 0; i < sPluginCount; i++, data++)
		if (data->desc.identifier == inIdentifier)
			break;
	
	// bail if we didn't find it, or if it's not enabled
	if (i == sPluginCount || !data->enabled)
		return false;

	// bail if the plugin doesn't have a config proc
	return (data->desc.displayAboutBox != NULL);
}


//===============================================================================
//	DisplayPluginAboutBox
//
//	Display an about box for the specified plugin.
//===============================================================================

void DisplayPluginAboutBox(UInt32 inIdentifier)
{
	const PluginData *data = sPluginList;
	SInt16 resourceRef;
	int i;
	
	// find the plugin with this identifier
	for (i = 0; i < sPluginCount; i++, data++)
		if (data->desc.identifier == inIdentifier)
			break;
	
	// bail if we didn't find it, or if it's not enabled
	if (i == sPluginCount || !data->enabled)
		return;

	// bail if the plugin doesn't have a config proc
	if (data->desc.displayAboutBox == NULL)
		return;
	
	// change the resource
	resourceRef = FSpOpenResFile(&data->spec, fsRdPerm);
	(*data->desc.displayAboutBox)();
	if (resourceRef != -1)
		CloseResFile(resourceRef);
}


#pragma mark -
#pragma mark ¥ GWorld & Window Management

//===============================================================================
//	CreateGWorldBitmap
//
//	Allocate a bitmap for MAME to use, whose contents are actually a GWorld
//===============================================================================

static Boolean CreateGWorldBitmap(UInt32 inWidth, UInt32 inHeight, UInt32 inDepth, CTabHandle inColorTable)
{
	Rect bounds;
	int i;
	
	// create a Mac rect to describe the bitmap
	bounds.left = bounds.top = 0;
	bounds.right = inWidth;
	bounds.bottom = inHeight + 2 * kBitmapSlop;
	
	// allocate a GWorld
	OSErr err = NewGWorld(&sScreenGWorld, inDepth, &bounds, inColorTable, NULL, 0);
	if (err || !sScreenGWorld)
	{
		printf("Error allocating offscreen bitmap");
		return false;
	}

	// round up the width and height to a dirty rect grid rectangle
	inWidth = (inWidth + DIRTY_X_MASK) & ~DIRTY_X_MASK;
	inHeight = (inHeight + DIRTY_Y_MASK) & ~DIRTY_Y_MASK;
	
	// allocate dirty buffers if they're supported
	if (sScreenAttributes & VIDEO_TYPE_VECTOR)
	{
		// determine the size of the dirty buffer
		gDisplay.dirtyrowshift = 0;
		for (i = (inWidth - 1) >> DIRTY_X_SHIFT; i; i >>= 1)
			gDisplay.dirtyrowshift += 1;
		sDirtyBufferSize = (1 << gDisplay.dirtyrowshift) * (inHeight >> DIRTY_Y_SHIFT);
		
		//if (inDepth == 32) sDirtyBufferSize *= 2;
		
		// allocate the dirty buffer
		sDirtyBuffer = malloc(sDirtyBufferSize);
		if (!sDirtyBuffer)
			return false;

		// dirty the whole buffer
		memset(sDirtyBuffer, 0xff, sDirtyBufferSize);
	}

	return true;
}


//===============================================================================
//	CreateGameWindow
//
//	Creates the game window and sets up the font information for it
//===============================================================================

static int CreateGameWindow(void)
{
	Rect windowBounds = { 0, 0, 10, 10 };
	Str255 windowName;
	FontInfo info;
	short id;
	
	// make sure we have an updated display state
	UpdateDisplayState(NULL);

	// make the window name
	GetIndString(windowName, rStrings, kWindowName);
	strcpy((char *)windowName + 7, Machine->gamedrv->description);
	windowName[0] = strlen((char *)windowName + 1);
	
	// create the window
	{
		OSStatus err;
		
		err = CreateNewWindow (kDocumentWindowClass, kWindowCloseBoxAttribute, &windowBounds, &gDisplay.window);
		if (err) return 0;
		SetWTitle (gDisplay.window, windowName);

		// Install a constrain handler to keep it in place
		InstallCarbonWindowEvents (gDisplay.window);

		// Attach the keyboard handler to the main window
		InitializeKeyboardEvents(gDisplay.window);
	}

	// point to the window
	SetGWorld(GetWindowPort(gDisplay.window), NULL);
	
	// set the font to monaco 9pt and get the height
	GetFNum("\pMonaco", &id);
	TextFont(id);
	TextSize(9);
	GetFontInfo(&info);
	
	return 1;
}

static void UpdateBitmap (mame_bitmap *inBitmap)
{
	// Is this the first time?
	if (sCurrentDisplayBitmap == NULL)
	{
		// yes, adjust the window size
		AdjustWindowSize(inBitmap);
	}
	
	// allocate a delta buffer if we're not a dirty rect game
	if (!(sScreenAttributes & VIDEO_TYPE_VECTOR))
	{
		// If the bitmap has changed, dispose of the prior data and create
		// a new delta buffer
		if (inBitmap != sCurrentDisplayBitmap)
		{
			// use the same rowbytes as the bitmap
			UInt32 rowbytes = (UInt8 *) inBitmap->line[1] - (UInt8 *) inBitmap->line[0];
		
			if (gDisplay.deltabits)
				free (gDisplay.deltabits);

			// allocate the memory
			gDisplay.deltabits = malloc(rowbytes * (inBitmap->height + 16));
			if (!gDisplay.deltabits)
				return;
		}
	}

	// note the last bitmap we were passed
	sCurrentDisplayBitmap = inBitmap;
}

//===============================================================================
//	AdjustWindowSize
//
//	Updates the window size and x,y offsets depending on the state of the
//	preferences
//===============================================================================

static void AdjustWindowSize(mame_bitmap *inBitmap)
{
	int diffDesktop, diffInterlace, diffScale;
	int hideDesktop2, hideMenubar;
	UInt32 scaledwidth, scaledheight;

	// bail if there's no bitmap
	if (!inBitmap)
		return;
	
	// redetermine the screen attributes
	UpdateDisplayState(inBitmap);

	// ensure we're pointing at the window
	SetGWorld(GetWindowPort(gDisplay.window), NULL);

	// determine if we're hiding the desktop and/or menubar
	hideDesktop2 = gDisplay.fullscreen && !sVideoSuspended;
	hideMenubar = hideDesktop2 && !gEmulationPaused;

	// now see if anything important is different from the previous frame
	diffDesktop = hideDesktop2 ^ sLastHideDesktop;
	diffScale = gDisplay.scale ^ sLastScale;
	diffInterlace = gDisplay.interlace ^ sLastInterlace;
	if ((*gDisplay.device)->gdRect.right != sLastDisplayBounds.right ||
		(*gDisplay.device)->gdRect.bottom != sLastDisplayBounds.bottom ||
		(*gDisplay.device)->gdRect.left != sLastDisplayBounds.left ||
		(*gDisplay.device)->gdRect.top != sLastDisplayBounds.top) diffScale = 1;
	
	// remember the current values
	sLastHideDesktop = hideDesktop2;
	sLastScale = gDisplay.scale;
	sLastInterlace = gDisplay.interlace;
	sLastDisplayBounds = (*gDisplay.device)->gdRect;
	
	// set the menubar state (these are no-ops if the menu is already in the right state)
	if (hideMenubar)
		HideMenubar();
	else
		ShowMenubar();
	
	// recompute the scaled width/height
	(*sPlugin->desc.computeDisplayArea)(&gDisplay, &scaledwidth, &scaledheight, NULL);

	// did anything change that would affect the window size?
	if (diffDesktop | diffScale)
	{
		// if we're going full screen...
		if (hideDesktop2)
		{
			Rect gray, portRect;
			GetRegionBounds(GetGrayRgn(), &gray);
			// always ignore the menu bar. otherwise, fullscreen window is displaced if changing video scale when paused.
			if (gray.top == GetMBarHeight()) gray.top = 0;

			// size the window to cover all screens
			MoveWindow(gDisplay.window, gray.left, gray.top, true);
			SizeWindow(gDisplay.window, gray.right - gray.left, gray.bottom - gray.top, true);
			ClipRect(GetWindowPortBounds(gDisplay.window, &portRect));
		}
	
		// else we're just a plain ol' window
		else
			PositionWindowOnscreen(scaledwidth, scaledheight);
			
		// mark the display changed
		SetDisplayChanged();
	}

	// if anything visual changed, erase to black
	if (diffScale | diffInterlace | diffDesktop)
	{
		Rect portRect;
		
		// reshow the window if it was hidden
		ShowWindow(gDisplay.window);

		// clear it to black
		ForeColor(blackColor);
		BackColor(whiteColor);
		PaintRect(GetWindowPortBounds(gDisplay.window, &portRect));
		FlushPortImmediate(GetWindowPort(gDisplay.window));

		// force a full update
		gDisplay.full = true;
	}
}


//===============================================================================
//	PositionWindowOnscreen
//
//	Positions the window onscreen, making sure it fits entirely on one screen.
//===============================================================================

static void PositionWindowOnscreen(UInt32 inWidth, UInt32 inHeight)
{
	Rect 		newBounds;
	Rect 		portRect;
	

	// determine the new bounds of the window
	newBounds.left = gPrefs.windowLocation.h;
	newBounds.right = newBounds.left + inWidth;
	newBounds.top = gPrefs.windowLocation.v;
	newBounds.bottom = newBounds.top + inHeight;

	// adjust bounds
	AlignMainWindow(& newBounds);

	// size the window
	MoveWindow(gDisplay.window, newBounds.left, newBounds.top, true);
	SizeWindow(gDisplay.window, inWidth, inHeight, true);
	ClipRect(GetWindowPortBounds(gDisplay.window, &portRect));
}


//===============================================================================
//	AlignMainWindow
//
//	Positions the window onscreen, making sure it fits entirely on one screen.
//===============================================================================

void AlignMainWindow(Rect *boundsToFix)
{
	GDHandle	mostDevice = NULL;
	UInt32		mostPixels = 0;
	GDHandle	device;
	Rect 		newBounds;
	Rect 		portRect;
	Rect 		deviceRect;
	UInt32		widthSlop = 0;
	UInt32		heightSlop = 0;
	UInt32		inWidth = boundsToFix->right - boundsToFix->left;
	UInt32		inHeight = boundsToFix->bottom - boundsToFix->top;


	newBounds = *boundsToFix;

	// find the device containing the most pixels
	for (device = GetDeviceList(); device != NULL; device = GetNextDevice(device))
		if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
		{
			UInt32 		pixels;
			
			// if this device rect intersects the proposed window rect, compute the number of pixels
			deviceRect = (*device)->gdRect;
			if (SectRect(&deviceRect, &newBounds, &portRect))
			{
				pixels = (portRect.right - portRect.left) * (portRect.bottom - portRect.top);
				if (pixels > mostPixels)
				{
					mostPixels = pixels;
					mostDevice = device;
				}
			}
		}
	
	// if no devices, use the main
	if (mostDevice == NULL)
		mostDevice = GetMainDevice();
	deviceRect = (*mostDevice)->gdRect;
	
	// adjust for the menubar and window title
	deviceRect.top += GetMBarHeight();
	if (mostDevice == GetMainDevice())
		deviceRect.top += GetMBarHeight();
	
	// determine where we have wiggle room
	if (deviceRect.right - deviceRect.left >= inWidth + 16)
		widthSlop = 8;
	else if (deviceRect.right - deviceRect.left >= inWidth)
		widthSlop = (deviceRect.right - deviceRect.left - inWidth) / 2;
	if (deviceRect.bottom - deviceRect.top >= inHeight + 16)
		heightSlop = 8;
	else if (deviceRect.bottom - deviceRect.top >= inHeight)
		heightSlop = (deviceRect.bottom - deviceRect.top - inHeight) / 2;

	// apply the adjustments
	deviceRect.left += widthSlop;
	deviceRect.right -= widthSlop;
	deviceRect.top += heightSlop;
	deviceRect.bottom -= heightSlop;
	
	// if we're offscreen, nudge the window
	if (newBounds.right > deviceRect.right)
		OffsetRect(&newBounds, deviceRect.right - newBounds.right, 0);
	if (newBounds.bottom > deviceRect.bottom)
		OffsetRect(&newBounds, 0, deviceRect.bottom - newBounds.bottom);
	if (newBounds.left < deviceRect.left)
		OffsetRect(&newBounds, deviceRect.left - newBounds.left, 0);
	if (newBounds.top < deviceRect.top)
		OffsetRect(&newBounds, 0, deviceRect.top - newBounds.top);

	// size the window
	{
		int align_offset = 0;
		int new_leftBound;

		// skip this tweak; it actually BREAKS alignment when the game has an odd height and is rotated, like Capcom Bowling.
		// the following window align to 8 pixels (16 or 32 bytes) will ensure altivec alignment in all cases.
		/*
		if (GetActivePluginDescription()->alignmentRule == alignDestOnSrc)
		{	// we want to align on 16-byte boundary
			// (non-altivec code would work fine with 8 bytes or 4 bytes)
			if (gDisplay.depth == 32)
			{
				align_offset = (gDisplay.flipx ? gDisplay.visxoffset+gDisplay.viswidth : -gDisplay.visxoffset) & 3;
			}
			else if (gDisplay.depth == 16)
			{
				align_offset = (gDisplay.flipx ? gDisplay.visxoffset+gDisplay.viswidth : -gDisplay.visxoffset) & 7;
			}
		}
		align_offset = align_offset * (gDisplay.scale >> 8);
		align_offset = (-align_offset) & 7;
		*/
		
		new_leftBound = (*mostDevice)->gdRect.left + ((newBounds.left - (*mostDevice)->gdRect.left) & ~7) + align_offset;
		OffsetRect(&newBounds, new_leftBound - newBounds.left, 0);
	}

	*boundsToFix = newBounds;
}

//===============================================================================
//	ShowMenubar
//
//	Ensures the state of the menubar is visible
//===============================================================================

void ShowMenubar(void)
{
//	OSErr err;
	
	// if we're already visible, bail
	if (!sMenubarHidden)
		return;
	
	ShowMenuBar();

	// redraw the menubar
	DrawMenuBar();

	// the menu is no longer hidden
	sMenubarHidden = false;

	// remember the last display change
	SetDisplayChanged();
}


//===============================================================================
//	HideMenubar
//
//	Ensures the state of the menubar is not visible
//===============================================================================

void HideMenubar(void)
{
//	OSErr err;
	
	// if we're already invisible, bail
	if (sMenubarHidden)
		return;
	
	HideMenuBar();
	// the menu is officially hidden
	sMenubarHidden = true;

	// remember the last display change
	SetDisplayChanged();
}


//===============================================================================
//	MenuBarVisible
//
//	Returns true if the menubar is visible
//===============================================================================

Boolean MenuBarVisible(void)
{
	return !sMenubarHidden;
}

//===============================================================================
// appWindowEventHandler
//
// This event handler is installed by InitCarbonEvents (below) to prevent
// Carbon from moving our windows when they're offscreen. This is most noticable
// when we've set up a blanking window that covers the entire gray region.
//===============================================================================
static pascal OSStatus appWindowEventHandler (EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
    OSStatus result = eventNotHandledErr;
	UInt32 eventKind;
	
	eventKind = GetEventKind (event);
	
//	SysBeep (0);
	switch (eventKind)
	{
		case kEventWindowConstrain:
		{
//			SysBeep (0);
			// We handled the event, eat it
			result = noErr;
			break;
		}
	}
    return result;
}

void InstallCarbonWindowEvents (WindowRef inWindow)
{
	OSStatus status;
	EventTypeSpec list[] = { {kEventClassWindow, kEventWindowConstrain } };

	// If we don't support Carbon events, bail
	if ((Ptr) InstallEventHandler == (Ptr) kUnresolvedCFragSymbolAddress) return;

	// Install an application event handler
	if (sWindowEventHandlerUPP == NULL)
	{
		sWindowEventHandlerUPP = NewEventHandlerUPP (appWindowEventHandler);
		if (!sWindowEventHandlerUPP) { SysBeep (0); return; }
	}
	
    status = InstallWindowEventHandler(inWindow, sWindowEventHandlerUPP, 1, list, 0, &sEventHandlerRef);
    if (status != noErr) SysBeep (0);
}

void DisposeCarbonWindowEvents (void)
{
	if (sEventHandlerRef == NULL) return;
	
	(void)RemoveEventHandler (sEventHandlerRef);
	sEventHandlerRef = NULL;
	
	if (sWindowEventHandlerUPP != NULL)
		DisposeEventHandlerUPP (sWindowEventHandlerUPP);
	sWindowEventHandlerUPP = NULL;
}


#pragma mark -
#pragma mark ¥ Display State Management

//===============================================================================
//	SuspendVideo
//
//	Called whenever we go into the background.
//===============================================================================

void SuspendVideo(void)
{
	// call the plugin's suspend first
	(*sPlugin->desc.suspend)(&gDisplay);
	
	// if we're full screen, hide our window and show the menubar
	if (gDisplay.fullscreen)
	{
		ShowMenubar();
		HideWindow(gDisplay.window);
	}
	
	// the video system is now suspended
	sVideoSuspended = true;
	
	// force a refresh
	UpdateScreen();
}


//===============================================================================
//	ResumeVideo
//
//	Called whenever we're brought back to the foreground.
//===============================================================================

void ResumeVideo(void)
{
	// make sure our window is visible
	ShowWindow(gDisplay.window);
	if (gDisplay.fullscreen)
		HideMenubar();

	// call the plugin's resume
	(*sPlugin->desc.resume)(&gDisplay);
	
	// the video system is now resumed
	sVideoSuspended = false;
	
	// force a refresh
	UpdateScreen();
}


//===============================================================================
//	PauseVideo
//
//	Called whenever we pause the game
//===============================================================================

void PauseVideo(Boolean inPause)
{
	if (inPause)
	{
		// call the plugin's pause routine
		if (*sPlugin->desc.pause != NULL)
			(*sPlugin->desc.pause)(&gDisplay);
	}
	else
	{
		// call the plugin's pause routine first
		if (*sPlugin->desc.unpause != NULL)
			(*sPlugin->desc.unpause)(&gDisplay);
	}
	
	// force a refresh
	UpdateScreen();
}


//===============================================================================
//	UpdateDisplayState
//
//	Updates the state of the display structure
//===============================================================================

static void UpdateDisplayState(mame_bitmap *inBitmap)
{
	// pick the device to consider
	gDisplay.device = (sPlugin->desc.device) ? sPlugin->desc.device : GetMainDevice();

	// start with the full screen rect
	sFullScreenRect = sEffectiveScreenRect = (*gDisplay.device)->gdRect;

	// if it's a window, compensate for the window title bar (assume same as menubar height)
	if (!gDisplay.fullscreen)
	{
		sEffectiveScreenRect.top += GetMBarHeight() * 2;
		
		// if we're the main screen, also account for the menubar
		if (gDisplay.device == GetMainDevice())
			sEffectiveScreenRect.top += GetMBarHeight();
	}
	
	// note: gamma and brightness are maintained in this structure

	// if we have a screen, we can reveal more
	if (inBitmap)
	{
		//UInt32 rowbytes = (UInt8 *) inBitmap->line[1] - (UInt8 *) inBitmap->line[0];
		
		// set the screen parameters
		gDisplay.bits				= inBitmap->line[0];
		gDisplay.rowbytes			= (UInt8 *) inBitmap->line[1] - (UInt8 *) inBitmap->line[0];
		
		// a dirty palette means a full update
		if (gDisplay.paletteDirty)
			gDisplay.full			= true;
	}
}


//===============================================================================
//	ComputeScalingFactor
//
//	Compute the highest scaling factor that will fit the current video
//===============================================================================

void ComputeScalingFactor(void)
{
	UInt32 tempwidth, tempheight;
	
	if (gDisplay.vector || !sPlugin->desc.scale)
	{
		// in these cases, the scale is fixed at 1.0
		gDisplay.scale = 0x100;
	}
	else
	{
		// in this case, the scale can be 1, 2 or 3
		gDisplay.scale = (gPrefs.windowScale > 2) ? 0x300 : (gPrefs.windowScale > 1) ? 0x200 : 0x100;
	}
	
	// compute the display size
	while (true)
	{
		Rect checkRect;
		
		// if we're here with a scale of 1, we'd better live with it
		if (gDisplay.scale <= 0x100)
			break;

		// if we're full screen, give some more slop for expanding
		if (gDisplay.fullscreen)
		{
			checkRect = sFullScreenRect;
			InsetRect(&checkRect, -8, -8);
		}
		else
			checkRect = sEffectiveScreenRect;

		// determine the current area
		UpdateDisplayState(NULL);
		sPlugin->desc.computeDisplayArea(&gDisplay, &tempwidth, &tempheight, NULL);
		
		// if we fit, we're done
		if (tempwidth <= checkRect.right - checkRect.left && tempheight <= checkRect.bottom - checkRect.top)
			break;

		// otherwise, bump and try again
		gDisplay.scale -= 0x100;
	}
}


#pragma mark -
#pragma mark ¥ Display Update Helpers

//===============================================================================
//	SpeedThrottle
//
//	Handles speed throttling. This code is pretty much the speed throttling
//	code from the DOS version, modified for the Mac. 
//===============================================================================

static UInt32 SpeedThrottle(UInt32 inCurrentFrameTime)
{
	UInt32 adjustedMicrosecondsPerFrame = sMicrosecondsPerFrame;
	UInt32 targetTime;
	
	// this counts as idle time
	profiler_mark(PROFILER_IDLE);
	
	// wait until enough time has passed since last frame...
	if (gPrefs.speedThrottle && autoframeskip && sIdleTimeMakeup)
		adjustedMicrosecondsPerFrame -= sIdleTimeMakeup / kFrameskipLevels;
	targetTime = sPreviousDisplayTime + sWaitTable[frameskip][sSkipFrameCount] * adjustedMicrosecondsPerFrame;

	// properly yield time to avoid MAIN FAN TURN ON.
	SInt32 naptime = targetTime - inCurrentFrameTime;
	if (naptime > 0)
		usleep((unsigned int)naptime);
	inCurrentFrameTime = GetMicroseconds();
	
	// stop counting this as idle time
	profiler_mark(PROFILER_END);
	return inCurrentFrameTime;
}
			

//===============================================================================
//	TweakFrameskip
//
//	Tweak the current frameskip level based on the speed of the emulation. This
//	algorithm is fairly complex, but its operation should be pretty 
//	straightforward to understand.
//===============================================================================

static UInt32 TweakFrameskip(UInt32 inCurrentFrameTime)
{
	UInt32 totalTakenTime, shouldHaveTakenTime, actuallyTakenTime;
	int i, new_frameskip = frameskip;
	
#if FRAMESKIP_LOG
	static FILE *skiplog = NULL;
	if (!skiplog) skiplog = fopen("skip.log", "w");
#endif

	// compute how long it should have taken for kFrameskipLevels frames
	shouldHaveTakenTime = kFrameskipLevels * sMicrosecondsPerFrame - sIdleTimeMakeup;
	
	// compute the total amount of time, including everything
	totalTakenTime = inCurrentFrameTime - sLastTweakTime;
	
	// compute how long it actually did take
	actuallyTakenTime = sSkippedFramesTime + sDrawnFramesTime + sBlitTime;
	
	// if our times aren't valid, just reset
	if (!sSkipTimesValid)
		goto ResetWithoutProcessing;
		
	// if this is the first valid blit time, use it to prime all the blit times
	if (sValidBlitTimes[0] == 0)
		for (i = 0; i < kFrameskipLevels; i++)
			sValidBlitTimes[i] = sBlitTime * (kFrameskipLevels - i) / sDrawnFrames;

	// update the valid numbers
	if (sBlitTime > sValidBlitTimes[frameskip])
		sValidBlitTimes[frameskip] = sBlitTime;
	if (sSkippedFrames != 0)
		sValidSkippedTime = sSkippedFramesTime / sSkippedFrames;
	if (sDrawnFrames != 0)
		sValidDrawnTime = sDrawnFramesTime / sDrawnFrames;
	
	// sanity check
	if (sValidDrawnTime < sValidSkippedTime)
		sValidDrawnTime = sValidSkippedTime;

	// sometimes the framerate is uneven, and our throttling will kick us over the allotted time
	sIdleTimeMakeup = 0;
	if (totalTakenTime > shouldHaveTakenTime)
	{
		// this means that we need to make up some time next batch
		sIdleTimeMakeup = totalTakenTime - shouldHaveTakenTime;

#if FRAMESKIP_LOG
		if (skiplog) fprintf(skiplog, "  (Idled too long, making up %d)\n", sIdleTimeMakeup);
#endif
	}
	
	// if we took too long, bump down the frameskip
	if (actuallyTakenTime >= shouldHaveTakenTime)
	{
		SInt32 timeToMakeUp = actuallyTakenTime - shouldHaveTakenTime;
		while (++new_frameskip <= kFrameskipLevels - 1)
		{
			SInt32 timeMadeUp;
			
			// compute how much time we would make up at this level
			timeMadeUp = (new_frameskip - frameskip) * (sValidDrawnTime - sValidSkippedTime);
			timeMadeUp += sValidBlitTimes[frameskip] - sValidBlitTimes[new_frameskip];
			
#if FRAMESKIP_LOG
			if (skiplog) fprintf(skiplog, "  (Trying %2d: Blit=%6d Need=%6d Getting=%6d)\n", 
				new_frameskip, sValidBlitTimes[new_frameskip], timeToMakeUp, timeMadeUp);
#endif
			
			// if this is enough, stop here
			if (timeMadeUp >= timeToMakeUp)
				break;
		}
	}
	
	// otherwise, see if we can bump up the frameskip
	else
	{
		SInt32 extraTimeWeHad = shouldHaveTakenTime - actuallyTakenTime;
		while (--new_frameskip >= 0)
		{
			SInt32 extraTimeTaken;
			
			// compute how much time we would make up at this level
			extraTimeTaken = (frameskip - new_frameskip) * (sValidDrawnTime - sValidSkippedTime);
			extraTimeTaken += sValidBlitTimes[new_frameskip] - sValidBlitTimes[frameskip];

#if FRAMESKIP_LOG
			if (skiplog) fprintf(skiplog, "  (Trying %2d: Blit=%6d Avail=%6d Extra=%6d)\n", 
				new_frameskip, sValidBlitTimes[new_frameskip], extraTimeWeHad, extraTimeTaken);
#endif
			
			// if this is enough, stop here
			if (extraTimeTaken >= extraTimeWeHad)
			{
				new_frameskip++;
				break;
			}
		}
	}
	
	// clamp the frameskip
	if (new_frameskip < 0)
		new_frameskip = 0;
	if (new_frameskip >= kFrameskipLevels)
		new_frameskip = kFrameskipLevels - 1;
	
#if FRAMESKIP_LOG
	if (skiplog) fprintf(skiplog, "t=%10ud  old_skip=%2d  Blit=%6d Skip=%5d Draw=%5d Idle=%6d  Should=%6d Actual=%6d Total=%6d  new_skip=%2d\n\n",
		inCurrentFrameTime, frameskip, sValidBlitTimes[frameskip], sValidSkippedTime, sValidDrawnTime, sIdleTime,
		shouldHaveTakenTime, actuallyTakenTime, totalTakenTime, new_frameskip);
#endif
	
	// adjust the existing frameskip based on the new one
	if (new_frameskip < frameskip)
		frameskip--;
	else if (new_frameskip > frameskip + 7)
		frameskip += 3;
	else if (new_frameskip > frameskip + 4)
		frameskip += 2;
	else if (new_frameskip > frameskip)
		frameskip++;
	
	// reset the timing
ResetWithoutProcessing:
	sSkippedFrames = 0;
	sDrawnFrames = 0;
	sSkippedFramesTime = 0;
	sDrawnFramesTime = 0;
	sIdleTime = 0;
	sBlitTime = 0;
	sLastTweakTime = inCurrentFrameTime;
	sSkipTimesValid = true;
	
	return inCurrentFrameTime;
}
	

//===============================================================================
//	RedrawVideo
//
//	Actually performs the drawing to the screen, after frameskipping and other
//	factors have determined that it's okay to do so.
//===============================================================================

static UInt32 RedrawVideo(UInt32 inCurrentFrameTime, mame_bitmap *inBitmap, int game_palette_entries)
{
	Boolean blitSuccessful = false;
	GDHandle oldDevice;
	GWorldPtr oldPort;
	Rect destBounds;

	// remember where we were, then point to the window
	GetGWorld(&oldPort, &oldDevice);
	SetGWorld(GetWindowPort(gDisplay.window), NULL);

	// update the window size & state (also updates the display state)
	AdjustWindowSize(inBitmap);
	
	// update the current palette and lookup tables
	if (gDisplay.paletteDirty)
		AdjustPalette(game_palette_entries);

	// determine where the blitter wants to draw
	(*sPlugin->desc.computeDisplayArea)(&gDisplay, NULL, NULL, &destBounds);

	// start counting time for the blit
	profiler_mark(PROFILER_BLIT);
	
	// handle the fast case
	if (CanBlitFast(&destBounds))
	{
		// if we just transitioned from slow to fast, and we're interlaced, erase the window
		if (sLastBlitWasSlow /*&& gDisplay.interlace*/)
		{
			Rect portRect;
			GrafPtr thePort = GetWindowPort (gDisplay.window);

			ForeColor(blackColor);
			BackColor(whiteColor);
			GetWindowPortBounds(gDisplay.window, &portRect);
			PaintRect (&portRect);

			// Flush the PaintRect before the blitter draws
			FlushPortImmediate(thePort);

			// Tell the core to redraw everything next frame. This is so any
			// renderers invalidate their dirty rects
			/*schedule_full_refresh ();
			ResetDirtyRects (true);*/
			gDisplay.full = true;
		}
		
		// attempt to blit using the plugin
		if ((gDisplay.viswidth > 0) && (gDisplay.visheight > 0))
		{
			blitSuccessful = (*sPlugin->desc.updateDisplay)(&gDisplay);
			sLastBlitWasSlow = false;
		}
	}
	
	// if we haven't managed to blit yet, do it the slow way
	if (!blitSuccessful)
	{
		// render the text over the screen
//		ui_text(inBitmap, "**", 0, Machine->uiheight - Machine->uifontheight);

		DoBlitSlow(&destBounds);
	}
		
	// stop counting blit time
	profiler_mark(PROFILER_END);

	// point back to the old port
	SetGWorld(oldPort, oldDevice);

	// swap the dirty rects and clear them
	//ResetDirtyRects(gDisplay.full);

	// palette no longer dirty
	gDisplay.paletteDirty = false;

	return GetMicroseconds();
}


//===============================================================================
//	UpdateScreen
//
//	This routine is called in response to update events. It is not intended 
//	to be called during actual gameplay.
//===============================================================================

void UpdateScreen(void)
{
	GDHandle oldDevice;
	GWorldPtr oldPort;
	Rect destBounds;
	Rect portRect;
	GrafPtr thePort;

	// don't bother if there's no window left
	if (!gDisplay.window)
		return;

	// remember where we were, then point to the window
	GetGWorld(&oldPort, &oldDevice);
	
	thePort = GetWindowPort(gDisplay.window);
	SetGWorld(thePort, NULL);

	// erase the window and redraw the menubar
	PaintRect(GetWindowPortBounds(gDisplay.window, &portRect));
	if (!sVideoSuspended && !sMenubarHidden)
		DrawMenuBar();

	// force a full update of the raster
	gDisplay.full = true;

	// update the window size & state (also updates the display state)
	AdjustWindowSize(NULL);

	// determine where the blitter wants to draw
	(*sPlugin->desc.computeDisplayArea)(&gDisplay, NULL, NULL, &destBounds);

	// do it ourself
	DoBlitSlow(&destBounds);
	
	// point back to the old port
	SetGWorld(oldPort, oldDevice);
}


//===============================================================================
//	CanBlitFast
//
//	Returns true if we are able to do a fast blit to the current destination
//	boundaries, or false if we should rely on CopyBits
//===============================================================================

static Boolean CanBlitFast(const Rect *inBounds)
{
	if (sPlugin->desc.identifier == 'OGL ')	// GL doesn't care about clipping, and will handle cross-display/GPU cases
		return true;

	{
		// The visRgn in OSX does not match the visible portion of the window,
		// and anyway the transparencies make the visRgn useless in this case,
		// so we use this trick.
		//
		// should we use kWindowOpaqueRgn instead???
		WindowRef theWindow;
		Rect displayBounds;
		Rect theBounds;
		OSStatus theErr;

		// get the bounds of our display
		(void)GetWindowPortBounds(gDisplay.window, &displayBounds);

		// check every window in front of our window
		for (theWindow = GetFrontWindowOfClass(kAllWindowClasses, false); theWindow && (theWindow != gDisplay.window); theWindow = GetNextWindowOfClass(theWindow, kAllWindowClasses, false))
		{
			theErr = GetWindowBounds(theWindow, kWindowStructureRgn, &theBounds);
			
			// The debugger window will sometimes be in front but hidden, so we ignore hidden windows in general.
			if ((theErr == noErr) && MacIsWindowVisible (theWindow))
			{
				if (SectRect(&displayBounds, &theBounds, &theBounds))
					return false;
			}
		}
	}

	
	// if we're a real window, also make sure our visRgn is appropriate
	if (!gDisplay.fullscreen)
	{
		RgnHandle	visRgn = NewRgn();
		Rect		box, portRect;
		Boolean		simpleRegion;
		
		if (!visRgn)
			return false;
		
		(void)GetPortVisibleRegion(GetWindowPort(gDisplay.window), visRgn);
		(void)GetRegionBounds(visRgn, &box);
		simpleRegion = IsRegionRectangular(visRgn);
		DisposeRgn(visRgn);
		
		(void)GetWindowPortBounds(gDisplay.window, &portRect);
		
		// if the window's visible region is not the entire window, bail
		if (!simpleRegion ||
			box.right - box.left != portRect.right - portRect.left ||
			box.bottom - box.top != portRect.bottom - portRect.top)
			return false;
	}

	// check to see that the destination bounds are entirely on the screen
	if ((*gDisplay.device)->gdRect.right  < inBounds->right  ||
		(*gDisplay.device)->gdRect.bottom < inBounds->bottom ||
		(*gDisplay.device)->gdRect.left   > inBounds->left   ||
		(*gDisplay.device)->gdRect.top    > inBounds->top)
		return false;
	
	// okay, we can do it!
	return true;
}


//===============================================================================
//	DoBlitSlow
//
//	Handles positioning the destination rectangle for blitting and calling
//	down to CopyBits to do an update
//===============================================================================

static void DoBlitSlow(const Rect *inDest)
{
	CGrafPtr windowPort = GetWindowPort (gDisplay.window);
	PixMapHandle spixmap = GetGWorldPixMap(sScreenGWorld);
	//PixMapHandle dpixmap = GetPortPixMap(windowPort);
	Rect sbounds, dbounds;

	if (sCurrentDisplayBitmap == NULL) return;

	// If the bitmap has zero width or height, don't try to draw
	if ((gDisplay.viswidth == 0) || (gDisplay.visheight == 0)) return;

	// update the offscreen GWorld
	CopyScreenToGWorld (sCurrentDisplayBitmap, sScreenGWorld, gDisplay);

	// compute the source rectangle
	sbounds.left = 0;
	sbounds.right = (gDisplay.swapxy) ? gDisplay.visheight : gDisplay.viswidth;
	sbounds.top = 0;
	sbounds.bottom = (gDisplay.swapxy) ? gDisplay.viswidth : gDisplay.visheight;

	// compute the destination rectangle
	if (!gPrefs.hideDesktop)
		GetWindowPortBounds(gDisplay.window, &dbounds);
	else
	{
		dbounds = *inDest;
		GlobalToLocal(&((Point *)&dbounds)[0]);
		GlobalToLocal(&((Point *)&dbounds)[1]);
	}
	
	// make sure CopyBits doesn't do any needless remapping
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	// copy it, locking the GWorld while we do it
	LockPixels (spixmap);
	CopyBits (GetPortBitMapForCopyBits ((CGrafPtr)sScreenGWorld), GetPortBitMapForCopyBits (windowPort),
		&sbounds, &dbounds, srcCopy, NULL);
	UnlockPixels (spixmap);

	FlushPortImmediate(windowPort);
	
	// swap the dirty rects and clear them
	ResetDirtyRects(true);

	// remember that we did a slow blit last time
	sLastBlitWasSlow = true;
	sSkipTimesValid = false;
}

//===============================================================================
//	CopyScreenToGWorld
//
//	Updates the offscreen GWorld with the contents of the current screen.
//===============================================================================

void CopyScreenToGWorld (mame_bitmap *inBitmap, GWorldPtr inGWorld, DisplayParameters inDisplay)
{
	PixMapHandle pixmap = GetGWorldPixMap(inGWorld);
	UInt32 sourceRowBytes, destRowBytes;
	UInt8 *sourceBits, *destBits;
	UInt32 viswidth, visheight;
	int sourceDepth, destDepth;
	
	sourceDepth = inBitmap->depth;
	if (sourceDepth == 15)
		sourceDepth = 16;
	
	// lock down the GWorld's pixels
	LockPixels(pixmap);

	// determine the source parameters
	sourceBits = (UInt8 *) inBitmap->line[inDisplay.visyoffset] + (inDisplay.visxoffset * sourceDepth / 8);
	if (inDisplay.swapxy)
	{
		viswidth = inDisplay.visheight;
		visheight = inDisplay.viswidth;
	}
	else
	{
		viswidth = inDisplay.viswidth;
		visheight = inDisplay.visheight;
	}

	sourceRowBytes = (unsigned char *) inBitmap->line[1] - (unsigned char *) inBitmap->line[0];

	if ((inDisplay.swapxy) ? (inDisplay.flipy) : (inDisplay.flipx))
		sourceBits += (inDisplay.swapxy ? inDisplay.viswidth-1 : inDisplay.viswidth) * sourceDepth / 8;

	if ((inDisplay.swapxy) ? (inDisplay.flipx) : (inDisplay.flipy))
		sourceBits += sourceRowBytes*(inDisplay.visheight-1);

	// determine the destination parameters
	destBits = (UInt8 *)GetPixBaseAddr(pixmap);
	destRowBytes = GetPixRowBytes(pixmap);

	destDepth = inDisplay.depth;

	// blit
	if (destDepth == 16)
	{
		if (sourceDepth == 16)
		{
			if ((sScreenAttributes & VIDEO_RGB_DIRECT)
#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
				&& (inGWorld != gDebugGWorld)
#endif
				)
			{
				if ((inDisplay.swapxy))
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to16d_sxy_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to16d_sxy_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to16d_sxy_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to16d_sxy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
				else
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to16d_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to16d_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to16d_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to16d(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
			}
			else
			{
				if ((inDisplay.swapxy))
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to16l_sxy_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to16l_sxy_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to16l_sxy_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to16l_sxy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
				else
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to16l_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to16l_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to16l_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to16l(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
			}
		}
		else
			logerror("aargh!");
	}
	else if (destDepth == 32)
	{
		if (sourceDepth == 16)
		{
			if ((sScreenAttributes & VIDEO_RGB_DIRECT)
#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
				&& (inGWorld != gDebugGWorld)
#endif
				)
				logerror("aargh!");
			else
			{
				if ((inDisplay.swapxy))
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to32l_sxy_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to32l_sxy_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to32l_sxy_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to32l_sxy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
				else
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_16to32l_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_16to32l_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_16to32l_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_16to32l(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
			}
		}
		else if (sourceDepth == 32)
		{
			if ((sScreenAttributes & VIDEO_RGB_DIRECT)
#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
				&& (inGWorld != gDebugGWorld)
#endif
				)
			{
				if ((inDisplay.swapxy))
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_32to32d_sxy_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_32to32d_sxy_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_32to32d_sxy_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_32to32d_sxy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				}
				else
				{
					if ((inDisplay.flipx) && (inDisplay.flipy))
						blit_1x1_full_32to32d_fx_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipx)
						blit_1x1_full_32to32d_fx(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else if (inDisplay.flipy)
						blit_1x1_full_32to32d_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
					else
						blit_1x1_full_32to32d(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth*2, visheight, &inDisplay);
				}
			}
			else
			{
				if ((inDisplay.swapxy) || (inDisplay.flipx) || (inDisplay.flipy))
					logerror("aargh!");
					//blit_1x1_full_32to32l_fy(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
				else

					blit_1x1_full_32to32l(sourceBits, destBits, sourceRowBytes, destRowBytes, viswidth, visheight, &inDisplay);
			}
		}
		else
			logerror("aargh!");
	}
	else
		logerror("aargh!");

	// unlock the pixels
	UnlockPixels(pixmap);
}
	
	
//===============================================================================
//	ResetDirtyRects
//
//	Resets the dirty rects for a bitmap, optionally clearing them
//===============================================================================

static void ResetDirtyRects(Boolean clear)
{
	// set the dirty globals
	gDisplay.curdirty 	= sDirtyBuffer;
	gDisplay.obsolete2	= NULL;

	// if we're to clear the dirty rects, do it now
	if (clear && (sDirtyBuffer != NULL))
		memset(sDirtyBuffer, 0, sDirtyBufferSize);
}


// unused
#if 0
//===============================================================================
//	UpdateFPS
//
//	Updates the FPS counter
//===============================================================================

static void UpdateFPS(mame_bitmap *inBitmap, UInt32 inCurrentFrameTime)
{
	static UInt32 fps = 0;
	char fpsTextBuffer[100];

	// get the time since the last update
	UInt32 timeSinceLast = inCurrentFrameTime - sFPSLastUpdateTime;
	
	// if more than half a second, update now
	if (timeSinceLast > 500000)
	{
		UInt32 i, totalFrames, totalTime;
	
		// set the final delta time
		sFPSDeltaTime[0] = timeSinceLast;
		
		// recompute the average FPS
		totalFrames = totalTime = 0;
		for (i = 0; i < kFPSAverageCount; i++)
			if (sFPSElapsedFrames[i] != 0)
			{
				totalFrames += sFPSElapsedFrames[i];
				totalTime += sFPSDeltaTime[i];
				sAvgTotalFrames += sFPSElapsedFrames[i];
				sAvgTotalTime += sFPSDeltaTime[i];
			}
		fps = totalFrames * 1000000 / totalTime;
		
		// No average if we're skipping frames
		if (frameskip != 0)
			sAvgFPS = 0;
		// Start the average counter 10 seconds into it
		else if (sAvgTotalTime > 10000000)
			sAvgFPS = sAvgTotalFrames * 1000000 / sAvgTotalTime;
		
		// round it toward the framerate
		if (fps < (UInt32)Machine->drv->frames_per_second)
			fps++;

		// roll the averages forward
		memmove(&sFPSElapsedFrames[1], &sFPSElapsedFrames[0], (kFPSAverageCount - 1) * sizeof(sFPSElapsedFrames[0]));
		memmove(&sFPSDeltaTime[1], &sFPSDeltaTime[0], (kFPSAverageCount - 1) * sizeof(sFPSDeltaTime[0]));
		
		// reset the counters
		sFPSElapsedFrames[0] = 0;
		sFPSLastUpdateTime = inCurrentFrameTime;
	}
	
	// print the text over the screen
	if (sFastForwarding)
		sprintf(fpsTextBuffer, "FPS:%4d/%2d (%3d%%) FAST:%2d", fps, (UInt32)Machine->drv->frames_per_second, (UInt32)(100 * fps / Machine->drv->frames_per_second), kFrameskipLevels - 1);
	else if (autoframeskip && gPrefs.speedThrottle)
		sprintf(fpsTextBuffer, "FPS:%4d/%2d (%3d%%) AUTO:%2d", fps, (UInt32)Machine->drv->frames_per_second, (UInt32)(100 * fps / Machine->drv->frames_per_second), frameskip);
	else
		sprintf(fpsTextBuffer, "FPS:%4d/%2d (%3d%%) AVG:%3d SKIP:%2d", fps, (UInt32)Machine->drv->frames_per_second, (UInt32)(100 * fps / Machine->drv->frames_per_second), sAvgFPS, frameskip);

	{
		// render the text over the screen
		extern int uirotcharwidth;
		ui_text(inBitmap, fpsTextBuffer, Machine->uiwidth - strlen(fpsTextBuffer) * uirotcharwidth, 0);
	}
}
#endif


#pragma mark -
#pragma mark ¥ Printing & Copying Support

//===============================================================================
//	InitPrinting
//
//	Allocates memory at startup time for printing.
//
//===============================================================================

// interfaces changed a lot
OSStatus InitPrinting(void)
{
	OSStatus status;
	PMPrintSession printSession;

	// if we're already initted, bail
	if (sPageFormat && sPrintSettings)
		return noErr;

	//	Initialize the printing manager. Now a required call.
	status = PMCreateSession(&printSession);
	if (status != noErr) 
		return status;		// pointless to continue if PMCreateSession fails

	//	Set up a valid PMPageFormat object and flatten it.
	if (! sPageFormat)
	{
		PMPageFormat pageFormat = kPMNoPageFormat;

		status = PMCreatePageFormat(& pageFormat);

		if ((status == noErr) && (pageFormat != kPMNoPageFormat))
			status = PMSessionDefaultPageFormat(printSession, pageFormat);

		if ((status == noErr) && (pageFormat != kPMNoPageFormat))
			status = PMFlattenPageFormat(pageFormat, & sPageFormat);
	}

	//	Set up a valid PMPrintSettings object and flatten it.
	if ((! sPrintSettings) && (status == noErr))
	{
		PMPrintSettings printSettings = kPMNoPrintSettings;

		status = PMCreatePrintSettings(& printSettings);
		
		if ((status == noErr) && (printSettings != kPMNoPrintSettings))
			status = PMSessionDefaultPrintSettings(printSession, printSettings);

		if ((status == noErr) && (printSettings != kPMNoPrintSettings))
			status = PMFlattenPrintSettings(printSettings, & sPrintSettings);
	}

	//	Terminate the current printing session.
	(void)PMRelease(printSession);
	printSession = NULL;

	return status;
}


//===============================================================================
//	PrintSetup
//
//	Displays the page setup dialog.
//
//	This code is based on code from from Frodo/MacOS by Richard Bannister
//	CopyToClipboard and PrintScreen based on code by Michael Bytnar
//	Carbon code by Raphael Nabet, based on Apple sample code
//===============================================================================

void PrintSetup(void)
{
	OSStatus status;
	Boolean accepted;

	PMPrintSession printSession;
	PMPageFormat pageFormat = kPMNoPageFormat;

	// make sure we're ready to print
	status = InitPrinting();
	if (status != noErr)
		return;		// stop if it failed - sPageFormat could be invalid

	// ensures cursor is visible
	ShowCursorAbsolute();

	//	Initialize the printing manager. Now a required call.
	status = PMCreateSession(&printSession);
	if (status != noErr)
		return;				// pointless to continue if PMCreateSession fails

	// retrieve the current page format
	status = PMUnflattenPageFormat(sPageFormat, & pageFormat);

	if (status == noErr)
		status = PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean);	// R Nabet 000214

	// Display the Page Setup dialog.
	if ((status == noErr) && (pageFormat != kPMNoPageFormat))
	{
		status = PMSessionPageSetupDialog(printSession, pageFormat, &accepted);
		if (!accepted)
			status = kPMCancel;		// user clicked Cancel button
	}

	// If the user did not cancel, flatten and save the PageFormat object.
	if ((status == noErr) && (pageFormat != kPMNoPageFormat))
		status = PMFlattenPageFormat(pageFormat, & sPageFormat);

	//	Terminate the current printing session.
	(void)PMRelease(printSession);
	printSession = NULL;
}

//===============================================================================
//	PrintScreen
//
//	Displays the print dialog and actually prints the screen.
//
//	This code is based on code from from Frodo/MacOS by Richard Bannister
//	CopyToClipboard and PrintScreen based on code by Michael Bytnar
//	Carbon code by Raphael Nabet, based on Apple sample code
//===============================================================================

void PrintScreen(void)
{
	OSStatus status;
	Boolean accepted;

	PMPrintSession printSession;
	PMPageFormat pageFormat = kPMNoPageFormat;
	PMPrintSettings printSettings = kPMNoPrintSettings;
	RgnHandle clipRgn;
	Rect bounds;

	// bail if we don't have a GWorld
	if (!sCurrentDisplayBitmap || !sScreenGWorld)
		return;
	
	// copy the current screen to the GWorld
	CopyScreenToGWorld (sCurrentDisplayBitmap, sScreenGWorld, gDisplay);

	// make sure we're ready to print
	status = InitPrinting();
	if (status != noErr) 
		return;		// stop if it failed

	// ensures cursor is visible
	ShowCursorAbsolute();
	SetGWorld(GetWindowPort(gDisplay.window), NULL);

	// remember the current clip region
	clipRgn = NewRgn();
  	if (!clipRgn)
  		return;
	GetClip(clipRgn);

	//	Initialize the printing manager. Now a required call.
	status = PMCreateSession(&printSession);
	if (status != noErr) 
		return;				// pointless to continue if PMCreateSession fails

	// retrieve the current page format
	status = PMUnflattenPageFormat(sPageFormat, & pageFormat);

	if (status == noErr)
		status = PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean);

	// retrieve the current print settings
	if (status == noErr)
		status = PMUnflattenPrintSettings(sPrintSettings, & printSettings);

	if (status == noErr)
		status = PMSessionValidatePrintSettings(printSession, printSettings, kPMDontWantBoolean);

	// make sure we clip to the screen bounds
	bounds.left = 0;
	bounds.top = 0;
	bounds.right = (gDisplay.swapxy) ? gDisplay.visheight : gDisplay.viswidth;
	bounds.bottom = (gDisplay.swapxy) ? gDisplay.viswidth : gDisplay.visheight;
	ClipRect(&bounds);

	// display the print dialog
	if ((status == noErr) && (printSettings != kPMNoPrintSettings) && (pageFormat != kPMNoPageFormat))
	{
		status = PMSessionPrintDialog(printSession, printSettings, pageFormat, & accepted);
		if (! accepted)
			status = kPMCancel;
	}

	if (status == noErr)
	{
		/*PixMapHandle spixmap = GetGWorldPixMap(sScreenGWorld);
		PixMapHandle dpixmap = GetPortPixMap(GetWindowPort(gDisplay.window));*/
		PicHandle thePic;

		status = PMSessionBeginDocument(printSession, printSettings, pageFormat);
		if (status == noErr)
		{
			status = PMSessionBeginPage(printSession, pageFormat, nil);			
			if (status == noErr)
			{

		// R Nabet 000212 : The code with CopyBits works fine with some printer drivers
		// (e.g. laserwriter), but breaks many other drivers (e.g. Fax Sender, Epson Stylus...).
		// The code with the PicHandle seems to work fine.
#if 0
		// ensure CopyBits doesn't do anything crazy
		ForeColor(blackColor);
		BackColor(whiteColor);

		// draw the picture
		CopyBits((BitMapPtr)*spixmap, (BitMapPtr)*dpixmap, &bounds, &bounds, srcCopy, NULL);
#else
		// make a PICT
		thePic = CopyScreenToPICT(sCurrentDisplayBitmap);
		if (! thePic)
		{
			SysBeep(1);
			return;
		}
		else
		{
			// if it worked, print it
			HLock((Handle) thePic);

			DrawPicture(thePic, &bounds);

			HUnlock((Handle) thePic);

			// Clean Up
			KillPicture(thePic);
		}
#endif

				status = PMSessionEndPage(printSession);
			}
			status = PMSessionEndDocument(printSession);
		}
	}

	// flatten and save the current print settings
	if (status == noErr)
		status = PMFlattenPrintSettings(printSettings, & sPrintSettings);

	// flatten and save the current PageFormat object
	if ((status == noErr) && (pageFormat != kPMNoPageFormat))
		status = PMFlattenPageFormat(pageFormat, & sPageFormat);

	//	Terminate the current printing session.
	(void)PMRelease(printSession);
	printSession = NULL;

	// R Nabet 000212 : the instruction order was wrong.
	// point back to the window port
	SetGWorld(GetWindowPort(gDisplay.window), NULL);

	// restore the old clip region
	SetClip(clipRgn);
	DisposeRgn(clipRgn);

	if (!gEmulationPaused)
		ActivateInputDevices(false);
}


//===============================================================================
//	SaveScreenshotAs
//
//	Creates a PNG from the current screen
//===============================================================================

void SaveScreenshotAs(void)
{
	CFURLRef newURL = NULL;
	CFStringRef defaultFileName = NULL;
	CFStringRef promptString = NULL;
	Boolean success;
	char fileName[kMacMaxPath];
	mame_file *file;

	// bail if we're not running yet
	if (!sCurrentDisplayBitmap)
		return;

	// force the cursor visible
	ShowCursorAbsolute();
	SetGWorld(GetWindowPort(gDisplay.window), NULL);
	if (!gEmulationPaused)
		DeactivateInputDevices(false);
	
	// make a reasonable default name
	defaultFileName = CFStringCreateWithFormat (NULL, NULL, CFSTR("%s.png"), Machine->gamedrv->name);
	if (!defaultFileName) return;
	
	promptString = CFCopyLocalizedString(CFSTR("PromptToSaveFile"), NULL);
	
	// dump the file...where?
	if (!_NavPutOneFile (promptString, defaultFileName, FILETYPE_SCREENSHOT, &newURL, NULL))
		goto bail;

	success = CFURLGetFileSystemRepresentation (newURL, true, (UInt8 *)fileName, sizeof(fileName));
	if (!success) goto bail;
	
	file = mame_fopen(Machine->gamedrv->name, fileName, FILETYPE_SCREENSHOT, 1);
	if (file)
	{
		double savedBrightness;

		// Save off the current screen brightness
		savedBrightness = palette_get_global_brightness();

		// Force a refresh of the game screen at full brightness
		palette_set_global_brightness (1.0);
		schedule_full_refresh();
		draw_screen();
	
		save_screen_snapshot_as(file, Machine->scrbitmap);
		mame_fclose(file);

		// Restore the screen brightness to what it was before
		palette_set_global_brightness (savedBrightness);
		schedule_full_refresh();
		draw_screen();
	}
	else
		SysBeep(1);

bail:
	if (newURL)
		CFRelease (newURL);
	if (defaultFileName)
		CFRelease (defaultFileName);
	if (promptString)
		CFRelease (promptString);

}
		

//===============================================================================
//	CopyToClipboard
//
//	Copies the current screen to the clipboard.
//
//	This code is based on code from from Frodo/MacOS by Richard Bannister
//	CopyToClipboard and PrintScreen based on code by Michael Bytnar
//===============================================================================

void CopyToClipboard(void)
{
	PicHandle clipPic;
	long maxheap;

	// bail if we don't have a GWorld
	if (!sCurrentDisplayBitmap || !sScreenGWorld)
		return;

	// zap the current Clipboard contents and free up some memory.
	ClearCurrentScrap();
	PurgeMem(-1);
	CompactMem(-1);
	MaxMem(&maxheap);

	// make a PICT
	clipPic = CopyScreenToPICT(sCurrentDisplayBitmap);
	if (!clipPic)
	{
		SysBeep(1);
		return;
	}
	
	// if it worked, put it on the scrap, beeping if there was an error
	HLock((Handle)clipPic);

	{
		ScrapRef theScrap;

		if (GetCurrentScrap(& theScrap) != noErr)
			SysBeep(1);
		else if (PutScrapFlavor(theScrap, 'PICT', kScrapFlavorMaskNone, GetHandleSize((Handle) clipPic), *clipPic) != noErr)
			SysBeep(1);
	}

	HUnlock((Handle)clipPic);

	// Clean Up
	KillPicture(clipPic);
}


//===============================================================================
//	CopyScreenToPICT
//
//	Creates a PICT from the current screen
//===============================================================================

static PicHandle CopyScreenToPICT(mame_bitmap *bitmap)
{
	PixMapHandle 		pixmap = GetGWorldPixMap(sScreenGWorld);
	OpenCPicParams		header;
	RgnHandle			saveClip;
	Rect 				bounds;
	GDHandle 			oldDevice;
	GWorldPtr 			oldPort;
	PicHandle 			pict;

	saveClip = NewRgn();
	if (!saveClip) return NULL;
	
	// copy the current screen into the GWorld
	CopyScreenToGWorld (sCurrentDisplayBitmap, sScreenGWorld, gDisplay);
		
	// save the port and point to the window
	GetGWorld(&oldPort, &oldDevice);
	SetGWorld(sScreenGWorld, NULL);
	GetClip(saveClip);

	// no funny stuff from CopyBits
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	// setup bounds
	bounds.left = 0;
	bounds.top = 0;
	bounds.right = (gDisplay.swapxy) ? gDisplay.visheight : gDisplay.viswidth;
	bounds.bottom = (gDisplay.swapxy) ? gDisplay.viswidth : gDisplay.visheight;
	
	// setup picture header
	header.srcRect = bounds;
	header.hRes = header.vRes = 0x00480000;	/* 72 dpi */
	header.version = -2;
	header.reserved1 = header.reserved2 = NULL;

	// try creating picture in application heap
	pict = OpenCPicture(&header);
	ClipRect(&bounds);
	
	// copy it
	CopyBits((BitMapPtr)*pixmap, (BitMapPtr)*pixmap, &bounds, &bounds, srcCopy, NULL);
	
	// close the picture
	ClosePicture();

	// if it failed, try again using the system heap
	if (EmptyRect(&(*pict)->picFrame))
	{
		
		KillPicture(pict);

		pict = OpenCPicture(&header);
		
		ClipRect(&bounds);
		
		// copy it
		CopyBits((BitMapPtr)*pixmap, (BitMapPtr)*pixmap, &bounds, &bounds, srcCopy, NULL);
		
		// close the picture
		ClosePicture();

		// if it failed, kill the PicHandle
		if (EmptyRect(&(*pict)->picFrame))
		{
			KillPicture(pict);
			pict = NULL;
		}
	}
	
	// restore clipping region and GWorld
	SetClip(saveClip);
	DisposeRgn(saveClip);
	SetGWorld(oldPort, oldDevice);
	
	// return the result
	return pict;
}


#pragma mark -
#pragma mark ¥ Video State Querying

//===============================================================================
//	SupportsFullScreen
//
//	Returns true if the given full screen setting is supported.
//===============================================================================

Boolean SupportsFullScreen(Boolean inFullscreen)
{
	// the only case where we can't do it is if the plug-in requires full screen
	if (!inFullscreen && sPlugin && sPlugin->desc.fullscreen)
		return false;
	
	return true;
}


//===============================================================================
//	SupportsThrottling
//
//	Returns true if the given throttling setting is supported.
//===============================================================================

Boolean SupportsThrottling(Boolean inThrottling)
{
	// always supported
	return true;
}


//===============================================================================
//	SupportsAutoFrameskip
//
//	Returns true if the given automatic frameskip setting is supported.
//===============================================================================

Boolean SupportsAutoFrameskip(Boolean inAutoSkip)
{
	// always supported
	return true;
}


//===============================================================================
//	SupportsFrameskip
//
//	Returns true if the given frameskip setting is supported.
//===============================================================================

Boolean SupportsFrameskip(UInt32 inFrameskip)
{
	// if we're autoframeskipping, we can't take external data
	if (gPrefs.autoSkip)
		return false;

	// otherwise, we're always supported, as long as it's in range
	return (inFrameskip < kFrameskipLevels);
}


//===============================================================================
//	SupportsScaleAndInterlace
//
//	Returns true if the given window scale and interlace settings are supported.
//===============================================================================

Boolean SupportsScaleAndInterlace(UInt32 inScale, Boolean inInterlace)
{
	// if the plugin doesn't support scaling, we can't do any of this
	if (!sPlugin->desc.scale)
		return false;

	// vector games only support 1x1, non interlaced
	if (gDisplay.vector)
		if (inScale != 0x100 || inInterlace)
			return false;

	// if the plugin doesn't support interlacing, we can't have that
	if (!sPlugin->desc.interlace)
		if (inInterlace)
			return false;

	// otherwise ask plugin if the scale is valid for the current resolution
	return (inScale <= (*sPlugin->desc.computeMaxScale)(&gDisplay));
}


#pragma mark -
#pragma mark ¥ Video State Setting

//===============================================================================
//	SetFullScreen
//
//	Sets the state of the full screen video.
//===============================================================================

void SetFullScreen(Boolean inFullscreen)
{
	// if we can't do it, bail
	if (!SupportsFullScreen(inFullscreen))
		return;
		
	// otherwise, this value will work
	gPrefs.hideDesktop = gDisplay.fullscreen = inFullscreen;

	// invalidate the autoskip times
	sSkipTimesValid = false;
}


//===============================================================================
//	SetThrottling
//
//	Sets the state of the full screen video.
//===============================================================================

void SetThrottling(Boolean inThrottling)
{
	// if we can't do it, bail
	if (!SupportsThrottling(inThrottling))
		return;

	// otherwise, this value will work
	gPrefs.speedThrottle = gDisplay.throttle = inThrottling;
	
	// note that the main code relies on this to be reset even if the new
	// value is the same as the old value
	ResetFrameCount();
	
	// invalidate the autoskip times
	sSkipTimesValid = false;
}


//===============================================================================
//	SetAutoFrameskip
//
//	Sets the state of the automatic frameskipping.
//===============================================================================

void SetAutoFrameskip(Boolean inAutoSkip)
{
	// if we can't do it, bail
	if (!SupportsAutoFrameskip(inAutoSkip))
		return;

	// otherwise, this value will work
	gPrefs.autoSkip = autoframeskip = inAutoSkip;
	
	// if we just turned off the autoskip, sync the frameskip
	if (!inAutoSkip)
		gPrefs.frameSkip = frameskip;
	
	// invalidate the autoskip times
	sSkipTimesValid = false;
}


//===============================================================================
//	SetFrameskip
//
//	Sets the state of the frameskipping.
//===============================================================================

void SetFrameskip(UInt32 inFrameskip)
{
	// if we can't do it, bail
	if (!SupportsFrameskip(inFrameskip))
		return;

	// otherwise, this value will work
	gPrefs.frameSkip = frameskip = inFrameskip;
	
	// invalidate the autoskip times
	sSkipTimesValid = false;
}


//===============================================================================
//	SetScaleAndInterlace
//
//	Sets the state of the full screen video.
//===============================================================================

void SetScaleAndInterlace(UInt32 inScale, Boolean inInterlace)
{
	// if we can't do it, bail
	if (!SupportsScaleAndInterlace(inScale, inInterlace))
		return;

	// otherwise, these values will work
	gDisplay.scale = inScale;
	gDisplay.interlace = inInterlace;
	gPrefs.windowScale = inScale >> 8;
	gPrefs.interlace = inInterlace;

	// all our blitting times are invalid now
	memset(sValidBlitTimes, 0, sizeof(sValidBlitTimes));
	
	// invalidate the autoskip times
	sSkipTimesValid = false;
}


#pragma mark -
#pragma mark ¥ Utility Functions

//===============================================================================
//	ResetGlobals
//
//	Resets all the globals to sensible values before we get started.
//===============================================================================

static void ResetGlobals(void)
{
	UInt32 currentTime = GetMicroseconds();

	// zap the display structure
	memset(&gDisplay, 0, sizeof(gDisplay));
	
	// reset all globals
	sEffectiveScreenRect	= (*GetMainDevice())->gdRect;
	sFullScreenRect			= sEffectiveScreenRect;
	
	sScreenAttributes		= 0;
	sScreenGWorld			= NULL;

	sDirtyBufferSize		= 0;
	sDirtyBuffer			= NULL;
	gDisplay.full			= true;
	
	sCurrentDisplayBitmap	= NULL;
	
	sFastForwarding			= false;
	sSkipFrameCount			= 0;
	
	memset(sValidBlitTimes, 0, sizeof(sValidBlitTimes));
	sValidSkippedTime		= 0;
	sValidDrawnTime			= 0;
	sIdleTimeMakeup			= 0;
	
	sSkipTimesValid			= false;
	sDrawnFrames			= 0;
	sSkippedFrames			= 0;
	sDrawnFramesTime		= 0;
	sSkippedFramesTime		= 0;
	sIdleTime				= 0;
	sBlitTime				= 0;
	sLastTweakTime			= currentTime;
	sStartFrameTime			= currentTime;

	sMicrosecondsPerFrame	= (UInt32)(1000000.0 / Machine->drv->frames_per_second);
	memset(sFPSElapsedFrames, 0, sizeof(sFPSElapsedFrames));
	memset(sFPSDeltaTime, 0, sizeof(sFPSDeltaTime));
	sFPSTotalElapsedTime	= 0;
	sFPSLastUpdateTime		= currentTime;
	sPreviousTargetTime		= currentTime;
	sPreviousDisplayTime	= currentTime;
	sFramesDisplayed		= 0;
	
	sLastInterlace			= -1;
	sLastScale				= -1;
	sLastHideDesktop		= -1;
	memset(&sLastDisplayBounds, 0, sizeof(sLastDisplayBounds));
	sLastBlitWasSlow		= true;
	
	sPalette 				= NULL;
	memset(sRawPalette, 0, sizeof(sRawPalette));

	SetDisplayChanged();
	
	// reset globals external to this module
	frameskip 				= gPrefs.frameSkip;
	autoframeskip 			= gPrefs.autoSkip;
}


//===============================================================================
//	ResetFrameCount
//
//	Resets the total number of frames (used for calculating the average)
//===============================================================================

void ResetFrameCount(void)
{
	UInt32 currentTime = GetMicroseconds();

	memset(sFPSElapsedFrames, 0, sizeof(sFPSElapsedFrames));
	memset(sFPSDeltaTime, 0, sizeof(sFPSDeltaTime));

	sAvgTotalFrames = sAvgTotalTime = sAvgFPS = 0;
	
	sFPSTotalElapsedTime	= 0;
	sFPSLastUpdateTime		= currentTime;
	sPreviousTargetTime		= currentTime;
	sPreviousDisplayTime	= currentTime;
	sFramesDisplayed		= 0;
	
	// invalidate the autoskip times
	sSkipTimesValid = false;
}


//===============================================================================
//	FreeAllDisplayData
//
//	Central location for freeing data
//===============================================================================

static void FreeAllDisplayData(void)
{
	// free the GWorld
	if (sScreenGWorld != NULL)
		DisposeGWorld(sScreenGWorld);
	sScreenGWorld = NULL;
		
	// free the dirty buffers
	if (sDirtyBuffer != NULL)
		free(sDirtyBuffer);
	sDirtyBuffer = NULL;
	
	// free the delta buffers
	if (gDisplay.deltabits != NULL)
		free(gDisplay.deltabits);
	gDisplay.deltabits = NULL;
	
	// free the 16bpp lookup
	if (gDisplay.lookup16 != NULL)
		free(gDisplay.lookup16);
	gDisplay.lookup16 = NULL;
	
	// free the 32bpp lookup
	if (gDisplay.lookup32 != NULL)
		free(gDisplay.lookup32);
	gDisplay.lookup32 = NULL;
	
	// free the window's palette
	if (sPalette != NULL)
		DisposePalette(sPalette);
	sPalette = NULL;

#if defined(MAME_DEBUG) & !defined(NEW_DEBUGGER)
	// now do the same job with the debugger
	DisposeDebugScreen();	// R. Nabet 001212
#endif
}


//===============================================================================
//	EmergencyVideoShutdown
//
//	Called on a non-clean exit from the system.
//===============================================================================

void EmergencyVideoShutdown(void)
{
	// important: redisplay the menubar before closing the display
	// if the plugin changes resolutions on close, we will restore the incorrect grayRgn
	ShowMenubar();
	
	// close down any active plugin
	if (sCurrentDisplayBitmap && sPlugin)
		(*sPlugin->desc.closeDisplay)(&gDisplay);
}


//===============================================================================
//	ShowCursorAbsolute
//
//	Use this routine in place of ShowCursor to ensure that the mouse pointer
//	is visible.
//===============================================================================

void ShowCursorAbsolute(void)
{
#if 1
	// LBO - InitCursor doesn't always reset the cursor state under OSX
	#undef ShowCursor
	while (gCursorHidden)
	{
		ShowCursor ();
		gCursorHidden --;
	}
#endif
	InitCursor ();
	CGAssociateMouseAndMouseCursorPosition(true);
}


//===============================================================================
//	HideCursorAbsolute
//
//	Use this routine in place of ShowCursor to ensure that the mouse pointer
//	is visible.
//===============================================================================

void HideCursorAbsolute(void)
{
	#undef HideCursor
	if (gCursorHidden) return;
	
//	if (sCursorHidden < 16)
	{
		// LBO - this counter could roll and screw us up
		HideCursor();
		gCursorHidden ++;
	}
	CGAssociateMouseAndMouseCursorPosition(false);
}


//===============================================================================
//	SetDisplayChanged
//
//	Mark the display as having changed
//===============================================================================

void SetDisplayChanged(void)
{
	sLastDisplayChangeTicks = TickCount();
	
	// anytime the display changed, our autoskip timing will be screwy
	sSkipTimesValid = false;
}


//===============================================================================
//	ComputeVectorSize
//
//	Compute the size of the vector rendering bitmap
//===============================================================================

void ComputeVectorSize(int *width, int *height)
{
	// make sure we have accurate information
	UpdateDisplayState(NULL);

	// determine the maximum screen bounds that work with our dirty system
	*width = (sEffectiveScreenRect.right - sEffectiveScreenRect.left) & ~DIRTY_X_MASK;
	*height = (sEffectiveScreenRect.bottom - sEffectiveScreenRect.top) & ~DIRTY_Y_MASK;

	if (gSwapxy)
	{
		*width ^= *height;
		*height ^= *width;
		*width ^= *height;
	}
}

//===============================================================================
//	FlushPortImmediate
//
//	Flush the contents of a buffered (OSX) port and don't return until the
//	flush is complete.
//===============================================================================

void FlushPortImmediate(GrafPtr inPort)
{
	if (QDIsPortBuffered(inPort))
	{
		UInt32 ticks;

		QDFlushPortBuffer(inPort, NULL);

		// wait for 2 vertical blanks
		ticks = TickCount();
		while (TickCount() < (ticks + 2))
		{
			usleep(1000);   			// properly yield a slice of time.
		}
	}
}