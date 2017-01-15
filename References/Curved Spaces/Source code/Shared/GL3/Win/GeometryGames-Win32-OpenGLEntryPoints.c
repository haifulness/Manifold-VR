//	GeometryGames-OpenGLEntryPoints.c
//
//	For use on Win32.
//	See GeometryGames-OpenGLEntryPoints.h for details.
//
//	© 2013 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGames-Win32-OpenGLEntryPoints.h"


static void __stdcall	DummyDeleteBuffers(signed int n, const unsigned int *buffers);
static void __stdcall	DummyUseProgram(unsigned int aProgram);
static void __stdcall	DummyDeleteProgram(unsigned int aProgram);
static void __stdcall	DummyDeleteArrays(signed int n, const unsigned int *arrays);
static void __stdcall	DummyDeleteFramebuffers(signed int n, const unsigned int *framebuffers);
static void __stdcall	DummyDeleteRenderbuffers(signed int n, const unsigned int *renderbuffers);
#ifdef DEBUG
static void __stdcall	DummyDebugMessageCallbackAMD(GLDEBUGPROCAMD callback, const GLvoid *userParam);
static void __stdcall	DummyDebugMessageCallbackARB(GLDEBUGPROCARB callback, const GLvoid *userParam);
#endif
static void __stdcall	DummyDeleteQueries(signed int n, const unsigned int *queries);


PFNGLGENBUFFERSPROC						glGenBuffers						= NULL;
PFNGLBINDBUFFERPROC						glBindBuffer						= NULL;
PFNGLDELETEBUFFERSPROC					glDeleteBuffers						= &DummyDeleteBuffers;
PFNGLBUFFERDATAPROC						glBufferData						= NULL;
PFNGLBUFFERSUBDATAPROC					glBufferSubData						= NULL;
PFNGLCREATESHADERPROC					glCreateShader						= NULL;
PFNGLSHADERSOURCEPROC					glShaderSource						= NULL;
PFNGLCOMPILESHADERPROC					glCompileShader						= NULL;
PFNGLGETSHADERIVPROC					glGetShaderiv						= NULL;
PFNGLGETSHADERINFOLOGPROC				glGetShaderInfoLog					= NULL;
PFNGLCREATEPROGRAMPROC					glCreateProgram						= NULL;
PFNGLATTACHSHADERPROC					glAttachShader						= NULL;
PFNGLBINDATTRIBLOCATIONPROC				glBindAttribLocation				= NULL;
PFNGLLINKPROGRAMPROC					glLinkProgram						= NULL;
PFNGLGETPROGRAMIVPROC					glGetProgramiv						= NULL;
PFNGLGETPROGRAMINFOLOGPROC				glGetProgramInfoLog					= NULL;
PFNGLUSEPROGRAMPROC						glUseProgram						= &DummyUseProgram;
PFNGLDELETESHADERPROC					glDeleteShader						= NULL;
PFNGLDELETEPROGRAMPROC					glDeleteProgram						= &DummyDeleteProgram;
PFNGLGETUNIFORMLOCATIONPROC				glGetUniformLocation				= NULL;
PFNGLUNIFORM1IPROC						glUniform1i							= NULL;
PFNGLUNIFORM1IVPROC						glUniform1iv						= NULL;
PFNGLUNIFORM1FPROC						glUniform1f							= NULL;
PFNGLUNIFORM2FPROC						glUniform2f							= NULL;
PFNGLUNIFORM3FPROC						glUniform3f							= NULL;
PFNGLUNIFORM4FPROC						glUniform4f							= NULL;
PFNGLUNIFORM1FVPROC						glUniform1fv						= NULL;
PFNGLUNIFORM2FVPROC						glUniform2fv						= NULL;
PFNGLUNIFORM3FVPROC						glUniform3fv						= NULL;
PFNGLUNIFORM4FVPROC						glUniform4fv						= NULL;
PFNGLUNIFORMMATRIX4FVPROC				glUniformMatrix4fv					= NULL;
PFNGLVERTEXATTRIBPOINTERPROC			glVertexAttribPointer				= NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC		glEnableVertexAttribArray			= NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC		glDisableVertexAttribArray			= NULL;
PFNGLVERTEXATTRIB1FPROC					glVertexAttrib1f					= NULL;
PFNGLVERTEXATTRIB2FPROC					glVertexAttrib2f					= NULL;
PFNGLVERTEXATTRIB3FPROC					glVertexAttrib3f					= NULL;
PFNGLVERTEXATTRIB4FPROC					glVertexAttrib4f					= NULL;
PFNGLVERTEXATTRIB1FVPROC				glVertexAttrib1fv					= NULL;
PFNGLVERTEXATTRIB2FVPROC				glVertexAttrib2fv					= NULL;
PFNGLVERTEXATTRIB3FVPROC				glVertexAttrib3fv					= NULL;
PFNGLVERTEXATTRIB4FVPROC				glVertexAttrib4fv					= NULL;
PFNGLBINDFRAGDATALOCATIONPROC			glBindFragDataLocation				= NULL;
PFNGLGETSTRINGIPROC						glGetStringi						= NULL;
PFNGLGENVERTEXARRAYSPROC				glGenVertexArrays					= NULL;
PFNGLDELETEVERTEXARRAYSPROC				glDeleteVertexArrays				= &DummyDeleteArrays;
PFNGLBINDVERTEXARRAYPROC				glBindVertexArray					= NULL;
PFNGLGENFRAMEBUFFERSPROC				glGenFramebuffers					= NULL;
PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers				= &DummyDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC				glBindFramebuffer					= NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D				= NULL;
PFNGLGENERATEMIPMAPPROC					glGenerateMipmap					= NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus			= NULL;
PFNGLGENRENDERBUFFERSPROC				glGenRenderbuffers					= NULL;
PFNGLBINDRENDERBUFFERPROC				glBindRenderbuffer					= NULL;
PFNGLRENDERBUFFERSTORAGEPROC			glRenderbufferStorage				= NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC		glFramebufferRenderbuffer			= NULL;
PFNGLDELETERENDERBUFFERSPROC			glDeleteRenderbuffers				= &DummyDeleteRenderbuffers;
PFNGLBLITFRAMEBUFFERPROC				glBlitFramebuffer					= NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC	glRenderbufferStorageMultisample	= NULL;
#ifdef DEBUG
PFNGLDEBUGMESSAGECALLBACKAMDPROC		glDebugMessageCallbackAMD			= &DummyDebugMessageCallbackAMD;
PFNGLDEBUGMESSAGECALLBACKARBPROC		glDebugMessageCallbackARB			= &DummyDebugMessageCallbackARB;
#endif
PFNGLGENQUERIESPROC						glGenQueries						= NULL;
PFNGLDELETEQUERIESPROC					glDeleteQueries						= &DummyDeleteQueries;
PFNGLBEGINQUERYPROC						glBeginQuery						= NULL;
PFNGLENDQUERYPROC						glEndQuery							= NULL;
PFNGLGETQUERYOBJECTUIVPROC				glGetQueryObjectuiv					= NULL;
PFNGLVERTEXATTRIBDIVISORPROC			glVertexAttribDivisor				= NULL;
PFNGLDRAWARRAYSINSTANCEDPROC			glDrawArraysInstanced				= NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC			glDrawElementsInstanced				= NULL;
PFNGLACTIVETEXTUREPROC					glActiveTexture						= NULL;


//	If an error occurs at startup, the program will call its OpenGL shutdown code.
//	On the Mac it's harmless to call a function like glDeleteBuffers() when
//	no OpenGL context is active, but on Win32 glDeleteBuffers might still be NULL.
//	Rather than dirtying up the platform-independent code with a test 
//	"if glDeleteBuffers != NULL", I'd prefer to dirty up the Win32 code
//	by initializing glDeleteBuffers to &DummyDeleteBuffers rather than to NULL, 
//	so we can always call it safely.  If Windows ever lets us compile 
//	for OpenGL 1.5 or later, the function DummyDeleteBuffers() can disappear
//	along with the code for loading the glDeleteBuffers entry point at runtime.
//	Similar comments apply to the other functions dummified below.

static void __stdcall DummyDeleteBuffers(
	signed int			n,
	const unsigned int	*buffers)
{
	(void) n;
	(void) buffers;
}

static void __stdcall DummyUseProgram(
	unsigned int	aProgram)
{
	(void) aProgram;
}

static void __stdcall DummyDeleteProgram(
	unsigned int	aProgram)
{
	(void) aProgram;
}

static void __stdcall DummyDeleteArrays(
	signed int			n,
	const unsigned int	*arrays)
{
	(void) n;
	(void) arrays;
}

static void __stdcall DummyDeleteFramebuffers(
	signed int			n,
	const unsigned int	*framebuffers)
{
	(void) n;
	(void) framebuffers;
}

static void __stdcall DummyDeleteRenderbuffers(
	signed int			n,
	const unsigned int	*renderbuffers)
{
	(void) n;
	(void) renderbuffers;
}

#ifdef DEBUG

static void __stdcall DummyDebugMessageCallbackAMD(
	GLDEBUGPROCAMD	callback,
	const GLvoid	*userParam)
{
	(void)callback;
	(void)userParam;
}

static void __stdcall DummyDebugMessageCallbackARB(
	GLDEBUGPROCARB	callback,
	const GLvoid	*userParam)
{
	(void)callback;
	(void)userParam;
}

#endif

static void __stdcall DummyDeleteQueries(
	signed int			n,
	const unsigned int	*queries)
{
	(void) n;
	(void) queries;
}
