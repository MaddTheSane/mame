#include <OpenGL/OpenGL.h>
#include <AGL/agl.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include "macblitters.h"
#include "mac.h"
#include "OpenGLUtils.h"


typedef struct GLTexture
{
	GLuint	id;
	GLint  	w;		//Real size of the texture
	GLint  	h;		//
	GLenum	target; // GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE
	
	float	fullx;  //texture coordinates of the full size
	float	fully;	//image
	
	GLenum	format;
	GLenum	type;
	
	GLubyte	*buffer;
	GLuint	rowbytes;
	
} GLTexture;

typedef struct OpenGLPluginPreferences
{
	Boolean  vblsync;
	Boolean  bilinear;
	Boolean	 overlay;
	CFStringRef overlayFileName;
	int	 persistence;
} OpenGLPluginPreferences;

struct recContext
{
	AGLPixelFormat 	aglPixFmt;
	AGLContext 		aglContext;
	WindowRef		window;
	
	Rect			globalBounds;
	
	int				sourceWidth;
	int				sourceHeight;
	int				viewportWidth;
	int				viewportHeight;
	
	float			screenTexCoords[8];
	
	GLTexture		screenTexture;
	GLTexture		overlayTexture;
	
	GLint			unpackRowLength;
	
	OpenGLPluginPreferences prefs;
};
typedef struct recContext recContext;
typedef struct recContext * pRecContext;

Boolean OpenGL_GetDisplayDescription(DisplayDescription *ioDescription);