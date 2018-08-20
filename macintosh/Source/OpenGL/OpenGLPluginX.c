#include "MacFiles.h"
#include "OpenGLPluginX.h"

#ifndef DECLARE_BLIT_FUNC
#define DECLARE_BLIT_FUNC(name) 						\
	void name(											\
		UInt8 *						inSource, 			\
		UInt8 *						inDest, 			\
		UInt32 						inSourceRowBytes, 	\
		UInt32 						inDestRowBytes, 	\
		UInt32 						inWidth, 			\
		UInt32 						inHeight,			\
		const DisplayParameters	*	inParams)
#endif

#define kOGLSignature	'OGLS'

/*##########################################################################
	PLUGIN FUNCTION PROTOTYPES
##########################################################################*/

static Boolean 	AllocateDisplay(const DisplayParameters *inParams);
static void	 	PrepareDisplay(const DisplayParameters *inParams);
static void		InitializePalette(const DisplayParameters *inParams);
static void		CloseDisplay(const DisplayParameters *inParams);

static void		ComputeDisplayArea(const DisplayParameters *inParams, UInt32 *outWidth, UInt32 *outHeight, Rect *outGlobalBounds);
static Boolean	UpdateDisplay(const DisplayParameters *inParams);

static void		Suspend(const DisplayParameters *inParams);
static void		Resume(const DisplayParameters *inParams);


static void		Pause(const DisplayParameters *inParams);
static void		Unpause(const DisplayParameters *inParams);
static void		UpdateVisibleArea(const DisplayParameters *inParams);

OSStatus 		rebuildGL (recContext *ctx, const DisplayParameters *inParams);
OSStatus		disposeGL (pRecContext pContextInfo);
static void 	ComputeSourceAndDest(const DisplayParameters *inParams, Rect *sbounds, Rect *dbounds, Boolean clip);
static UInt32 	ComputeMaxScale(const DisplayParameters *inParams);
OSStatus 		aglReportError (void);
static void 	ConfigureScreenTexture(const DisplayParameters *inParams);
static void 	ConfigureOverlayTexture(const DisplayParameters *inParams);

static CFURLRef GetOverlayPath(void);
void 			OpenGL_PluginDisplayAboutBox(short inResID, short inVersionTextItem);
static void 	OpenGL_Configure(void);
static void 	OpenGL_ReadPrefs (pRecContext c);
static void 	ConfigureOpenGLToPrefs(pRecContext c);

pRecContext ctx;

//===============================================================================
//	GetDisplayDescription
//
//	Fills in the ioDescription field describing our blitter.
//===============================================================================

Boolean OpenGL_GetDisplayDescription(DisplayDescription *ioDescription)
{
	GDHandle	theGD;
	// fill in the informational stuff
	ioDescription->identifier			= 'OGL ';
	ioDescription->shortName			= "\pOpenGL";
	ioDescription->longName				= "\pMacMAME video renderer using OpenGL";
	
	// we require version 1.0 or later
	if (ioDescription->version < VIDEOAPI_VERSION)
		return false;
	
	theGD = GetMainDevice();
	// we are only filling in fields from the current version
	ioDescription->version				= VIDEOAPI_VERSION;
		
	// fill in the informational stuff
	ioDescription->device	 			= NULL;				// not attached to any particular device
	ioDescription->fullscreen 			= false;			// don't require full screen
	ioDescription->eightbit				= false;			// doesn't do 8 bits per pixel
	ioDescription->sixteenbit			= true;				// supports 16 bits per pixel
	ioDescription->thirtytwobit			= true;				// supports 32 bits per pixel
	ioDescription->scale	 			= true;				// supports scaling
	ioDescription->interlace 			= false;			// doesn't support interlacing
	ioDescription->alignmentRule		= alignDest;		// GPU doesn't care about window alignment
	
	// fill in the function pointers
	ioDescription->configure			= OpenGL_Configure;
	ioDescription->allocateDisplay		= AllocateDisplay;
	ioDescription->prepareDisplay		= PrepareDisplay;
	ioDescription->initializePalette	= InitializePalette;
	ioDescription->closeDisplay			= CloseDisplay;
	ioDescription->computeDisplayArea	= ComputeDisplayArea;
	ioDescription->updateDisplay		= UpdateDisplay;
	ioDescription->suspend				= Suspend;
	ioDescription->resume				= Resume;
	ioDescription->displayAboutBox		= (void (*)())OpenGL_PluginDisplayAboutBox;
	ioDescription->pause				= Pause;
	ioDescription->unpause				= Unpause;
	ioDescription->updateVisibleArea	= UpdateVisibleArea;
	ioDescription->computeMaxScale		= ComputeMaxScale;
	
	return true;
}


//===============================================================================
//	UpdateDisplay
//
//	Updates the display.
//===============================================================================
static DECLARE_BLIT_FUNC((*opengl_blit_table[8])) =
{
	//16 bit blits
	blit_1x1_full_16to16d,			//direct
	blit_1x1_full_16to16l,			//lookup table
	blit_1x1_full_16to16d_vec,		//direct altivec
	blit_1x1_full_16to16l_vec,		//lookup altivec
	//32 bit blits
	blit_1x1_full_32to32d,			
	blit_1x1_full_16to32l,
	blit_1x1_full_32to32d_vec,
	blit_1x1_full_16to32l_vec
};

static void DoBlit(const DisplayParameters *inParams)
{
	pRecContext c = ctx;
	int srcRB;
	UInt8 *src;
	int		srcpixelsize = 2;
	int		key = 0;
	int		xoffset, yoffset, width, height;

	xoffset = inParams->visxoffset;
	width = inParams->viswidth;
	yoffset = inParams->visyoffset;
	height = inParams->visheight;

	if ((inParams->depth == 32) && (0 == inParams->lookup32))
		srcpixelsize=4;

	if (inParams->depth == 32)
	{
		key +=4;
		if (inParams->lookup32)
			key+=1;
	}
	else if (inParams->lookup16)
		key+=1;

	// compute the source parameters
	srcRB = inParams->rowbytes;
	src = (UInt8 *)inParams->bits + yoffset * srcRB + srcpixelsize * xoffset;
	
	//There is about 7% here if you can figure out how to align
	//the source buffer.  But we'll take advantage when it is aligned.
	if ((gHasAltivec) && (0 == ( (unsigned long)src & 0xF)))
	{	
		key +=2;
	}
	
	// For the 16d and 32d formats, the blit here could be skipped by updating directly
	// from the MAME bitmap. However, there is no guarantee that the padding added in
	// bitmap_alloc_core will result in DMA-friendly alignment, so it turns out to be
	// better to use the blit as a memcpy.
	// TO DO: add SAI, Eagle, HQ2X etc blitters. Richard Bannister has optimized source.
	
	(*opengl_blit_table[key])(src, ctx->screenTexture.buffer, srcRB, 
				ctx->screenTexture.rowbytes, width, height, inParams);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, c->screenTexture.w);
	glTexSubImage2D(c->screenTexture.target, 0, 0, 0, width, height,
			c->screenTexture.format, c->screenTexture.type, c->screenTexture.buffer);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}




static Boolean UpdateDisplay(const DisplayParameters *inParams)
{
//  only clear when resizing, since raster content always fills the viewport
//	glClear(GL_COLOR_BUFFER_BIT);
	
	DoBlit(inParams);

	// fill the entire game area with the new frame, allowing for phosphor persistence
	float phosphor = ctx->prefs.persistence;
	if (phosphor < 5) phosphor = 0;				// avoid extreme values
	else if (phosphor > 250) phosphor = 250;
	if (phosphor) glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0-(phosphor/255.0));

	if (ctx->prefs.overlay)
	{
		glBegin(GL_QUADS);	
			glMultiTexCoord2fv(GL_TEXTURE0, &ctx->screenTexCoords[0]);
			glMultiTexCoord2f(GL_TEXTURE1,0,0);					
			glVertex2f(-1.0, -1.0);
			glMultiTexCoord2fv(GL_TEXTURE0, &ctx->screenTexCoords[2]);
			glMultiTexCoord2f(GL_TEXTURE1,ctx->overlayTexture.fullx,0);					
			glVertex2f(1.0, -1.0);
			glMultiTexCoord2fv(GL_TEXTURE0, &ctx->screenTexCoords[4]);
			glMultiTexCoord2f(GL_TEXTURE1,ctx->overlayTexture.fullx,ctx->overlayTexture.fully);					
			glVertex2f(1.0, 1.0);
			glMultiTexCoord2fv(GL_TEXTURE0, &ctx->screenTexCoords[6]);
			glMultiTexCoord2f(GL_TEXTURE1,0,ctx->overlayTexture.fully);					
			glVertex2f(-1.0, 1.0);
		glEnd();
	}
	else
	{
		glBegin(GL_QUADS);	
			glTexCoord2fv(&ctx->screenTexCoords[0]);
			glVertex2f(-1.0, -1.0);
			glTexCoord2fv(&ctx->screenTexCoords[2]);
			glVertex2f(1.0, -1.0);
			glTexCoord2fv(&ctx->screenTexCoords[4]);
			glVertex2f(1.0, 1.0);
			glTexCoord2fv(&ctx->screenTexCoords[6]);
			glVertex2f(-1.0, 1.0);
		glEnd();
	}
	
	glDisable(GL_BLEND);
	aglSwapBuffers(ctx->aglContext);
	
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
	GLint attrib[] = { AGL_RGBA, AGL_DOUBLEBUFFER, AGL_NONE };
	OSErr	err = noErr;

	ctx = calloc(1, sizeof(recContext));

	// build context
	
	ctx->aglPixFmt = aglChoosePixelFormat(NULL, 0, attrib);
	err = aglReportError ();
	if (ctx->aglPixFmt)
	{
		ctx->aglContext = aglCreateContext(ctx->aglPixFmt, NULL);
		err = aglReportError ();
		if (!aglSetCurrentContext(ctx->aglContext))
			err = aglReportError ();
	}
	
	if (err == noErr)
	{
		// now that we have a context, we need to check the max texture size.
		// if the game's pixel dimensions are too big for the video card, abort to SW.
		// several games have dimensions > 1024, which is the max on old Rage128 cards.
		const GLubyte * strExt = glGetString (GL_EXTENSIONS);
		GLint maxtex = 0;
		int padding = (inParams->depth == 32)?7:15;		// align to 32 bytes for DMA
		
		// check the appropriate texture target (2D and RECT have different limits on i.e. GF2MX)
		if (gluCheckExtension ((const unsigned char *)"GL_EXT_texture_rectangle", strExt))
			glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &maxtex);
		else
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtex);

		if ((((inParams->viswidth+padding)&~padding) > maxtex) || (inParams->visheight > maxtex) ||
			// additionally check to see if the game will fit into the maximum viewport at 1x scale.
			(ComputeMaxScale(inParams) < 0x100))
		{
			disposeGL(ctx);
			// we could warn the user with something like the following, but stay silent for now.
			// printf("This game's resolution is larger than your OpenGL video card can handle. Reverting to the software renderer.");
			return false;
		}
		
		// note that, even if the bitmap will fit in the hardware limitations, we can still get
		// a broken texture in low-VRAM situations (i.e. 8 meg Rage 128 machines running at
		// 1024x768x32bpp.) Unfortunately, neither GL_PROXY_TEXTURE_2D nor kCGLRPTextureMemory
		// seem to catch this...
		
	}
	
	OpenGL_ReadPrefs(ctx);
	if (err != noErr)
		return false;

	return true;
}


//===============================================================================
//	PrepareDisplay
//
//	Prepares the display after everything has been allocated.
//===============================================================================

static void PrepareDisplay(const DisplayParameters *inParams)
{
	ctx->window = inParams->window;
	rebuildGL(ctx, inParams);
	aglSetCurrentContext(ctx->aglContext);
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
//	ComputeDisplayArea
//
//	Determines the full and clipped size of our display.
//===============================================================================
static void ComputeDisplayArea(const DisplayParameters *inParams, UInt32 *outWidth, UInt32 *outHeight, Rect *outGlobalBounds)
{
	Rect sbounds, dbounds;
	
	ctx->window = inParams->window;

	// compute once with no clipping to get the full width and height
	if (outWidth || outHeight)
	{
		ComputeSourceAndDest(inParams, &sbounds, &dbounds, false);
		*outWidth = dbounds.right - dbounds.left;
		*outHeight = dbounds.bottom - dbounds.top;
	}
	
	// compute with clipping to get the real dest bounds
	if (outGlobalBounds)
	{
		ComputeSourceAndDest(inParams, &sbounds, outGlobalBounds, true);
	
		if (! EqualRect(&ctx->globalBounds, outGlobalBounds))
		{
			ctx->globalBounds = *outGlobalBounds;
			rebuildGL(ctx, inParams);

			ctx->overlayTexture.fullx = (float)ctx->viewportWidth / (float) ctx->overlayTexture.w;
			ctx->overlayTexture.fully = (float)ctx->viewportHeight / (float) ctx->overlayTexture.h;	
		}
	}

	ctx->unpackRowLength = inParams->rowbytes / (inParams->depth>>3);
	if (0==ctx->unpackRowLength) return;

	if (ctx->screenTexture.id == 0)
		ConfigureScreenTexture(inParams);

	if (ctx->overlayTexture.id == 0)
		ConfigureOverlayTexture(inParams);
	

}

//===============================================================================
//	ComputeMaxScale
//
//	Determines the maximum supported scale for the current game.
//===============================================================================

static UInt32 ComputeMaxScale(const DisplayParameters *inParams)
{
	if (ctx->aglContext)
	{
		// check the maximum viewport size, typically 2048x2048 but bigger for some
		// video cards. Return the maximum scale factor that allows the window to
		// stay within this size.
		
		GLint maxport[2];
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxport);

		int width = inParams->width;
		int height = inParams->height;
		float xScale = 1.0f, yScale = 1.0f;
		
		// compute aspect correction factor, always increasing in one direction
		float aspect = (float)width/height;
		if (aspect < inParams->aspect)
			xScale = inParams->aspect / aspect;
		else
			yScale = aspect / inParams->aspect;
		
		// find the corrected viewport size
		width = roundf(width * xScale);
		height = roundf(height * yScale);
		
		int maxX = maxport[0]/(inParams->swapxy?height:width);
		int maxY = maxport[1]/(inParams->swapxy?width:height);
		
		UInt32 max = (maxX<maxY?maxX:maxY);
		if (max > 3) max = 3;		// limit to 3x for software blitter fallbacks, during pause etc.
		
		return max<<8;				// 0x000, 0x100, 0x200, or 0x300
	}
	return 0x300;
}

//===============================================================================
//	Suspend
//
//	Called when we need to pause the system.
//===============================================================================

static void Suspend(const DisplayParameters *inParams)
{
}


//===============================================================================
//	Resume
//
//	Called when we need to resume the system.
//===============================================================================

static void Resume(const DisplayParameters *inParams)
{
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
	float w = ctx->screenTexture.fullx * ctx->screenTexture.w;
	float h = ctx->screenTexture.fully * ctx->screenTexture.h;
	if (w && h && ctx->screenTexture.buffer)
	{
		// scale the portion of the bitmap that we're texturing from to fit the viewport
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glScalef((float)inParams->viswidth/w, (float)inParams->visheight/h, 1);
		
		// with smoothing on, the edge texels will be filtered with adjacent texels from
		// a previous larger resolution, producing garbage at the edge of the display.
		// we'd need to do strip duplication each frame to get proper edge clamping behavior,
		// but we can at least avoid the garbage by clearing the texture to filter with black: 
		memset(ctx->screenTexture.buffer, 0, ctx->screenTexture.h * ctx->screenTexture.rowbytes);
	}
}


//===============================================================================
//	CloseDisplay
//
//	Tears down everything we've allocated.
//===============================================================================

static void CloseDisplay(const DisplayParameters *inParams)
{
	if (!ctx) return;
	
	glFinish();
	if (ctx->screenTexture.id != 0)
		glDeleteTextures(1, &ctx->screenTexture.id);
	if (ctx->screenTexture.buffer != 0)
		free(ctx->screenTexture.buffer);
	if (ctx->overlayTexture.id != 0)
		glDeleteTextures(1, &ctx->overlayTexture.id);
	free (ctx);
	ctx = NULL;
}

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
#define PRINTRECT(X) printf("%s: %d %d %d %d\n", #X, (X).top, (X).left, (X).bottom, (X).right)

static void ComputeSourceAndDest(const DisplayParameters *inParams, Rect *sbounds, Rect *dbounds, Boolean clip)
{
	float xScale = 1.0f, yScale = 1.0f;
	int scale = inParams->scale >> 8;
	int xoffs, yoffs;
	Rect fullDestBounds;
	int visxoffset, visyoffset, viswidth, visheight;

	// for 0.87a, change to always use the initial game size, to fix resolution switches
	visxoffset = 0;					//inParams->visxoffset;
	visyoffset = 0;					//inParams->visyoffset;
	viswidth = inParams->width;		//viswidth;
	visheight = inParams->height;	//visheight;

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

	// compute aspect correction factor, always increasing in one direction
	float aspect = (float)inParams->width/inParams->height;
	if (aspect < inParams->aspect)
		xScale = inParams->aspect / aspect;
	else
		yScale = aspect / inParams->aspect;

	// scale and translate to the default destination boundaries
	if (inParams->swapxy)
	{
		dbounds->left = roundf((sbounds->top - visyoffset) * scale * yScale);
		dbounds->top = roundf((sbounds->left - visxoffset) * scale * xScale);
		dbounds->right = roundf((sbounds->bottom - visyoffset) * scale * yScale);
		dbounds->bottom = roundf((sbounds->right - visxoffset) * scale * xScale);
	}
	else
	{
		dbounds->left = roundf((sbounds->left - visxoffset) * scale * xScale);
		dbounds->top = roundf((sbounds->top - visyoffset) * scale * yScale);
		dbounds->right = roundf((sbounds->right - visxoffset) * scale * xScale);
		dbounds->bottom = roundf((sbounds->bottom - visyoffset) * scale * yScale);
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
		}

		align_offset = align_offset * scale;
		align_offset = (-align_offset) & 7;

		xoffs = (xoffs & ~7) + align_offset;
	}
	dbounds->left += xoffs;
	dbounds->right += xoffs;
	dbounds->top += yoffs;
	dbounds->bottom += yoffs;
}
	
// ---------------------------------
#define LOG printf
// if error dump agl errors to debugger string, return error
OSStatus aglReportError (void)
{
	GLenum err = aglGetError();
	if (AGL_NO_ERROR != err)
		LOG ((char *) aglErrorString(err));
	// ensure we are returning an OSStatus noErr if no error condition
	if (err == AGL_NO_ERROR)
		return noErr;
	else
		return (OSStatus) err;
}



// handles resizing of GL need context update and if the window dimensions change, a
// a window dimension update, reseting of viewport and an update of the projection matrix
#define OffsetToCenter(p1, p2) ( ((p1) - (p2)) /2 )

OSStatus resizeGL (pRecContext pContextInfo, const DisplayParameters *inParams)
{
    OSStatus err = noErr;
    if (!pContextInfo || !(pContextInfo->aglContext))
        return paramErr;

    if (!aglSetCurrentContext (pContextInfo->aglContext)) err = aglReportError ();
    if (!aglUpdateContext (pContextInfo->aglContext)) err = aglReportError ();

	// ensure camera knows size changed
	
	if (inParams->fullscreen)
	{
		//In this case we do something different from the ComputeSourceAndDest routine which i
		// took from the software renderer.  In this case, we want to stretch the screen to the screen limits
		// in an aspect correct way.
		int w = (*inParams->device)->gdRect.right - (*inParams->device)->gdRect.left;
		int h = (*inParams->device)->gdRect.bottom - (*inParams->device)->gdRect.top;
		Rect dbounds = {0,0,0,0};
		GLint viewport[4];

/*		
#define kHack 0
h -= kHack;
		
		{
			int i = 1;
			GDHandle gd = GetDeviceList();
			while (gd)
			{
				gd = (**gd).gdNextGD;
			}
		}
*/
		{
			// use the screen aspect specified by the machine driver.
			// this is usually 4:3, but some games have 2 or 3 displays, H or V.
			// it could conceivably be something like a 16:9 display as well.
			double aspect = inParams->aspect;
			if (inParams->swapxy) aspect = 1.0/aspect;
			
			// now proportionally scale the aspect rect to fit the host display,
			// using all available room on a widescreen display, if possible.
			if (aspect <= 1.0) 
			{
				//fit in the h direction if possible
				dbounds.right = w;
				dbounds.bottom = floor(w / aspect) ;
			}
			else
			{
				//fit in the v direction if possible
				dbounds.bottom = h;
				dbounds.right =   floor(aspect * h);
			}
			if (aspect < 1.0) aspect = 1.0/aspect;
			
			// if this rect won't fit to the w,h of the screen, shrink it so it will
			if ( dbounds.right > w )
			{
				dbounds.right = w;
				dbounds.bottom = floor(w / aspect);
			}
			else if (dbounds.bottom > h)
			{
				dbounds.bottom = h;
				dbounds.right = floor(h / aspect);
			}
		}		
		
		//Now, finally, set the glViewport to be this rectangle, centered in the window
		{
			Rect gdGlobalRect;  //Rect of the gdevice in global coordinates
 			Rect viewportRect; 	//Rect of the viewport in global coordinates (converted to local)
 			Rect windowRect;	//Window rect in local coordinates
			
			GetWindowPortBounds( ctx->window, &windowRect);
			gdGlobalRect = (*inParams->device)->gdRect;			

			//windowRect.bottom -= kHack;
			
			//Figure out where we want the game in global coordinates
			viewportRect.left = gdGlobalRect.left + OffsetToCenter(w, dbounds.right);
			viewportRect.top = gdGlobalRect.top + OffsetToCenter(h, dbounds.bottom);
			viewportRect.right = viewportRect.left + dbounds.right;
			viewportRect.bottom = viewportRect.top + dbounds.bottom;
			
			//PRINTRECT(windowRect);
			//PRINTRECT(viewportRect);
			//Convert to local coordinates
			GlobalToLocal( (Point *) &viewportRect.top);
			GlobalToLocal( (Point *) &viewportRect.bottom);
			
			//PRINTRECT(viewportRect);
			//Set the AGL buffer rect - this is lower left based so flip the Y coord 
			viewport[0] = (viewportRect.left & ~0x01);
			viewport[1] = windowRect.bottom - viewportRect.bottom;

			viewport[2] = dbounds.right;
			viewport[3] = dbounds.bottom;

			aglSetInteger(pContextInfo->aglContext, AGL_BUFFER_RECT, viewport);
			aglEnable(pContextInfo->aglContext, AGL_BUFFER_RECT);
			
			//Now the glViewport is relative to the buffer rect, so it is simply the dest size of the game
			glViewport ( 0, 0, dbounds.right, dbounds.bottom);
		{
			long swap = pContextInfo->prefs.vblsync;
			if (!aglSetInteger (ctx->aglContext, AGL_SWAP_INTERVAL, &swap))
				aglReportError ();
		}
			ctx->viewportWidth = dbounds.right;
			ctx->viewportHeight = dbounds.bottom;
		}
	}
	else
	{
		Rect sbounds, dbounds;

		ComputeSourceAndDest(inParams, &sbounds, &dbounds, true);

		GlobalToLocal((Point *) &dbounds.top);
		GlobalToLocal((Point *) &dbounds.bottom);

		aglDisable(pContextInfo->aglContext, AGL_BUFFER_RECT); // fix rect when exiting fullscreen mode

		// note that the viewport size for this window is guaranteed to be valid for the current renderer
		// because of prior checks in ComputeMaxScale during display allocation
		
		glViewport (0, 0, dbounds.right - dbounds.left, dbounds.bottom - dbounds.top);

		ctx->viewportWidth = dbounds.right - dbounds.left;
		ctx->viewportHeight = dbounds.bottom - dbounds.top;
	}

    return err;
}


OSStatus rebuildGL (recContext *ctx, const DisplayParameters *inParams)
{
	OSStatus err = noErr;
	Rect rectPort;

	if (ctx->aglContext) {
        GrafPtr portSave = NULL;
        GetPort (&portSave);
        SetPort ((GrafPtr) GetWindowPort (ctx->window));

		GetWindowPortBounds(ctx->window, &rectPort);
		if(!aglSetDrawable(ctx->aglContext, GetWindowPort (ctx->window)))
			err = aglReportError ();
		if (!aglSetCurrentContext(ctx->aglContext))
			err = aglReportError ();
  		aglUpdateContext (ctx->aglContext);
  		// VBL SYNC
		long swap = ctx->prefs.vblsync;
		if (!aglSetInteger (ctx->aglContext, AGL_SWAP_INTERVAL, &swap))
			aglReportError ();

		// setup viewport and prespective
		resizeGL (ctx, inParams); // forces projection matrix update
		
        SetPort (portSave);
    }
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	
    return err;
}

// ---------------------------------

// dump window data structures and OpenGL context and related data structures
OSStatus disposeGL (pRecContext pContextInfo)
{
	// release our data
    if ( pContextInfo != NULL )
    {
		aglSetCurrentContext (NULL);
		aglSetDrawable (pContextInfo->aglContext, NULL);
		if (pContextInfo->aglContext)
		{
			aglDestroyContext (pContextInfo->aglContext);
			pContextInfo->aglContext = NULL;
		}
		if (pContextInfo->aglPixFmt)
		{
			aglDestroyPixelFormat (pContextInfo->aglPixFmt);
			pContextInfo->aglPixFmt = NULL;
		}
    }
    
    return noErr;
}

static void ConfigureScreenTexture(const DisplayParameters *inParams)
{
	GLint	texw = inParams->viswidth, paddedw;
	GLint	texh = inParams->visheight, paddedh;
	const GLubyte * strExt = glGetString (GL_EXTENSIONS);
	int padding = 0;

	
	ctx->screenTexture.format = GL_BGRA;

	if (inParams->depth == 32)
	{
		ctx->screenTexture.type = GL_UNSIGNED_INT_8_8_8_8_REV;
		padding = 7;
	}
	else
	{
		ctx->screenTexture.type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		padding = 15;
	}
	
	if (gluCheckExtension ((const unsigned char *)"GL_EXT_texture_rectangle", strExt))
		ctx->screenTexture.target = GL_TEXTURE_RECTANGLE_EXT;
	else
		ctx->screenTexture.target = GL_TEXTURE_2D;

	paddedw = texw;
	paddedh = texh;

	if (ctx->screenTexture.target == GL_TEXTURE_2D)
	{
		if ( 0 ==IsPowerOf2(texw) ) 
			paddedw = NextPowerOf2(texw);
		if ( 0 ==IsPowerOf2(texh) ) 
			paddedh = NextPowerOf2(texh);
	}
	else
	{
		// align the rows to 8 pixels (32 bytes for DMA)
		if ( 0 ==IsPowerOf2(texw) ) 
			paddedw = (texw+padding) & ~padding;

	}
		
	ctx->screenTexture.h = paddedh;
	ctx->screenTexture.w = paddedw;
	ctx->screenTexture.fullx = (float) texw  / (float) paddedw;
	ctx->screenTexture.fully = (float) texh / (float) paddedh;
	
	ctx->screenTexture.rowbytes = ctx->screenTexture.w * (inParams->depth>>3);
	ctx->screenTexture.buffer = malloc(ctx->screenTexture.h * ctx->screenTexture.rowbytes);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &ctx->screenTexture.id);
	glBindTexture(ctx->screenTexture.target, ctx->screenTexture.id);
	glEnable(ctx->screenTexture.target);
	
	if (gluCheckExtension ((const unsigned char *)"GL_APPLE_texture_range", strExt))
	{
		//Ask OpenGL to map our memory into AGP for DMA or texturing
		glTextureRangeAPPLE(ctx->screenTexture.target,  ctx->screenTexture.h * ctx->screenTexture.rowbytes, ctx->screenTexture.buffer);
		glTexParameteri(ctx->screenTexture.target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
	}

	//Tell OpenGL we will keep the texture around ourselves so don't copy it.
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);				// all OS X renderers support APPLE_client_storage
    glTexParameteri(ctx->screenTexture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// and SGIS_texture_edge_clamp
    glTexParameteri(ctx->screenTexture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(ctx->screenTexture.target, 0,
		ctx->screenTexture.type == GL_UNSIGNED_INT_8_8_8_8_REV ? GL_RGBA8 : GL_RGB5_A1,
		paddedw, paddedh, 0, ctx->screenTexture.format, ctx->screenTexture.type,
		ctx->screenTexture.buffer);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 0);				// disable for overlay texture

	// because the optimal BGRA bitmap coming in has zeroed alpha, we need to fix the combine mode for persistence to work
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);		// all OS X renderers support ARB_texture_env_combine
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);	// this value will have the phosphor alpha, usually 1.0
			
	//Figure out the texture coordinates of the screen based on the swapping
	//
	{
		int  swapKey = ( (inParams->flipx!=0) << 2) |  ( (inParams->flipy!=0) << 1) | ( (inParams->swapxy!=0));
		float	xyTex2D[] = { 0, 						0, 
						ctx->screenTexture.fullx, 	0, 
						ctx->screenTexture.fullx, 	ctx->screenTexture.fully, 
						0, 							ctx->screenTexture.fully };
		float	xyTexRectangle[] = { 0, 	0, 
									texw, 	0, 
									texw, 	texh, 
									0, 		texh };
		float	*xy = xyTexRectangle;
		if (ctx->screenTexture.target == GL_TEXTURE_2D)
			xy = xyTex2D;
			
			
		switch (swapKey)
		{
			case 0:
				ctx->screenTexCoords[0] = xy[0]; ctx->screenTexCoords[1] = xy[7];
				ctx->screenTexCoords[2] = xy[2]; ctx->screenTexCoords[3] = xy[5];
				ctx->screenTexCoords[4] = xy[4]; ctx->screenTexCoords[5] = xy[3];
				ctx->screenTexCoords[6] = xy[6]; ctx->screenTexCoords[7] = xy[1];
				break;

			case 1:
				ctx->screenTexCoords[0] = xy[4]; ctx->screenTexCoords[1] = xy[3];
				ctx->screenTexCoords[2] = xy[2]; ctx->screenTexCoords[3] = xy[5];
				ctx->screenTexCoords[4] = xy[0]; ctx->screenTexCoords[5] = xy[7];
				ctx->screenTexCoords[6] = xy[6]; ctx->screenTexCoords[7] = xy[1];
				break;
			
			case 2:
				ctx->screenTexCoords[0] = xy[0]; ctx->screenTexCoords[1] = xy[1];
				ctx->screenTexCoords[2] = xy[2]; ctx->screenTexCoords[3] = xy[3];
				ctx->screenTexCoords[4] = xy[4]; ctx->screenTexCoords[5] = xy[5];
				ctx->screenTexCoords[6] = xy[6]; ctx->screenTexCoords[7] = xy[7];
				break;
			
			case 3:
				ctx->screenTexCoords[0] = xy[0]; ctx->screenTexCoords[1] = xy[1];
				ctx->screenTexCoords[2] = xy[6]; ctx->screenTexCoords[3] = xy[7];
				ctx->screenTexCoords[4] = xy[4]; ctx->screenTexCoords[5] = xy[5];
				ctx->screenTexCoords[6] = xy[2]; ctx->screenTexCoords[7] = xy[3];
				break;
			
			case 4:
				ctx->screenTexCoords[0] = xy[2]; ctx->screenTexCoords[1] = xy[5];
				ctx->screenTexCoords[2] = xy[0]; ctx->screenTexCoords[3] = xy[7];
				ctx->screenTexCoords[4] = xy[6]; ctx->screenTexCoords[5] = xy[1];
				ctx->screenTexCoords[6] = xy[4]; ctx->screenTexCoords[7] = xy[3];
				break;

			case 5:
				ctx->screenTexCoords[0] = xy[4]; ctx->screenTexCoords[1] = xy[5];
				ctx->screenTexCoords[2] = xy[2]; ctx->screenTexCoords[3] = xy[3];
				ctx->screenTexCoords[4] = xy[0]; ctx->screenTexCoords[5] = xy[1];
				ctx->screenTexCoords[6] = xy[6]; ctx->screenTexCoords[7] = xy[7];
				break;
			
			case 6:
				ctx->screenTexCoords[0] = xy[4]; ctx->screenTexCoords[1] = xy[3];
				ctx->screenTexCoords[2] = xy[6]; ctx->screenTexCoords[3] = xy[1];
				ctx->screenTexCoords[4] = xy[0]; ctx->screenTexCoords[5] = xy[7];
				ctx->screenTexCoords[6] = xy[2]; ctx->screenTexCoords[7] = xy[5];
				break;
				
			case 7:
				ctx->screenTexCoords[0] = xy[6]; ctx->screenTexCoords[1] = xy[7];
				ctx->screenTexCoords[2] = xy[0]; ctx->screenTexCoords[3] = xy[1];
				ctx->screenTexCoords[4] = xy[2]; ctx->screenTexCoords[5] = xy[3];
				ctx->screenTexCoords[6] = xy[4]; ctx->screenTexCoords[7] = xy[5];
				break;
			
			default:
				ctx->screenTexCoords[0] = xy[0]; ctx->screenTexCoords[1] = xy[1];
				ctx->screenTexCoords[2] = xy[2]; ctx->screenTexCoords[3] = xy[3];
				ctx->screenTexCoords[4] = xy[4]; ctx->screenTexCoords[5] = xy[5];
				ctx->screenTexCoords[6] = xy[6]; ctx->screenTexCoords[7] = xy[7];
				break;
		}
		
	}
	ConfigureOpenGLToPrefs(ctx);	
}

static UInt32	defaultOverlay[] = {0xB3B3B300, 0xB3B3B300, 0xB3B3B300, 0xB3B3B300, 
							0xDEFF947f, 0x94DEFF7f, 0xB3B3B300, 0xFF94DE7f,
							0xDEFF947f, 0x94DEFF7f, 0xB3B3B300, 0xFF94DE7f,
							0xDEFF947f, 0x94DEFF7f, 0xB3B3B300, 0xFF94DE7f,
							0xB3B3B300, 0xB3B3B300, 0xB3B3B300, 0xB3B3B300, 
							0xB3B3B300, 0xFF94DE7f, 0xDEFF947f, 0x94DEFF7f,
							0xB3B3B300, 0xFF94DE7f, 0xDEFF947f, 0x94DEFF7f,
							0xB3B3B300, 0xFF94DE7f, 0xDEFF947f, 0x94DEFF7f
							};
static UInt32	defaultOverlayWidth 	= 4;
static UInt32	defaultOverlayHeight 	= 8;

static void ConfigureOverlayTexture(const DisplayParameters *inParams)
{
	CFURLRef	overlay = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, GetOverlayPath(), ctx->prefs.overlayFileName, false);
	
	glActiveTexture(GL_TEXTURE1);
	ctx->overlayTexture.type = GL_UNSIGNED_INT_8_8_8_8_REV;
	ctx->overlayTexture.format = GL_BGRA;
	LoadGLTextureOfTypeSizeFromURLRef(overlay, &ctx->overlayTexture.id, GL_RGBA8, 
			&ctx->overlayTexture.w,  &ctx->overlayTexture.h);
	
	if (ctx->overlayTexture.id == 0)
	{
		glGenTextures(1, &ctx->overlayTexture.id);
		glBindTexture(GL_TEXTURE_2D, ctx->overlayTexture.id);
		ctx->overlayTexture.w = defaultOverlayWidth;
		ctx->overlayTexture.h = defaultOverlayHeight;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ctx->overlayTexture.w,ctx->overlayTexture.h, 
				0, GL_RGBA, GL_UNSIGNED_BYTE, defaultOverlay);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTexture(GL_TEXTURE0);
	ctx->overlayTexture.fullx = (float)ctx->screenTexture.w / (float) ctx->overlayTexture.w;
	ctx->overlayTexture.fully = (float)ctx->screenTexture.h / (float) ctx->overlayTexture.h;
	
	CFRelease(overlay);

	ConfigureOpenGLToPrefs(ctx);	
}
#pragma mark -

//===============================================================================
//	DisplayAboutBox
//
//	Displays the about box.
//===============================================================================

static pascal OSStatus openGLDialogEventHandler (EventHandlerCallRef inCallRef, EventRef inEvent, void *inUserData)
{
	OSStatus  err = eventNotHandledErr;
	WindowRef windowRef;
	HICommandExtended cmd;
	Boolean value;
	ControlID  menuID = { kOGLSignature, 4 };
	ControlRef menuControl;
	
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
				case 'Arcd':
					value = GetControl32BitValue (cmd.source.control);
					SetPrefAsBoolean (value, CFSTR("openGL_arcade"));

					GetControlByID( windowRef, &menuID, &menuControl );

					if (value)
						EnableControl (menuControl);
					else
						DisableControl (menuControl);
						
					err = noErr;
					break;
				case 'Ovrl':
					{
						CFStringRef stringRef;
						value = GetControl32BitValue (cmd.source.control);
						CopyMenuItemTextAsCFString (GetControlPopupMenuHandle (cmd.source.control), value, &stringRef);
						if (stringRef)
						{
							SetPrefAsCFString (stringRef, CFSTR("openGL_Overlay"));
							CFRelease (stringRef);
						}
					}
					err = noErr;
					break;
				case 'Smth':
					value = GetControl32BitValue (cmd.source.control);
					SetPrefAsBoolean (value, CFSTR("openGL_bilinear"));
					err = noErr;
					break;
				case 'Sync':
					value = GetControl32BitValue (cmd.source.control);
					SetPrefAsBoolean (value, CFSTR("openGL_vblsync"));
					err = noErr;
					break;
				case 'Prst':
					value = GetControl32BitValue (cmd.source.control);
					SetPrefAsInt (value, CFSTR("openGL_persistence"));
					err = noErr;
					break;
			}
			break;
	}
 
	return err;
}

CFURLRef overlayPath = NULL;
static CFURLRef GetOverlayPath(void)
{
	if (overlayPath != NULL)
		return overlayPath;

	overlayPath = CopyCFURLForMAMEFolder (MAC_FILETYPE_OPENGL_OVERLAY);

	return overlayPath;
}

void LoadOverlayMenu(ControlRef control)
{
	CFURLRef	pathURL = GetOverlayPath();
	MenuRef		menu = GetControlPopupMenuHandle(control);
	OSStatus	err;
	CFStringRef overlayPrefString;
	MenuItemIndex itemNum;
	
	verify_action(menu, return);
	
	DeleteMenuItems (menu, 1, CountMenuItems (menu));
	AppendMenuItemTextWithCFString (menu, CFSTR("Default"), 0, 0, 0);
	AppendMenuItemTextWithCFString (menu, NULL, kMenuItemAttrSeparator, 0, 0);
	itemNum = 1;

	SetPrefID_App ();
	GetPrefAsCFString (&overlayPrefString, CFSTR("openGL_Overlay"), CFSTR("Default"));
	require(overlayPrefString, cantParseDirectory);
	
	// Add any valid textures found in the "OpenGL Overlays" folder
	if (pathURL)	
	{
		FSRef		pathRef;
		FSIterator	fsIterator;
		ItemCount	itemsFound;
		FSSpec		fsSpec;
		HFSUniStr255 fileName;

		require(CFURLGetFSRef (pathURL, &pathRef), cantParseDirectory);
		
		err = FSOpenIterator (&pathRef, kFSIterateFlat, &fsIterator);
		require_noerr(err, cantParseDirectory);
		
		// Given a list of files in this directory, see which are valid texture types.
		while (1)
		{
			if (FSGetCatalogInfoBulk (fsIterator, 1, &itemsFound, NULL, NULL, NULL, NULL, &fsSpec, &fileName))
				break;
			
			if (IsFileATexture (&fsSpec))
			{
				MenuItemIndex newItemNum;
				CFStringRef fileString = CFStringCreateWithCharacters (kCFAllocatorDefault, fileName.unicode, fileName.length);
				verify_action(fileString, continue);
				
				// This file is a valid texture, add it to the menu.
				AppendMenuItemTextWithCFString (menu, fileString, 0, 0, &newItemNum);
				
				// If the name matches our overlay preference, then mark it as selected.
				if (CFStringCompare (fileString, overlayPrefString, 0) == 0)
					itemNum = newItemNum;

				CFRelease (fileString);
			}
		}

		FSCloseIterator (fsIterator);
	}

cantParseDirectory:
	if (overlayPrefString)
		CFRelease (overlayPrefString);

	SetControlMinimum (control, 1);
	SetControlMaximum (control, CountMenuItems (menu));
	SetControl32BitValue (control, itemNum);
}


void OpenGL_PluginDisplayAboutBox(short inResID, short inVersionTextItem)
{
	WindowRef myDialog;
	OSStatus err;
	IBNibRef dialogNib;
	EventTypeSpec      dialogEvents[] = {{ kEventClassCommand, kEventCommandProcess } };

	err = CreateNibReference (CFSTR("OpenGLPrefs"), &dialogNib);
	if (err) return;

	err = CreateWindowFromNib (dialogNib, CFSTR("About"), &myDialog); 
	if (err) return;

	InstallWindowEventHandler(myDialog, NewEventHandlerUPP(openGLDialogEventHandler), GetEventTypeCount(dialogEvents), dialogEvents, myDialog, NULL); 

	ShowWindow (myDialog);
	RunAppModalLoopForWindow (myDialog);
	DisposeWindow (myDialog);
	DisposeNibReference (dialogNib);
}

static void OpenGL_Configure(void)
{
	WindowRef myDialog;
	OSStatus err;
	IBNibRef dialogNib;
	EventTypeSpec      dialogEvents[] = {{ kEventClassCommand, kEventCommandProcess } };

	err = CreateNibReference (CFSTR("OpenGLPrefs"), &dialogNib);
	if (err) return;

	err = CreateWindowFromNib (dialogNib, CFSTR("Prefs"), &myDialog); 
	if (err) return;

	{
		ControlRef  control, menuControl;
		ControlID  smoothID = { kOGLSignature, 2 };
		ControlID  arcadeID = { kOGLSignature, 3 };
		ControlID  menuID = { kOGLSignature, 4 };
		ControlID  syncID = { kOGLSignature, 5 };
		ControlID  prstID = { kOGLSignature, 6 };
		Boolean		b;
		int			i;

		SetPrefID_App ();
		GetPrefAsBoolean(&b, CFSTR("openGL_bilinear"),true);
		GetControlByID( myDialog, &smoothID, &control );
		if (control)
			SetControlValue( control, b );

		GetPrefAsBoolean(&b, CFSTR("openGL_vblsync"),true);
		GetControlByID( myDialog, &syncID, &control );
		if (control)
			SetControlValue( control, b );

		GetPrefAsBoolean(&b, CFSTR("openGL_arcade"),false);
		GetControlByID( myDialog, &arcadeID, &control );
		if (control)
			SetControlValue( control, b );

		err = GetControlByID( myDialog, &menuID, &menuControl );
		if (err) return;
		
		LoadOverlayMenu(menuControl);

		if (b)
			EnableControl (menuControl);
		else
			DisableControl (menuControl);
			
		GetPrefAsInt(&i, CFSTR("openGL_persistence"),0);
		GetControlByID( myDialog, &prstID, &control );
		if (control)
			SetControlValue( control, i );
	}

	InstallWindowEventHandler(myDialog, NewEventHandlerUPP(openGLDialogEventHandler), GetEventTypeCount(dialogEvents), dialogEvents, myDialog, NULL); 

	// draw the dialog and find the ROMsets before going into modal land
	ShowWindow (myDialog);

	RunAppModalLoopForWindow (myDialog);

	DisposeWindow (myDialog);
	DisposeNibReference (dialogNib);
}


static void OpenGL_ReadPrefs (pRecContext c)
{
	SetPrefID_App ();
	GetPrefAsBoolean(&c->prefs.bilinear,CFSTR("openGL_bilinear"),true);
	GetPrefAsBoolean(&c->prefs.vblsync,CFSTR("openGL_vblsync"),true);
	GetPrefAsBoolean(&c->prefs.overlay,CFSTR("openGL_arcade"),false);
	if (c->prefs.overlayFileName)
		CFRelease (c->prefs.overlayFileName);
	GetPrefAsCFString (&c->prefs.overlayFileName, CFSTR("openGL_Overlay"), CFSTR("Default"));
	GetPrefAsInt(&c->prefs.persistence,CFSTR("openGL_persistence"),0);
}

/* Call whenever the preferences change, perhaps by a key combo */
static void ConfigureOpenGLToPrefs(pRecContext c)
{
	OpenGL_ReadPrefs(c);
	if (c->prefs.overlay)
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	else
	{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	

	if (ctx->prefs.bilinear)
	{
		glTexParameteri(c->screenTexture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(c->screenTexture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(c->screenTexture.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(c->screenTexture.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}	
	
	long swap = ctx->prefs.vblsync;
	if (!aglSetInteger (ctx->aglContext, AGL_SWAP_INTERVAL, &swap))
		aglReportError ();
}
