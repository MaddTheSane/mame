/*##########################################################################

  videoapi.h

  API for talking to plug-in video drivers.
  
############################################################################
  
  Version history:
  
  	0x0100 - initial release (0.36b12)
  	0x0110 - added about box support (0.36b13)
  	0x0111 - fixed video for resolution switching (0.36b13a)
  	0x0120 - added hooks for pause/unpause, mainly for Glide (0.36b16)
  	0x0130 - added hooks for visible area changing and 32bpp mode (0.37b4)
  	0x0140 - added hooks for 3D hardware acceleration (0.37b6?)

##########################################################################*/

#if TARGET_RT_MAC_CFM
#include <Files.h>
#include <MacTypes.h>
#include <QDOffscreen.h>
#endif


// these define the grid size for the dirty grid
#define DIRTY_X_SHIFT		3
#define DIRTY_Y_SHIFT		3

// some useful derived masks
#define DIRTY_X_MASK		((1 << DIRTY_X_SHIFT) - 1)
#define DIRTY_Y_MASK		((1 << DIRTY_Y_SHIFT) - 1)


// version number
//#define VIDEOAPI_VERSION		0x0140
#define VIDEOAPI_VERSION		0x0130


// the main structure describing the display
typedef struct
{
	// these parameters are fixed, and valid at allocate time:
	UInt32					width;					// full bitmap width
	UInt32					height;					// full bitmap height
	UInt32					depth;					// supported depth
	double					aspect;					// aspect ratio = width / height
	
	// these parameters are valid at allocate time but may change frame-to-frame:
	UInt32					visxoffset;				// x offset of the visible area
	UInt32					visyoffset;				// y offset of the visible area
	UInt32					viswidth;				// width of the visible area
	UInt32					visheight;				// height of the visible area

	// this parameter is fixed, and valid at allocate time:
	Boolean					vector;					// true if a vector game
	
	// these parameters may change frame-to-frame, and are not valid at allocate time:
	GDHandle				device;					// device we're drawing to
	WindowRef				window;					// window we're drawing to
	
	Boolean					fullscreen;				// full screen?
	UInt32					scale;					// scale factor
	Boolean					interlace;				// interlace?
	
	CTabHandle				/*colortable*/obsolete;	// handle to the adjusted color table (8bpp only; NULL for 16bpp)
	UInt32 *				lookup16;				// adjusted 16bpp lookup table (16bpp only; NULL for 8bpp)
	Boolean					paletteDirty;			// true if the palette is dirty
	
	void *					bits;					// pointer to the screen bits
	void *					deltabits;				// pointer to extra screen bits for delta blits
	UInt32					rowbytes;				// rowbytes of the screen bits

	Boolean					full;					// true to do a full update regardless of dirty
	UInt8 *					curdirty;				// pointer to the current dirty bits
	UInt8 *					/*prevdirty*/obsolete2;	// pointer to the previous dirty bits
	UInt32					dirtyrowshift;			// log2 of the rowbytes of the dirty maps

	//
	//	This was added to the 1.2 API, and may change frame-to-frame
	//
	Boolean					throttle;				// is rendering speed throttled?

	//
	// Added for 1.4, shouldn't affect earlier versions
	//
	UInt32 *				lookup32;				// adjusted 32bpp lookup table (32bpp only; NULL for 8bpp)
	CGrafPtr				grafPort;				// grafPort we're drawing into
	//
	// Blit time rotation support
	Boolean					flipx, flipy, swapxy;

	Boolean					direct;					// true if this is a non-paletteized display mode
} DisplayParameters;


// callback functions for the plug-in
typedef void		(*ConfigureProc)(void);
typedef Boolean 	(*AllocateDisplayProc)(const DisplayParameters *inParams);
typedef void	 	(*PrepareDisplayProc)(const DisplayParameters *inParams);
typedef void		(*InitializePaletteProc)(const DisplayParameters *inParams);
typedef void		(*CloseDisplayProc)(const DisplayParameters *inParams);
typedef void		(*ComputeDisplayAreaProc)(const DisplayParameters *inParams, UInt32 *outWidth, UInt32 *outHeight, Rect *outGlobalBounds);
typedef UInt32		(*ComputeMaxScaleProc)(const DisplayParameters *inParams);
typedef Boolean		(*UpdateDisplayProc)(const DisplayParameters *inParams);
typedef void		(*SuspendProc)(const DisplayParameters *inParams);
typedef void		(*ResumeProc)(const DisplayParameters *inParams);

typedef void		(*DisplayAboutBoxProc)(void);

typedef void		(*PauseProc)(const DisplayParameters *inParams);
typedef void		(*UnpauseProc)(const DisplayParameters *inParams);

typedef void		(*UpdateVisibleAreaProc)(const DisplayParameters *inParams);

// the structure describing the plug-in
typedef struct
{
	//
	//	This is how the 1.0 API was laid out
	//
	UInt32					version;				// version of this API; 0x0100 is 1.0
	UInt32					identifier;				// unique identifier for this plugin
	const unsigned char *	shortName;				// pascal short name of this renderer (for menus)
	const unsigned char *	longName;				// pascal long name of this renderer

	GDHandle				device;					// set to the specific device to use
	Boolean					fullscreen;				// set to true if we require full screen
	Boolean					eightbit;				// set to true if we support 8bpp
	Boolean					scale;					// set to true if we support the scaling options
	Boolean					interlace;				// set to true if we support interlacing
	
	ConfigureProc			configure;				// called to display the configuration dialog
	AllocateDisplayProc		allocateDisplay;		// called to allocate the display resources
	PrepareDisplayProc		prepareDisplay;			// called to prepare the display
	InitializePaletteProc	initializePalette;		// called to initialize the palette
	CloseDisplayProc		closeDisplay;			// called to tear it down
	ComputeDisplayAreaProc	computeDisplayArea;		// called to determine scaled display area
	UpdateDisplayProc		updateDisplay;			// called to blit
	SuspendProc				suspend;				// called to suspend (app switched out)
	ResumeProc				resume;					// called to resume (app switched back in)

	//
	//	This was added to the 1.1 API
	//
	DisplayAboutBoxProc		displayAboutBox;		// called to display the about box
	
	//
	//	This was added to the 1.2 API
	//
	PauseProc				pause;					// called to pause
	UnpauseProc				unpause;				// called to unpause
	
	//
	//	This was added to the 1.3 API
	//
	Boolean					sixteenbit;				// set to true if we support 16bpp
	Boolean					thirtytwobit;			// set to true if we support 32bpp
	UpdateVisibleAreaProc	updateVisibleArea;		// called to update the visible area when changed
	
	//
	//	This was added to the 1.4 API
	//
//	Boolean					three_d;				// set to true if we support 3D lines and polygons

	//
	//	Any future additions need to go here for backwards compatibility
	//
	enum { alignDest, alignDestOnSrc }	alignmentRule;
	ComputeMaxScaleProc	computeMaxScale;			// called to determine maximum scale

} DisplayDescription;


// the main entry point for the shared library must take this form
typedef Boolean		(*GetDescriptionProc)(DisplayDescription *ioDescription);
