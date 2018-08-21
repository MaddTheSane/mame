#ifndef __OPENGLUTILS__
#define __OPENGLUTILS__

// GLUtils.h
#include <Carbon/Carbon.h>
#include <OpenGL/gl.h>

#define NextPowerOf2(x) 1 << (32 - __cntlzw(x-1))
#define	IsPowerOf2(n) (0 == (n & (n-1)) )

//Count trailing zeros, like PPC cntlzw
#define cnttzw(n) (32 - __cntlzw(~n & (n-1)));

GLboolean 	LoadGLTexture(char * filename, GLuint *textureID);

OSErr	 	FSpLoadGLTexture(FSSpec * f, GLuint *textureID);
GLboolean 	LoadGLTextureOfType(char * filename, GLuint *textureID, GLenum textureType);
GLboolean	LoadGLTextureOfTypeSize(char * filename, GLuint *textureID, GLenum textureType, GLint *width, GLint *height);
GLboolean 	LoadGLTextureOfTypeSizeFromURLRef(CFURLRef url, GLuint *textureID, GLenum textureType, GLint *width, GLint *height);
GLboolean LoadGLTextureOfTypeSizeFromBundle(char *filename, GLuint *textureID, GLenum textureType, GLint *width, GLint *height);

OSErr	 	FSpLoadGLTextureOfType(FSSpec * f, GLuint *textureID, GLenum textureType);
OSErr	 	FSpLoadGLTextureOfTypeSize(FSSpec * f, GLuint *textureID, GLenum textureType, GLint *width, GLint *height);


GLuint 		extensionSupported(const char *extension);
OSErr 		DrawFileIn32BitGWorld ( FSSpec * fs, GWorldPtr  *g );
OSErr 		Build2DTexturesFrom32BitGWorld(GWorldPtr g, GLuint *textureID, GLenum textureType);
OSErr 		Build2DTexturesFrom32BitGWorldH(GWorldPtr g, GLuint *textureID, GLenum textureType, GLint height);
Boolean		IsFileATexture(FSSpec *fs);
GLboolean 	HasPackedPixel(void);


OSErr 		GetApplicationFileSpec(FSSpec * fs);


#endif