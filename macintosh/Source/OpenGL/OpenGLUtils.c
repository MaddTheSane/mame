/*##########################################################################

	OpenGL Utils.c

	Kent Miller, kpmiller@mac.com
	


	Revision History:
	
	8/14/2001	KM		Rewrite the texture creation function to use
						packed pixel formats.  Had to add a GWorld packing
						function because the version of OpenGL I'm using seems
						to ignore the GL_UNPACK_ROW_LENGTH settings.
	4/21/2001	KM		First revision

##########################################################################*/

#include "OpenGLUtils.h"
#include <QuickTime/ImageCompression.h>

static unsigned int truncPowerOf2(unsigned int x)
{
    int i = 0;
    while ((x = x>>1)) i++;
    return (1<<i);
}

GLuint extensionSupported(const char *extension)
{
  const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;

  if (!extensions)
    extensions = glGetString(GL_EXTENSIONS);

  start = extensions;
  for (;;) {
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return 1;
      }
    }
    start = terminator;
  }
  return 0;
}


// LoadGLTextures ----------------------------------------------------------
OSErr GetApplicationFileSpec(FSSpec * fs)
{
	ProcessInfoRec			p;
	ProcessSerialNumber		ps;
	OSErr					err = noErr;

	//Assume the file is in the same directory as the application
	p.processAppSpec = fs;
	p.processName = nil;
	p.processInfoLength = sizeof(ProcessInfoRec);
	GetCurrentProcess(&ps);
	err = GetProcessInformation(&ps, &p);
	return err;
}


OSErr FSpLoadGLTexture(FSSpec * f, GLuint *textureID)
{

	return FSpLoadGLTextureOfType(f, textureID, GL_RGBA8);
}

OSErr FSpLoadGLTextureOfType(FSSpec * f, GLuint *textureID, GLenum textureType)
{
        GLint		w,h;
        return FSpLoadGLTextureOfTypeSize(f, textureID, textureType, &w, &h);
}

OSErr FSpLoadGLTextureOfTypeSize(FSSpec * f, GLuint *textureID, GLenum textureType, GLint *width, GLint *height)
{
	GWorldPtr	g;
	OSErr		err = noErr;

	err = DrawFileIn32BitGWorld ( f, &g );
	if (noErr != err) return err;
	
	err = Build2DTexturesFrom32BitGWorld(g, textureID, textureType);
	if (noErr != err) return err;
    
	DisposeGWorld(g);

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, height);
    
	return noErr;
}



GLboolean LoadGLTexture(char * filename, GLuint *textureID)
{
	return LoadGLTextureOfType(filename, textureID, GL_RGBA8);
}

GLboolean LoadGLTextureOfType(char * filename, GLuint *textureID, GLenum textureType)
{
    GLint	w, h;

    return LoadGLTextureOfTypeSize(filename,  textureID, textureType, &w, &h);
}

static void printBundleURL(CFBundleRef bndl)
{
    CFStringRef s;
    CFURLRef	url;

    if (bndl == NULL)
    {
        printf("printBundleURL: bndl == NULL\n");
        return;
    }
    
    url = CFBundleCopyBundleURL(bndl);
    if (url == NULL) return;

    s = CFURLGetString(url);
    if (s == NULL) return;

    CFShow(s);
    CFRelease(url);
    CFRelease(s);
}

GLboolean LoadGLTextureOfTypeSize(char * filename, GLuint *textureID, GLenum textureType, GLint *width, GLint *height)
{
	OSErr		err = noErr;
	FSRef		ref;
	CFBundleRef 	myBundle;
	FSSpec			fs;
	CFStringRef	fname;
	CFURLRef		fileURL;
	Boolean		ok;

	myBundle = CFBundleGetMainBundle();

	fname = CFStringCreateWithCString(NULL, (const char *)filename, kCFStringEncodingMacRoman);
	fileURL = CFBundleCopyResourceURL( myBundle, fname, NULL, NULL );

	CFRelease(fname);

	if (NULL == fileURL) return GL_FALSE;
	ok= CFURLGetFSRef(fileURL, &ref);
	CFRelease(fileURL);
	if (!ok)
		return GL_FALSE;
	
    err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &fs, NULL);
    if (noErr != err)
        return GL_FALSE;

    CFRelease( myBundle );
    FSpLoadGLTextureOfTypeSize(&fs, textureID, textureType, width, height);
    return GL_TRUE;
 }

GLboolean LoadGLTextureOfTypeSizeFromURLRef(CFURLRef url, GLuint *textureID, GLenum textureType, GLint *width, GLint *height)
 {
     OSErr		err = noErr;
     FSRef		ref;
     FSSpec		fs;
     Boolean		ok;
     
	if (NULL == url) return GL_FALSE;
	ok= CFURLGetFSRef(url, &ref);
	if (!ok)
		return GL_FALSE;

    err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &fs, NULL);
    if (noErr != err)
        return GL_FALSE;

    FSpLoadGLTextureOfTypeSize(&fs, textureID, textureType, width, height);
    return GL_TRUE;
 }

GLboolean LoadGLTextureOfTypeSizeFromBundle(char *filename, GLuint *textureID, GLenum textureType, GLint *width, GLint *height)
 {
    CFURLRef	bundleURL;
    CFURLRef	url;
    CFStringRef	fname;
    GLboolean	ret;
    
    fname = CFStringCreateWithCString(NULL, (const char *)filename, kCFStringEncodingMacRoman);
    bundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());

    url = CFURLCreateCopyAppendingPathComponent(NULL, bundleURL, fname, FALSE);
    if (url == 0)
        return GL_FALSE;
    ret = LoadGLTextureOfTypeSizeFromURLRef(url, textureID, textureType, width, height);

    CFRelease(fname);
    CFRelease(url);
    CFRelease(bundleURL);

    return ret;
 }

OSErr DrawFileIn32BitGWorld ( FSSpec * fs, GWorldPtr  *g )
{
    GraphicsImportComponent 	gi;
    ComponentResult				cErr;
    ImageDescriptionHandle 		desc;
    GWorldPtr					theGWorld = nil;
    OSErr						err = noErr;
    Rect						r = {0,0,256,256};
    CGrafPtr					savePort;
    GDHandle					saveGD;
    PixMapHandle				pm;
	
	if (kUnresolvedCFragSymbolAddress == (UInt32)GetGraphicsImporterForFile)
		return -1;
	
	GetGWorld ( &savePort, &saveGD );
	err = GetGraphicsImporterForFile(fs, &gi);
	if (err != noErr) goto exit;

	err = GraphicsImportGetNaturalBounds ( gi, &r );
	if (err != noErr) goto exit;
	
	err = NewGWorld(&theGWorld, 32, &r, nil, nil, useTempMem);
	if (err != noErr) goto exit;

	pm = GetGWorldPixMap( theGWorld );
	LockPixels( pm );
	
	GraphicsImportSetGWorld( gi, theGWorld, nil );
	GraphicsImportSetBoundsRect(gi, &r);
/*
	//OpenGL likes it's images upside down (from a QuickDraw perspective)
	//  This call will set the transformation matrix to flip the image
	//
	//	Note:  this is commented out because in the MAME plugin the screen
	//	is already upside down in memory.  So having the texture upside down
	//	is the right thing to do for us
	flipR.top = r.bottom; flipR.left = r.left; 
	flipR.bottom = r.top; flipR.right = r.right;
	GraphicsImportSetDestRect (gi, &flipR);
*/
	GraphicsImportDraw(gi);

	//If the image didn't have an alpha channel, put one in
	//	the Alpha component
	cErr = GraphicsImportGetImageDescription(gi, &desc);
	if (cErr == noErr)
	{
		if ((**desc).depth != 32)
		{
			UInt32 		 	*buf;
			UInt32	 		rowBytes;
			UInt32			longCount;

		 	buf 		 = (UInt32 *)GetPixBaseAddr(pm);
			rowBytes 	 = GetPixRowBytes(pm);
			
			longCount = (rowBytes * r.bottom)/4;
			
			while( --longCount) {
				*buf = *buf | 0xFF000000;
				buf++;
			}
		}
		DisposeHandle((Handle)desc);
	}

	CloseComponent(gi);

	UnlockPixels( pm );
 	*g = theGWorld;

exit:
	SetGWorld ( savePort, saveGD );
	return err;
	}


OSErr Build2DTexturesFrom32BitGWorldH(GWorldPtr g, GLuint *textureID, GLenum textureType, GLint height)
{
    PixMapHandle	gWorldPixMap;
    void		*buffer;
    unsigned long	rowBytes;
    Rect		r;

    if (g == nil) return -1;
    gWorldPixMap = GetGWorldPixMap(g);
    LockPixels(gWorldPixMap);
    
    buffer 	= GetPixBaseAddr(gWorldPixMap);
    rowBytes 	= GetPixRowBytes(gWorldPixMap);
    r		= (**gWorldPixMap).bounds;
    *textureID 	= 0;
    glGenTextures(1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (rowBytes/4));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, textureType, truncPowerOf2(r.right - r.left), height, 0,
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (void *) buffer);
    UnlockPixels(gWorldPixMap);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    return noErr;
}


OSErr Build2DTexturesFrom32BitGWorld(GWorldPtr g, GLuint *textureID, GLenum textureType)
{
    PixMapHandle	gWorldPixMap;
    void		*buffer;
    unsigned long	rowBytes;
    Rect		r;

    if (g == nil) return -1;
    gWorldPixMap = GetGWorldPixMap(g);
    LockPixels(gWorldPixMap);
    
    buffer 	= GetPixBaseAddr(gWorldPixMap);
    rowBytes 	= GetPixRowBytes(gWorldPixMap);
    r		= (**gWorldPixMap).bounds;
    *textureID 	= 0;
    glGenTextures(1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (rowBytes/4));
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, textureType, truncPowerOf2(r.right - r.left), truncPowerOf2(r.bottom - r.top), 0,
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (void *) buffer);
    UnlockPixels(gWorldPixMap);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    return noErr;
}


Boolean		IsFileATexture(FSSpec *fs)
{
    GraphicsImportComponent 	gi;
    OSErr						err = noErr;
    if (kUnresolvedCFragSymbolAddress == (UInt32)GetGraphicsImporterForFile)
        return false;
    err = GetGraphicsImporterForFile(fs, &gi);
    CloseComponent(gi);
    if (err == noErr)
        return true;
	else
		return false;
}


//===============================================================================
//	HasPackedPixel
//
//	Determine if the OpenGL Implementation supports the pixel format we want to
//	use.
//===============================================================================

GLboolean HasPackedPixel(void)
{
	// get version string
	const GLubyte * strVersion = glGetString (GL_VERSION);
	// get extension string
	const GLubyte * strExtension = glGetString (GL_EXTENSIONS);
	if ((strVersion == NULL) && (strExtension == NULL))
		return GL_TRUE;  //OpenGL hasn't been setup yet
	if (strstr ((const char *) strVersion, "1.2") || strstr ((const char *) strExtension, "GL_APPLE_packed_pixel"))
		return GL_TRUE;
	else 
		return GL_FALSE;
}

