//	ExtensionsForOpenGLES.h
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

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
#import <OpenGLES/ES2/gl.h>	//	for basic types GLuint, GLsizei, etc.
#elif defined(__ANDROID__)				//	Android - OpenGL ES 3
#include <GLES3/gl3.h>		//	for basic types GLuint, GLsizei, etc.
#else
#error No OpenGL ES support
#endif

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
//	Desktop OpenGL defines
//
//		GL_MAX_SAMPLES				0x8D57
//		GL_READ_FRAMEBUFFER			0x8CA8
//		GL_DRAW_FRAMEBUFFER			0x8CA9
//
//	while Apple's OpenGL ES extension GL_APPLE_framebuffer_multisample defines
//
//		GL_MAX_SAMPLES_APPLE		0x8D57
//		GL_READ_FRAMEBUFFER_APPLE	0x8CA8
//		GL_DRAW_FRAMEBUFFER_APPLE	0x8CA9
//
//	Even if the corresponding symbols had different values
//	it would be safe (under OpenGL ES) to define the former
//	as synonyms for the latter.  The fact that the values
//	are equal provides an extra level of future compatibility.
#define GL_MAX_SAMPLES		GL_MAX_SAMPLES_APPLE
#define GL_READ_FRAMEBUFFER	GL_READ_FRAMEBUFFER_APPLE
#define GL_DRAW_FRAMEBUFFER	GL_DRAW_FRAMEBUFFER_APPLE
#endif

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
//	iOS 3.2 does not support Vertex Array Objects.
//	iOS 4.2 does support them, but as gl*VertexArrayOES(),
//	for which the following functions serve as wrappers.
extern GLvoid	glBindVertexArray(GLuint array);
extern GLvoid	glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
extern GLvoid	glGenVertexArrays(GLsizei n, GLuint *arrays);
#endif

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
//	iOS 4.2 supports glRenderbufferStorageMultisampleAPPLE()
//	and glResolveMultisampleFramebufferAPPLE() on all hardware.
extern GLvoid	glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern GLvoid	glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
#endif

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
//	iOS 7 supports instanced arrays as an extension, on all supported devices.
//	The following three functions serve as wrappers around the extension functions' names.
//	That is, glVertexAttribDivisor() wraps glVertexAttribDivisorEXT()
//	and similarly for the other two.
extern GLvoid	glVertexAttribDivisor(GLuint index, GLuint divisor);
extern GLvoid	glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
extern GLvoid	glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
#endif
