//	GeometryGames-Win32-WndProc.c
//
//	Provides supporting functions for MainWndProc() and DrawingWndProc()
//	in the Win32 versions of the various Geometry Games.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGames-Win32.h"
#include "GeometryGamesUtilities-Win.h"
#include "GeometryGamesLocalization.h"
#include "GeometryGames-Win32-LoadEntryPoints.h"
#ifdef SUPPORT_OPENGL
#include "GeometryGames-OpenGL.h"
#endif
#include <stdio.h>
#include <shlobj.h>		//	for SHGetFolderPath()
#include <commdlg.h>	//	for SaveTheImage dialog
#include <shellapi.h>	//	for ShellExecute()

//	The MinGW implementations of swprintf() and vswprintf()
//	differ from the ISO standard versions in that
//	they lack a buffer-length parameter.
extern int __cdecl __MINGW_NOTHROW swprintf(wchar_t*, const wchar_t*, ...);


//	During development it's helpful to know
//	how long it takes the GPU to render each frame.
//#define DISPLAY_GPU_TIME_PER_FRAME

//	Uncomment the following line to save the OpenGL version number
//	and extensions list to a file.  For use during development only.
//#define SAVE_VERSION_AND_EXTENSION_LIST_TO_FILE

//	What is the minimum acceptable size for the window's client area?
#define MIN_CLIENT_SIZE	256


//	Support for WGL extensions.

#define WGL_DRAW_TO_WINDOW_ARB		0x2001
#define WGL_ACCELERATION_ARB		0x2003
#define WGL_SUPPORT_OPENGL_ARB		0x2010
#define WGL_DOUBLE_BUFFER_ARB		0x2011
#define WGL_COLOR_BITS_ARB			0x2014
#define WGL_ALPHA_BITS_ARB			0x201B
#define WGL_DEPTH_BITS_ARB			0x2022
#define WGL_STENCIL_BITS_ARB		0x2023
#define WGL_FULL_ACCELERATION_ARB	0x2027
#define WGL_SAMPLE_BUFFERS_ARB		0x2041
#define WGL_SAMPLES_ARB				0x2042

#define WGL_CONTEXT_MAJOR_VERSION_ARB				0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB				0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB					0x2093
#define WGL_CONTEXT_FLAGS_ARB						0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB					0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB		0x00000002
#define ERROR_INVALID_VERSION_ARB					0x2095
#define WGL_CONTEXT_PROFILE_MASK_ARB				0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB			0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002

typedef BOOL	(WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC	(WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL	(WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);


//	Don't include gl.h.  
//	All OpenGL code should live in the platform-independent files.
//	If any OpenGL code sneaks into the Win32-specific files, 
//	we want it to trigger a compile-time warning.
//	The only exception is glGetString(), which SetUpDrawingPanel() calls 
//	to test whether it got a hardware-accelerated driver or "GDI Generic".
extern const unsigned char * __stdcall	glGetString(unsigned int name);
#define GL_RENDERER	0x1F01


static BOOL CALLBACK	PostCloseMessage(HWND aWindow, LPARAM anUnusedParameter);
#ifdef DISPLAY_GPU_TIME_PER_FRAME
static void				ShowGPUTimeInTitleBar(unsigned int anElapsedTime, HWND aWindow);
#endif
static HANDLE			FetchTheImage(GeometryGamesWindowData *ggwd);
#ifdef SAVE_VERSION_AND_EXTENSION_LIST_TO_FILE
static void				SaveVersionAndExtensionListToFile(void);
#endif


bool SetUpDrawingPanel(
	GeometryGamesWindowData	*ggwd)
{
	ErrorText							theErrorMessage					= NULL,
										theErrorTitle					= u"SetUpDrawingPanel() failed";	//	default title, may be overridden
	RECT								theClientRect;
	HWND								theDummyDrawingPanel			= NULL;
	HDC									theDummyDeviceContext			= NULL;
	HGLRC								theDummyRenderingContext		= NULL;
	PFNWGLCREATECONTEXTATTRIBSARBPROC	theWglCreateContextAttribsARB	= NULL;
	bool								theFormatIsValid;
	PFNWGLCHOOSEPIXELFORMATARBPROC		theWglChoosePixelFormatARB		= NULL;
	signed int							thePixelFormat;
	unsigned int						theNumPixelFormats;
	PIXELFORMATDESCRIPTOR				theDummyPixelFormatDescriptor =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	//	size
		1,								//	version number
		  PFD_DRAW_TO_WINDOW			//	format
		| PFD_SUPPORT_OPENGL
	//	| PFD_STEREO	//	could be useful in the future, but not needed for anaglyphic stereo 3D
		| PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,					//	preferred format is rgba (not palette)
		32, 							//	preferred color buffer depth (including alpha bits)
		0, 0, 0, 0, 0, 0,				//	color bits ignored
		8,								//	alpha buffer depth
		0,								//	shift bit ignored
		0,								//	no accumulation buffer
		0, 0, 0, 0, 					//	accumulation bits ignored
		24,								//	24-bit Z-buffer
		0,								//	no stencil buffer
		0,								//	no auxiliary buffer
		0,								//	ignored
		PFD_MAIN_PLANE, 				//	underlay/overlay planes
		0,								//	ignored
		0,								//	underlay color
		0								//	ignored
	};
	signed int							thePixelFormatIntAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,	true,
		WGL_SUPPORT_OPENGL_ARB,	true,
		WGL_DOUBLE_BUFFER_ARB,	true,
		WGL_ACCELERATION_ARB,	WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,		32,	//	including alpha bits (contrary to some online documentation)
		WGL_ALPHA_BITS_ARB,		8,
		WGL_DEPTH_BITS_ARB,		ggwd->itsDepthBufferFlag ? 24 : 0,	//	24-bit Z-buffer or no Z-buffer
		WGL_STENCIL_BITS_ARB,	0,
		WGL_SAMPLE_BUFFERS_ARB,	0,	//	overridden below;  hardcoded for array positions 16-17
		WGL_SAMPLES_ARB,		0,	//	overridden below;  hardcoded for array positions 18-19
		0,						0
	};
	PIXELFORMATDESCRIPTOR				theActualPixelFormatDescriptor;
	PFNWGLSWAPINTERVALEXTPROC			wglSwapInterval	= NULL;

	static const int					theContextAttributes[] =
	{
		//	Many OpenGL 3.0 drivers are buggy, so let's ask for OpenGL 3.3
		//	in hopes of getting a less buggy driver.
		//
		//	According to www.opengl.org/registry/specs/ARB/wgl_create_context.txt
		//
		//		If version 3.2 or greater is requested, 
		//		the context returned may implement any of the following versions:
		//
		//		* The requested profile of the requested version.
		//		* The requested profile of any later version, so long as 
		//		  no features have been removed from that later version and profile.
		//
		//	so if version 3.3 isn't available, wglCreateContextAttribsARB should fail.
		//
		//	Note:  All Nvidia and AMD GPUs that support OpenGL 3.0 also support OpenGL 3.3,
		//	so we're not excluding any hardware there.  However, Intel HD Graphics 2000/3000
		//	supports OpenGL 3.0 but no higher.
		//
		WGL_CONTEXT_MAJOR_VERSION_ARB,	3,
		WGL_CONTEXT_MINOR_VERSION_ARB,	3,
#ifdef DEBUG
		WGL_CONTEXT_FLAGS_ARB,			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
		WGL_CONTEXT_FLAGS_ARB,			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#endif
		WGL_CONTEXT_PROFILE_MASK_ARB,	WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	//	Output variables should all be NULL.
	if (ggwd->itsDrawingPanel != NULL)
	{
		theErrorMessage = u"Window has pre-existing drawing panel.";
		goto CleanUpSetUpDrawingPanel;
	}
	if (ggwd->itsDeviceContext != NULL)
	{
		theErrorMessage = u"Window has pre-existing device context.";
		goto CleanUpSetUpDrawingPanel;
	}
	if (ggwd->itsRenderingContext != NULL)
	{
		theErrorMessage = u"Window has pre-existing rendering context.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Due to chicken-and-egg problems, we must
	//		create a dummy window,
	//		use the dummy window to load needed function pointers,
	//		destroy the dummy window, and finally
	//		create the real window.
	//
	//	If we weren't using multisampling we could keep the window
	//	and replace only the OpenGL rendering context, but
	//	with multisampling this isn't possible, because
	//		We need to load extensions to get a multisample-capable pixel format.
	//		To load extensions we need an OpenGL rendering context.
	//		To create an OpenGL rendering context we need to have
	//			set a (single-sample) pixel format.
	//		It's illegal to change a device context's pixel format.
	//			Specifically, Microsoft's documentation for SetPixelFormat() says
	//
	//			Setting the pixel format of a window more than once 
	//			can lead to significant complications for the Window Manager 
	//			and for multithread applications, so it is not allowed. 
	//			An application can only set the pixel format of a window one time.
	//			Once a window's pixel format is set, it cannot be changed.
	//			  You should select a pixel format in the device context 
	//			before calling the wglCreateContext function.
	//
	//	Caution:  CreateWindow() sends a WM_SIZE message.
	//
	GetClientRect(ggwd->itsWindow, &theClientRect);
	theDummyDrawingPanel = CreateWindow(
		DRAWING_WINDOW_CLASS_NAME,
		NULL,
		WS_CHILD | WS_VISIBLE,
		theClientRect.left,	//	should be 0
		theClientRect.top,	//	should be 0
		theClientRect.right - theClientRect.left,
		theClientRect.bottom - theClientRect.top,
		ggwd->itsWindow, 0, GetModuleHandle(NULL), NULL);
	if (theDummyDrawingPanel == NULL)
	{
		theErrorMessage = u"Can't create a dummy window for OpenGL graphics.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Get a device context for the drawing panel.
	theDummyDeviceContext = GetDC(theDummyDrawingPanel);
	if (theDummyDeviceContext == NULL)
	{
		theErrorMessage = u"Can't get a dummy device context.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Assign a pixel format to the dummy device context.
	thePixelFormat = ChoosePixelFormat(theDummyDeviceContext, &theDummyPixelFormatDescriptor);
	if (thePixelFormat == 0)
	{
		theErrorMessage = u"Can't get a dummy pixel format.";
		goto CleanUpSetUpDrawingPanel;
	}
	if ( ! SetPixelFormat(theDummyDeviceContext, thePixelFormat, &theDummyPixelFormatDescriptor) )
	{
		theErrorMessage = u"Can't set the dummy pixel format.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Get a dummy OpenGL rendering context for the drawing window.
	theDummyRenderingContext = wglCreateContext(theDummyDeviceContext);
	if (theDummyRenderingContext == NULL)
	{
		theErrorMessage = u"Can't get a dummy OpenGL rendering context.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Make the dummy context current.
	wglMakeCurrent(theDummyDeviceContext, theDummyRenderingContext);
	
	//	Get function pointers.
	//
	//	In theory they are guaranteed to be valid only in this dummy context,
	//	but in practice, due to chicken-and-egg problems, we must assume
	//	they'll be valid in the final context as well.
	
	theWglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
	if (theWglChoosePixelFormatARB == NULL)
	{
		theErrorMessage	= GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"wglChoosePixelFormatARB not found";
		goto CleanUpSetUpDrawingPanel;
	}

	theWglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
	if (theWglCreateContextAttribsARB == NULL)
	{
		theErrorMessage	= GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"wglCreateContextAttribsARB not found";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Destroy the dummy rendering context, device context and window.
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(theDummyRenderingContext);
	ReleaseDC(theDummyDrawingPanel, theDummyDeviceContext);
	DestroyWindow(theDummyDrawingPanel);
	theDummyRenderingContext	= NULL;
	theDummyDeviceContext		= NULL;
	theDummyDrawingPanel		= NULL;

	//	Create the final window.
	GetClientRect(ggwd->itsWindow, &theClientRect);
	ggwd->itsDrawingPanel = CreateWindow(
		DRAWING_WINDOW_CLASS_NAME,
		NULL,
		WS_CHILD | WS_VISIBLE,
		theClientRect.left,	//	should be 0
		theClientRect.top,	//	should be 0
		theClientRect.right - theClientRect.left,
		theClientRect.bottom - theClientRect.top,
		ggwd->itsWindow, 0, GetModuleHandle(NULL), NULL);
	if (ggwd->itsDrawingPanel == NULL)
	{
		theErrorMessage = u"Can't create a final window for OpenGL graphics.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Create the final device context.
	ggwd->itsDeviceContext = GetDC(ggwd->itsDrawingPanel);
	if (ggwd->itsDeviceContext == NULL)
	{
		theErrorMessage = u"Can't get a final device context.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Select the final pixel format.

	//		Set a flag, so we can record whether the wglChoosePixelFormatARB() succeeds,
	//		and thus whether the fallback ChoosePixelFormat() will be needed.
	theFormatIsValid = false;

	//		Make sure the parameters sit where we expect them to.
	if (thePixelFormatIntAttributes[16] != WGL_SAMPLE_BUFFERS_ARB
	 || thePixelFormatIntAttributes[18] != WGL_SAMPLES_ARB)
	{
		theErrorMessage = u"Internal Error: WGL_SAMPLE_BUFFERS_ARB and WGL_SAMPLES_ARB in unexpected locations.";
		goto CleanUpSetUpDrawingPanel;
	}

	if (ggwd->itsMultisampleFlag)
	{
		//	Try for 8-sample multisampling, then 6 samples, then 4, then 2, then give up.
		thePixelFormatIntAttributes[17] = 1;	//	enable multisampling
		thePixelFormatIntAttributes[19] = 8;	//	start with 8 samples/pixel
	}
	else
	{
		//	Try for single-sampling.
		thePixelFormatIntAttributes[17] = 0;	//	disable multisampling
		thePixelFormatIntAttributes[19] = 0;
	}

	do
	{
		theFormatIsValid = theWglChoosePixelFormatARB(
								ggwd->itsDeviceContext,
								thePixelFormatIntAttributes,
								NULL,
								1,
								&thePixelFormat,
								&theNumPixelFormats)
						&&
							theNumPixelFormats >= 1;

		thePixelFormatIntAttributes[19] -= 2;

	} while ( ggwd->itsMultisampleFlag
		   && ! theFormatIsValid
		   && thePixelFormatIntAttributes[19] > 0);

	if ( ! theFormatIsValid )
	{
		theErrorMessage	= GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"No valid pixel format found";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Get a description of thePixelFormat.
	if (DescribePixelFormat(ggwd->itsDeviceContext,
							thePixelFormat,
							sizeof(PIXELFORMATDESCRIPTOR),
							&theActualPixelFormatDescriptor)
		<= 0)
	{
		theErrorMessage = u"Can't get final pixel format description.";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Does thePixelFormat meet our needs?
	if (theActualPixelFormatDescriptor.iPixelType != PFD_TYPE_RGBA
	 || (	theActualPixelFormatDescriptor.cColorBits	!= 24	//	Microsoft's documentation claims that cColorBits
		 && theActualPixelFormatDescriptor.cColorBits	!= 32))	//	excludes the alpha bits, but in reality it includes them.
	{
		theErrorMessage	= GetLocalizedText(u"ErrorColorMessage");
		theErrorTitle	= GetLocalizedText(u"ErrorColorTitle");
		goto CleanUpSetUpDrawingPanel;
	}
	if (theActualPixelFormatDescriptor.cAlphaBits != 8)
	{
		theErrorMessage = GetLocalizedText(u"ErrorAlphaMessage");
		theErrorTitle	= GetLocalizedText(u"ErrorAlphaTitle");
		goto CleanUpSetUpDrawingPanel;
	}

	//	Set thePixelFormat in itsDeviceContext.
	//
	//	Note:  It's strange that SetPixelFormat() wants theActualPixelFormatDescriptor
	//	along with thePixelFormat, but that's what it wants, so we must comply.
	//
	if ( ! SetPixelFormat(ggwd->itsDeviceContext, thePixelFormat, &theActualPixelFormatDescriptor) )
	{
		theErrorMessage = GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"Can't set pixel format";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Create the final rendering context.
	ggwd->itsRenderingContext = theWglCreateContextAttribsARB(ggwd->itsDeviceContext, NULL, theContextAttributes);
	if (ggwd->itsRenderingContext == NULL)
	{
		theErrorMessage = GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"Can't create rendering context";
		goto CleanUpSetUpDrawingPanel;
	}

	//	Make itsRenderingContext current.
	//
	//		Note:  This is the only place that the Geometry Games
	//		software checks wglMakeCurrent()'s return value.
	//		Elsewhere it's not worth cluttering up the code.
	//		In the highly unlikely event that wglMakeCurrent()
	//		succeeds now and fails later on, nothing too terrible
	//		should happen because when wglMakeCurrent() fails to set
	//		the requested context it instead sets wglMakeCurrent(NULL, NULL),
	//		meaning that all subsequent OpenGL calls will be ignored.
	//
	if ( ! wglMakeCurrent(ggwd->itsDeviceContext, ggwd->itsRenderingContext) )
	{
		theErrorMessage	= GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"Can't make rendering context current";
		goto CleanUpSetUpDrawingPanel;
	}

	//	The generic Windows OpenGL driver ignores 3D graphics hardware.
	//	In practice this means that a computer from, say, Dell will run
	//	OpenGL fine because Dell provides a good driver, but a standard
	//	installation from a Windows XP CD will run OpenGL in software emulation
	//	(no doubt because Microsoft wants Microsoft-only DirectX graphics
	//	to run well while platform-independent OpenGL graphics runs poorly).
	//	I don't know any direct way to query whether hardware acceleration
	//	is active, but while our rendering context is current we can at least
	//	check the renderer name, and if it's "GDI Generic" notify the user.
	if (strcmp((const char *)glGetString(GL_RENDERER), "GDI Generic") == 0)
	{
		theErrorMessage = GetLocalizedText(u"ErrorDriverMessage");
		theErrorTitle	= GetLocalizedText(u"ErrorDriverTitle");
		goto CleanUpSetUpDrawingPanel;
	}

	//	On Win32 we must explicitly load functions from OpenGL 1.2 or later.
	if ( ! LoadOpenGLFunctions() )
	{
		theErrorMessage = GetLocalizedText(u"ErrorPleaseUseGL2");
		theErrorTitle	= u"LoadOpenGLFunctions() failed";
		goto CleanUpSetUpDrawingPanel;
	}
	
	//	Synchronize the animation to the monitor's refresh rate.
	wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapInterval != NULL)
		wglSwapInterval(1);

#ifdef SAVE_VERSION_AND_EXTENSION_LIST_TO_FILE
	//	If someone reports a problem on a remote computer, it's sometimes useful
	//	to know what version of OpenGL is running and which extensions are available.
	//
	//	Note:
	//		This was important when the Geometry Games supported OpenGL 2 + extensions.
	//		It's less important now that we require OpenGL 3.3 or newer.
	//
	SaveVersionAndExtensionListToFile();
#endif

CleanUpSetUpDrawingPanel:

	wglMakeCurrent(NULL, NULL);
	
	if (theDummyRenderingContext != NULL)
	{
		wglDeleteContext(theDummyRenderingContext);
		theDummyRenderingContext = NULL;
	}
	
	if (theDummyDeviceContext != NULL)
	{
		ReleaseDC(theDummyDrawingPanel, theDummyDeviceContext);
		theDummyDeviceContext = NULL;
	}
	
	if (theDummyDrawingPanel != NULL)
	{
		DestroyWindow(theDummyDrawingPanel);
		theDummyDrawingPanel = NULL;
	}

	if (theErrorMessage != NULL)
	{
		ErrorMessage(theErrorMessage, theErrorTitle);
		ShutDownDrawingPanel(ggwd);
		return false;
	}
	else
		return true;
}

void ShutDownDrawingPanel(
	GeometryGamesWindowData	*ggwd)
{
	wglMakeCurrent(NULL, NULL);	//	redundant, but safe

	if (ggwd->itsRenderingContext != NULL)
	{
		wglDeleteContext(ggwd->itsRenderingContext);
		ggwd->itsRenderingContext = NULL;
	}

	if (ggwd->itsDeviceContext != NULL)
	{
		ReleaseDC(ggwd->itsDrawingPanel, ggwd->itsDeviceContext);
		ggwd->itsDeviceContext = NULL;
	}

	if (ggwd->itsDrawingPanel != NULL)
	{
		DestroyWindow(ggwd->itsDrawingPanel);	//	automatically notifies parent window
		ggwd->itsDrawingPanel = NULL;
	}
}


void CloseAllGeometryGamesWindows(void)
{
	//	Send each Geometry Games main window a WM_CLOSE message.
	//	If all windows comply, our message loop in WinMain() will terminate.
	EnumThreadWindows(GetCurrentThreadId(), PostCloseMessage, 0);
}

static BOOL CALLBACK PostCloseMessage(
	HWND	aWindow,
	LPARAM	anUnusedParameter)
{
	UNUSED_PARAMETER(anUnusedParameter);

	//	Skip windows that aren't ours.
	//
	//	Note:  On my system the Input Method Environment (IME) creates
	//	secret windows that shadow the main window.  They are top-level windows,
	//	so EnumThreadWindows() finds them.  Specifically, each main window owns
	//	a secret window of class "IME", which in turn owns another secret window
	//	of class "MSCTFIME UI".
	//
	if (IsGeometryGamesMainWindow(aWindow))
		PostMessage(aWindow, WM_CLOSE, 0, 0);

	return true;	//	keep going
}


void PaintWindow(
	GeometryGamesWindowData	*ggwd)
{
	RECT			theRect;
	LONG			theWidthPx,
					theHeightPx;
	ErrorText		theSetUpError				= NULL,
					theRenderError				= NULL;
	HDC				theSavedDeviceContext		= NULL;
	HGLRC			theSavedRenderingContext	= NULL;
#ifdef DISPLAY_GPU_TIME_PER_FRAME
	unsigned int	theElapsedTime				= 0;	//	in nanoseconds
#endif

	//	client rectangle size in pixels
	GetClientRect(ggwd->itsDrawingPanel, &theRect);
	theWidthPx	= theRect.right - theRect.left;
	theHeightPx	= theRect.bottom - theRect.top;

	//	In the normal flow of events there's no need to save and restore
	//	the rendering context.  However, we may get "unexpected" WM_PAINT
	//	messages -- for example if a MessageBox takes it upon itself
	//	to send WM_PAINT messages to underlying windows -- so play it safe
	//	and leave the rendering context as we found it.

	theSavedDeviceContext		= wglGetCurrentDC();		//	typically NULL
	theSavedRenderingContext	= wglGetCurrentContext();	//	typically NULL
	wglMakeCurrent(ggwd->itsDeviceContext, ggwd->itsRenderingContext);
		theSetUpError = SetUpGraphicsAsNeeded(ggwd->mdp, ggwd->gdp);
		if (theSetUpError == NULL)
			theRenderError = Render(ggwd->mdp, ggwd->gdp, theWidthPx, theHeightPx,
#ifdef DISPLAY_GPU_TIME_PER_FRAME
				&theElapsedTime);
#else
				NULL);
#endif
		if (theSetUpError == NULL && theRenderError == NULL)
			SwapBuffers(ggwd->itsDeviceContext);
	wglMakeCurrent(theSavedDeviceContext, theSavedRenderingContext);

	ValidateRect(ggwd->itsDrawingPanel, NULL);

	if (theSetUpError != NULL)
		FatalError(theSetUpError,  u"OpenGL Setup Error");
	if (theRenderError != NULL)
		FatalError(theRenderError, u"OpenGL Rendering Error");

#ifdef DISPLAY_GPU_TIME_PER_FRAME
	ShowGPUTimeInTitleBar(theElapsedTime, ggwd->itsWindow);
#endif
}

#ifdef DISPLAY_GPU_TIME_PER_FRAME
static void ShowGPUTimeInTitleBar(
	unsigned int	anElapsedTime,	//	in nanoseconds
	HWND			aWindow)
{
	WCHAR	theWindowTitle[64];
	
	static unsigned int	theFrameCount	= 0,
						theTotalTime	= 0;	//	in µs

	//	Display the frame rate once every 64 frames.
	theTotalTime += anElapsedTime / 1000;	//	convert ns to µs before adding to theTotalTime
	theFrameCount++;
	if (theFrameCount == 64)
	{
		//	Note:  The MinGW version of swprintf() doesn't check for buffer overruns.
		swprintf(	theWindowTitle,
					/* BUFFER_LENGTH(theWindowTitle), */
					u"%d µs/frame",
					theTotalTime / 64);
		SetWindowText(aWindow, theWindowTitle);
		
		theFrameCount	= 0;
		theTotalTime	= 0;
	}
}
#endif


void SetMinWindowSize(
	HWND	aWindow,
	POINT	*aMinSize)
{
	RECT	theRect;

	// Prevent the client area from going smaller than MIN_CLIENT_SIZE.
	SetRect(&theRect, 0, 0, MIN_CLIENT_SIZE, MIN_CLIENT_SIZE);
	AdjustWindowRect(&theRect, (DWORD) GetWindowLongPtr(aWindow, GWL_STYLE), true);
	aMinSize->x = theRect.right - theRect.left;
	aMinSize->y = theRect.bottom - theRect.top;
}

void SetMaxWindowSize(
	RECT	aZoomRect,
	POINT	*aMaxPosition,
	POINT	*aMaxSize)
{
	//	According to msdn.microsoft.com/en-us/library/ms632605(VS.85).aspx
	//
	//		For systems with multiple monitors, the ptMaxSize and ptMaxPosition
	//		members describe the maximized size and position of the window 
	//		on the primary monitor, even if the window ultimately maximizes 
	//		onto a secondary monitor.  In that case, the window manager adjusts 
	//		these values to compensate for differences between the primary monitor 
	//		and the monitor that displays the window. Thus, if the user leaves 
	//		ptMaxSize untouched, a window on a monitor larger than 
	//		the primary monitor maximizes to the size of the larger monitor.
	//
	//	It sounds hopeless to get a square window on a secondary monitor
	//	(unless its size happens to equal that of the primary monitor).
	//	So let's just restore the original window position and size,
	//	and hope for the best.
	
	aMaxPosition->x = aZoomRect.left;
	aMaxPosition->y = aZoomRect.top;

	aMaxSize->x = aZoomRect.right  - aZoomRect.left;
	aMaxSize->y = aZoomRect.bottom - aZoomRect.top;
}


void RefreshMirroring(HWND aWindow)
{
	//	Reset the window layout, left-to-right (LTR) or right-to-left (RTL).

	if (CurrentLanguageReadsRightToLeft())
	{
		//	Mirror the main window.
		//	Mirror child windows only if you want to -- it isn't necessary.
		SetWindowLongPtr(	aWindow,
							GWL_EXSTYLE,
							GetWindowLongPtr(aWindow, GWL_EXSTYLE) |  WS_EX_LAYOUTRTL);
	}
	else
	{
		SetWindowLongPtr(	aWindow,
							GWL_EXSTYLE,
							GetWindowLongPtr(aWindow, GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
	}
}


void RefreshMenuBar(GeometryGamesWindowData *ggwd)
{
	HMENU	theOldMenu,
			theNewMenu;

	//	Replace the window's old menu with a new menu
	//	in the current language, using the current ModelData.
	theOldMenu = GetMenu(ggwd->itsWindow);	//	may be NULL
	theNewMenu = BuildLocalizedMenuBar(ggwd->mdp);
	SetMenu(ggwd->itsWindow, theNewMenu);
	if (theOldMenu != NULL)
		DestroyMenu(theOldMenu);
	DrawMenuBar(ggwd->itsWindow);
}


void DisplayChange(
	GeometryGamesWindowData	*ggwd)
{
	//	If the user changes the display's color depth,
	//	re-create the OpenGL drawing panel.
	//
	//	Note:	It's illegal to call SetPixelFormat()
	//			twice for the same window, so we must
	//			completely destroy the old drawing panel
	//			and replace it with a new one.
	//

	//	Shut down application-specific OpenGL objects,
	//	clear the ModelData's references to them,
	//	and -- most important of all -- clear the flags
	//
	//		itsPreparedGLVersion
	//		itsPreparedShaders
	//		itsPreparedTextures
	//		etc.
	//
	//	so that on the next pass through PaintWindow(),
	//	SetUpGraphicsAsNeeded() will know that all OpenGL objects
	//	have been released.
	//
	wglMakeCurrent(ggwd->itsDeviceContext, ggwd->itsRenderingContext);
	ShutDownGraphicsAsNeeded(ggwd->mdp, ggwd->gdp);
	wglMakeCurrent(NULL, NULL);

	//	Shut down the drawing panel along with its OpenGL context.
	ShutDownDrawingPanel(ggwd);

	//	Create a new drawing panel with a new OpenGL context.
	SetUpDrawingPanel(ggwd);
}


void ToggleFullScreen(GeometryGamesWindowData *ggwd)
{
	if ( ! ggwd->itsFullscreenFlag )
		EnterFullScreen(ggwd);
	else
		ExitFullScreen(ggwd);
}

void EnterFullScreen(GeometryGamesWindowData *ggwd)
{
	HMENU	theOldMenu;

	if ( ! ggwd->itsFullscreenFlag )
	{
		//	Save the window's position.
		GetWindowRect(ggwd->itsWindow, &ggwd->itsSavedFrame);

		//	Delete the window's menu.
		theOldMenu = GetMenu(ggwd->itsWindow);
		SetMenu(ggwd->itsWindow, NULL);
		DestroyMenu(theOldMenu);
		DrawMenuBar(ggwd->itsWindow);

		//	Remove the window's frame and title bar.
		SetWindowLongPtr(	ggwd->itsWindow,
							GWL_STYLE,
							WS_POPUP | WS_VISIBLE);

		//	Expand the window to full screen.
		SetWindowPos(	ggwd->itsWindow,
						HWND_TOP,
						0,
						0,
						GetSystemMetrics(SM_CXSCREEN),
						GetSystemMetrics(SM_CYSCREEN),
						SWP_SHOWWINDOW);

		//	Set itsFullscreenFlag.
		ggwd->itsFullscreenFlag = true;
	}
}

void ExitFullScreen(GeometryGamesWindowData *ggwd)
{
	if (ggwd->itsFullscreenFlag)
	{
		//	Restore the window's frame and title bar.
		SetWindowLongPtr(	ggwd->itsWindow,
							GWL_STYLE,
							WS_OVERLAPPEDWINDOW | WS_VISIBLE);

		//	Restore the window's saved position.
		SetWindowPos(	ggwd->itsWindow,
						HWND_TOP,
						ggwd->itsSavedFrame.left,
						ggwd->itsSavedFrame.top,
						ggwd->itsSavedFrame.right  - ggwd->itsSavedFrame.left,
						ggwd->itsSavedFrame.bottom - ggwd->itsSavedFrame.top,
						SWP_SHOWWINDOW);

		//	Restore the window's menu bar.
		//	(The current menu should be NULL.)
		RefreshMenuBar(ggwd);

		//	Unset itsFullscreenFlag.
		ggwd->itsFullscreenFlag = false;
	}
}


void CopyTheImage(
	GeometryGamesWindowData	*ggwd)
{
	ErrorText	theErrorMessage		= NULL;
	HANDLE		theDibHandle		= NULL;
	bool		theClipboardIsOpen	= false;

	//	Read the image as a DIB.
	theDibHandle = FetchTheImage(ggwd);
	if (theDibHandle == NULL)
	{
		theErrorMessage = u"Couldn't read image to copy to clipboard.";
		goto CleanUpCopyTheImage;
	}

	//	Transfer the DIB to the clipboard.

	if (OpenClipboard(NULL) == 0)
	{
		theErrorMessage = u"Couldn't open clipboard.";
		goto CleanUpCopyTheImage;
	}
	theClipboardIsOpen = true;

	if (EmptyClipboard() == 0)
	{
		theErrorMessage = u"Couldn't delete previous clipboard contents.";
		goto CleanUpCopyTheImage;
	}

	if (SetClipboardData(CF_DIB, theDibHandle) == NULL)
	{
		theErrorMessage = u"Couldn't transfer image to clipboard.";
		goto CleanUpCopyTheImage;
	}
	//	The clipboard now owns the DIB.
	theDibHandle = NULL;

CleanUpCopyTheImage:

	if (theClipboardIsOpen)
		CloseClipboard();

	if (theDibHandle != NULL)
		GlobalFree(theDibHandle);

	if (theErrorMessage != NULL)
		ErrorMessage(theErrorMessage, u"Copy Error");
}

void SaveTheImage(
	GeometryGamesWindowData	*ggwd)
{
	//	The user's preferred directory for saving images may differ
	//	from his/her preferred directory for saving or opening other files.
	static WCHAR	theSaveImageDirectory[MAX_PATH] = {0};

	ErrorText			theErrorMessage		= NULL;
	WCHAR				theFileName[1024]	= {0};
	OPENFILENAME		theFileInfo			= {sizeof(OPENFILENAME),
		NULL, NULL, u"32-bit Bitmap (*.bmp)\0*.bmp\0", NULL, 0, 0,
		theFileName, BUFFER_LENGTH(theFileName), NULL, 0,
		theSaveImageDirectory, NULL, OFN_OVERWRITEPROMPT, 0, 0,
		u"bmp", 0L, NULL, NULL};
	HANDLE				theDibHandle		= NULL;
	unsigned int		theDIBSize			= 0;
	HANDLE				theFile				= NULL;
	BITMAPFILEHEADER	theFileHeader		= {0x4D42, 0, 0, 0, 0};
	DWORD				theNumBytes			= 0;
	Byte				*theDib				= NULL;


	//	One-time initialization.
	if (theSaveImageDirectory[0] == 0)
	{
		//	SHGetFolderPath() requires a buffer of length MAX_PATH.
		GEOMETRY_GAMES_ASSERT(	BUFFER_LENGTH(theSaveImageDirectory) == MAX_PATH,
								"Bad buffer length for theSaveImageDirectory");
		SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0 /* = SHGFP_TYPE_CURRENT */, theSaveImageDirectory);
	}

	//	Prompt the user for a file name.
	//	The extension will be .bmp (see theFileInfo above).
	if (GetSaveFileName(&theFileInfo) == 0)
	{
		//	The user cancelled or an error occurred.
		goto CleanUpSaveTheImage;
	}

	//	Remember the directory name for next time.
	CopyDirectoryName(	theFileName,
						BUFFER_LENGTH(theFileName),
						theSaveImageDirectory,
						BUFFER_LENGTH(theSaveImageDirectory));

	//	Read the image as a DIB.
	theDibHandle = FetchTheImage(ggwd);
	if (theDibHandle == NULL)
	{
		theErrorMessage = u"Couldn't read image to save to file.";
		goto CleanUpSaveTheImage;
	}
	
	//	How big is the DIB?
	theDIBSize = GlobalSize(theDibHandle);
	if (theDIBSize == 0)
	{
		theErrorMessage = u"Couldn't get image size.";
		goto CleanUpSaveTheImage;
	}
	
	//	Open the output file.
	theFile = CreateFile(	theFileName,
							GENERIC_WRITE,
							0,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (theFile == NULL)
	{
		theErrorMessage = u"Couldn't create file to save image.";
		goto CleanUpSaveTheImage;
	}
	
	//	Save the DIB to the file.
	
	//		First the BITMAPFILEHEADER.
	theFileHeader.bfSize	= sizeof(BITMAPFILEHEADER) + theDIBSize;
	theFileHeader.bfOffBits	= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if (WriteFile(theFile, &theFileHeader, sizeof(BITMAPFILEHEADER), &theNumBytes, NULL) == 0
	 || theNumBytes != sizeof(BITMAPFILEHEADER))
	{
		theErrorMessage = u"Couldn't write bitmap file header.";
		goto CleanUpSaveTheImage;
	}
	
	//		Then the DIB itself.
	theDib = (Byte *) GlobalLock(theDibHandle);
	if (theDib == NULL)
	{
		theErrorMessage = u"Couldn't lock the image memory to save to file.";
		goto CleanUpSaveTheImage;
	}
	if (WriteFile(theFile, theDib, theDIBSize, &theNumBytes, NULL) == 0
	 || theNumBytes != theDIBSize)
	{
		theErrorMessage = u"Couldn't write DIB to file.";
		goto CleanUpSaveTheImage;
	}
	GlobalUnlock(theDibHandle);
	theDib = NULL;

CleanUpSaveTheImage:

	if (theDib != NULL)
		GlobalUnlock(theDibHandle);

	if (theFile != NULL)
		CloseHandle(theFile);

	if (theDibHandle != NULL)
		GlobalFree(theDibHandle);

	if (theErrorMessage != NULL)
		ErrorMessage(theErrorMessage, u"Save Error");
}

static HANDLE FetchTheImage(
	GeometryGamesWindowData	*ggwd)
{
	ErrorText			theErrorMessage			= NULL;
	RECT				theClientRect			= {0,0,0,0};
	unsigned int		theImageWidth			= 0,
						theImageHeight			= 0,
						theImageSize			= 0,
						theHeaderSize			= 0,
						theTotalSize			= 0;
	HANDLE				theDibHandle			= NULL;
	Byte				*theDib					= NULL;
	BITMAPINFOHEADER	*theHeader				= NULL;
	PixelRGBA			*theImage				= NULL;
	unsigned int		theCount				= 0;
	PixelRGBA			*thePixel				= NULL;
	Byte				theSwap					= 0;

	//	theImageWidth and theImageHeight can be anything we want.
	//	They don't have to be the same as the view size,
	//	even though using the view size is the simplest choice.
	if (GetClientRect(ggwd->itsDrawingPanel, &theClientRect) == 0
	 || theClientRect.left != 0
	 || theClientRect.top  != 0
	 || (theImageWidth  = theClientRect.right)  == 0
	 || (theImageHeight = theClientRect.bottom) == 0)
	{
		theErrorMessage = u"Couldn't read window size.";
		goto CleanUpFetchTheImage;
	}

	//	How many bytes do we have?
	//	(If we were reading RGB instead of RGBA we'd need to pad the rows
	//	to insure a row-stride that's a multiple of four, but for RGBA
	//	it's automatic.  Unfortunately the alpha channel will be ignored,
	//	but let's include the alpha values anyhow.)
	theImageSize = theImageWidth * theImageHeight * 4;

	//	Create a DIB.

	theHeaderSize	= sizeof(BITMAPINFOHEADER);
	theTotalSize	= theHeaderSize + theImageSize;

	theDibHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, theTotalSize);
	if (theDibHandle == NULL)
	{
		theErrorMessage = u"Not enough memory to copy image.";
		goto CleanUpFetchTheImage;
	}

	theDib = (Byte *) GlobalLock(theDibHandle);
	if (theDib == NULL)
	{
		theErrorMessage = u"Couldn't lock the image memory to write image data.";
		goto CleanUpFetchTheImage;
	}
	theHeader	= (BITMAPINFOHEADER *) theDib;
	theImage	= (PixelRGBA *)(theDib + theHeaderSize);

	theHeader->biSize			= sizeof(BITMAPINFOHEADER);
	theHeader->biWidth			= theImageWidth;
	theHeader->biHeight			= theImageHeight;
	theHeader->biPlanes			= 1;
	theHeader->biBitCount		= 32;
	theHeader->biCompression	= BI_RGB;
	theHeader->biSizeImage		= 0;
	theHeader->biXPelsPerMeter	= 0;
	theHeader->biYPelsPerMeter	= 0;
	theHeader->biClrUsed		= 0;
	theHeader->biClrImportant	= 0;

	//	Draw the image.
	//
	//	Note:  Microsoft's BITMAP format does not support transparency --
	//	the last byte of each pixel gets ignored!
	//	RenderToBuffer() returns pixels with premultiplied alpha,
	//	which the BITMAP format interprets as fading to black.
	//	Given the lack of transparency support, this is the best
	//	outcome we could have hoped for.
	wglMakeCurrent(ggwd->itsDeviceContext, ggwd->itsRenderingContext);
	theErrorMessage = RenderToBuffer(
						ggwd->mdp,
						ggwd->gdp,
						ggwd->itsMultisampleFlag,	//	May use multisampling even if live animation does not.
						ggwd->itsDepthBufferFlag,
						&Render,	//	in <ApplicationName>Graphics-OpenGL.c
						theImageWidth, theImageHeight, theImage);
	wglMakeCurrent(NULL, NULL);
	if (theErrorMessage != NULL)
		goto CleanUpFetchTheImage;

	//	Swap the byte order manually to conform to .bmp conventions.
	thePixel = theImage;
	for (theCount = theImageWidth * theImageHeight; theCount-- > 0; )
	{
		theSwap		= thePixel->r;
		thePixel->r	= thePixel->b;
		thePixel->b	= theSwap;
		
		thePixel++;
	}

	//	Unlock the handle.
	GlobalUnlock(theDibHandle);
	theDib = NULL;

CleanUpFetchTheImage:

	if (theDib != NULL)
		GlobalUnlock(theDibHandle);

	if (theErrorMessage != NULL)
	{
		if (theDibHandle != NULL)
		{
			GlobalFree(theDibHandle);
			theDibHandle = NULL;
		}

		ErrorMessage(theErrorMessage, u"Image Export Error");
	}
	
	return theDibHandle;
}

bool CopyDirectoryName(
	WCHAR	*aPathName,				//	input, 	NULL-terminated string of WCHARs
	DWORD	aPathNameLength,		//	input, 	size of input  buffer in WCHARs, including terminating zero
	WCHAR	*aDirectoryName,		//	output, NULL-terminated string of WCHARs
	DWORD	aDirectoryNameLength)	//	input,	size of output buffer in WCHARs, including terminating zero
{
	//	Copy aPathName to aDirectoryName, excluding the file name itself.
	
	DWORD	thePosition,
			i;
	
	//	Make sure output buffer is valid.
	if (aDirectoryName == NULL || aDirectoryNameLength == 0)
		FatalError(u"CopyDirectoryName() received no output buffer.", u"Internal Error");

	//	Preinitialize an empty output string, in case of error.
	aDirectoryName[0] = 0;

	//	Find the terminating zero at the end of the input path name.
	for (thePosition = 0; thePosition < aPathNameLength; thePosition++)
		if (aPathName[thePosition] == 0)
			break;
	if (thePosition == aPathNameLength)
		return false;	//	no terminating zero
	
	//	Back up to the last backslash character.
	while (thePosition > 0
	    && aPathName[thePosition] != '\\')
	{
		thePosition--;
	}
	if (thePosition == 0)
		return false;	//	no backslash found
	
	if (aDirectoryNameLength < thePosition + 2)
		return false;	//	output buffer too small
	
	//	Copy the directory name from aPathName to aDirectoryName.
	for (i = 0; i <= thePosition; i++)
		aDirectoryName[i] = aPathName[i];
	
	//	Append a terminating zero.
	aDirectoryName[thePosition + 1] = 0;
	
	//	Success
	return true;
}


void OpenHelpPage(
	const Char16	*aFolderName,		//	zero-terminated UTF-16 string
	const Char16	*aFileBaseName,		//	just "Foo", not "Foo.html" or "Foo-xx.html", as zero-terminated UTF-16 string
	bool			aFileIsLocalized)
{
	Char16			theFileName[64]			= u"",
					theFilePath[2048]		= u"";
	const Char16	*aTwoLetterLanguageCode	= NULL;
	Char16			theLocalizedSuffix[]	= u"-??.html",
					thePlainSuffix[]		= u".html";

	if (aFileIsLocalized)
	{
		aTwoLetterLanguageCode = GetCurrentLanguage();

		theLocalizedSuffix[1] = aTwoLetterLanguageCode[0];
		theLocalizedSuffix[2] = aTwoLetterLanguageCode[1];

		Strcpy16(theFileName, BUFFER_LENGTH(theFileName), aFileBaseName		);
		Strcat16(theFileName, BUFFER_LENGTH(theFileName), theLocalizedSuffix);
	}
	else
	{
		Strcpy16(theFileName, BUFFER_LENGTH(theFileName), aFileBaseName		);
		Strcat16(theFileName, BUFFER_LENGTH(theFileName), thePlainSuffix	);
	}

	if (GetAbsolutePath(aFolderName, theFileName, theFilePath, BUFFER_LENGTH(theFilePath)) == NULL)
	{
		ShellExecute(NULL, u"open", theFilePath, NULL, NULL, SW_SHOWNORMAL);
	}
}


#ifdef SAVE_VERSION_AND_EXTENSION_LIST_TO_FILE

#include "GeometryGames-Win32-OpenGLEntryPoints.h"
#include <stdio.h>

static void SaveVersionAndExtensionListToFile(void)
{
	const char		*theVendorString		= NULL,
					*theRendererString		= NULL,
					*theVersionString		= NULL,
					*theShaderString		= NULL;
	FILE			*fp						= NULL;
	unsigned int	theNumExtensions		= 0,
					i;
	
	static bool	theVersionWasChecked = false;
	
	if ( ! theVersionWasChecked )
	{
		theVersionWasChecked = true;

		theVendorString		= (const char *) glGetString(GL_VENDOR);
		theRendererString	= (const char *) glGetString(GL_RENDERER);
		theVersionString	= (const char *) glGetString(GL_VERSION);
		theShaderString		= (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
	
		fp = fopen("OpenGL info.txt", "w");
		if (fp != NULL)
		{
			fprintf(fp,
				"Vendor\n\t%s\n\nRenderer\n\t%s\n\nVersion\n\t%s\n\nShading language\n\t%s\n\nExtensions\n",
				theVendorString   != NULL ? theVendorString   : "N/A",
				theRendererString != NULL ? theRendererString : "N/A",
				theVersionString  != NULL ? theVersionString  : "N/A",
				theShaderString   != NULL ? theShaderString   : "N/A");
		
			glGetIntegerv(GL_NUM_EXTENSIONS, (signed int *)&theNumExtensions);
			for (i = 0; i < theNumExtensions; i++)
				fprintf(fp, "\t%s\n", (const char *)glGetStringi(GL_EXTENSIONS, i));

			fclose(fp);
		}
	}
}

#endif
