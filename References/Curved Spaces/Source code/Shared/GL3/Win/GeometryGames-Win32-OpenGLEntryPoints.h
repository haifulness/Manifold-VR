//	GeometryGames-OpenGLEntryPoints.h
//
//	WindowsXP/Vista/7 still uses the same OpenGL 1.1 library that it used back in 1996 (!).
//	To access OpenGL 1.2 and later on Win32 we must use the "extension" mechanism.
//	The following code defines the needed entry points.
//
//	Technical Note:  The same platform-independent code compiles a little differently
//	under Win32 than under Mac OS X.  Under Mac OS X, the program compiles against
//	the Open GL 3.0 (or later) headers, and a call to, say, glVertexAttribPointer()
//	gets resolved by automatic (soft) linking to the built-in OpenGL library's 
//	implementation of glVertexAttribPointer().  Under WinXP, the token
//	glVertexAttribPointer is treated not like an ordinary function that gets
//	linked at program launch, but rather as a program-defined global variable,
//	which happens to be a function pointer.  Ultimately the effect is the same.
//
//	© 2013 by Jeff Weeks
//	See TermsOfUse.txt

//	If we #define GL3_PROTOTYPES,
//		the traditionally-linked functions (those from OpenGL 1.1 or earlier)
//			will work fine, but
//		the loaded-on-the-fly functions (those from OpenGL 1.2 or later)
//			will suffer conflicting definitions.
//	If we don't #define GL3_PROTOTYPES,
//		the loaded-on-the-fly functions will work fine, but
//		the traditionally-linked functions will lack prototypes.
//	The best solution seems to be to leave GL3_PROTOTYPES undefined,
//	and then manually provide prototypes for the traditionally-linked functions.
#include "gl3.h"	//	#include <GL3/gl3.h>

//	gl3.h defines GLDEBUGPROCAMD but not PFNGLDEBUGMESSAGECALLBACKAMDPROC.
#ifdef DEBUG
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKAMDPROC) (GLDEBUGPROCAMD callback, const GLvoid *userParam);
#endif

//	Prototypes for traditionally-linked functions (from OpenGL 1.1 or earlier)
GLAPI void APIENTRY				glCullFace(GLenum);
GLAPI void APIENTRY				glFrontFace(GLenum);
GLAPI void APIENTRY				glTexParameterf(GLenum, GLenum, GLfloat);
GLAPI void APIENTRY				glTexParameteri(GLenum, GLenum, GLint);
GLAPI void APIENTRY				glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
GLAPI void APIENTRY				glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
GLAPI void APIENTRY				glClear(GLbitfield);
GLAPI void APIENTRY				glClearColor(GLclampf, GLclampf, GLclampf, GLclampf);
GLAPI void APIENTRY				glColorMask(GLboolean, GLboolean, GLboolean, GLboolean);
GLAPI void APIENTRY				glDisable(GLenum);
GLAPI void APIENTRY				glEnable(GLenum);
GLAPI void APIENTRY				glBlendFunc(GLenum, GLenum);
GLAPI void APIENTRY				glReadBuffer(GLenum);
GLAPI void APIENTRY				glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
GLAPI GLenum APIENTRY			glGetError(void);
GLAPI void APIENTRY				glGetFloatv(GLenum, GLfloat *);
GLAPI void APIENTRY				glGetIntegerv(GLenum, GLint *);
GLAPI const GLubyte * APIENTRY	glGetString(GLenum);
GLAPI void APIENTRY				glViewport(GLint, GLint, GLsizei, GLsizei);
GLAPI void APIENTRY				glDrawArrays(GLenum, GLint, GLsizei);
GLAPI void APIENTRY				glDrawElements(GLenum, GLsizei, GLenum, const GLvoid *);
GLAPI void APIENTRY				glBindTexture(GLenum, GLuint);
GLAPI void APIENTRY				glDeleteTextures(GLsizei, const GLuint *);
GLAPI void APIENTRY				glGenTextures(GLsizei, GLuint *);
GLAPI void APIENTRY				glPixelStorei(GLenum, GLint);
GLAPI void APIENTRY				glLineWidth(GLfloat);

//	Function pointers for loaded-on-the-fly functions (from OpenGL 1.2 or later)
extern PFNGLGENBUFFERSPROC						glGenBuffers;
extern PFNGLBINDBUFFERPROC						glBindBuffer;
extern PFNGLDELETEBUFFERSPROC					glDeleteBuffers;
extern PFNGLBUFFERDATAPROC						glBufferData;
extern PFNGLBUFFERSUBDATAPROC					glBufferSubData;
extern PFNGLCREATESHADERPROC					glCreateShader;
extern PFNGLSHADERSOURCEPROC					glShaderSource;
extern PFNGLCOMPILESHADERPROC					glCompileShader;
extern PFNGLGETSHADERIVPROC						glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC				glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC					glCreateProgram;
extern PFNGLATTACHSHADERPROC					glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC				glBindAttribLocation;
extern PFNGLLINKPROGRAMPROC						glLinkProgram;
extern PFNGLGETPROGRAMIVPROC					glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC				glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC						glUseProgram;
extern PFNGLDELETESHADERPROC					glDeleteShader;
extern PFNGLDELETEPROGRAMPROC					glDeleteProgram;
extern PFNGLGETUNIFORMLOCATIONPROC				glGetUniformLocation;
extern PFNGLUNIFORM1IPROC						glUniform1i;
extern PFNGLUNIFORM1IVPROC						glUniform1iv;
extern PFNGLUNIFORM1FPROC						glUniform1f;
extern PFNGLUNIFORM2FPROC						glUniform2f;
extern PFNGLUNIFORM3FPROC						glUniform3f;
extern PFNGLUNIFORM4FPROC						glUniform4f;
extern PFNGLUNIFORM1FVPROC						glUniform1fv;
extern PFNGLUNIFORM2FVPROC						glUniform2fv;
extern PFNGLUNIFORM3FVPROC						glUniform3fv;
extern PFNGLUNIFORM4FVPROC						glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC				glUniformMatrix4fv;
extern PFNGLVERTEXATTRIBPOINTERPROC				glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC			glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC		glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIB1FPROC					glVertexAttrib1f;
extern PFNGLVERTEXATTRIB2FPROC					glVertexAttrib2f;
extern PFNGLVERTEXATTRIB3FPROC					glVertexAttrib3f;
extern PFNGLVERTEXATTRIB4FPROC					glVertexAttrib4f;
extern PFNGLVERTEXATTRIB1FVPROC					glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB2FVPROC					glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB3FVPROC					glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB4FVPROC					glVertexAttrib4fv;
extern PFNGLBINDFRAGDATALOCATIONPROC			glBindFragDataLocation;
extern PFNGLGETSTRINGIPROC						glGetStringi;
extern PFNGLGENVERTEXARRAYSPROC					glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC				glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC					glBindVertexArray;
extern PFNGLGENFRAMEBUFFERSPROC					glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC					glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D;
extern PFNGLGENERATEMIPMAPPROC					glGenerateMipmap;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus;
extern PFNGLGENRENDERBUFFERSPROC				glGenRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC				glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC				glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC			glFramebufferRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC				glDeleteRenderbuffers;
extern PFNGLBLITFRAMEBUFFERPROC					glBlitFramebuffer;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC	glRenderbufferStorageMultisample;
#ifdef DEBUG
extern PFNGLDEBUGMESSAGECALLBACKAMDPROC			glDebugMessageCallbackAMD;
extern PFNGLDEBUGMESSAGECALLBACKARBPROC			glDebugMessageCallbackARB;
#endif
extern PFNGLGENQUERIESPROC						glGenQueries;
extern PFNGLDELETEQUERIESPROC					glDeleteQueries;
extern PFNGLBEGINQUERYPROC						glBeginQuery;
extern PFNGLENDQUERYPROC						glEndQuery;
extern PFNGLGETQUERYOBJECTUIVPROC				glGetQueryObjectuiv;
extern PFNGLVERTEXATTRIBDIVISORPROC				glVertexAttribDivisor;
extern PFNGLDRAWARRAYSINSTANCEDPROC				glDrawArraysInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDPROC			glDrawElementsInstanced;
extern PFNGLACTIVETEXTUREPROC					glActiveTexture;
