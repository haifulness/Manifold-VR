//	GeometryGames-Win32.h
//
//	Definitions for the various Geometry Games' Win32 user interface.
//	The user interface knows about the ModelData,
//	but doesn't know anything about OpenGL.
//
//	The Win32 versions of the Geometry Games typically
//	require the import libraries
//
//		kernel32.lib  user32.lib  gdi32.lib  opengl32.lib
//		comdlg32.lib  comctl32.lib
//
//	as well as  vfw32.lib  if the AVI video feature is used.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once


//	Win32 headers
#define WIN32_LEAN_AND_MEAN		//	Include basic Win32 headers only.
#define UNICODE					//	Use Win32 API with 16-bit Unicode characters.
#include <windows.h>

//	The GeometryGames-Common.h header file contains
//	an opaque type for the application-specific ModelData.
#include "GeometryGames-Common.h"

//	The GeometryGames-OpenGL.h header file contains
//	an opaque type for the application-specific GraphicsDataGL.
#include "GeometryGames-OpenGL.h"


//	Window class names
//
//		Microsoft's page
//
//			http://msdn.microsoft.com/en-us/library/windows/desktop/ff381397(v=vs.85).aspx
//
//		titled "Creating a Window" says
//
//			Class names are local to the current process,
//			so the name only needs to be unique within the process.
//			However, the standard Windows controls also have classes.
//			If you use any of those controls, you must pick class names
//			that do not conflict with the control class names.
//			For example, the window class for the button control
//			is named "Button".
//
//	The user never sees the window class names.
//
#define MAIN_WINDOW_CLASS_NAME		u"Geometry Games Main Window Class"
#define DRAWING_WINDOW_CLASS_NAME	u"Geometry Games Drawing Window Class"


//	Each application's WindowData structure contains,
//	as its first field, a GeometryGamesWindowData structure.
//	Because the GeometryGamesWindowData is the first field
//	in the WindowData, we may safely typecast a pointer
//	to the GeometryGamesWindowData into a pointer
//	to the whole WindowData, and vice versa.
//
//	Conceptually, the application-independent GeometryGamesWindowData
//	acts like a "superclass" of the application-specific WindowData,
//	but implemented in straight C instead of Objective-C or C++.
//
typedef struct
{
	//	The main window.
	HWND			itsWindow;

	//	Draw into a child window of the main window.
	HWND			itsDrawingPanel;

	//	OpenGL rendering context.
	HDC				itsDeviceContext;
	HGLRC			itsRenderingContext;
	
	//	Framebuffer options.
	bool			itsDepthBufferFlag,
					itsMultisampleFlag;

	//	When going fullscreen, save the frame so it can be restored later.
	//	Note:  Fullscreen mode is optional.  Individual applications
	//	may choose to offer it or not, as they wish.
	bool			itsFullscreenFlag;
	RECT			itsSavedFrame;

	//	Each application may, if it wishes, store a file name
	//	(without the path or extension) for use as a window title.
	//	As of April 2013, only Curved Spaces and Kristalle set itsFileTitle.
	WCHAR			itsFileTitle[64];

	//	The application-specific code will give us
	//	an opaque pointer to its ModelData.
	ModelData		*mdp;
	
#ifdef SUPPORT_OPENGL
	//	The application-specific code will give us
	//	an opaque pointer to its GraphicsDataGL.
	GraphicsDataGL	*gdp;
#endif

} GeometryGamesWindowData;


//	Package up information for the idle-time routine.
typedef struct
{
	double	itsFramePeriod;			//	Inverse of frame rate
	bool	itsKeepGoingFlag,		//	Is at least one window present?
			itsAnimationFlag;		//	Is at least one animation active?
} IdleTimeData;



//	Win32-specific global variables

//	We measure the frame rate for the whole program -- not an individual
//	window -- so it makes sense to toggle its display on and off globally.
//	gShowFrameRate enables DisplayFrameRate(), which shows the total time
//	per frame.  While this may be useful for some purposes, during development
//	it may be more helpful to enable DISPLAY_GPU_TIME_PER_FRAME instead.
extern bool		gShowFrameRate;


//	in GeometryGames-Win32-WinMain.c
extern void				TestSupportingFiles(WORD aPrimaryLangID);
extern void				InitLocalization(WORD aLanguageID);
extern bool				RegisterGeometryGamesWindowClasses(void);
extern bool				IsGeometryGamesMainWindow(HWND aWindow);
extern HWND				CreateGeometryGamesWindow(unsigned int aMultipleH, unsigned int aMultipleV, unsigned int aToolbarHeight);
extern void				GetInitialWindowRect(DWORD aWindowStyles, unsigned int aMultipleH, unsigned int aMultipleV, unsigned int aToolbarHeight, RECT *aBestWindowRect);
extern double			MeasureFramePeriod(void);
extern BOOL CALLBACK	DoIdleTime(HWND aWindow, LPARAM anIdleTimeDataPtr);
extern void				DisplayFrameRate(IdleTimeData *someIdleTimeData);
extern BOOL CALLBACK	SetWindowTitle(HWND aWindow, LPARAM aTitle);

//	in GeometryGames-Win32-WinProc.c
extern bool				SetUpDrawingPanel(GeometryGamesWindowData *ggwd);
extern void				ShutDownDrawingPanel(GeometryGamesWindowData *ggwd);
extern void				CloseAllGeometryGamesWindows(void);
extern void				PaintWindow(GeometryGamesWindowData *ggwd);
extern void				SetMinWindowSize(HWND aWindow, POINT *aMinSize);
extern void				SetMaxWindowSize(RECT aZoomRect, POINT *aMaxPosition, POINT *aMaxSize);
extern void				RefreshMirroring(HWND aWindow);
extern void				RefreshMenuBar(GeometryGamesWindowData *ggwd);
extern void				DisplayChange(GeometryGamesWindowData *ggwd);
extern void				ToggleFullScreen(GeometryGamesWindowData *ggwd);
extern void				EnterFullScreen(GeometryGamesWindowData *ggwd);
extern void				ExitFullScreen(GeometryGamesWindowData *ggwd);
extern void				CopyTheImage(GeometryGamesWindowData *ggwd);
extern void				SaveTheImage(GeometryGamesWindowData *ggwd);
extern bool				CopyDirectoryName(WCHAR *aPathName, DWORD aPathNameLength, WCHAR *aDirectoryName, DWORD aDirectoryNameLength);
extern void				OpenHelpPage(const Char16 *aFolderName, const Char16 *aFileBaseName, bool aFileIsLocalized);

//	in <ApplicationName>-Win32-WndProc.c
extern LRESULT CALLBACK	MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK	DrawingWndProc(HWND aDrawingWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK	RefreshLanguage(HWND aWindow, LPARAM anUnusedParameter);
extern HMENU			BuildLocalizedMenuBar(ModelData *md);
