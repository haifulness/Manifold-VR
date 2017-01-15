//	ExtensionsForOpenGLES.c
//
//	These implementations of future-compatible gl*VertexArrays()
//	pass their arguments through to the current extension functions
//	gl*VertexArraysOES().  On iOS, the gl*VertexArraysOES()
//	are available on iOS 4.2, but not on iOS 3.2.
//	On Android it's not clear how widely available they are,
//	even on Android 4.0 and higher.
//
//	On iOS, glRenderbufferStorageMultisample() and glBlitFramebuffer()
//	are pass-throughs to glRenderbufferStorageMultisampleAPPLE()
//	and glResolveMultisampleFramebufferAPPLE(), respectively.
//	On Android, they pass through to empty functions which
//	never get called.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#define SUPPORT_OPENGL_ES_2
#include "ExtensionsForOpenGLES.h"	//	declares gl*VertexArrays()

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2

#import <OpenGLES/ES2/glext.h>	//	declares gl*VertexArraysOES()

#elif defined(__ANDROID__)				//	Android - OpenGL ES 3

//	The Geometry Games apps currently need no OpenGL ES extensions.
//	In case they are ever need them in the future, here's some
//	quick-and-dirty old code (for loading a GLES2 extension)
//	that shows how to load a pointer:
//
//		static PFNGLGENVERTEXARRAYSOESPROC	theGenVertexArraysOES;
//
//		if (theGenVertexArraysOES == NULL)
//			theGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC) eglGetProcAddress("glGenVertexArraysOES");
//
//		if (theGenVertexArraysOES != NULL)
//			theGenVertexArraysOES(n, arrays);
//
//	Warning:  The function pointer gets loaded only once,
//	and thereafter remains available in the static variable.
//	If the OpenGL ES context gets replaced, we're not guaranteed
//	that the same function pointer will still work.
//
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3ext.h>		//	declares extension functions
#include <EGL/egl.h>			//	declares eglGetProcAddress()

#else
#error No OpenGL ES support
#endif


#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2

void glGenVertexArrays(GLsizei n, GLuint *arrays)
{
	//	Note:  If I ever support GLES2 and GLES3 simultaneously,
	//	I'll need to rename this function as glGenVertexArraysWRAPPER()
	//	and provide a switch here to choose between
	//	this GLES2 implementation and a plain GLES3 call.
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.

	glGenVertexArraysOES(n, arrays);
}

void glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
	//	Note:  If I ever support GLES2 and GLES3 simultaneously,
	//	I'll need to rename this function as glDeleteVertexArraysWRAPPER()
	//	and provide a switch here to choose between
	//	this GLES2 implementation and a plain GLES3 call.
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.

	glDeleteVertexArraysOES(n, arrays);
}

void glBindVertexArray(GLuint array)
{
	//	Note:  If I ever support GLES2 and GLES3 simultaneously,
	//	I'll need to rename this function as glBindVertexArrayWRAPPER()
	//	and provide a switch here to choose between
	//	this GLES2 implementation and a plain GLES3 call.
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.

	glBindVertexArrayOES(array);
}

#endif


#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2

GLvoid glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	//	Note:  If I ever support GLES2 and GLES3 simultaneously,
	//	I'll need to rename this function as glRenderbufferStorageMultisampleWRAPPER()
	//	and provide a switch here to choose between
	//	this GLES2 implementation and a plain GLES3 call.
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.

	glRenderbufferStorageMultisampleAPPLE(target, samples, internalformat, width, height);
}

GLvoid glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
	//	Note:  If I ever support GLES2 and GLES3 simultaneously,
	//	I'll need to rename this function as glBlitFramebufferWRAPPER()
	//	and provide a switch here to choose between
	//	this GLES2 implementation and a plain GLES3 call.
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.

	(void)srcX0;
	(void)srcY0;
	(void)srcX1;
	(void)srcY1;
	(void)dstX0;
	(void)dstY0;
	(void)dstX1;
	(void)dstY1;
	(void)mask;
	(void)filter;

	glResolveMultisampleFramebufferAPPLE();
}

#endif


#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2

void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	//	To support GLES2+GL_EXT_instanced_arrays
	glVertexAttribDivisorEXT(index, divisor);

	//	Note:  If I ever simultaneously support GLES3 and GLES2+GL_EXT_instanced_arrays,
	//	I'll need to rename this function as glVertexAttribDivisorWRAPPER()
	//	and provide a switch here to choose between
	//	glVertexAttribDivisor() and glVertexAttribDivisorEXT().
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	//	To support GLES2+GL_EXT_instanced_arrays
	glDrawArraysInstancedEXT(mode, first, count, instancecount);

	//	Note:  If I ever simultaneously support GLES3 and GLES2+GL_EXT_instanced_arrays,
	//	I'll need to rename this function as glDrawArraysInstancedWRAPPER()
	//	and provide a switch here to choose between
	//	glDrawArraysInstanced() and glDrawArraysInstancedEXT().
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
	//	To support GLES2+GL_EXT_instanced_arrays
	glDrawElementsInstancedEXT(mode, count, type, indices, instancecount);

	//	Note:  If I ever simultaneously support GLES3 and GLES2+GL_EXT_instanced_arrays,
	//	I'll need to rename this function as glDrawElementsInstancedWRAPPER()
	//	and provide a switch here to choose between
	//	glDrawElementsInstanced() and glDrawElementsInstancedEXT().
	//	See "software/Notes & Utilities/scraps/Mac OS 10.6 and OpenGL 2 code" for sample code.
	//
	//	If I later move to GLES3 only, this function may be eliminated.
}

#endif
