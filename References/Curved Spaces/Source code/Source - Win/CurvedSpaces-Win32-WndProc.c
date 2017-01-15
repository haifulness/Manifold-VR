//	CurvedSpaces-Win32-WndProc.c
//
//	Provides MainWndProc(), DrawingWndProc() and supporting functions
//	for the Win32 version of Curved Spaces.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Win32.h"
#include "CurvedSpacesIDs.h"
#include "GeometryGamesUtilities-Win.h"
#include "GeometryGamesLocalization.h"
#include <commdlg.h>


//	Command IDs

#define IDC_FILE_OPEN_NEW				0x0000
#define IDC_FILE_EXIT					0x0001

#define IDC_SPACE_CHANGE				0x0100

#define IDC_EXPORT_COPY					0x0200
#define IDC_EXPORT_SAVE					0x0201

#define IDC_VIEW_CENTERPIECE_NONE		0x0300
#define IDC_VIEW_CENTERPIECE_EARTH		0x0301
#define IDC_VIEW_CENTERPIECE_GALAXY		0x0302
#define IDC_VIEW_CENTERPIECE_GYROSCOPE	0x0303
#define IDC_VIEW_OBSERVER				0x0310
#define IDC_VIEW_COLOR_CODING			0x0320
#define IDC_VIEW_CLIFFORD_NONE			0x0330
#define IDC_VIEW_CLIFFORD_BICOLOR		0x0331
#define IDC_VIEW_CLIFFORD_ONE_SET		0x0332
#define IDC_VIEW_CLIFFORD_TWO_SETS		0x0333
#define IDC_VIEW_CLIFFORD_THREE_SETS	0x0334
#define IDC_VIEW_VERTEX_FIGURES			0x0340
#define IDC_VIEW_FOG					0x0350
#define	IDC_VIEW_FULLSCREEN				0x0360
#define	IDC_VIEW_STEREO_NONE			0x0370
#define	IDC_VIEW_STEREO_GREYSCALE		0x0371
#define	IDC_VIEW_STEREO_COLOR			0x0372

#define IDC_HELP_HELP					0x0400
#define IDC_HELP_CONTACT				0x0401
#define IDC_HELP_TRANSLATORS			0x0402
#define IDC_HELP_NSF					0x0403
#define IDC_HELP_ABOUT					0x0404


//	Even when successful, GetOpenFileName() throws access violation exceptions,
//	apparently from within RPCRT4.dll (a Remote Procedure Call library).
//	This problem is widely discussed on the web, but unfortunately
//	the sites I found provided neither an explanation nor a fix.
//	Under normal operation the exceptions aren't a problem:
//	we ignore them, they get passed back to Windows, and GetOpenFileName()
//	returns successfully.  However, they thoroughly confuse
//	the gdb debugger (running under CodeBlocks):  they trash the stack
//	and debugging comes screeching to a halt.  To avoid this problem
//	and permit meaningful debugging, bypass the GetOpenFileName() call.
//#define BYPASS_GETOPENFILENAME


static bool					SetUpWindowData(HWND aWindow);
static void					ShutDownWindowData(WindowData *wd);
static void					MenuPrepare(WindowData *wd, HMENU aMenu);
static void					MenuCommand(WindowData *wd, WORD aCommand);
static ErrorText			ReadRawText(WCHAR *aFileName, Byte **someRawText);
#ifdef CURVED_SPACES_MOUSE_INTERFACE
static void					SteerWithMouse(WindowData *wd, WPARAM wParam, LPARAM lParam, bool aSteeringFlag);
#endif
#ifdef CURVED_SPACES_TOUCH_INTERFACE
static DisplayPoint			GetMouseLocation(HWND aDrawingWindow, LPARAM lParam);
static DisplayPointMotion	GetMouseMotion(HWND aDrawingWindow, LPARAM lParam, POINT aPreviousCursorPosition);
#endif

//	Callback function to process messages in the main window.
LRESULT CALLBACK MainWndProc(
	HWND	hWnd,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	WindowData	*wd = NULL;

	//	For all messages except WM_CREATE and WM_DESTROY, locate the WindowData.
	//	If we confirm once and for all that wd != NULL, then
	//	the routines we call in the message loop can safely rely on it.
	if (hWnd != NULL)
	{
		if (uMsg != WM_CREATE
		 && uMsg != WM_DESTROY)
		{
			wd = (WindowData *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (wd == NULL)
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		else
		{
			wd = NULL;
		}
	}
	else
		return 0;	//	should never occur

	//	Process the message.
	switch (uMsg)
	{
		case WM_CREATE:
			if (SetUpWindowData(hWnd))
				return 0;	//	success -- continue normally
			else
				return -1;	//	failure -- abort window creation

		case WM_CLOSE:
			//	Shut down our window data, including itsRenderingContext,
			//	itsDeviceContext and itsDrawingPanel.  Otherwise Windows will destroy
			//	child windows prematurely, leaving itsRenderingContext invalid.
			ShutDownWindowData(wd);
			DestroyWindow(hWnd);
			return 0;

//		case WM_DESTROY:
//			return 0;

		case WM_KEYDOWN:
			switch (wParam)	//	virtual key code
			{
				//	Note:  Return, Escape and Space could be handled
				//	in WM_CHAR (as 0x0D, 0x1B and ' ', respectively),
				//	but the arrow keys don't generate characters 
				//	so they must be handled here.

				case VK_UP:		wd->md.itsUserSpeed += USER_SPEED_INCREMENT;	break;
				case VK_DOWN:	wd->md.itsUserSpeed -= USER_SPEED_INCREMENT;	break;
				case ' ':		wd->md.itsUserSpeed = 0.0;						break;

				case VK_LEFT:
					ChangeAperture(&wd->md, true);
					break;
				case VK_RIGHT:
					ChangeAperture(&wd->md, false);
					break;

#ifdef START_OUTSIDE
				case VK_RETURN:
					if (wd->md.itsViewpoint == ViewpointExtrinsic)
						wd->md.itsViewpoint = ViewpointEntering;
					break;
#endif

				case VK_ESCAPE:

#ifdef CURVED_SPACES_MOUSE_INTERFACE
					//	A right-click (or even a left-click) would be a better way
					//	to exit navigational mode.  Nevertheless, we don't want
					//	to leave a novice user stranded without a mouse.

					//	Done steering.  Release the mouse.
					ReleaseCapture();

					//	HMMM... NEED TO THINK ABOUT ESC DOING TWO THINGS!
#endif

					if (wd->ggwd.itsFullscreenFlag)
						ExitFullScreen(&wd->ggwd);

					break;
			}
			return 0;

		case WM_CHAR:
			switch ((WCHAR) wParam)	//	character code
			{
				case 'f':	//	"secret" keyboard command to show frame rate
					gShowFrameRate = ! gShowFrameRate;
					EnumThreadWindows(	GetCurrentThreadId(),
										SetWindowTitle,
										(LPARAM) (gShowFrameRate ? u"…" : NULL));
					break;
			}
			return 0;

		case WM_MOUSEWHEEL:
			wd->md.itsUserSpeed += USER_SPEED_INCREMENT * ((short)HIWORD(wParam))/120.0;
			return 0;

		case WM_INITMENUPOPUP:
			MenuPrepare(wd, (HMENU) wParam);
			return 0;

		case WM_COMMAND:
			MenuCommand(wd, LOWORD(wParam));
			return 0;

//		case WM_EXITMENULOOP:
//		case WM_EXITSIZEMOVE:
//		case WM_ACTIVATE:
//			//	Warning:  wd will be NULL when a WM_ACTIVATE event
//			//	arrives during window destruction.
//			return 0;

		case WM_GETMINMAXINFO:
			SetMinWindowSize(hWnd, &((MINMAXINFO*)lParam)->ptMinTrackSize);
			return 0;

		case WM_SIZE:
			MoveWindow(wd->ggwd.itsDrawingPanel, 0, 0, LOWORD(lParam), HIWORD(lParam), false);
			return 0;

		case WM_DISPLAYCHANGE:
			DisplayChange(&wd->ggwd);
			return 0;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}


//	Callback function to process messages in the drawing panel.
LRESULT CALLBACK DrawingWndProc(
	HWND	aDrawingWindow,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	WindowData	*wd;

	//	If we confirm once and for all that wd != NULL, then the routines
	//	we call in the message switch can safely rely on it.
	//	See MainWndProc() for an explanation of why the WindowData
	//	isn't available at WM_DESTROY time.
	if (aDrawingWindow != NULL)
	{
		if (uMsg != WM_CREATE
		 && uMsg != WM_DESTROY)
		{
			wd = (WindowData *) GetWindowLongPtr(GetParent(aDrawingWindow), GWLP_USERDATA);
			if (wd == NULL)
				return DefWindowProc(aDrawingWindow, uMsg, wParam, lParam);
		}
		else
		{
			wd = NULL;
		}
	}
	else
		return 0;	//	should never occur

	switch (uMsg)
	{
		case WM_CREATE:
			return 0;

		case WM_LBUTTONDBLCLK:
			//	For consistency with the Torus Games,
			//	we want a double-click to get us out of navigational mode.
			//	Nevertheless, for the convenience of CurvedSpaces-only users,
			//	a single-click should also do the job.  The solution is
			//	to exit navigational mode on the first click, and then
			//	ignore the second click (if any) so that it doesn't put us
			//	back into navigational mode.
			return 0;

#ifdef CURVED_SPACES_MOUSE_INTERFACE

		case WM_LBUTTONDOWN:
			if (GetCapture() != aDrawingWindow)
			{
				//	Capture the mouse for steering.
				SetCapture(aDrawingWindow);
				SetCursor(NULL);
				SteerWithMouse(wd, wParam, lParam, false);
			}
			else
			{
				//	Done steering.  Release the mouse.
				ReleaseCapture();
			}
			return 0;

		case WM_RBUTTONDOWN:
			//	Done steering.  Release the mouse.
			ReleaseCapture();
			return 0;

		case WM_MOUSEMOVE:
			if (GetCapture() == aDrawingWindow)
				SteerWithMouse(wd, wParam, lParam, true);
			return 0;

#endif	//	CURVED_SPACES_MOUSE_INTERFACE

#ifdef CURVED_SPACES_TOUCH_INTERFACE

		case WM_LBUTTONDOWN:

			SetCapture(aDrawingWindow);

			wd->itsPrevCursorPosition.x = (signed short int) LOWORD(lParam);
			wd->itsPrevCursorPosition.y = (signed short int) HIWORD(lParam);

			return 0;

		case WM_LBUTTONUP:
			ReleaseCapture();
			return 0;

		case WM_MOUSEMOVE:
			if (GetCapture() == aDrawingWindow)
			{
				MouseMoved(	&wd->md,
							GetMouseLocation(aDrawingWindow, lParam),
							GetMouseMotion(aDrawingWindow, lParam, wd->itsPrevCursorPosition),
							wParam & MK_SHIFT,				//	shift key down?
							wParam & MK_CONTROL,			//	control key down?
							GetKeyState(VK_MENU) & 0x8000);	//	alt key down?
				
				wd->itsPrevCursorPosition.x = (signed short int) LOWORD(lParam);
				wd->itsPrevCursorPosition.y = (signed short int) HIWORD(lParam);
			}
			return 0;

#endif	//	CURVED_SPACES_TOUCH_INTERFACE

		case WM_MBUTTONDOWN:	//	includes mouse-wheel clicks
			wd->md.itsUserSpeed = 0.0;
			return 0;

		case WM_PAINT:
			PaintWindow(&wd->ggwd);
			return 0;

		case WM_SIZE:
			InvalidateRect(aDrawingWindow, NULL, false);;
			return 0;

		default:
			return DefWindowProc(aDrawingWindow, uMsg, wParam, lParam);
	}
}


static bool SetUpWindowData(
	HWND	aWindow)
{
	WindowData	*wd;

	//	Attempt to allocate the WindowData.
	wd = GET_MEMORY(sizeof(WindowData));

	//	If the memory allocation failed, return false
	//	to tell Windows to abort window construction.
	if (wd == NULL)
	{
		SetWindowLongPtr(aWindow, GWLP_USERDATA, 0);
		ErrorMessage(u"Couldn't get memory for WindowData.", u"SetUpWindowData() Error");
		return false;
	}

	//	The window keeps a pointer to the WindowData and vice versa.
	SetWindowLongPtr(aWindow, GWLP_USERDATA, (LONG_PTR) wd);
	wd->ggwd.itsWindow = aWindow;

	//	Set pointers to NULL for robust error handling.
	wd->ggwd.itsDrawingPanel		= NULL;
	wd->ggwd.itsDeviceContext		= NULL;
	wd->ggwd.itsRenderingContext	= NULL;
	
	//	Curved Spaces needs a depth buffer,
	//	and looks best with multisampling.
	wd->ggwd.itsDepthBufferFlag	= true;
	wd->ggwd.itsMultisampleFlag	= true;

	//	Fullscreen mode is initially off.
	wd->ggwd.itsFullscreenFlag	= false;
	wd->ggwd.itsSavedFrame		= (RECT){0,0,0,0};

	//	Initialize itsFileTitle to an empty string
	//	for use until a file gets read.
	wd->ggwd.itsFileTitle[0] = 0;
	
	//	Give the GeometryGamesWindowData
	//	an opaque pointer to the ModelData.
	wd->ggwd.mdp = &wd->md;

	//	Initialize the model's internal data.
	SetUpModelData(&wd->md);

#ifdef SUPPORT_OPENGL

	//	Give the GeometryGamesWindowData
	//	an opaque pointer to the GraphicsDataGL.
	wd->ggwd.gdp = &wd->gd;
	
	//	Initialize the OpenGL data to all zero values.
	//	(We can't call SetUpGraphicsAsNeeded() until
	//	the OpenGL context is active.)
	ZeroGraphicsDataGL(&wd->gd);

#endif

#ifdef CURVED_SPACES_TOUCH_INTERFACE
	//	No known cursor position yet.
	wd->itsPrevCursorPosition.x = 0;
	wd->itsPrevCursorPosition.y = 0;
#endif

	//	Create itsDrawingPanel and set up OpenGL within it.
	if ( ! SetUpDrawingPanel(&wd->ggwd) )
		goto FailedToSetUpWindowData;

	//	Report success.
	return true;

FailedToSetUpWindowData:

	//	Clean up.
	ShutDownWindowData(wd);	//	wd is known to be non-NULL here

	//	Report failure.
	ErrorMessage(u"Failed to set up window.", u"SetUpWindowData() Error");
	return false;
}

static void ShutDownWindowData(WindowData *wd)
{
	//	Shut down application-specific OpenGL objects
	//	and clear the ModelData's references to them.
	wglMakeCurrent(wd->ggwd.itsDeviceContext, wd->ggwd.itsRenderingContext);
	ShutDownGraphicsAsNeeded(&wd->md, &wd->gd);
	wglMakeCurrent(NULL, NULL);

	//	Shut down the drawing panel along with its OpenGL context.
	ShutDownDrawingPanel(&wd->ggwd);

	//	Let the platform-independent code
	//	free any memory it may have allocated.
	ShutDownModelData(&wd->md);

	//	Once all dependent data have been cleaned up,
	//	then free the WindowData itself.
	SetWindowLongPtr(wd->ggwd.itsWindow, GWLP_USERDATA, 0);
	FREE_MEMORY(wd);
}


static void MenuPrepare(
	WindowData	*wd,
	HMENU		aMenu)
{
	CheckMenuItem(aMenu, IDC_VIEW_CENTERPIECE_NONE,
		wd->md.itsCenterpiece == CenterpieceNone      ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CENTERPIECE_EARTH,
		wd->md.itsCenterpiece == CenterpieceEarth     ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CENTERPIECE_GALAXY,
		wd->md.itsCenterpiece == CenterpieceGalaxy    ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CENTERPIECE_GYROSCOPE,
		wd->md.itsCenterpiece == CenterpieceGyroscope ? MF_CHECKED : MF_UNCHECKED);

	//	Enable the spaceship only in monoscopic 3D,
	//	for reasons explained in DrawTheSceneIntrinsically().
	EnableMenuItem(aMenu, IDC_VIEW_OBSERVER,
		wd->md.itsStereoMode == StereoNone ? MF_ENABLED : MF_GRAYED);
	CheckMenuItem(aMenu, IDC_VIEW_OBSERVER,
		wd->md.itsShowObserver && wd->md.itsStereoMode == StereoNone ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(aMenu, IDC_VIEW_COLOR_CODING,
		wd->md.itsShowColorCoding ? MF_CHECKED : MF_UNCHECKED);

	EnableMenuItem(aMenu, IDC_VIEW_CLIFFORD_NONE,		wd->md.itsThreeSphereFlag ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(aMenu, IDC_VIEW_CLIFFORD_BICOLOR,	wd->md.itsThreeSphereFlag ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(aMenu, IDC_VIEW_CLIFFORD_ONE_SET,	wd->md.itsThreeSphereFlag ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(aMenu, IDC_VIEW_CLIFFORD_TWO_SETS,	wd->md.itsThreeSphereFlag ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(aMenu, IDC_VIEW_CLIFFORD_THREE_SETS,	wd->md.itsThreeSphereFlag ? MF_ENABLED : MF_GRAYED);
	CheckMenuItem(aMenu, IDC_VIEW_CLIFFORD_NONE,		(wd->md.itsThreeSphereFlag && wd->md.itsCliffordMode == CliffordNone     ) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CLIFFORD_BICOLOR,		(wd->md.itsThreeSphereFlag && wd->md.itsCliffordMode == CliffordBicolor  ) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CLIFFORD_ONE_SET,		(wd->md.itsThreeSphereFlag && wd->md.itsCliffordMode == CliffordOneSet   ) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CLIFFORD_TWO_SETS,	(wd->md.itsThreeSphereFlag && wd->md.itsCliffordMode == CliffordTwoSets  ) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_CLIFFORD_THREE_SETS,	(wd->md.itsThreeSphereFlag && wd->md.itsCliffordMode == CliffordThreeSets) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(aMenu, IDC_VIEW_VERTEX_FIGURES,
		wd->md.itsShowVertexFigures ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(aMenu, IDC_VIEW_FOG,
		wd->md.itsFogFlag ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(aMenu, IDC_VIEW_FULLSCREEN,	//	Checkmark should never get seen!
		wd->ggwd.itsFullscreenFlag ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(aMenu, IDC_VIEW_STEREO_NONE,
		wd->md.itsStereoMode == StereoNone		? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_STEREO_GREYSCALE,
		wd->md.itsStereoMode == StereoGreyscale	? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(aMenu, IDC_VIEW_STEREO_COLOR,
		wd->md.itsStereoMode == StereoColor		? MF_CHECKED : MF_UNCHECKED);
}


static void MenuCommand(
	WindowData	*wd,
	WORD		aCommand)
{
	HWND			theNewWindow;
	WindowData		*theNewWindowData;

	switch (aCommand)
	{
		//	file menu

		case IDC_FILE_OPEN_NEW:
			theNewWindow = CreateGeometryGamesWindow(1, 1, 0);
			if (theNewWindow != NULL)
			{
				theNewWindowData = (WindowData *) GetWindowLongPtr(theNewWindow, GWLP_USERDATA);
				if (theNewWindowData != NULL)
					DoFileOpen(theNewWindowData);
			}
			break;

		case IDC_FILE_EXIT:
			//	Send each window a WM_CLOSE message.
			//	If all windows comply, our message loop in WinMain() will terminate.
			CloseAllGeometryGamesWindows();
			break;
		
		//	space menu

		case IDC_SPACE_CHANGE:
			DoFileOpen(wd);
			break;
		
		//	export menu

		case IDC_EXPORT_COPY:
			CopyTheImage(&wd->ggwd);
			break;

		case IDC_EXPORT_SAVE:
			SaveTheImage(&wd->ggwd);
			break;
		
		//	view menu

		case IDC_VIEW_CENTERPIECE_NONE:
			SetCenterpiece(&wd->md, CenterpieceNone);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CENTERPIECE_EARTH:
			SetCenterpiece(&wd->md, CenterpieceEarth);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CENTERPIECE_GALAXY:
			SetCenterpiece(&wd->md, CenterpieceGalaxy);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CENTERPIECE_GYROSCOPE:
			SetCenterpiece(&wd->md, CenterpieceGyroscope);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_OBSERVER:
			SetShowObserver(&wd->md, ! wd->md.itsShowObserver );
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_COLOR_CODING:
			SetShowColorCoding(&wd->md, ! wd->md.itsShowColorCoding );
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CLIFFORD_NONE:
			SetShowCliffordParallels(&wd->md, CliffordNone);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CLIFFORD_BICOLOR:
			SetShowCliffordParallels(&wd->md, CliffordBicolor);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CLIFFORD_ONE_SET:
			SetShowCliffordParallels(&wd->md, CliffordOneSet);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CLIFFORD_TWO_SETS:
			SetShowCliffordParallels(&wd->md, CliffordTwoSets);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_CLIFFORD_THREE_SETS:
			SetShowCliffordParallels(&wd->md, CliffordThreeSets);
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_VERTEX_FIGURES:
			SetShowVertexFigures(&wd->md, ! wd->md.itsShowVertexFigures );
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_FOG:
			SetFogFlag(&wd->md, ! wd->md.itsFogFlag );
			break;

		case IDC_VIEW_FULLSCREEN:
			ToggleFullScreen(&wd->ggwd);
			break;

		case IDC_VIEW_STEREO_NONE:
			SetStereo3DMode(&wd->md, StereoNone		);
			wd->gd.itsPreparedTextures	= false;
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_STEREO_GREYSCALE:
			SetStereo3DMode(&wd->md, StereoGreyscale);
			wd->gd.itsPreparedTextures	= false;
			wd->gd.itsPreparedVBOs		= false;
			break;

		case IDC_VIEW_STEREO_COLOR:
			SetStereo3DMode(&wd->md, StereoColor	);
			wd->gd.itsPreparedTextures	= false;
			wd->gd.itsPreparedVBOs		= false;
			break;
		
		//	help menu

		case IDC_HELP_HELP:
			OpenHelpPage(u"Help",	u"CurvedSpacesWelcome", true	);
			break;
		
		case IDC_HELP_CONTACT:
			OpenHelpPage(u"Help",	u"Contact",				true	);
			break;
		
		case IDC_HELP_TRANSLATORS:
			OpenHelpPage(u"Thanks",	u"Translators",			false	);
			break;
		
		case IDC_HELP_NSF:
			OpenHelpPage(u"Thanks",	u"NSF",					false	);
			break;

		case IDC_HELP_ABOUT:
			//	Building a fancy dialog box with graphics is tricky
			//	because of the conflict between the "dialog layout units"
			//	and the pixel dimensions of bitmaps.  A simple but humble
			//	solution relays basic information in a MessageBox().
			ErrorMessage(GetLocalizedText(u"AboutBoxMessage"), GetLocalizedText(u"AboutBoxTitle"));
			break;
	}
}


void DoFileOpen(WindowData *wd)
{
	//	The user's preferred directory for reading manifolds may differ
	//	from his/her preferred directory for saving images.
	static Char16	theLastManifoldDirectory[MAX_PATH] = {0};

	ErrorText		theErrorMessage					= NULL;
	Char16			*theChar,
					theFileName[MAX_PATH]			= u"",
					*theExtension					= NULL;
	OPENFILENAME	theFileInfo						= {
						sizeof(OPENFILENAME), wd->ggwd.itsWindow, NULL,
						u"Curved Spaces Generator Files (*.gen)\0*.gen\0",
						NULL, 0, 1,
						theFileName, BUFFER_LENGTH(theFileName),
						wd->ggwd.itsFileTitle, BUFFER_LENGTH(wd->ggwd.itsFileTitle),
						theLastManifoldDirectory, NULL, 0, 0, 0,
						u"gen", 0L, NULL, NULL};
	Byte			*theRawText						= NULL;

	//	One-time initialization.
	if (theLastManifoldDirectory[0] == 0)
	{
		if (GetAbsolutePath(u"Sample Spaces", NULL, theLastManifoldDirectory, BUFFER_LENGTH(theLastManifoldDirectory)) == NULL)
		{
			//	In theory Win32 accepts both forward slashes / and backslashes \ as
			//	path separators (see http://en.wikipedia.org/wiki/Path_(computing) ).
			//	In practice CreateFile() happily accepts either.
			//	However, GetOpenFileName() wants its OPENFILENAME structure's
			//	lpstrInitialDir field to use a backslash, and ignores it
			//	if it uses forward slashes.  So we must convert.
			for (theChar = theLastManifoldDirectory; *theChar != 0; theChar++)
			{
				if (*theChar == u'/')
					*theChar = u'\\';
			}
		}
	}

#ifdef BYPASS_GETOPENFILENAME
//	Strcpy(theFileName, BUFFER_LENGTH(theFileName), u"E:\\Jeff\\1 General\\Software - Current\\Curved Spaces\\Curved Spaces 3\\Curved Spaces - Win\\Sample Spaces\\Basic\\3-Torus.gen");
	Strcpy(theFileName, BUFFER_LENGTH(theFileName), u"E:\\Jeff\\1 General\\Software - Current\\Curved Spaces\\Curved Spaces 3\\Curved Spaces - Win\\Sample Spaces\\Basic\\HyperbolicDodecahedron.gen");
	wd->ggwd.itsFileTitle[0] = 0;
#else
	//	Invite the user to select a file.
	//	If the user cancels, return with no error.
	if (GetOpenFileName(&theFileInfo) == 0)
		return;
	
	//	Remember the directory name for next time.
	CopyDirectoryName(	theFileName,
						BUFFER_LENGTH(theFileName),
						theLastManifoldDirectory,
						BUFFER_LENGTH(theLastManifoldDirectory));

	//	Remove the .gen filename extension.
	theExtension = wd->ggwd.itsFileTitle;
	while (*theExtension != 0)
		theExtension++;
	if ((theExtension - wd->ggwd.itsFileTitle) >= 5
	 && theExtension[-4] == '.'
	 && theExtension[-3] == 'g'
	 && theExtension[-2] == 'e'
	 && theExtension[-1] == 'n')
	{
		theExtension[-4] = 0;
	}
#endif

	//	Read the raw data from the UTF-8 or Latin-1 file.
	//	Append a terminating zero.
	theErrorMessage = ReadRawText(theFileName, &theRawText);
	if (theErrorMessage != NULL)
		goto CleanUpDoFileOpen;

	//	Read the generating matrices and set up the manifold.
	theErrorMessage = LoadGeneratorFile(&wd->md, theRawText);
		//	Test for theErrorMessage != NULL follows below.
		//
		//	First, though, whether we succeed or fail,
		//	we need to update the shaders, VBOs and VAOs.
	wd->gd.itsPreparedShaders	= false;
	wd->gd.itsPreparedVBOs		= false;	//	triggers a VAO refresh as well
	if (theErrorMessage != NULL)
		goto CleanUpDoFileOpen;


CleanUpDoFileOpen:

	FREE_MEMORY_SAFELY(theRawText);

	if (theErrorMessage != NULL)
	{
		wd->ggwd.itsFileTitle[0] = 0;
		ErrorMessage(theErrorMessage, u"Couldn't open file");
	}

	//	Update the window title.
	//	If wd->ggwd.itsFileTitle[0] == 0,
	//	the title will revert to "Curved Spaces".
	SetWindowTitle(wd->ggwd.itsWindow, 0);
}


static ErrorText ReadRawText(
	WCHAR			*aFileName,
	Byte			**someRawText)
{
	ErrorText	theErrorMessage		= NULL;
	HANDLE		theFile				= NULL;
	DWORD		theNumBytes			= 0,
				theHighOrderBytes	= 0,
				theNumBytesRead		= 0;

	if (*someRawText != NULL)
		return u"ReadRawText() was passed a non-NULL output pointer.";

	theFile = CreateFile(aFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (theFile == NULL)
	{
		theErrorMessage = u"Couldn't open matrix file.";
		goto CleanUpReadRawBytes;
	}

	theNumBytes = GetFileSize(theFile, &theHighOrderBytes);
	if (theNumBytes == INVALID_FILE_SIZE
	 || theHighOrderBytes != 0)
	{
		theErrorMessage = u"Couldn't get matrix file size.";
		goto CleanUpReadRawBytes;
	}
	
	*someRawText = GET_MEMORY(theNumBytes + 1);	//	Allow room for a terminating zero.
	if (*someRawText == NULL)
	{
		theErrorMessage = u"Couldn't allocate memory to read matrix file.";
		goto CleanUpReadRawBytes;
	}

	if (ReadFile(theFile, *someRawText, theNumBytes, &theNumBytesRead, NULL) == 0
	 || theNumBytesRead != theNumBytes)
	{
		theErrorMessage = u"Couldn't read raw bytes from matrix file.";
		goto CleanUpReadRawBytes;
	}
	
	(*someRawText)[theNumBytes] = 0;	//	terminating zero

CleanUpReadRawBytes:

	if (theFile != NULL)
		CloseHandle(theFile);

	if (theErrorMessage != NULL)
		FREE_MEMORY_SAFELY(*someRawText);

	return theErrorMessage;
}


BOOL CALLBACK RefreshLanguage(
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
	{
		//	Reset the window's title.
		//	(If a file is open, the title won't change.)
		SetWindowTitle(aWindow, 0);

		//	Reset the mirroring.
		RefreshMirroring(aWindow);

		//	Replace the window's old menu with a new menu in the new language.
		RefreshMenuBar( (GeometryGamesWindowData *)GetWindowLongPtr(aWindow, GWLP_USERDATA) );
	}

	return true;	//	keep going
}


HMENU BuildLocalizedMenuBar(
	ModelData	*md)
{
	HMENU	theMainMenu		= NULL,
			theSubMenu		= NULL,
			theSubSubMenu	= NULL;

	UNUSED_PARAMETER(md);

	theMainMenu = CreateMenu();
	
	theSubMenu = CreateMenu();
	AppendMenu(theMainMenu, MF_POPUP, (UINT_PTR)theSubMenu, GetLocalizedText(u"File"));
#ifdef ALLOW_MULTIPLE_WINDOWS
	AppendMenu(theSubMenu, MF_STRING,    IDC_FILE_OPEN_NEW, GetLocalizedText(u"Open New…")	);
	AppendMenu(theSubMenu, MF_SEPARATOR, 0,                 NULL							);
#endif
	AppendMenu(theSubMenu, MF_STRING,    IDC_FILE_EXIT,     GetLocalizedText(u"Exit")		);
	theSubMenu = NULL;
	
	theSubMenu = CreateMenu();
	AppendMenu(theMainMenu, MF_POPUP, (UINT_PTR)theSubMenu, GetLocalizedText(u"Space"));
	AppendMenu(theSubMenu, MF_STRING,    IDC_SPACE_CHANGE,  GetLocalizedText(u"Change Space…"));
	theSubMenu = NULL;
	
	theSubMenu = CreateMenu();
	AppendMenu(theMainMenu, MF_POPUP, (UINT_PTR)theSubMenu, GetLocalizedText(u"Export"));
	AppendMenu(theSubMenu, MF_STRING,    IDC_EXPORT_COPY,   GetLocalizedText(u"Copy Image")	);
	AppendMenu(theSubMenu, MF_STRING,    IDC_EXPORT_SAVE,   GetLocalizedText(u"Save Image…"));
	theSubMenu = NULL;
	
	theSubMenu = CreateMenu();
	AppendMenu(theMainMenu, MF_POPUP, (UINT_PTR)theSubMenu, GetLocalizedText(u"View"));
	theSubSubMenu = CreateMenu();
		AppendMenu(theSubMenu, MF_POPUP | MF_STRING, (UINT_PTR)theSubSubMenu, GetLocalizedText(u"Centerpiece"));
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CENTERPIECE_NONE,      GetLocalizedText(u"No Centerpiece"));
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CENTERPIECE_EARTH,     GetLocalizedText(u"Earth")			);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CENTERPIECE_GALAXY,    GetLocalizedText(u"Galaxy")		);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CENTERPIECE_GYROSCOPE, GetLocalizedText(u"Gyroscope")		);
	theSubSubMenu = NULL;
	AppendMenu(theSubMenu, MF_STRING,    IDC_VIEW_OBSERVER,       GetLocalizedText(u"Spaceship")	);
	AppendMenu(theSubMenu, MF_STRING,    IDC_VIEW_COLOR_CODING,   GetLocalizedText(u"Color Coding")	);
	theSubSubMenu = CreateMenu();
		AppendMenu(theSubMenu, MF_POPUP | MF_STRING, (UINT_PTR)theSubSubMenu, GetLocalizedText(u"Clifford Parallels"));
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CLIFFORD_NONE,       GetLocalizedText(u"CliffordNone"));
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CLIFFORD_BICOLOR,    GetLocalizedText(u"Bicolor")		);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CLIFFORD_ONE_SET,    GetLocalizedText(u"One Set")		);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CLIFFORD_TWO_SETS,   GetLocalizedText(u"Two Sets")	);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_CLIFFORD_THREE_SETS, GetLocalizedText(u"Three Sets")	);
	theSubSubMenu = NULL;
	AppendMenu(theSubMenu, MF_STRING,    IDC_VIEW_VERTEX_FIGURES, GetLocalizedText(u"Vertex Figures")	);
	AppendMenu(theSubMenu, MF_SEPARATOR, 0,                       NULL									);
	AppendMenu(theSubMenu, MF_STRING,    IDC_VIEW_FOG,            GetLocalizedText(u"Fog")				);
	AppendMenu(theSubMenu, MF_SEPARATOR, 0,                       NULL									);
	AppendMenu(theSubMenu, MF_STRING,    IDC_VIEW_FULLSCREEN,     GetLocalizedText(u"Full Screen")		);
	theSubSubMenu = CreateMenu();
		AppendMenu(theSubMenu, MF_POPUP | MF_STRING, (UINT_PTR)theSubSubMenu, GetLocalizedText(u"Stereoscopic 3D"));
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_STEREO_NONE,      GetLocalizedText(u"No Stereo 3D")	);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_STEREO_GREYSCALE, GetLocalizedText(u"Greyscale")		);
		AppendMenu(theSubSubMenu, MF_STRING, IDC_VIEW_STEREO_COLOR,     GetLocalizedText(u"Color")			);
	theSubSubMenu = NULL;
	theSubMenu = NULL;
	
	theSubMenu = CreateMenu();
	AppendMenu(theMainMenu, MF_POPUP, (UINT_PTR)theSubMenu, GetLocalizedText(u"Help"));
	AppendMenu(theSubMenu, MF_STRING,    IDC_HELP_HELP,        GetLocalizedText(u"Help")				);
	AppendMenu(theSubMenu, MF_STRING,    IDC_HELP_CONTACT,     GetLocalizedText(u"Contact")				);
	AppendMenu(theSubMenu, MF_SEPARATOR, 0,                    NULL										);
	AppendMenu(theSubMenu, MF_STRING,    IDC_HELP_TRANSLATORS, GetLocalizedText(u"Translators")			);
	AppendMenu(theSubMenu, MF_STRING,    IDC_HELP_NSF,         GetLocalizedText(u"NSF")					);
	AppendMenu(theSubMenu, MF_SEPARATOR, 0,                    NULL										);
	AppendMenu(theSubMenu, MF_STRING,    IDC_HELP_ABOUT,       GetLocalizedText(u"About Curved Spaces…"));
	theSubMenu = NULL;
	
	return theMainMenu;
}


#ifdef CURVED_SPACES_MOUSE_INTERFACE

static void SteerWithMouse(
	WindowData	*wd,
	WPARAM		wParam,
	LPARAM		lParam,			//	cursor position in drawing panel coordinates
	bool		aSteeringFlag)	//	steer?  or just get set up?
{
	RECT	theWindowRect;		//	in screen coordinates
	POINT	theWindowCenter,	//	in screen coordinates
			theCursorPosition;	//	in screen coordinates
	LONG	dx,
			dy;
	RECT	theClientRect;		//	in client coordinates
	LONG	w,
			h;

	UNUSED_PARAMETER(lParam);

	if ( ! GetWindowRect(wd->ggwd.itsDrawingPanel, &theWindowRect))
		return;

	//	In most circumstances it's a good idea to use the cursor position
	//	as it was when the event occurred, but in present circumstances
	//	it's not, because if several mouse-moved events piled up on the event queue,
	//	and then when processing the first one we reset the cursor to the center
	//	of the window, then when we got to the later ones we'd be comparing
	//	an old observed cursor position to a freshly reset center-of-the-window position.
	//	To avoid such confusion, let's just read the current cursor position instead.
	//
	//	Risky code:
	//		theCursorPosition.x = (signed short int) LOWORD(lParam);
	//		theCursorPosition.y = (signed short int) HIWORD(lParam);
	//		ClientToScreen(wd->ggwd.itsDrawingPanel, &theCursorPosition);
	//
	//	Safer code:
	GetCursorPos(&theCursorPosition);

	theWindowCenter.x = (theWindowRect.right  + theWindowRect.left)/2;
	theWindowCenter.y = (theWindowRect.bottom + theWindowRect.top )/2;
	
	dx = theCursorPosition.x - theWindowCenter.x;
	dy = theCursorPosition.y - theWindowCenter.y;
	
	//	Flip top-down Win32 coordinates to bottom-up OpenGL coordinates.
	dy = - dy;

	if (dx != 0 || dy != 0)
	{
#warning Calling SetCursorPos() on a Windows 10 touchscreen computer \
screws up the mouse/touch handling.  See correspondence with Bevin Maultsby, \
regarding Torus Games as well as Curved Spaces.

		//	Caution:  Calling SetCursorPos() will generate another WM_MOUSEMOVE event!
		SetCursorPos(theWindowCenter.x, theWindowCenter.y);

		if (aSteeringFlag)
		{
			GetClientRect(wd->ggwd.itsDrawingPanel, &theClientRect);
			w = theClientRect.right - theClientRect.left;
			h = theClientRect.bottom - theClientRect.top;
			
			MouseMoved(	&wd->md,
						(DisplayPoint      ){w/2, h/2, w, h},	//	plausible default
						(DisplayPointMotion){ dx,  dy, w, h},
						wParam & MK_SHIFT,				//	shift key down?
						wParam & MK_CONTROL,			//	control key down?
						GetKeyState(VK_MENU) & 0x8000);	//	alt key down?
		}
	}
}

#endif	//	CURVED_SPACES_MOUSE_INTERFACE


#ifdef CURVED_SPACES_TOUCH_INTERFACE

static DisplayPoint GetMouseLocation(
	HWND		aDrawingWindow,
	LPARAM		lParam)			//	cursor position in drawing panel coordinates
{
	RECT			theViewRect;
	DisplayPoint	theMousePoint;

	//	Get the client rectangle size in pixels.
	GetClientRect(aDrawingWindow, &theViewRect);
	theMousePoint.itsViewWidth	= theViewRect.right - theViewRect.left;
	theMousePoint.itsViewHeight	= theViewRect.bottom - theViewRect.top;

	//	Get the mouse location in range (0,0) to (theWidth, theHeight), in pixels.
	//	Flip the vertical coordinate from Win32's top-down coordinates 
	//	to OpenGL's bottom-up coordinates.
	theMousePoint.itsX = (float) ( (signed short int)LOWORD(lParam) - theViewRect.left );
	theMousePoint.itsY = (float) (theViewRect.bottom - (signed short int)HIWORD(lParam));
	
	return theMousePoint;
}

static DisplayPointMotion GetMouseMotion(
	HWND	aDrawingWindow,
	LPARAM	lParam,			//	cursor position in drawing panel coordinates
	POINT	aPreviousCursorPosition)
{
	RECT				theViewRect;
	DisplayPointMotion	theMouseMotion;

	//	Get the client rectangle size in pixels.
	GetClientRect(aDrawingWindow, &theViewRect);
	theMouseMotion.itsViewWidth  = theViewRect.right - theViewRect.left;
	theMouseMotion.itsViewHeight = theViewRect.bottom - theViewRect.top;

	//	Compute the mouse motion, in pixels.
	//	Flip the vertical component from Win32's top-down coordinates
	//	to OpenGL's bottom-up coordinates.
	theMouseMotion.itsDeltaX = (float) ( (signed short int)LOWORD(lParam) - aPreviousCursorPosition.x );
	theMouseMotion.itsDeltaY = (float) ( aPreviousCursorPosition.y - (signed short int)HIWORD(lParam) );
	
	return theMouseMotion;
}

#endif	//	CURVED_SPACES_TOUCH_INTERFACE

