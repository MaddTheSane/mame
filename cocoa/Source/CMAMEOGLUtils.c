//
//  CMAMEOGLUtils.c
//  MacMAME
//
//  Created by C.W. Betts on 9/18/18.
//

#include "CMAMEOGLUtils.h"
#include <OpenGL/gl.h>

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
