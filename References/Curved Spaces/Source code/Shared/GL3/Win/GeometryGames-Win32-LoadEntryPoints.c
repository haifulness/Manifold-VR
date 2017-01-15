//	GeometryGames-Win32-LoadEntryPoints.c
//
//	WindowsXP/Vista/7 still uses the same OpenGL 1.1 library that it used back in 1996 (!).
//	To access OpenGL 1.2 and later on Win32 we must use the "extension" mechanism.
//	The following code loads the needed entry points.
//
//	Our program's Win32 GUI calls LoadOpenGLFunctions() to load the entry points,
//	but never sees those entry points.   Indeed, the platform-dependent GUIs
//	don't see OpenGL at all.  Only our platform-independent code sees OpenGL
//	and makes calls using the function pointers loaded below along with
//	a few of the built-in OpenGL 1.1 calls.
//
//	© 2013 by Jeff Weeks
//	See TermsOfUse.txt

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <string.h>	//	for strcmp()
#include "GeometryGames-Win32-LoadEntryPoints.h"
#include "GeometryGames-Win32-OpenGLEntryPoints.h"


static bool	OpenGL3ExtensionIsAvailable(const char *anExtensionName);


bool LoadOpenGLFunctions(void)
{
	GLint	theMajorVersionNumber	= 0,
			theMinorVersionNumber	= 0;
				
	//	SetUpDrawingPanel() either gets an OpenGL 3.3 or newer context or fails,
	//	so we may safely assume OpenGL 3.3 or newer is present.

	glGetIntegerv(GL_MAJOR_VERSION, &theMajorVersionNumber);
	glGetIntegerv(GL_MINOR_VERSION, &theMinorVersionNumber);
	
	//	Load OpenGL 2 core functions.
	glGenBuffers				= (PFNGLGENBUFFERSPROC)					wglGetProcAddress("glGenBuffers"				);
	glBindBuffer				= (PFNGLBINDBUFFERPROC)					wglGetProcAddress("glBindBuffer"				);
	glDeleteBuffers				= (PFNGLDELETEBUFFERSPROC)				wglGetProcAddress("glDeleteBuffers"				);
	glBufferData				= (PFNGLBUFFERDATAPROC)					wglGetProcAddress("glBufferData"				);
	glBufferSubData				= (PFNGLBUFFERSUBDATAPROC)				wglGetProcAddress("glBufferSubData"				);
	glCreateShader				= (PFNGLCREATESHADERPROC)				wglGetProcAddress("glCreateShader"				);
	glShaderSource				= (PFNGLSHADERSOURCEPROC)				wglGetProcAddress("glShaderSource"				);
	glCompileShader				= (PFNGLCOMPILESHADERPROC)				wglGetProcAddress("glCompileShader"				);
	glGetShaderiv				= (PFNGLGETSHADERIVPROC)				wglGetProcAddress("glGetShaderiv"				);
	glGetShaderInfoLog			= (PFNGLGETSHADERINFOLOGPROC)			wglGetProcAddress("glGetShaderInfoLog"			);
	glCreateProgram				= (PFNGLCREATEPROGRAMPROC)				wglGetProcAddress("glCreateProgram"				);
	glAttachShader				= (PFNGLATTACHSHADERPROC)				wglGetProcAddress("glAttachShader"				);
	glBindAttribLocation		= (PFNGLBINDATTRIBLOCATIONPROC)			wglGetProcAddress("glBindAttribLocation"		);
	glLinkProgram				= (PFNGLLINKPROGRAMPROC)				wglGetProcAddress("glLinkProgram"				);
	glGetProgramiv				= (PFNGLGETPROGRAMIVPROC)				wglGetProcAddress("glGetProgramiv"				);
	glGetProgramInfoLog			= (PFNGLGETPROGRAMINFOLOGPROC)			wglGetProcAddress("glGetProgramInfoLog"			);
	glUseProgram				= (PFNGLUSEPROGRAMPROC)					wglGetProcAddress("glUseProgram"				);
	glDeleteShader				= (PFNGLDELETESHADERPROC)				wglGetProcAddress("glDeleteShader"				);
	glDeleteProgram				= (PFNGLDELETEPROGRAMPROC)				wglGetProcAddress("glDeleteProgram"				);
	glGetUniformLocation		= (PFNGLGETUNIFORMLOCATIONPROC)			wglGetProcAddress("glGetUniformLocation"		);
	glUniform1i					= (PFNGLUNIFORM1IPROC)					wglGetProcAddress("glUniform1i"					);
	glUniform1iv				= (PFNGLUNIFORM1IVPROC)					wglGetProcAddress("glUniform1iv"				);
	glUniform1f					= (PFNGLUNIFORM1FPROC)					wglGetProcAddress("glUniform1f"					);
	glUniform2f					= (PFNGLUNIFORM2FPROC)					wglGetProcAddress("glUniform2f"					);
	glUniform3f					= (PFNGLUNIFORM3FPROC)					wglGetProcAddress("glUniform3f"					);
	glUniform4f					= (PFNGLUNIFORM4FPROC)					wglGetProcAddress("glUniform4f"					);
	glUniform1fv				= (PFNGLUNIFORM1FVPROC)					wglGetProcAddress("glUniform1fv"				);
	glUniform2fv				= (PFNGLUNIFORM2FVPROC)					wglGetProcAddress("glUniform2fv"				);
	glUniform3fv				= (PFNGLUNIFORM3FVPROC)					wglGetProcAddress("glUniform3fv"				);
	glUniform4fv				= (PFNGLUNIFORM4FVPROC)					wglGetProcAddress("glUniform4fv"				);
	glUniformMatrix4fv			= (PFNGLUNIFORMMATRIX4FVPROC)			wglGetProcAddress("glUniformMatrix4fv"			);
	glVertexAttribPointer		= (PFNGLVERTEXATTRIBPOINTERPROC)		wglGetProcAddress("glVertexAttribPointer"		);
	glEnableVertexAttribArray	= (PFNGLENABLEVERTEXATTRIBARRAYPROC)	wglGetProcAddress("glEnableVertexAttribArray"	);
	glDisableVertexAttribArray	= (PFNGLDISABLEVERTEXATTRIBARRAYPROC)	wglGetProcAddress("glDisableVertexAttribArray"	);
	glVertexAttrib1f			= (PFNGLVERTEXATTRIB1FPROC)				wglGetProcAddress("glVertexAttrib1f"			);
	glVertexAttrib2f			= (PFNGLVERTEXATTRIB2FPROC)				wglGetProcAddress("glVertexAttrib2f"			);
	glVertexAttrib3f			= (PFNGLVERTEXATTRIB3FPROC)				wglGetProcAddress("glVertexAttrib3f"			);
	glVertexAttrib4f			= (PFNGLVERTEXATTRIB4FPROC)				wglGetProcAddress("glVertexAttrib4f"			);
	glVertexAttrib1fv			= (PFNGLVERTEXATTRIB1FVPROC)			wglGetProcAddress("glVertexAttrib1fv"			);
	glVertexAttrib2fv			= (PFNGLVERTEXATTRIB2FVPROC)			wglGetProcAddress("glVertexAttrib2fv"			);
	glVertexAttrib3fv			= (PFNGLVERTEXATTRIB3FVPROC)			wglGetProcAddress("glVertexAttrib3fv"			);
	glVertexAttrib4fv			= (PFNGLVERTEXATTRIB4FVPROC)			wglGetProcAddress("glVertexAttrib4fv"			);
	glActiveTexture				= (PFNGLACTIVETEXTUREPROC)				wglGetProcAddress("glActiveTexture"				);

	//	Load OpenGL 3.0 core functions.
	glGetStringi				= (PFNGLGETSTRINGIPROC)					wglGetProcAddress("glGetStringi"				);
	glBindFragDataLocation		= (PFNGLBINDFRAGDATALOCATIONPROC)		wglGetProcAddress("glBindFragDataLocation"		);
	glGenVertexArrays			= (PFNGLGENVERTEXARRAYSPROC)			wglGetProcAddress("glGenVertexArrays"			);	
	glDeleteVertexArrays		= (PFNGLDELETEVERTEXARRAYSPROC)			wglGetProcAddress("glDeleteVertexArrays"		);	
	glBindVertexArray			= (PFNGLBINDVERTEXARRAYPROC)			wglGetProcAddress("glBindVertexArray"			);	
	glGenFramebuffers			= (PFNGLGENFRAMEBUFFERSPROC)			wglGetProcAddress("glGenFramebuffers"			);
	glDeleteFramebuffers		= (PFNGLDELETEFRAMEBUFFERSPROC)			wglGetProcAddress("glDeleteFramebuffers"		);
	glBindFramebuffer			= (PFNGLBINDFRAMEBUFFERPROC)			wglGetProcAddress("glBindFramebuffer"			);
	glFramebufferTexture2D		= (PFNGLFRAMEBUFFERTEXTURE2DPROC)		wglGetProcAddress("glFramebufferTexture2D"		);
	glGenerateMipmap			= (PFNGLGENERATEMIPMAPPROC)				wglGetProcAddress("glGenerateMipmap"			);
	glCheckFramebufferStatus	= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)		wglGetProcAddress("glCheckFramebufferStatus"	);
	glGenRenderbuffers			= (PFNGLGENRENDERBUFFERSPROC)			wglGetProcAddress("glGenRenderbuffers"			);
	glBindRenderbuffer			= (PFNGLBINDRENDERBUFFERPROC)			wglGetProcAddress("glBindRenderbuffer"			);
	glRenderbufferStorage		= (PFNGLRENDERBUFFERSTORAGEPROC)		wglGetProcAddress("glRenderbufferStorage"		);
	glFramebufferRenderbuffer	= (PFNGLFRAMEBUFFERRENDERBUFFERPROC)	wglGetProcAddress("glFramebufferRenderbuffer"	);
	glDeleteRenderbuffers		= (PFNGLDELETERENDERBUFFERSPROC)		wglGetProcAddress("glDeleteRenderbuffers"		);
	glBlitFramebuffer			= (PFNGLBLITFRAMEBUFFERPROC)			wglGetProcAddress("glBlitFramebuffer"			);
	glRenderbufferStorageMultisample	= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) wglGetProcAddress("glRenderbufferStorageMultisample");

	//	Query objects already appear in OpenGL 2.0,
	//	but we'll use them only for timer queries,
	//	which first appear in OpenGL 3.3.
	glGenQueries				= (PFNGLGENQUERIESPROC)					wglGetProcAddress("glGenQueries"				);
	glDeleteQueries				= (PFNGLDELETEQUERIESPROC)				wglGetProcAddress("glDeleteQueries"				);
	glBeginQuery				= (PFNGLBEGINQUERYPROC)					wglGetProcAddress("glBeginQuery"				);
	glEndQuery					= (PFNGLENDQUERYPROC)					wglGetProcAddress("glEndQuery"					);
	glGetQueryObjectuiv			= (PFNGLGETQUERYOBJECTUIVPROC)			wglGetProcAddress("glGetQueryObjectuiv"			);

	//	OpenGL 3.3
	//	or ARB_instanced_arrays
	if ((theMajorVersionNumber == 3 && theMinorVersionNumber >= 3)
	 || theMajorVersionNumber >= 4)
	{
		glVertexAttribDivisor	= (PFNGLVERTEXATTRIBDIVISORPROC)	wglGetProcAddress("glVertexAttribDivisor");
		glDrawArraysInstanced	= (PFNGLDRAWARRAYSINSTANCEDPROC)	wglGetProcAddress("glDrawArraysInstanced");
		glDrawElementsInstanced	= (PFNGLDRAWELEMENTSINSTANCEDPROC)	wglGetProcAddress("glDrawElementsInstanced");
	}
	else
	if (OpenGL3ExtensionIsAvailable("GL_ARB_instanced_arrays"))
	{
		glVertexAttribDivisor	= (PFNGLVERTEXATTRIBDIVISORPROC)	wglGetProcAddress("glVertexAttribDivisorARB");
		glDrawArraysInstanced	= (PFNGLDRAWARRAYSINSTANCEDPROC)	wglGetProcAddress("glDrawArraysInstancedARB");
		glDrawElementsInstanced	= (PFNGLDRAWELEMENTSINSTANCEDPROC)	wglGetProcAddress("glDrawElementsInstancedARB");
	}
	else
	{
		glVertexAttribDivisor	= NULL;
		glDrawArraysInstanced	= NULL;
		glDrawElementsInstanced	= NULL;
	}
	
#ifdef DEBUG

	//	AMD_debug_output
	if (OpenGL3ExtensionIsAvailable("GL_AMD_debug_output"))
		glDebugMessageCallbackAMD = (PFNGLDEBUGMESSAGECALLBACKAMDPROC) wglGetProcAddress("glDebugMessageCallbackAMD");
	else
		glDebugMessageCallbackAMD = NULL;

	//	ARB_debug_output
	if (OpenGL3ExtensionIsAvailable("GL_ARB_debug_output"))
		glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) wglGetProcAddress("glDebugMessageCallbackARB");
	else
		glDebugMessageCallbackARB = NULL;

#endif

	return true;	//	loaded entry points
}


static bool OpenGL3ExtensionIsAvailable(const char *anExtensionName)
{
	unsigned int	theNumExtensions	= 0,
					i;

	glGetIntegerv(GL_NUM_EXTENSIONS, (signed int *)&theNumExtensions);

	for (i = 0; i < theNumExtensions; i++)
		if (strcmp(anExtensionName, (const char *)glGetStringi(GL_EXTENSIONS, i)) == 0)
			return true;

	return false;
}
