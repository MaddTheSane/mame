/*##########################################################################

	software.c

	Implementation of the software-only display system.

##########################################################################*/

#include <Carbon/Carbon.h>

#include <math.h>
#include <strings.h>

#include "driver.h"
#include "osdepend.h"

#include "macblitters.h"
#include "macutils.h"
#include "software.h"

#include "mac.h"

#define kWindowToFront ((WindowRef)-1L)

extern UInt32 gCursorHidden;

/*##########################################################################
	CONSTANTS
##########################################################################*/

enum
{
	kAboutBoxDialogID		= 31000,
	kAboutBoxVersionItem	= 5
};


/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

static UInt32			sOriginalDepth;				// depth the user set before starting
static Boolean			sUse1x2Blitters;			// use 2x1 blitters instead of square-pixel blitters


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

static Boolean 			AllocateDisplay(const DisplayParameters *inParams);
static void	 			PrepareDisplay(const DisplayParameters *inParams);
static void				InitializePalette(const DisplayParameters *inParams);
static void				CloseDisplay(const DisplayParameters *inParams);

static void				ComputeDisplayArea(const DisplayParameters *inParams, UInt32 *outWidth, UInt32 *outHeight, Rect *outGlobalBounds);
static Boolean			UpdateDisplay(const DisplayParameters *inParams);

static void				Suspend(const DisplayParameters *inParams);
static void				Resume(const DisplayParameters *inParams);

static void				DisplayAboutBox(void);

static void				Pause(const DisplayParameters *inParams);
static void				Unpause(const DisplayParameters *inParams);

static void				UpdateVisibleArea(const DisplayParameters *inParams);

// local function prototypes
static void 			GetGlobalWindowBounds(WindowRef inWindow, Rect *outBounds);
static void				ComputeSourceAndDest(const DisplayParameters *inParams, Rect *sbounds, Rect *dbounds, Boolean clip);
static UInt32			ComputeMaxScale(const DisplayParameters *inParams);
static void				DoBlitFast16to16d(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest);
static void				DoBlitFast16to16l(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest);
static void				DoBlitFast16to32l(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest);
static void				DoBlitFast32to32d(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest);

static OSErr			SetDepthImmediate(GDHandle inGD, short inDepth, short inWhichFlags, short inFlags);

static void DoBlitWithClip(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest, RgnHandle clipRgn,
							void *lookup, void (*RowBlitFunc)(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup), int sourceDepth);
static void DoBlitRow16to16dWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *dummy);
static void DoBlitRow16to16lWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup);
static void DoBlitRow16to32lWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup);
static void DoBlitRow32to32dWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *dummy);


static DECLARE_BLIT_FUNC((*blit_table_16to16d[9*32])) =
{
	// vector
	blit_1x1_full_16to16d,
	blit_1x1_dirty_vector_16to16d,
	blit_1x1_full_16to16d_fx,
	blit_1x1_dirty_vector_16to16d_fx,
	blit_1x1_full_16to16d_fy,
	blit_1x1_dirty_vector_16to16d_fy,
	blit_1x1_full_16to16d_fx_fy,
	blit_1x1_dirty_vector_16to16d_fx_fy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_dirty_vector_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_dirty_vector_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_dirty_vector_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_dirty_vector_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_vec,
	blit_1x1_dirty_vector_16to16d_vec,
	blit_1x1_full_16to16d_fx_vec,
	blit_1x1_dirty_vector_16to16d_fx_vec,
	blit_1x1_full_16to16d_fy_vec,
	blit_1x1_dirty_vector_16to16d_fy_vec,
	blit_1x1_full_16to16d_fx_fy_vec,
	blit_1x1_dirty_vector_16to16d_fx_fy_vec,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_dirty_vector_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_dirty_vector_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_dirty_vector_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_dirty_vector_16to16d_sxy_fx_fy,

	// 1x1
	blit_1x1_full_16to16d,
	blit_1x1_full_16to16d,
	blit_1x1_full_16to16d_fx,
	blit_1x1_full_16to16d_fx,
	blit_1x1_full_16to16d_fy,
	blit_1x1_full_16to16d_fy,
	blit_1x1_full_16to16d_fx_fy,
	blit_1x1_full_16to16d_fx_fy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_vec,
	blit_1x1_full_16to16d_vec,
	blit_1x1_full_16to16d_fx_vec,
	blit_1x1_full_16to16d_fx_vec,
	blit_1x1_full_16to16d_fy_vec,
	blit_1x1_full_16to16d_fy_vec,
	blit_1x1_full_16to16d_fx_fy_vec,
	blit_1x1_full_16to16d_fx_fy_vec,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,

	// 1x1i
	blit_1x1_full_16to16d,
	blit_1x1_full_16to16d,
	blit_1x1_full_16to16d_fx,
	blit_1x1_full_16to16d_fx,
	blit_1x1_full_16to16d_fy,
	blit_1x1_full_16to16d_fy,
	blit_1x1_full_16to16d_fx_fy,
	blit_1x1_full_16to16d_fx_fy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_vec,
	blit_1x1_full_16to16d_vec,
	blit_1x1_full_16to16d_fx_vec,
	blit_1x1_full_16to16d_fx_vec,
	blit_1x1_full_16to16d_fy_vec,
	blit_1x1_full_16to16d_fy_vec,
	blit_1x1_full_16to16d_fx_fy_vec,
	blit_1x1_full_16to16d_fx_fy_vec,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fx,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,
	blit_1x1_full_16to16d_sxy_fx_fy,

	// 1x2
	blit_1x2_delta_full_16to16d,
	blit_1x2_delta_16to16d,
	blit_1x2_delta_full_16to16d_fx,
	blit_1x2_delta_16to16d_fx,
	blit_1x2_delta_full_16to16d_fy,
	blit_1x2_delta_16to16d_fy,
	blit_1x2_delta_full_16to16d_fx_fy,
	blit_1x2_delta_16to16d_fx_fy,
	blit_1x2_delta_full_16to16d_sxy,
	blit_1x2_delta_16to16d_sxy,
	blit_1x2_delta_full_16to16d_sxy_fx,
	blit_1x2_delta_16to16d_sxy_fx,
	blit_1x2_delta_full_16to16d_sxy_fy,
	blit_1x2_delta_16to16d_sxy_fy,
	blit_1x2_delta_full_16to16d_sxy_fx_fy,
	blit_1x2_delta_16to16d_sxy_fx_fy,
	blit_1x2_delta_full_16to16d_vec,
	blit_1x2_delta_16to16d_vec,
	blit_1x2_delta_full_16to16d_fx_vec,
	blit_1x2_delta_16to16d_fx_vec,
	blit_1x2_delta_full_16to16d_fy_vec,
	blit_1x2_delta_16to16d_fy_vec,
	blit_1x2_delta_full_16to16d_fx_fy_vec,
	blit_1x2_delta_16to16d_fx_fy_vec,
	blit_1x2_delta_full_16to16d_sxy,
	blit_1x2_delta_16to16d_sxy,
	blit_1x2_delta_full_16to16d_sxy_fx,
	blit_1x2_delta_16to16d_sxy_fx,
	blit_1x2_delta_full_16to16d_sxy_fy,
	blit_1x2_delta_16to16d_sxy_fy,
	blit_1x2_delta_full_16to16d_sxy_fx_fy,
	blit_1x2_delta_16to16d_sxy_fx_fy,

	// 1x2i
	blit_1x2i_delta_full_16to16d,
	blit_1x2i_delta_16to16d,
	blit_1x2i_delta_full_16to16d_fx,
	blit_1x2i_delta_16to16d_fx,
	blit_1x2i_delta_full_16to16d_fy,
	blit_1x2i_delta_16to16d_fy,
	blit_1x2i_delta_full_16to16d_fx_fy,
	blit_1x2i_delta_16to16d_fx_fy,
	blit_1x2i_delta_full_16to16d_sxy,
	blit_1x2i_delta_16to16d_sxy,
	blit_1x2i_delta_full_16to16d_sxy_fx,
	blit_1x2i_delta_16to16d_sxy_fx,
	blit_1x2i_delta_full_16to16d_sxy_fy,
	blit_1x2i_delta_16to16d_sxy_fy,
	blit_1x2i_delta_full_16to16d_sxy_fx_fy,
	blit_1x2i_delta_16to16d_sxy_fx_fy,
	blit_1x2i_delta_full_16to16d_vec,
	blit_1x2i_delta_16to16d_vec,
	blit_1x2i_delta_full_16to16d_fx_vec,
	blit_1x2i_delta_16to16d_fx_vec,
	blit_1x2i_delta_full_16to16d_fy_vec,
	blit_1x2i_delta_16to16d_fy_vec,
	blit_1x2i_delta_full_16to16d_fx_fy_vec,
	blit_1x2i_delta_16to16d_fx_fy_vec,
	blit_1x2i_delta_full_16to16d_sxy,
	blit_1x2i_delta_16to16d_sxy,
	blit_1x2i_delta_full_16to16d_sxy_fx,
	blit_1x2i_delta_16to16d_sxy_fx,
	blit_1x2i_delta_full_16to16d_sxy_fy,
	blit_1x2i_delta_16to16d_sxy_fy,
	blit_1x2i_delta_full_16to16d_sxy_fx_fy,
	blit_1x2i_delta_16to16d_sxy_fx_fy,

	// 2x2
	blit_2x2_delta_full_16to16d,
	blit_2x2_delta_16to16d,
	blit_2x2_delta_full_16to16d_fx,
	blit_2x2_delta_16to16d_fx,
	blit_2x2_delta_full_16to16d_fy,
	blit_2x2_delta_16to16d_fy,
	blit_2x2_delta_full_16to16d_fx_fy,
	blit_2x2_delta_16to16d_fx_fy,
	blit_2x2_delta_full_16to16d_sxy,
	blit_2x2_delta_16to16d_sxy,
	blit_2x2_delta_full_16to16d_sxy_fx,
	blit_2x2_delta_16to16d_sxy_fx,
	blit_2x2_delta_full_16to16d_sxy_fy,
	blit_2x2_delta_16to16d_sxy_fy,
	blit_2x2_delta_full_16to16d_sxy_fx_fy,
	blit_2x2_delta_16to16d_sxy_fx_fy,
	blit_2x2_delta_full_16to16d_vec,
	blit_2x2_delta_16to16d_vec,
	blit_2x2_delta_full_16to16d_fx_vec,
	blit_2x2_delta_16to16d_fx_vec,
	blit_2x2_delta_full_16to16d_fy_vec,
	blit_2x2_delta_16to16d_fy_vec,
	blit_2x2_delta_full_16to16d_fx_fy_vec,
	blit_2x2_delta_16to16d_fx_fy_vec,
	blit_2x2_delta_full_16to16d_sxy,
	blit_2x2_delta_16to16d_sxy,
	blit_2x2_delta_full_16to16d_sxy_fx,
	blit_2x2_delta_16to16d_sxy_fx,
	blit_2x2_delta_full_16to16d_sxy_fy,
	blit_2x2_delta_16to16d_sxy_fy,
	blit_2x2_delta_full_16to16d_sxy_fx_fy,
	blit_2x2_delta_16to16d_sxy_fx_fy,

	// 2x2i
	blit_2x2i_delta_full_16to16d,
	blit_2x2i_delta_16to16d,
	blit_2x2i_delta_full_16to16d_fx,
	blit_2x2i_delta_16to16d_fx,
	blit_2x2i_delta_full_16to16d_fy,
	blit_2x2i_delta_16to16d_fy,
	blit_2x2i_delta_full_16to16d_fx_fy,
	blit_2x2i_delta_16to16d_fx_fy,
	blit_2x2i_delta_full_16to16d_sxy,
	blit_2x2i_delta_16to16d_sxy,
	blit_2x2i_delta_full_16to16d_sxy_fx,
	blit_2x2i_delta_16to16d_sxy_fx,
	blit_2x2i_delta_full_16to16d_sxy_fy,
	blit_2x2i_delta_16to16d_sxy_fy,
	blit_2x2i_delta_full_16to16d_sxy_fx_fy,
	blit_2x2i_delta_16to16d_sxy_fx_fy,
	blit_2x2i_delta_full_16to16d_vec,
	blit_2x2i_delta_16to16d_vec,
	blit_2x2i_delta_full_16to16d_fx_vec,
	blit_2x2i_delta_16to16d_fx_vec,
	blit_2x2i_delta_full_16to16d_fy_vec,
	blit_2x2i_delta_16to16d_fy_vec,
	blit_2x2i_delta_full_16to16d_fx_fy_vec,
	blit_2x2i_delta_16to16d_fx_fy_vec,
	blit_2x2i_delta_full_16to16d_sxy,
	blit_2x2i_delta_16to16d_sxy,
	blit_2x2i_delta_full_16to16d_sxy_fx,
	blit_2x2i_delta_16to16d_sxy_fx,
	blit_2x2i_delta_full_16to16d_sxy_fy,
	blit_2x2i_delta_16to16d_sxy_fy,
	blit_2x2i_delta_full_16to16d_sxy_fx_fy,
	blit_2x2i_delta_16to16d_sxy_fx_fy,

	// 3x3
	blit_3x3_delta_full_16to16d,
	blit_3x3_delta_16to16d,
	blit_3x3_delta_full_16to16d_fx,
	blit_3x3_delta_16to16d_fx,
	blit_3x3_delta_full_16to16d_fy,
	blit_3x3_delta_16to16d_fy,
	blit_3x3_delta_full_16to16d_fx_fy,
	blit_3x3_delta_16to16d_fx_fy,
	blit_3x3_delta_full_16to16d_sxy,
	blit_3x3_delta_16to16d_sxy,
	blit_3x3_delta_full_16to16d_sxy_fx,
	blit_3x3_delta_16to16d_sxy_fx,
	blit_3x3_delta_full_16to16d_sxy_fy,
	blit_3x3_delta_16to16d_sxy_fy,
	blit_3x3_delta_full_16to16d_sxy_fx_fy,
	blit_3x3_delta_16to16d_sxy_fx_fy,
	blit_3x3_delta_full_16to16d_vec,
	blit_3x3_delta_16to16d_vec,
	blit_3x3_delta_full_16to16d_fx_vec,
	blit_3x3_delta_16to16d_fx_vec,
	blit_3x3_delta_full_16to16d_fy_vec,
	blit_3x3_delta_16to16d_fy_vec,
	blit_3x3_delta_full_16to16d_fx_fy_vec,
	blit_3x3_delta_16to16d_fx_fy_vec,
	blit_3x3_delta_full_16to16d_sxy,
	blit_3x3_delta_16to16d_sxy,
	blit_3x3_delta_full_16to16d_sxy_fx,
	blit_3x3_delta_16to16d_sxy_fx,
	blit_3x3_delta_full_16to16d_sxy_fy,
	blit_3x3_delta_16to16d_sxy_fy,
	blit_3x3_delta_full_16to16d_sxy_fx_fy,
	blit_3x3_delta_16to16d_sxy_fx_fy,

	// 3x3i
	blit_3x3i_delta_full_16to16d,
	blit_3x3i_delta_16to16d,
	blit_3x3i_delta_full_16to16d_fx,
	blit_3x3i_delta_16to16d_fx,
	blit_3x3i_delta_full_16to16d_fy,
	blit_3x3i_delta_16to16d_fy,
	blit_3x3i_delta_full_16to16d_fx_fy,
	blit_3x3i_delta_16to16d_fx_fy,
	blit_3x3i_delta_full_16to16d_sxy,
	blit_3x3i_delta_16to16d_sxy,
	blit_3x3i_delta_full_16to16d_sxy_fx,
	blit_3x3i_delta_16to16d_sxy_fx,
	blit_3x3i_delta_full_16to16d_sxy_fy,
	blit_3x3i_delta_16to16d_sxy_fy,
	blit_3x3i_delta_full_16to16d_sxy_fx_fy,
	blit_3x3i_delta_16to16d_sxy_fx_fy,
	blit_3x3i_delta_full_16to16d_vec,
	blit_3x3i_delta_16to16d_vec,
	blit_3x3i_delta_full_16to16d_fx_vec,
	blit_3x3i_delta_16to16d_fx_vec,
	blit_3x3i_delta_full_16to16d_fy_vec,
	blit_3x3i_delta_16to16d_fy_vec,
	blit_3x3i_delta_full_16to16d_fx_fy_vec,
	blit_3x3i_delta_16to16d_fx_fy_vec,
	blit_3x3i_delta_full_16to16d_sxy,
	blit_3x3i_delta_16to16d_sxy,
	blit_3x3i_delta_full_16to16d_sxy_fx,
	blit_3x3i_delta_16to16d_sxy_fx,
	blit_3x3i_delta_full_16to16d_sxy_fy,
	blit_3x3i_delta_16to16d_sxy_fy,
	blit_3x3i_delta_full_16to16d_sxy_fx_fy,
	blit_3x3i_delta_16to16d_sxy_fx_fy
};


static DECLARE_BLIT_FUNC((*blit_table_16to16l[9*32])) =
{
	// vector
	blit_1x1_full_16to16l,
	blit_1x1_dirty_vector_16to16l,
	blit_1x1_full_16to16l_fx,
	blit_1x1_dirty_vector_16to16l_fx,
	blit_1x1_full_16to16l_fy,
	blit_1x1_dirty_vector_16to16l_fy,
	blit_1x1_full_16to16l_fx_fy,
	blit_1x1_dirty_vector_16to16l_fx_fy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_dirty_vector_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_dirty_vector_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_dirty_vector_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_dirty_vector_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_vec,
	blit_1x1_dirty_vector_16to16l_vec,
	blit_1x1_full_16to16l_fx_vec,
	blit_1x1_dirty_vector_16to16l_fx_vec,
	blit_1x1_full_16to16l_fy_vec,
	blit_1x1_dirty_vector_16to16l_fy_vec,
	blit_1x1_full_16to16l_fx_fy_vec,
	blit_1x1_dirty_vector_16to16l_fx_fy_vec,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_dirty_vector_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_dirty_vector_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_dirty_vector_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_dirty_vector_16to16l_sxy_fx_fy,

	// 1x1
	blit_1x1_full_16to16l,
	blit_1x1_full_16to16l,
	blit_1x1_full_16to16l_fx,
	blit_1x1_full_16to16l_fx,
	blit_1x1_full_16to16l_fy,
	blit_1x1_full_16to16l_fy,
	blit_1x1_full_16to16l_fx_fy,
	blit_1x1_full_16to16l_fx_fy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_vec,
	blit_1x1_full_16to16l_vec,
	blit_1x1_full_16to16l_fx_vec,
	blit_1x1_full_16to16l_fx_vec,
	blit_1x1_full_16to16l_fy_vec,
	blit_1x1_full_16to16l_fy_vec,
	blit_1x1_full_16to16l_fx_fy_vec,
	blit_1x1_full_16to16l_fx_fy_vec,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,

	// 1x1i
	blit_1x1_full_16to16l,
	blit_1x1_full_16to16l,
	blit_1x1_full_16to16l_fx,
	blit_1x1_full_16to16l_fx,
	blit_1x1_full_16to16l_fy,
	blit_1x1_full_16to16l_fy,
	blit_1x1_full_16to16l_fx_fy,
	blit_1x1_full_16to16l_fx_fy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_vec,
	blit_1x1_full_16to16l_vec,
	blit_1x1_full_16to16l_fx_vec,
	blit_1x1_full_16to16l_fx_vec,
	blit_1x1_full_16to16l_fy_vec,
	blit_1x1_full_16to16l_fy_vec,
	blit_1x1_full_16to16l_fx_fy_vec,
	blit_1x1_full_16to16l_fx_fy_vec,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fx,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,
	blit_1x1_full_16to16l_sxy_fx_fy,

	// 1x2
	blit_1x2_delta_full_16to16l,
	blit_1x2_delta_16to16l,
	blit_1x2_delta_full_16to16l_fx,
	blit_1x2_delta_16to16l_fx,
	blit_1x2_delta_full_16to16l_fy,
	blit_1x2_delta_16to16l_fy,
	blit_1x2_delta_full_16to16l_fx_fy,
	blit_1x2_delta_16to16l_fx_fy,
	blit_1x2_delta_full_16to16l_sxy,
	blit_1x2_delta_16to16l_sxy,
	blit_1x2_delta_full_16to16l_sxy_fx,
	blit_1x2_delta_16to16l_sxy_fx,
	blit_1x2_delta_full_16to16l_sxy_fy,
	blit_1x2_delta_16to16l_sxy_fy,
	blit_1x2_delta_full_16to16l_sxy_fx_fy,
	blit_1x2_delta_16to16l_sxy_fx_fy,
	blit_1x2_delta_full_16to16l_vec,
	blit_1x2_delta_16to16l_vec,
	blit_1x2_delta_full_16to16l_fx_vec,
	blit_1x2_delta_16to16l_fx_vec,
	blit_1x2_delta_full_16to16l_fy_vec,
	blit_1x2_delta_16to16l_fy_vec,
	blit_1x2_delta_full_16to16l_fx_fy_vec,
	blit_1x2_delta_16to16l_fx_fy_vec,
	blit_1x2_delta_full_16to16l_sxy,
	blit_1x2_delta_16to16l_sxy,
	blit_1x2_delta_full_16to16l_sxy_fx,
	blit_1x2_delta_16to16l_sxy_fx,
	blit_1x2_delta_full_16to16l_sxy_fy,
	blit_1x2_delta_16to16l_sxy_fy,
	blit_1x2_delta_full_16to16l_sxy_fx_fy,
	blit_1x2_delta_16to16l_sxy_fx_fy,

	// 1x2i
	blit_1x2i_delta_full_16to16l,
	blit_1x2i_delta_16to16l,
	blit_1x2i_delta_full_16to16l_fx,
	blit_1x2i_delta_16to16l_fx,
	blit_1x2i_delta_full_16to16l_fy,
	blit_1x2i_delta_16to16l_fy,
	blit_1x2i_delta_full_16to16l_fx_fy,
	blit_1x2i_delta_16to16l_fx_fy,
	blit_1x2i_delta_full_16to16l_sxy,
	blit_1x2i_delta_16to16l_sxy,
	blit_1x2i_delta_full_16to16l_sxy_fx,
	blit_1x2i_delta_16to16l_sxy_fx,
	blit_1x2i_delta_full_16to16l_sxy_fy,
	blit_1x2i_delta_16to16l_sxy_fy,
	blit_1x2i_delta_full_16to16l_sxy_fx_fy,
	blit_1x2i_delta_16to16l_sxy_fx_fy,
	blit_1x2i_delta_full_16to16l_vec,
	blit_1x2i_delta_16to16l_vec,
	blit_1x2i_delta_full_16to16l_fx_vec,
	blit_1x2i_delta_16to16l_fx_vec,
	blit_1x2i_delta_full_16to16l_fy_vec,
	blit_1x2i_delta_16to16l_fy_vec,
	blit_1x2i_delta_full_16to16l_fx_fy_vec,
	blit_1x2i_delta_16to16l_fx_fy_vec,
	blit_1x2i_delta_full_16to16l_sxy,
	blit_1x2i_delta_16to16l_sxy,
	blit_1x2i_delta_full_16to16l_sxy_fx,
	blit_1x2i_delta_16to16l_sxy_fx,
	blit_1x2i_delta_full_16to16l_sxy_fy,
	blit_1x2i_delta_16to16l_sxy_fy,
	blit_1x2i_delta_full_16to16l_sxy_fx_fy,
	blit_1x2i_delta_16to16l_sxy_fx_fy,

	// 2x2
	blit_2x2_delta_full_16to16l,
	blit_2x2_delta_16to16l,
	blit_2x2_delta_full_16to16l_fx,
	blit_2x2_delta_16to16l_fx,
	blit_2x2_delta_full_16to16l_fy,
	blit_2x2_delta_16to16l_fy,
	blit_2x2_delta_full_16to16l_fx_fy,
	blit_2x2_delta_16to16l_fx_fy,
	blit_2x2_delta_full_16to16l_sxy,
	blit_2x2_delta_16to16l_sxy,
	blit_2x2_delta_full_16to16l_sxy_fx,
	blit_2x2_delta_16to16l_sxy_fx,
	blit_2x2_delta_full_16to16l_sxy_fy,
	blit_2x2_delta_16to16l_sxy_fy,
	blit_2x2_delta_full_16to16l_sxy_fx_fy,
	blit_2x2_delta_16to16l_sxy_fx_fy,
	blit_2x2_delta_full_16to16l_vec,
	blit_2x2_delta_16to16l_vec,
	blit_2x2_delta_full_16to16l_fx_vec,
	blit_2x2_delta_16to16l_fx_vec,
	blit_2x2_delta_full_16to16l_fy_vec,
	blit_2x2_delta_16to16l_fy_vec,
	blit_2x2_delta_full_16to16l_fx_fy_vec,
	blit_2x2_delta_16to16l_fx_fy_vec,
	blit_2x2_delta_full_16to16l_sxy,
	blit_2x2_delta_16to16l_sxy,
	blit_2x2_delta_full_16to16l_sxy_fx,
	blit_2x2_delta_16to16l_sxy_fx,
	blit_2x2_delta_full_16to16l_sxy_fy,
	blit_2x2_delta_16to16l_sxy_fy,
	blit_2x2_delta_full_16to16l_sxy_fx_fy,
	blit_2x2_delta_16to16l_sxy_fx_fy,

	// 2x2i
	blit_2x2i_delta_full_16to16l,
	blit_2x2i_delta_16to16l,
	blit_2x2i_delta_full_16to16l_fx,
	blit_2x2i_delta_16to16l_fx,
	blit_2x2i_delta_full_16to16l_fy,
	blit_2x2i_delta_16to16l_fy,
	blit_2x2i_delta_full_16to16l_fx_fy,
	blit_2x2i_delta_16to16l_fx_fy,
	blit_2x2i_delta_full_16to16l_sxy,
	blit_2x2i_delta_16to16l_sxy,
	blit_2x2i_delta_full_16to16l_sxy_fx,
	blit_2x2i_delta_16to16l_sxy_fx,
	blit_2x2i_delta_full_16to16l_sxy_fy,
	blit_2x2i_delta_16to16l_sxy_fy,
	blit_2x2i_delta_full_16to16l_sxy_fx_fy,
	blit_2x2i_delta_16to16l_sxy_fx_fy,
	blit_2x2i_delta_full_16to16l_vec,
	blit_2x2i_delta_16to16l_vec,
	blit_2x2i_delta_full_16to16l_fx_vec,
	blit_2x2i_delta_16to16l_fx_vec,
	blit_2x2i_delta_full_16to16l_fy_vec,
	blit_2x2i_delta_16to16l_fy_vec,
	blit_2x2i_delta_full_16to16l_fx_fy_vec,
	blit_2x2i_delta_16to16l_fx_fy_vec,
	blit_2x2i_delta_full_16to16l_sxy,
	blit_2x2i_delta_16to16l_sxy,
	blit_2x2i_delta_full_16to16l_sxy_fx,
	blit_2x2i_delta_16to16l_sxy_fx,
	blit_2x2i_delta_full_16to16l_sxy_fy,
	blit_2x2i_delta_16to16l_sxy_fy,
	blit_2x2i_delta_full_16to16l_sxy_fx_fy,
	blit_2x2i_delta_16to16l_sxy_fx_fy,

	// 3x3
	blit_3x3_delta_full_16to16l,
	blit_3x3_delta_16to16l,
	blit_3x3_delta_full_16to16l_fx,
	blit_3x3_delta_16to16l_fx,
	blit_3x3_delta_full_16to16l_fy,
	blit_3x3_delta_16to16l_fy,
	blit_3x3_delta_full_16to16l_fx_fy,
	blit_3x3_delta_16to16l_fx_fy,
	blit_3x3_delta_full_16to16l_sxy,
	blit_3x3_delta_16to16l_sxy,
	blit_3x3_delta_full_16to16l_sxy_fx,
	blit_3x3_delta_16to16l_sxy_fx,
	blit_3x3_delta_full_16to16l_sxy_fy,
	blit_3x3_delta_16to16l_sxy_fy,
	blit_3x3_delta_full_16to16l_sxy_fx_fy,
	blit_3x3_delta_16to16l_sxy_fx_fy,
	blit_3x3_delta_full_16to16l_vec,
	blit_3x3_delta_16to16l_vec,
	blit_3x3_delta_full_16to16l_fx_vec,
	blit_3x3_delta_16to16l_fx_vec,
	blit_3x3_delta_full_16to16l_fy_vec,
	blit_3x3_delta_16to16l_fy_vec,
	blit_3x3_delta_full_16to16l_fx_fy_vec,
	blit_3x3_delta_16to16l_fx_fy_vec,
	blit_3x3_delta_full_16to16l_sxy,
	blit_3x3_delta_16to16l_sxy,
	blit_3x3_delta_full_16to16l_sxy_fx,
	blit_3x3_delta_16to16l_sxy_fx,
	blit_3x3_delta_full_16to16l_sxy_fy,
	blit_3x3_delta_16to16l_sxy_fy,
	blit_3x3_delta_full_16to16l_sxy_fx_fy,
	blit_3x3_delta_16to16l_sxy_fx_fy,

	// 3x3i
	blit_3x3i_delta_full_16to16l,
	blit_3x3i_delta_16to16l,
	blit_3x3i_delta_full_16to16l_fx,
	blit_3x3i_delta_16to16l_fx,
	blit_3x3i_delta_full_16to16l_fy,
	blit_3x3i_delta_16to16l_fy,
	blit_3x3i_delta_full_16to16l_fx_fy,
	blit_3x3i_delta_16to16l_fx_fy,
	blit_3x3i_delta_full_16to16l_sxy,
	blit_3x3i_delta_16to16l_sxy,
	blit_3x3i_delta_full_16to16l_sxy_fx,
	blit_3x3i_delta_16to16l_sxy_fx,
	blit_3x3i_delta_full_16to16l_sxy_fy,
	blit_3x3i_delta_16to16l_sxy_fy,
	blit_3x3i_delta_full_16to16l_sxy_fx_fy,
	blit_3x3i_delta_16to16l_sxy_fx_fy,
	blit_3x3i_delta_full_16to16l_vec,
	blit_3x3i_delta_16to16l_vec,
	blit_3x3i_delta_full_16to16l_fx_vec,
	blit_3x3i_delta_16to16l_fx_vec,
	blit_3x3i_delta_full_16to16l_fy_vec,
	blit_3x3i_delta_16to16l_fy_vec,
	blit_3x3i_delta_full_16to16l_fx_fy_vec,
	blit_3x3i_delta_16to16l_fx_fy_vec,
	blit_3x3i_delta_full_16to16l_sxy,
	blit_3x3i_delta_16to16l_sxy,
	blit_3x3i_delta_full_16to16l_sxy_fx,
	blit_3x3i_delta_16to16l_sxy_fx,
	blit_3x3i_delta_full_16to16l_sxy_fy,
	blit_3x3i_delta_16to16l_sxy_fy,
	blit_3x3i_delta_full_16to16l_sxy_fx_fy,
	blit_3x3i_delta_16to16l_sxy_fx_fy
};


static DECLARE_BLIT_FUNC((*blit_table_16to32l[9*32])) =
{
	// vector
	blit_1x1_full_16to32l,
	blit_1x1_dirty_vector_16to32l,
	blit_1x1_full_16to32l_fx,
	blit_1x1_dirty_vector_16to32l_fx,
	blit_1x1_full_16to32l_fy,
	blit_1x1_dirty_vector_16to32l_fy,
	blit_1x1_full_16to32l_fx_fy,
	blit_1x1_dirty_vector_16to32l_fx_fy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_dirty_vector_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_dirty_vector_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_dirty_vector_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_dirty_vector_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_vec,
	blit_1x1_dirty_vector_16to32l_vec,
	blit_1x1_full_16to32l_fx_vec,
	blit_1x1_dirty_vector_16to32l_fx_vec,
	blit_1x1_full_16to32l_fy_vec,
	blit_1x1_dirty_vector_16to32l_fy_vec,
	blit_1x1_full_16to32l_fx_fy_vec,
	blit_1x1_dirty_vector_16to32l_fx_fy_vec,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_dirty_vector_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_dirty_vector_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_dirty_vector_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_dirty_vector_16to32l_sxy_fx_fy,

	// 1x1
	blit_1x1_full_16to32l,
	blit_1x1_full_16to32l,
	blit_1x1_full_16to32l_fx,
	blit_1x1_full_16to32l_fx,
	blit_1x1_full_16to32l_fy,
	blit_1x1_full_16to32l_fy,
	blit_1x1_full_16to32l_fx_fy,
	blit_1x1_full_16to32l_fx_fy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_vec,
	blit_1x1_full_16to32l_vec,
	blit_1x1_full_16to32l_fx_vec,
	blit_1x1_full_16to32l_fx_vec,
	blit_1x1_full_16to32l_fy_vec,
	blit_1x1_full_16to32l_fy_vec,
	blit_1x1_full_16to32l_fx_fy_vec,
	blit_1x1_full_16to32l_fx_fy_vec,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,

	// 1x1i
	blit_1x1_full_16to32l,
	blit_1x1_full_16to32l,
	blit_1x1_full_16to32l_fx,
	blit_1x1_full_16to32l_fx,
	blit_1x1_full_16to32l_fy,
	blit_1x1_full_16to32l_fy,
	blit_1x1_full_16to32l_fx_fy,
	blit_1x1_full_16to32l_fx_fy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_vec,
	blit_1x1_full_16to32l_vec,
	blit_1x1_full_16to32l_fx_vec,
	blit_1x1_full_16to32l_fx_vec,
	blit_1x1_full_16to32l_fy_vec,
	blit_1x1_full_16to32l_fy_vec,
	blit_1x1_full_16to32l_fx_fy_vec,
	blit_1x1_full_16to32l_fx_fy_vec,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fx,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,
	blit_1x1_full_16to32l_sxy_fx_fy,

	// 1x2
	blit_1x2_delta_full_16to32l,
	blit_1x2_delta_16to32l,
	blit_1x2_delta_full_16to32l_fx,
	blit_1x2_delta_16to32l_fx,
	blit_1x2_delta_full_16to32l_fy,
	blit_1x2_delta_16to32l_fy,
	blit_1x2_delta_full_16to32l_fx_fy,
	blit_1x2_delta_16to32l_fx_fy,
	blit_1x2_delta_full_16to32l_sxy,
	blit_1x2_delta_16to32l_sxy,
	blit_1x2_delta_full_16to32l_sxy_fx,
	blit_1x2_delta_16to32l_sxy_fx,
	blit_1x2_delta_full_16to32l_sxy_fy,
	blit_1x2_delta_16to32l_sxy_fy,
	blit_1x2_delta_full_16to32l_sxy_fx_fy,
	blit_1x2_delta_16to32l_sxy_fx_fy,
	blit_1x2_delta_full_16to32l_vec,
	blit_1x2_delta_16to32l_vec,
	blit_1x2_delta_full_16to32l_fx_vec,
	blit_1x2_delta_16to32l_fx_vec,
	blit_1x2_delta_full_16to32l_fy_vec,
	blit_1x2_delta_16to32l_fy_vec,
	blit_1x2_delta_full_16to32l_fx_fy_vec,
	blit_1x2_delta_16to32l_fx_fy_vec,
	blit_1x2_delta_full_16to32l_sxy,
	blit_1x2_delta_16to32l_sxy,
	blit_1x2_delta_full_16to32l_sxy_fx,
	blit_1x2_delta_16to32l_sxy_fx,
	blit_1x2_delta_full_16to32l_sxy_fy,
	blit_1x2_delta_16to32l_sxy_fy,
	blit_1x2_delta_full_16to32l_sxy_fx_fy,
	blit_1x2_delta_16to32l_sxy_fx_fy,

	// 1x2i
	blit_1x2i_delta_full_16to32l,
	blit_1x2i_delta_16to32l,
	blit_1x2i_delta_full_16to32l_fx,
	blit_1x2i_delta_16to32l_fx,
	blit_1x2i_delta_full_16to32l_fy,
	blit_1x2i_delta_16to32l_fy,
	blit_1x2i_delta_full_16to32l_fx_fy,
	blit_1x2i_delta_16to32l_fx_fy,
	blit_1x2i_delta_full_16to32l_sxy,
	blit_1x2i_delta_16to32l_sxy,
	blit_1x2i_delta_full_16to32l_sxy_fx,
	blit_1x2i_delta_16to32l_sxy_fx,
	blit_1x2i_delta_full_16to32l_sxy_fy,
	blit_1x2i_delta_16to32l_sxy_fy,
	blit_1x2i_delta_full_16to32l_sxy_fx_fy,
	blit_1x2i_delta_16to32l_sxy_fx_fy,
	blit_1x2i_delta_full_16to32l_vec,
	blit_1x2i_delta_16to32l_vec,
	blit_1x2i_delta_full_16to32l_fx_vec,
	blit_1x2i_delta_16to32l_fx_vec,
	blit_1x2i_delta_full_16to32l_fy_vec,
	blit_1x2i_delta_16to32l_fy_vec,
	blit_1x2i_delta_full_16to32l_fx_fy_vec,
	blit_1x2i_delta_16to32l_fx_fy_vec,
	blit_1x2i_delta_full_16to32l_sxy,
	blit_1x2i_delta_16to32l_sxy,
	blit_1x2i_delta_full_16to32l_sxy_fx,
	blit_1x2i_delta_16to32l_sxy_fx,
	blit_1x2i_delta_full_16to32l_sxy_fy,
	blit_1x2i_delta_16to32l_sxy_fy,
	blit_1x2i_delta_full_16to32l_sxy_fx_fy,
	blit_1x2i_delta_16to32l_sxy_fx_fy,

	// 2x2
	blit_2x2_delta_full_16to32l,
	blit_2x2_delta_16to32l,
	blit_2x2_delta_full_16to32l_fx,
	blit_2x2_delta_16to32l_fx,
	blit_2x2_delta_full_16to32l_fy,
	blit_2x2_delta_16to32l_fy,
	blit_2x2_delta_full_16to32l_fx_fy,
	blit_2x2_delta_16to32l_fx_fy,
	blit_2x2_delta_full_16to32l_sxy,
	blit_2x2_delta_16to32l_sxy,
	blit_2x2_delta_full_16to32l_sxy_fx,
	blit_2x2_delta_16to32l_sxy_fx,
	blit_2x2_delta_full_16to32l_sxy_fy,
	blit_2x2_delta_16to32l_sxy_fy,
	blit_2x2_delta_full_16to32l_sxy_fx_fy,
	blit_2x2_delta_16to32l_sxy_fx_fy,
	blit_2x2_delta_full_16to32l_vec,
	blit_2x2_delta_16to32l_vec,
	blit_2x2_delta_full_16to32l_fx_vec,
	blit_2x2_delta_16to32l_fx_vec,
	blit_2x2_delta_full_16to32l_fy_vec,
	blit_2x2_delta_16to32l_fy_vec,
	blit_2x2_delta_full_16to32l_fx_fy_vec,
	blit_2x2_delta_16to32l_fx_fy_vec,
	blit_2x2_delta_full_16to32l_sxy,
	blit_2x2_delta_16to32l_sxy,
	blit_2x2_delta_full_16to32l_sxy_fx,
	blit_2x2_delta_16to32l_sxy_fx,
	blit_2x2_delta_full_16to32l_sxy_fy,
	blit_2x2_delta_16to32l_sxy_fy,
	blit_2x2_delta_full_16to32l_sxy_fx_fy,
	blit_2x2_delta_16to32l_sxy_fx_fy,

	// 2x2i
	blit_2x2i_delta_full_16to32l,
	blit_2x2i_delta_16to32l,
	blit_2x2i_delta_full_16to32l_fx,
	blit_2x2i_delta_16to32l_fx,
	blit_2x2i_delta_full_16to32l_fy,
	blit_2x2i_delta_16to32l_fy,
	blit_2x2i_delta_full_16to32l_fx_fy,
	blit_2x2i_delta_16to32l_fx_fy,
	blit_2x2i_delta_full_16to32l_sxy,
	blit_2x2i_delta_16to32l_sxy,
	blit_2x2i_delta_full_16to32l_sxy_fx,
	blit_2x2i_delta_16to32l_sxy_fx,
	blit_2x2i_delta_full_16to32l_sxy_fy,
	blit_2x2i_delta_16to32l_sxy_fy,
	blit_2x2i_delta_full_16to32l_sxy_fx_fy,
	blit_2x2i_delta_16to32l_sxy_fx_fy,
	blit_2x2i_delta_full_16to32l_vec,
	blit_2x2i_delta_16to32l_vec,
	blit_2x2i_delta_full_16to32l_fx_vec,
	blit_2x2i_delta_16to32l_fx_vec,
	blit_2x2i_delta_full_16to32l_fy_vec,
	blit_2x2i_delta_16to32l_fy_vec,
	blit_2x2i_delta_full_16to32l_fx_fy_vec,
	blit_2x2i_delta_16to32l_fx_fy_vec,
	blit_2x2i_delta_full_16to32l_sxy,
	blit_2x2i_delta_16to32l_sxy,
	blit_2x2i_delta_full_16to32l_sxy_fx,
	blit_2x2i_delta_16to32l_sxy_fx,
	blit_2x2i_delta_full_16to32l_sxy_fy,
	blit_2x2i_delta_16to32l_sxy_fy,
	blit_2x2i_delta_full_16to32l_sxy_fx_fy,
	blit_2x2i_delta_16to32l_sxy_fx_fy,

	// 3x3
	blit_3x3_delta_full_16to32l,
	blit_3x3_delta_16to32l,
	blit_3x3_delta_full_16to32l_fx,
	blit_3x3_delta_16to32l_fx,
	blit_3x3_delta_full_16to32l_fy,
	blit_3x3_delta_16to32l_fy,
	blit_3x3_delta_full_16to32l_fx_fy,
	blit_3x3_delta_16to32l_fx_fy,
	blit_3x3_delta_full_16to32l_sxy,
	blit_3x3_delta_16to32l_sxy,
	blit_3x3_delta_full_16to32l_sxy_fx,
	blit_3x3_delta_16to32l_sxy_fx,
	blit_3x3_delta_full_16to32l_sxy_fy,
	blit_3x3_delta_16to32l_sxy_fy,
	blit_3x3_delta_full_16to32l_sxy_fx_fy,
	blit_3x3_delta_16to32l_sxy_fx_fy,
	blit_3x3_delta_full_16to32l_vec,
	blit_3x3_delta_16to32l_vec,
	blit_3x3_delta_full_16to32l_fx_vec,
	blit_3x3_delta_16to32l_fx_vec,
	blit_3x3_delta_full_16to32l_fy_vec,
	blit_3x3_delta_16to32l_fy_vec,
	blit_3x3_delta_full_16to32l_fx_fy_vec,
	blit_3x3_delta_16to32l_fx_fy_vec,
	blit_3x3_delta_full_16to32l_sxy,
	blit_3x3_delta_16to32l_sxy,
	blit_3x3_delta_full_16to32l_sxy_fx,
	blit_3x3_delta_16to32l_sxy_fx,
	blit_3x3_delta_full_16to32l_sxy_fy,
	blit_3x3_delta_16to32l_sxy_fy,
	blit_3x3_delta_full_16to32l_sxy_fx_fy,
	blit_3x3_delta_16to32l_sxy_fx_fy,

	// 3x3i
	blit_3x3i_delta_full_16to32l,
	blit_3x3i_delta_16to32l,
	blit_3x3i_delta_full_16to32l_fx,
	blit_3x3i_delta_16to32l_fx,
	blit_3x3i_delta_full_16to32l_fy,
	blit_3x3i_delta_16to32l_fy,
	blit_3x3i_delta_full_16to32l_fx_fy,
	blit_3x3i_delta_16to32l_fx_fy,
	blit_3x3i_delta_full_16to32l_sxy,
	blit_3x3i_delta_16to32l_sxy,
	blit_3x3i_delta_full_16to32l_sxy_fx,
	blit_3x3i_delta_16to32l_sxy_fx,
	blit_3x3i_delta_full_16to32l_sxy_fy,
	blit_3x3i_delta_16to32l_sxy_fy,
	blit_3x3i_delta_full_16to32l_sxy_fx_fy,
	blit_3x3i_delta_16to32l_sxy_fx_fy,
	blit_3x3i_delta_full_16to32l_vec,
	blit_3x3i_delta_16to32l_vec,
	blit_3x3i_delta_full_16to32l_fx_vec,
	blit_3x3i_delta_16to32l_fx_vec,
	blit_3x3i_delta_full_16to32l_fy_vec,
	blit_3x3i_delta_16to32l_fy_vec,
	blit_3x3i_delta_full_16to32l_fx_fy_vec,
	blit_3x3i_delta_16to32l_fx_fy_vec,
	blit_3x3i_delta_full_16to32l_sxy,
	blit_3x3i_delta_16to32l_sxy,
	blit_3x3i_delta_full_16to32l_sxy_fx,
	blit_3x3i_delta_16to32l_sxy_fx,
	blit_3x3i_delta_full_16to32l_sxy_fy,
	blit_3x3i_delta_16to32l_sxy_fy,
	blit_3x3i_delta_full_16to32l_sxy_fx_fy,
	blit_3x3i_delta_16to32l_sxy_fx_fy
};


static DECLARE_BLIT_FUNC((*blit_table_32to32d[9*32])) =
{
	// vector
	blit_1x1_full_32to32d,
	blit_1x1_dirty_vector_32to32d,
	blit_1x1_full_32to32d_fx,
	blit_1x1_dirty_vector_32to32d_fx,
	blit_1x1_full_32to32d_fy,
	blit_1x1_dirty_vector_32to32d_fy,
	blit_1x1_full_32to32d_fx_fy,
	blit_1x1_dirty_vector_32to32d_fx_fy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_dirty_vector_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_dirty_vector_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_dirty_vector_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_dirty_vector_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_vec,
	blit_1x1_dirty_vector_32to32d_vec,
	blit_1x1_full_32to32d_fx_vec,
	blit_1x1_dirty_vector_32to32d_fx_vec,
	blit_1x1_full_32to32d_fy_vec,
	blit_1x1_dirty_vector_32to32d_fy_vec,
	blit_1x1_full_32to32d_fx_fy_vec,
	blit_1x1_dirty_vector_32to32d_fx_fy_vec,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_dirty_vector_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_dirty_vector_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_dirty_vector_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_dirty_vector_32to32d_sxy_fx_fy,

	// 1x1
	blit_1x1_full_32to32d,
	blit_1x1_full_32to32d,
	blit_1x1_full_32to32d_fx,
	blit_1x1_full_32to32d_fx,
	blit_1x1_full_32to32d_fy,
	blit_1x1_full_32to32d_fy,
	blit_1x1_full_32to32d_fx_fy,
	blit_1x1_full_32to32d_fx_fy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_vec,
	blit_1x1_full_32to32d_vec,
	blit_1x1_full_32to32d_fx_vec,
	blit_1x1_full_32to32d_fx_vec,
	blit_1x1_full_32to32d_fy_vec,
	blit_1x1_full_32to32d_fy_vec,
	blit_1x1_full_32to32d_fx_fy_vec,
	blit_1x1_full_32to32d_fx_fy_vec,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,

	// 1x1i
	blit_1x1_full_32to32d,
	blit_1x1_full_32to32d,
	blit_1x1_full_32to32d_fx,
	blit_1x1_full_32to32d_fx,
	blit_1x1_full_32to32d_fy,
	blit_1x1_full_32to32d_fy,
	blit_1x1_full_32to32d_fx_fy,
	blit_1x1_full_32to32d_fx_fy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_vec,
	blit_1x1_full_32to32d_vec,
	blit_1x1_full_32to32d_fx_vec,
	blit_1x1_full_32to32d_fx_vec,
	blit_1x1_full_32to32d_fy_vec,
	blit_1x1_full_32to32d_fy_vec,
	blit_1x1_full_32to32d_fx_fy_vec,
	blit_1x1_full_32to32d_fx_fy_vec,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fx,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,
	blit_1x1_full_32to32d_sxy_fx_fy,

	// 1x2
	blit_1x2_delta_full_32to32d,
	blit_1x2_delta_32to32d,
	blit_1x2_delta_full_32to32d_fx,
	blit_1x2_delta_32to32d_fx,
	blit_1x2_delta_full_32to32d_fy,
	blit_1x2_delta_32to32d_fy,
	blit_1x2_delta_full_32to32d_fx_fy,
	blit_1x2_delta_32to32d_fx_fy,
	blit_1x2_delta_full_32to32d_sxy,
	blit_1x2_delta_32to32d_sxy,
	blit_1x2_delta_full_32to32d_sxy_fx,
	blit_1x2_delta_32to32d_sxy_fx,
	blit_1x2_delta_full_32to32d_sxy_fy,
	blit_1x2_delta_32to32d_sxy_fy,
	blit_1x2_delta_full_32to32d_sxy_fx_fy,
	blit_1x2_delta_32to32d_sxy_fx_fy,
	blit_1x2_delta_full_32to32d_vec,
	blit_1x2_delta_32to32d_vec,
	blit_1x2_delta_full_32to32d_fx_vec,
	blit_1x2_delta_32to32d_fx_vec,
	blit_1x2_delta_full_32to32d_fy_vec,
	blit_1x2_delta_32to32d_fy_vec,
	blit_1x2_delta_full_32to32d_fx_fy_vec,
	blit_1x2_delta_32to32d_fx_fy_vec,
	blit_1x2_delta_full_32to32d_sxy,
	blit_1x2_delta_32to32d_sxy,
	blit_1x2_delta_full_32to32d_sxy_fx,
	blit_1x2_delta_32to32d_sxy_fx,
	blit_1x2_delta_full_32to32d_sxy_fy,
	blit_1x2_delta_32to32d_sxy_fy,
	blit_1x2_delta_full_32to32d_sxy_fx_fy,
	blit_1x2_delta_32to32d_sxy_fx_fy,

	// 1x2i
	blit_1x2i_delta_full_32to32d,
	blit_1x2i_delta_32to32d,
	blit_1x2i_delta_full_32to32d_fx,
	blit_1x2i_delta_32to32d_fx,
	blit_1x2i_delta_full_32to32d_fy,
	blit_1x2i_delta_32to32d_fy,
	blit_1x2i_delta_full_32to32d_fx_fy,
	blit_1x2i_delta_32to32d_fx_fy,
	blit_1x2i_delta_full_32to32d_sxy,
	blit_1x2i_delta_32to32d_sxy,
	blit_1x2i_delta_full_32to32d_sxy_fx,
	blit_1x2i_delta_32to32d_sxy_fx,
	blit_1x2i_delta_full_32to32d_sxy_fy,
	blit_1x2i_delta_32to32d_sxy_fy,
	blit_1x2i_delta_full_32to32d_sxy_fx_fy,
	blit_1x2i_delta_32to32d_sxy_fx_fy,
	blit_1x2i_delta_full_32to32d_vec,
	blit_1x2i_delta_32to32d_vec,
	blit_1x2i_delta_full_32to32d_fx_vec,
	blit_1x2i_delta_32to32d_fx_vec,
	blit_1x2i_delta_full_32to32d_fy_vec,
	blit_1x2i_delta_32to32d_fy_vec,
	blit_1x2i_delta_full_32to32d_fx_fy_vec,
	blit_1x2i_delta_32to32d_fx_fy_vec,
	blit_1x2i_delta_full_32to32d_sxy,
	blit_1x2i_delta_32to32d_sxy,
	blit_1x2i_delta_full_32to32d_sxy_fx,
	blit_1x2i_delta_32to32d_sxy_fx,
	blit_1x2i_delta_full_32to32d_sxy_fy,
	blit_1x2i_delta_32to32d_sxy_fy,
	blit_1x2i_delta_full_32to32d_sxy_fx_fy,
	blit_1x2i_delta_32to32d_sxy_fx_fy,

	// 2x2
	blit_2x2_delta_full_32to32d,
	blit_2x2_delta_32to32d,
	blit_2x2_delta_full_32to32d_fx,
	blit_2x2_delta_32to32d_fx,
	blit_2x2_delta_full_32to32d_fy,
	blit_2x2_delta_32to32d_fy,
	blit_2x2_delta_full_32to32d_fx_fy,
	blit_2x2_delta_32to32d_fx_fy,
	blit_2x2_delta_full_32to32d_sxy,
	blit_2x2_delta_32to32d_sxy,
	blit_2x2_delta_full_32to32d_sxy_fx,
	blit_2x2_delta_32to32d_sxy_fx,
	blit_2x2_delta_full_32to32d_sxy_fy,
	blit_2x2_delta_32to32d_sxy_fy,
	blit_2x2_delta_full_32to32d_sxy_fx_fy,
	blit_2x2_delta_32to32d_sxy_fx_fy,
	blit_2x2_delta_full_32to32d_vec,
	blit_2x2_delta_32to32d_vec,
	blit_2x2_delta_full_32to32d_fx_vec,
	blit_2x2_delta_32to32d_fx_vec,
	blit_2x2_delta_full_32to32d_fy_vec,
	blit_2x2_delta_32to32d_fy_vec,
	blit_2x2_delta_full_32to32d_fx_fy_vec,
	blit_2x2_delta_32to32d_fx_fy_vec,
	blit_2x2_delta_full_32to32d_sxy,
	blit_2x2_delta_32to32d_sxy,
	blit_2x2_delta_full_32to32d_sxy_fx,
	blit_2x2_delta_32to32d_sxy_fx,
	blit_2x2_delta_full_32to32d_sxy_fy,
	blit_2x2_delta_32to32d_sxy_fy,
	blit_2x2_delta_full_32to32d_sxy_fx_fy,
	blit_2x2_delta_32to32d_sxy_fx_fy,

	// 2x2i
	blit_2x2i_delta_full_32to32d,
	blit_2x2i_delta_32to32d,
	blit_2x2i_delta_full_32to32d_fx,
	blit_2x2i_delta_32to32d_fx,
	blit_2x2i_delta_full_32to32d_fy,
	blit_2x2i_delta_32to32d_fy,
	blit_2x2i_delta_full_32to32d_fx_fy,
	blit_2x2i_delta_32to32d_fx_fy,
	blit_2x2i_delta_full_32to32d_sxy,
	blit_2x2i_delta_32to32d_sxy,
	blit_2x2i_delta_full_32to32d_sxy_fx,
	blit_2x2i_delta_32to32d_sxy_fx,
	blit_2x2i_delta_full_32to32d_sxy_fy,
	blit_2x2i_delta_32to32d_sxy_fy,
	blit_2x2i_delta_full_32to32d_sxy_fx_fy,
	blit_2x2i_delta_32to32d_sxy_fx_fy,
	blit_2x2i_delta_full_32to32d_vec,
	blit_2x2i_delta_32to32d_vec,
	blit_2x2i_delta_full_32to32d_fx_vec,
	blit_2x2i_delta_32to32d_fx_vec,
	blit_2x2i_delta_full_32to32d_fy_vec,
	blit_2x2i_delta_32to32d_fy_vec,
	blit_2x2i_delta_full_32to32d_fx_fy_vec,
	blit_2x2i_delta_32to32d_fx_fy_vec,
	blit_2x2i_delta_full_32to32d_sxy,
	blit_2x2i_delta_32to32d_sxy,
	blit_2x2i_delta_full_32to32d_sxy_fx,
	blit_2x2i_delta_32to32d_sxy_fx,
	blit_2x2i_delta_full_32to32d_sxy_fy,
	blit_2x2i_delta_32to32d_sxy_fy,
	blit_2x2i_delta_full_32to32d_sxy_fx_fy,
	blit_2x2i_delta_32to32d_sxy_fx_fy,

	// 3x3
	blit_3x3_delta_full_32to32d,
	blit_3x3_delta_32to32d,
	blit_3x3_delta_full_32to32d_fx,
	blit_3x3_delta_32to32d_fx,
	blit_3x3_delta_full_32to32d_fy,
	blit_3x3_delta_32to32d_fy,
	blit_3x3_delta_full_32to32d_fx_fy,
	blit_3x3_delta_32to32d_fx_fy,
	blit_3x3_delta_full_32to32d_sxy,
	blit_3x3_delta_32to32d_sxy,
	blit_3x3_delta_full_32to32d_sxy_fx,
	blit_3x3_delta_32to32d_sxy_fx,
	blit_3x3_delta_full_32to32d_sxy_fy,
	blit_3x3_delta_32to32d_sxy_fy,
	blit_3x3_delta_full_32to32d_sxy_fx_fy,
	blit_3x3_delta_32to32d_sxy_fx_fy,
	blit_3x3_delta_full_32to32d_vec,
	blit_3x3_delta_32to32d_vec,
	blit_3x3_delta_full_32to32d_fx_vec,
	blit_3x3_delta_32to32d_fx_vec,
	blit_3x3_delta_full_32to32d_fy_vec,
	blit_3x3_delta_32to32d_fy_vec,
	blit_3x3_delta_full_32to32d_fx_fy_vec,
	blit_3x3_delta_32to32d_fx_fy_vec,
	blit_3x3_delta_full_32to32d_sxy,
	blit_3x3_delta_32to32d_sxy,
	blit_3x3_delta_full_32to32d_sxy_fx,
	blit_3x3_delta_32to32d_sxy_fx,
	blit_3x3_delta_full_32to32d_sxy_fy,
	blit_3x3_delta_32to32d_sxy_fy,
	blit_3x3_delta_full_32to32d_sxy_fx_fy,
	blit_3x3_delta_32to32d_sxy_fx_fy,

	// 3x3i
	blit_3x3i_delta_full_32to32d,
	blit_3x3i_delta_32to32d,
	blit_3x3i_delta_full_32to32d_fx,
	blit_3x3i_delta_32to32d_fx,
	blit_3x3i_delta_full_32to32d_fy,
	blit_3x3i_delta_32to32d_fy,
	blit_3x3i_delta_full_32to32d_fx_fy,
	blit_3x3i_delta_32to32d_fx_fy,
	blit_3x3i_delta_full_32to32d_sxy,
	blit_3x3i_delta_32to32d_sxy,
	blit_3x3i_delta_full_32to32d_sxy_fx,
	blit_3x3i_delta_32to32d_sxy_fx,
	blit_3x3i_delta_full_32to32d_sxy_fy,
	blit_3x3i_delta_32to32d_sxy_fy,
	blit_3x3i_delta_full_32to32d_sxy_fx_fy,
	blit_3x3i_delta_32to32d_sxy_fx_fy,
	blit_3x3i_delta_full_32to32d_vec,
	blit_3x3i_delta_32to32d_vec,
	blit_3x3i_delta_full_32to32d_fx_vec,
	blit_3x3i_delta_32to32d_fx_vec,
	blit_3x3i_delta_full_32to32d_fy_vec,
	blit_3x3i_delta_32to32d_fy_vec,
	blit_3x3i_delta_full_32to32d_fx_fy_vec,
	blit_3x3i_delta_32to32d_fx_fy_vec,
	blit_3x3i_delta_full_32to32d_sxy,
	blit_3x3i_delta_32to32d_sxy,
	blit_3x3i_delta_full_32to32d_sxy_fx,
	blit_3x3i_delta_32to32d_sxy_fx,
	blit_3x3i_delta_full_32to32d_sxy_fy,
	blit_3x3i_delta_32to32d_sxy_fy,
	blit_3x3i_delta_full_32to32d_sxy_fx_fy,
	blit_3x3i_delta_32to32d_sxy_fx_fy
};

/*##########################################################################
	IMPLEMENTATION
##########################################################################*/

#pragma mark ¥ Core Implementation

//===============================================================================
//	Software_GetDisplayDescription
//
//	Fills in the ioDescription field describing our blitter.
//===============================================================================

Boolean Software_GetDisplayDescription(DisplayDescription *ioDescription)
{
	// fill in the informational stuff
	ioDescription->identifier			= 'Soft';
	ioDescription->shortName			= "\pSoftware";
	ioDescription->longName				= "\pStandard software blitter";
	
	// we require the version we're building against, or later
	if (ioDescription->version < VIDEOAPI_VERSION)
		return false;
	
	// we are only filling in fields from the current version
	ioDescription->version				= VIDEOAPI_VERSION;
		
	// fill in the informational stuff
	ioDescription->device	 			= NULL;			// not attached to any particular device
	ioDescription->fullscreen 			= false;		// don't require full screen
	ioDescription->eightbit				= true;			// supports 8 bits per pixel
	ioDescription->sixteenbit			= true;			// supports 16 bits per pixel
	ioDescription->thirtytwobit			= true;			// supports 32 bits per pixel
	ioDescription->scale	 			= true;			// supports scaling
	ioDescription->interlace 			= true;			// supports interlacing

	ioDescription->alignmentRule		= gHasAltivec ? alignDestOnSrc : alignDest;

	// fill in the function pointers
	ioDescription->configure			= NULL;
	ioDescription->allocateDisplay		= AllocateDisplay;
	ioDescription->prepareDisplay		= PrepareDisplay;
	ioDescription->initializePalette	= InitializePalette;
	ioDescription->closeDisplay			= CloseDisplay;
	ioDescription->computeDisplayArea	= ComputeDisplayArea;
	ioDescription->updateDisplay		= UpdateDisplay;
	ioDescription->suspend				= Suspend;
	ioDescription->resume				= Resume;
	ioDescription->displayAboutBox		= DisplayAboutBox;
	ioDescription->pause				= Pause;
	ioDescription->unpause				= Unpause;
	ioDescription->updateVisibleArea	= UpdateVisibleArea;
	ioDescription->computeMaxScale		= ComputeMaxScale;

	return true;
}


//===============================================================================
//	AllocateDisplay
//
//	Allocates any resources we might need for our display. This is the only
//	point at which we can bail and return an error.
//===============================================================================

static Boolean AllocateDisplay(const DisplayParameters *inParams)
{
	double sourceAspect, destAspect;
	
	// see if we should use the 1x2 blitters
	sourceAspect = (double)inParams->viswidth / (double)inParams->visheight;
	destAspect = inParams->aspect;
	sUse1x2Blitters = (sourceAspect > 1.75 * destAspect);

	return true;
}


//===============================================================================
//	PrepareDisplay
//
//	Prepares the display after everything has been allocated.
//===============================================================================

static void PrepareDisplay(const DisplayParameters *inParams)
{
	// remember the original depth
	sOriginalDepth = (*(*inParams->device)->gdPMap)->pixelSize;
	
	// Don't try to switch to 8-bit under OSX
	if (inParams->depth == 8) return;

	// attempt to switch to the proper color depth
	if (sOriginalDepth != inParams->depth)
		SetDepthImmediate(inParams->device, inParams->depth, 1, true);
}


//===============================================================================
//	InitializePalette
//
//	Initializes the palette.
//===============================================================================

static void InitializePalette(const DisplayParameters *inParams)
{
}
	

//===============================================================================
//	CloseDisplay
//
//	Tears down everything we've allocated.
//===============================================================================

static void CloseDisplay(const DisplayParameters *inParams)
{
	// restore the original depth if necessary
	if ((*(*inParams->device)->gdPMap)->pixelSize != sOriginalDepth)
		SetDepthImmediate(inParams->device, sOriginalDepth, 1, true);
	
	// slam the palette back to defaults otherwise
	else if (sOriginalDepth == 8)
	{
		PaletteHandle hackPalette = NewPalette(256, GetCTable(8), pmTolerant + pmExplicit, 0);
		NSetPalette(inParams->window, hackPalette, pmAllUpdates);
		ActivatePalette(inParams->window);
		DisposePalette(hackPalette);
	}
}


//===============================================================================
//	ComputeDisplayArea
//
//	Determines the full and clipped size of our display.
//===============================================================================

static void ComputeDisplayArea(const DisplayParameters *inParams, UInt32 *outWidth, UInt32 *outHeight, Rect *outGlobalBounds)
{
	Rect sbounds;
	
	// compute once with no clipping to get the full width and height
	if (outWidth || outHeight)
	{
		Rect dbounds;
		ComputeSourceAndDest(inParams, &sbounds, &dbounds, false);
		*outWidth = dbounds.right - dbounds.left;
		*outHeight = dbounds.bottom - dbounds.top;
	}
	
	// compute with clipping to get the real dest bounds
	if (outGlobalBounds)
		ComputeSourceAndDest(inParams, &sbounds, outGlobalBounds, true);
}


//===============================================================================
//	ComputeMaxScale
//
//	Determines the maximum supported scale for the current game.
//===============================================================================

static UInt32 ComputeMaxScale(const DisplayParameters *inParams)
{
	return 0x300;	// software blitters support up to 3x
}


//===============================================================================
//	UpdateDisplay
//
//	Updates the display.
//===============================================================================

static Boolean UpdateDisplay(const DisplayParameters *inParams)
{
	Point zeroPoint = { 0, 0 };
	Rect sbounds, dbounds;
	PixMapHandle thePixmap;

	thePixmap = ((*inParams->device)->gdPMap);

	// make sure we can blit here (matching depths only!)
	if ((*thePixmap)->pixelSize != inParams->depth)
	{
		logerror("Software plugin blit failed expected depth %d real depth %d\n", (int) inParams->depth, (int) (*thePixmap)->pixelSize);
		return false;
	}
	
	// set up the standard source and destination boundaries
	ComputeSourceAndDest(inParams, &sbounds, &dbounds, true);
	
	if (!gCursorHidden)
		// we need to shield the cursor
		ShieldCursor(&dbounds, zeroPoint);
	
	// now blit the raster
	if (inParams->depth == 32)
	{
		if (inParams->lookup32)
			DoBlitFast16to32l(inParams, thePixmap, &sbounds, &dbounds);
		else
			DoBlitFast32to32d(inParams, thePixmap, &sbounds, &dbounds);
	}
	else if (inParams->depth == 16)
	{
		if (inParams->lookup16)
			DoBlitFast16to16l(inParams, thePixmap, &sbounds, &dbounds);
		else
			DoBlitFast16to16d(inParams, thePixmap, &sbounds, &dbounds);
	}
		
	// show the cursor again
	if (!gCursorHidden)
		ShowCursorAbsolute();
	
	return true;
}


//===============================================================================
//	Suspend
//
//	Called when we need to pause the system.
//===============================================================================

static void Suspend(const DisplayParameters *inParams)
{
	// restore the original depth if necessary
	if ((*(*inParams->device)->gdPMap)->pixelSize != sOriginalDepth)
		SetDepthImmediate(inParams->device, sOriginalDepth, 1, true);
}


//===============================================================================
//	Resume
//
//	Called when we need to resume the system.
//===============================================================================

static void Resume(const DisplayParameters *inParams)
{
	// attempt to switch to the proper color depth if things have changed
	if ((*(*inParams->device)->gdPMap)->pixelSize != inParams->depth)
		SetDepthImmediate(inParams->device, inParams->depth, 1, true);
}


//===============================================================================
//	DisplayAboutBox
//
//	Displays the about box.
//===============================================================================

static void	DisplayAboutBox(void)
{
	DialogItemIndex itemHit = cancel;
	DialogRef aboutDialog;
	GDHandle savedDevice;
	GWorldPtr savedPort;

	// be sure that we can load the dialog from our own resource fork
	if (!Get1Resource('DITL', kAboutBoxDialogID) || !Get1Resource('DLOG', kAboutBoxDialogID))
	{
		SysBeep(1);
		return;
	}

	// get the dialog box and point to it
	aboutDialog = GetNewDialog(kAboutBoxDialogID, NULL, kWindowToFront);
	GetGWorld(&savedPort, &savedDevice);
	SetPortDialogPort(aboutDialog);

	// set the default item and show the window
	SetDialogDefaultItem(aboutDialog, ok);
	ShowWindow(GetDialogWindow(aboutDialog));
	
	// wait for the OK button to be hit
	while (itemHit != ok)
		ModalDialog(NULL, &itemHit);
	
	// tear it down
	DisposeDialog(aboutDialog);
	SetGWorld(savedPort, savedDevice);
}


//===============================================================================
//	Pause
//
//	Called when we need to pause the emulation.
//===============================================================================

static void Pause(const DisplayParameters *inParams)
{
}


//===============================================================================
//	Unpause
//
//	Called when we need to resume the emulation.
//===============================================================================

static void Unpause(const DisplayParameters *inParams)
{
}


//===============================================================================
//	UpdateVisibleArea
//
//	Called when the visible area changes dynamically.
//===============================================================================

static void UpdateVisibleArea(const DisplayParameters *inParams)
{
}


#pragma mark -
#pragma mark ¥ Utilities

//===============================================================================
//	GetGlobalWindowBounds
//
//	Determines the global bounds of a window.
//===============================================================================

static void GetGlobalWindowBounds(WindowRef inWindow, Rect *outBounds)
{
	GWorldPtr oldPort;
	GDHandle oldDevice;
	
	GetGWorld(&oldPort, &oldDevice);
	SetGWorld(GetWindowPort(inWindow), NULL);
	GetWindowPortBounds(inWindow, outBounds);
//	if (!QDIsPortBuffered (GetWindowPort (inWindow)))
	{
		LocalToGlobal(&((Point *)outBounds)[0]);
		LocalToGlobal(&((Point *)outBounds)[1]);
	}
	SetGWorld(oldPort, oldDevice);
}

//===============================================================================
//	ComputeSourceAndDest
//
//	Given a destination bounding rectangle, compute the source and destination
//	boundaries, with clipping
//===============================================================================

static void ComputeSourceAndDest(const DisplayParameters *inParams, Rect *sbounds, Rect *dbounds, Boolean clip)
{
	int yScale = sUse1x2Blitters ? 2 : 1, xScale = 1;
	int xoffs, yoffs;
	Rect fullDestBounds;
	int visxoffset, visyoffset, viswidth, visheight;

	visxoffset = inParams->visxoffset;
	visyoffset = inParams->visyoffset;
	viswidth = inParams->viswidth;
	visheight = inParams->visheight;

	// determine the entire destination bounds, in global coordinates
	if (inParams->fullscreen)
		fullDestBounds = (*inParams->device)->gdRect;
	else
		GetGlobalWindowBounds(inParams->window, &fullDestBounds);

	// set the default source boundaries
	sbounds->left = visxoffset;
	sbounds->right = visxoffset + viswidth;
	sbounds->top = visyoffset;
	sbounds->bottom = visyoffset + visheight;

	// scale and translate to the default destination boundaries
	if (inParams->swapxy)
	{
		dbounds->left = ((sbounds->top - visyoffset) * inParams->scale * xScale) >> 8;
		dbounds->top = ((sbounds->left - visxoffset) * inParams->scale * yScale) >> 8;
		dbounds->right = ((sbounds->bottom - visyoffset) * inParams->scale * xScale) >> 8;
		dbounds->bottom = ((sbounds->right - visxoffset) * inParams->scale * yScale) >> 8;
	}
	else
	{
		dbounds->left = ((sbounds->left - visxoffset) * inParams->scale * xScale) >> 8;
		dbounds->top = ((sbounds->top - visyoffset) * inParams->scale * yScale) >> 8;
		dbounds->right = ((sbounds->right - visxoffset) * inParams->scale * xScale) >> 8;
		dbounds->bottom = ((sbounds->bottom - visyoffset) * inParams->scale * yScale) >> 8;
	}
	
	// center within the full destination
	xoffs = fullDestBounds.left + ((fullDestBounds.right - fullDestBounds.left) - (dbounds->right - dbounds->left)) / 2;
	yoffs = fullDestBounds.top + ((fullDestBounds.bottom - fullDestBounds.top) - (dbounds->bottom - dbounds->top)) / 2;
	if (!inParams->swapxy)
	{
		int align_offset = 0;

		if (gHasAltivec)
		{	// we want to align on 16-byte boundary
			if (inParams->depth == 32)
			{
				align_offset = (inParams->flipx ? inParams->visxoffset+inParams->viswidth : -inParams->visxoffset) & 3;
			}
			else if (inParams->depth == 16)
			{
				align_offset = (inParams->flipx ? inParams->visxoffset+inParams->viswidth : -inParams->visxoffset) & 7;
			}
		}
		else
		{
			align_offset = 0;
			/*// we want to align on 8-byte boundary
			if (inParams->depth == 32)
			{
				align_offset = (-inParams->visxoffset) & 3;
			}
			else if (inParams->depth == 16)
			{
				align_offset = (-inParams->visxoffset) & 7;
			}*/
		}

		align_offset = align_offset * (inParams->scale >> 8);
		align_offset = (-align_offset) & 7;

		xoffs = (xoffs & ~7) + align_offset;
	}
	dbounds->left += xoffs;
	dbounds->right += xoffs;
	dbounds->top += yoffs;
	dbounds->bottom += yoffs;
	
	// if requested, clip to the screen boundaries
	/*if (clip)
	{
		Rect fullScreenRect;
		
		fullScreenRect = (*inParams->device)->gdRect;

		if (inParams->swapxy)
		{
			if (dbounds->left < fullScreenRect.left)
				sbounds->top += (((fullScreenRect.left - dbounds->left) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->right > fullScreenRect.right)
				sbounds->bottom -= (((dbounds->right - fullScreenRect.right) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->top < fullScreenRect.top)
				sbounds->left += (((fullScreenRect.top - dbounds->top) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->bottom > fullScreenRect.bottom)
				sbounds->right -= (((dbounds->bottom - fullScreenRect.bottom) << 8) + inParams->scale - 1) / inParams->scale;

			// re-scale and re-translate to the destination boundaries after clipping
			dbounds->left = xoffs + (((sbounds->top - visyoffset) * inParams->scale * xScale) >> 8);
			dbounds->top = yoffs + (((sbounds->left - visxoffset) * inParams->scale * yScale) >> 8);
			dbounds->right = xoffs + (((sbounds->bottom - visyoffset) * inParams->scale * xScale) >> 8);
			dbounds->bottom = yoffs + (((sbounds->right - visxoffset) * inParams->scale * yScale) >> 8);
		}
		else
		{
			if (dbounds->left < fullScreenRect.left)
				sbounds->left += (((fullScreenRect.left - dbounds->left) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->right > fullScreenRect.right)
				sbounds->right -= (((dbounds->right - fullScreenRect.right) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->top < fullScreenRect.top)
				sbounds->top += (((fullScreenRect.top - dbounds->top) << 8) + inParams->scale - 1) / inParams->scale;
			if (dbounds->bottom > fullScreenRect.bottom)
				sbounds->bottom -= (((dbounds->bottom - fullScreenRect.bottom) << 8) + inParams->scale - 1) / inParams->scale;

			// re-scale and re-translate to the destination boundaries after clipping
			dbounds->left = xoffs + (((sbounds->left - visxoffset) * inParams->scale * xScale) >> 8);
			dbounds->top = yoffs + (((sbounds->top - visyoffset) * inParams->scale * yScale) >> 8);
			dbounds->right = xoffs + (((sbounds->right - visxoffset) * inParams->scale * xScale) >> 8);
			dbounds->bottom = yoffs + (((sbounds->bottom - visyoffset) * inParams->scale * yScale) >> 8);
		}
	}*/
}
	

//===============================================================================
//	DoBlitFast_16to16d
//
//	direct 16-bit ("15-bit") blitter
//===============================================================================

static void DoBlitFast16to16d(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest)
{
	int srcRB, dstRB, width, height;
	UInt8 *src, *dst;
	int index;


	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + inSource->top * srcRB + 2 * inSource->left;
	width = inSource->right - inSource->left;
	height = inSource->bottom - inSource->top;

	if ((inParams->swapxy) ? (inParams->flipy) : (inParams->flipx))
		src += (inParams->swapxy ? width-1 : width) * 2;

	if ((inParams->swapxy) ? (inParams->flipx) : (inParams->flipy))
		src += srcRB*(height-1);

	// compute the destination parameters
	dstRB = GetPixRowBytes(thePixmap);
	dst = (UInt8 *)GetPixBaseAddr(thePixmap) + inDest->top * dstRB + 2 * inDest->left;

 	// vector case
	if (inParams->vector)
	{
		index = 0;
	}
	// general case
	else
	{
		index = 32;

		// each scale factor increases by 64
		if (sUse1x2Blitters)
			index += 64;
		else if (inParams->scale == 0x100)
			index += 0;
		else if (inParams->scale == 0x200)
			index += 128;
		else
			index += 192;

		// interlacing adds 32 more
		if (inParams->interlace)
			index += 32;
	}

	// use altivec if available
	if (gHasAltivec)
		index += 16;

	// apply rotation
	if (inParams->swapxy)
		index += 8;

	if (inParams->flipy)
		index += 4;

	if (inParams->flipx)
		index += 2;

	// if we don't need to do a full blit, support dirty
	if (!inParams->full)
		index += 1;

	// call the blitter
	(*blit_table_16to16d[index])(src, dst, srcRB, dstRB, (inParams->swapxy) ? height : width, (inParams->swapxy) ? width : height, inParams);
}


//===============================================================================
//	DoBlitFast16
//
//	16-bit version of the above
//===============================================================================

static void DoBlitFast16to16l(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest)
{
	int srcRB, dstRB, width, height;
	UInt8 *src, *dst;
	int index;


	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + inSource->top * srcRB + 2 * inSource->left;
	width = inSource->right - inSource->left;
	height = inSource->bottom - inSource->top;

	if ((inParams->swapxy) ? (inParams->flipy) : (inParams->flipx))
		src += (inParams->swapxy ? width-1 : width) * 2;

	if ((inParams->swapxy) ? (inParams->flipx) : (inParams->flipy))
		src += srcRB*(height-1);

	// compute the destination parameters
	dstRB = GetPixRowBytes(thePixmap);
	dst = (UInt8 *)GetPixBaseAddr(thePixmap) + inDest->top * dstRB + 2 * inDest->left;

 	// vector case
	if (inParams->vector)
	{
		index = 0;
	}
	// general case
	else
	{
		index = 32;

		// each scale factor increases by 64
		if (sUse1x2Blitters)
			index += 64;
		else if (inParams->scale == 0x100)
			index += 0;
		else if (inParams->scale == 0x200)
			index += 128;
		else
			index += 192;

		// interlacing adds 32 more
		if (inParams->interlace)
			index += 32;
	}

	// use altivec if available
	if (gHasAltivec)
		index += 16;

	// apply rotation
	if (inParams->swapxy)
		index += 8;

	if (inParams->flipy)
		index += 4;

	if (inParams->flipx)
		index += 2;

	// if we don't need to do a full blit, support dirty
	if (!inParams->full)
		index += 1;

	// call the blitter
	(*blit_table_16to16l[index])(src, dst, srcRB, dstRB, (inParams->swapxy) ? height : width, (inParams->swapxy) ? width : height, inParams);
}


//===============================================================================
//	DoBlitFast32
//
//	32-bit version of the above
//===============================================================================

static void DoBlitFast16to32l(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest)
{
	int srcRB, dstRB, width, height;
	UInt8 *src, *dst;
	int index;


	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + inSource->top * srcRB + 2 * inSource->left;
	width = inSource->right - inSource->left;
	height = inSource->bottom - inSource->top;

	if ((inParams->swapxy) ? (inParams->flipy) : (inParams->flipx))
		src += (inParams->swapxy ? width-1 : width) * 2;

	if ((inParams->swapxy) ? (inParams->flipx) : (inParams->flipy))
		src += srcRB*(height-1);

	// compute the destination parameters
	dstRB = GetPixRowBytes(thePixmap);
	dst = (UInt8 *)GetPixBaseAddr(thePixmap) + inDest->top * dstRB + 4 * inDest->left;

 	// vector case
	if (inParams->vector)
	{
		index = 0;
	}
	// general case
	else
	{
		index = 32;

		// each scale factor increases by 64
		if (sUse1x2Blitters)
			index += 64;
		else if (inParams->scale == 0x100)
			index += 0;
		else if (inParams->scale == 0x200)
			index += 128;
		else
			index += 192;

		// interlacing adds 32 more
		if (inParams->interlace)
			index += 32;
	}

	// use altivec if available
	if (gHasAltivec)
		index += 16;

	// apply rotation
	if (inParams->swapxy)
		index += 8;

	if (inParams->flipy)
		index += 4;

	if (inParams->flipx)
		index += 2;

	// if we don't need to do a full blit, support dirty
	if (!inParams->full)
		index += 1;

	// call the blitter
	(*blit_table_16to32l[index])(src, dst, srcRB, dstRB, (inParams->swapxy) ? height : width, (inParams->swapxy) ? width : height, inParams);
}


//===============================================================================
//	DoBlitFast32to32d
//
//	32-bit version of the above
//===============================================================================

static void DoBlitFast32to32d(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest)
{
	int srcRB, dstRB, width, height;
	UInt8 *src, *dst;
	int index;


	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + inSource->top * srcRB + 4 * inSource->left;
	width = inSource->right - inSource->left;
	height = inSource->bottom - inSource->top;

	if ((inParams->swapxy) ? (inParams->flipy) : (inParams->flipx))
		src += (inParams->swapxy ? width-1 : width) * 4;

	if ((inParams->swapxy) ? (inParams->flipx) : (inParams->flipy))
		src += srcRB*(height-1);

	// compute the destination parameters
	dstRB = GetPixRowBytes(thePixmap);
	dst = (UInt8 *)GetPixBaseAddr(thePixmap) + inDest->top * dstRB + 4 * inDest->left;

 	// vector case
	if (inParams->vector)
	{
		index = 0;
	}
	// general case
	else
	{
		index = 32;

		// each scale factor increases by 64
		if (sUse1x2Blitters)
			index += 64;
		else if (inParams->scale == 0x100)
			index += 0;
		else if (inParams->scale == 0x200)
			index += 128;
		else
			index += 192;

		// interlacing adds 32 more
		if (inParams->interlace)
			index += 32;
	}

	// use altivec if available
	if (gHasAltivec)
		index += 16;

	// apply rotation
	if (inParams->swapxy)
		index += 8;

	if (inParams->flipy)
		index += 4;

	if (inParams->flipx)
		index += 2;

	// if we don't need to do a full blit, support dirty
	if (!inParams->full)
		index += 1;

	// call the blitter
	(*blit_table_32to32d[index])(src, dst, srcRB, dstRB, (inParams->swapxy) ? height : width, (inParams->swapxy) ? width : height, inParams);
}


//===============================================================================
//	SetDepthImmediate
//
//	Switch the monitor depth and don't return until the depth switch is complete.
//===============================================================================

static OSErr SetDepthImmediate(GDHandle inGD, short inDepth, short inWhichFlags, short inFlags)
{
	OSErr result;
	
	result = SetDepth(inGD, inDepth, inWhichFlags, inFlags);
	{
		// wait for WindowServer to make the switch
		UInt32 tempTicks;
		Delay(60L, &tempTicks);
	}
	return result;
}


#if 1

// **Experimental hack**
//
// These functions are more a proof of concept than an efficient implementation
// of a clipped blitter.  Functionally, they are equivalent to the usual
// software plugin blit functions, but they take an additional Region handle as
// a parameter, and they clip blitting to this region.
//
// Note:
// The Region structure is not documented anywhere in Apple developper
// documentations, but it is described in US patent 4,622,545, which was
// registered by Apple and Bill Atkinson in 1986.  It appears that the Region
// struct has never changed in 15 years: it makes me wonder why Apple never
// accepted to document it to developpers.

Boolean UpdateDisplayWithClip(const DisplayParameters *inParams, RgnHandle clipRgn)
{
	Point zeroPoint = { 0, 0 };
	Rect sbounds, dbounds;
	PixMapHandle thePixmap;

	thePixmap = ((*inParams->device)->gdPMap);

	// make sure we can blit here (matching depths only!)
	if ((*thePixmap)->pixelSize != inParams->depth)
	{
		logerror("Software plugin blit failed expected depth %d real depth %d\n", (int) inParams->depth, (int) (*thePixmap)->pixelSize);
		return false;
	}
	
	// set up the standard source and destination boundaries
	ComputeSourceAndDest(inParams, &sbounds, &dbounds, true);
	
	if (!gCursorHidden)
		// we need to shield the cursor
		ShieldCursor(&dbounds, zeroPoint);
	
	// now blit the raster
	if (inParams->depth == 32)
	{
		if (inParams->lookup32)
			DoBlitWithClip(inParams, thePixmap, &sbounds, &dbounds, clipRgn,
							inParams->lookup32, DoBlitRow16to32lWithClip, 16);
		else
			DoBlitWithClip(inParams, thePixmap, &sbounds, &dbounds, clipRgn,
							NULL, DoBlitRow32to32dWithClip, 32);
	}
	else if (inParams->depth == 16)
	{
		if (inParams->lookup16)
			DoBlitWithClip(inParams, thePixmap, &sbounds, &dbounds, clipRgn,
							inParams->lookup16, DoBlitRow16to16lWithClip, 16);
		else
			DoBlitWithClip(inParams, thePixmap, &sbounds, &dbounds, clipRgn,
							NULL, DoBlitRow16to16dWithClip, 16);
	}

	// show the cursor again
	if (!gCursorHidden)
		ShowCursorAbsolute();
	
	return true;
}

static void DoBlitWithClip(const DisplayParameters *inParams, PixMapHandle thePixmap, const Rect *inSource, const Rect *inDest, RgnHandle clipRgn,
							void *lookup, void (*RowBlitFunc)(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup), int sourceDepth)
{
	enum
	{
		kMaxWidth = 65536/*4096*/
	};
	//OSStatus theErr;
	SInt8 handleState;
	SInt16 *rawRgn;
	int dataLen;
	int x1, x2, y1, y2;
	int i, x, y, next_x, next_y;
	Boolean state;
	UInt8 buffer[kMaxWidth/8];

	int srcRB, dstRB, width, height;
	UInt8 *src, *dst;
	int zoom;
	int yy;


	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + inSource->top * srcRB + (sourceDepth/8) * inSource->left;
	width = inSource->right - inSource->left;
	height = inSource->bottom - inSource->top;

	// compute the destination parameters
	dstRB = GetPixRowBytes(thePixmap);
	dst = (UInt8 *)GetPixBaseAddr(thePixmap) + inDest->top * dstRB + (inParams->depth/8) * inDest->left;

	zoom = inParams->scale >> 8;


	// Save handle state
	handleState = HGetState((Handle) clipRgn);
	HLock((Handle) clipRgn);

	rawRgn = (SInt16 *) (* (Handle) clipRgn);

	// read data lenght
	//dataLen = rawRgn[0];
	dataLen = GetHandleSize((Handle) clipRgn);
	if ((dataLen != rawRgn[0]) || (dataLen < 10) || (dataLen % 2))
		goto error;

	// convert to words
	dataLen /= 2;

	// Read bounds
	y1 = rawRgn[1];
	x1 = rawRgn[2];
	y2 = rawRgn[3];
	x2 = rawRgn[4];

	if ((x2 - x1) > kMaxWidth)
		goto error;

	for (x=x1; x<x2; x/*++*/+=8)
		buffer[(x-x1)>>3] = 0;

	i = 5;
	y = y1;
	yy = 0;
	while (i<dataLen)
	{
		// read y pos of next transition
		next_y = rawRgn[i++];
		if (next_y == 0x7fff)
			// legal end of Region
			break;
		if ((next_y < y) || (next_y > y2))
			goto error;
		while (y < next_y)
		{
			/* apply current buffer to line y */
			RowBlitFunc(src, dst, buffer, width, zoom, lookup);
			dst += dstRB;
			y++;
			if ((++yy) == zoom)
			{
				src += srcRB;
				yy = 0;
			}
		}
		// now apply the transitions
		x = x1;
		state = 0;
		while (i<dataLen)
		{
			next_x = rawRgn[i++];
			if (next_x == 0x7fff)
				// legal end of line
				break;
			if ((next_x < x) || (next_x > x2))
				goto error;
			if (state)
				while (x < next_x)
				{
					buffer[(x-x1)>>3] ^= (0x80 >> ((x-x1) & 7));
					x++;
				}
			else
				x = next_x;

			state = ! state;
		}
		if (state)
			goto error;
	}

	for (x=x1; x<x2; x/*++*/+=8)
		if (buffer[(x-x1)>>3] != 0)
			goto error;

	if (i < dataLen)
		// Mmmh, everything parsed OK, but there are trailing bytes we don't know about
		goto error;

error:
	HSetState((Handle) clipRgn, handleState);
}

static void DoBlitRow16to16dWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *dummy)
{
	UInt16 *s, *d;
	int x, xx;
	UInt16 c;


	s = ((UInt16 *)src)-1;
	d = ((UInt16 *)dst)-1;
	for (x=0; x<width; x++)
	{
		c = *(++s);
		for (xx=0; xx<zoom; xx++)
		{
			if (mask[(x*zoom+xx)>>3] & (0x80 >> ((x*zoom+xx) & 7)))
				*(++d) = c;
			else
				d++;
		}
	}
}

static void DoBlitRow16to16lWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup)
{
	UInt16 *s, *d;
	int x, xx;
	UInt16 c;
	UInt32 *lookup16 = lookup;


	s = ((UInt16 *)src)-1;
	d = ((UInt16 *)dst)-1;
	for (x=0; x<width; x++)
	{
		c = lookup16[*(++s)];
		for (xx=0; xx<zoom; xx++)
		{
			if (mask[(x*zoom+xx)>>3] & (0x80 >> ((x*zoom+xx) & 7)))
				*(++d) = c;
			else
				d++;
		}
	}
}

static void DoBlitRow16to32lWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *lookup)
{
	UInt16 *s;
	UInt32 *d;
	int x, xx;
	UInt32 c;
	UInt32 *lookup32 = lookup;


	s = ((UInt16 *)src)-1;
	d = ((UInt32 *)dst)-1;
	for (x=0; x<width; x++)
	{
		c = lookup32[(*(++s))*2];
		for (xx=0; xx<zoom; xx++)
		{
			if (mask[(x*zoom+xx)>>3] & (0x80 >> ((x*zoom+xx) & 7)))
				*(++d) = c;
			else
				d++;
		}
	}
}

static void DoBlitRow32to32dWithClip(void *src, void *dst, UInt8 mask[], int width, int zoom, void *dummy)
{
	UInt32 *s, *d;
	int x, xx;
	UInt32 c;


	s = ((UInt32 *)src)-1;
	d = ((UInt32 *)dst)-1;
	for (x=0; x<width; x++)
	{
		c = *(++s);
		for (xx=0; xx<zoom; xx++)
		{
			if (mask[(x*zoom+xx)>>3] & (0x80 >> ((x*zoom+xx) & 7)))
				*(++d) = c;
			else
				d++;
		}
	}
}

#endif
