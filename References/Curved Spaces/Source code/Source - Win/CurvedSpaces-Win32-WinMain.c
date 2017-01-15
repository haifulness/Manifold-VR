//	CurvedSpaces-Win32-WinMain.c
//
//	Provides WinMain() for the Win32 version of Curved Spaces.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Win32.h"
#include "GeometryGamesUtilities-Win.h"
#include "GeometryGamesLocalization.h"


int WINAPI WinMain(
	HINSTANCE	anInstance,
	HINSTANCE	aPreviousInstance,
	LPSTR		aCommandLine,
	int			aCommandShow)
{
	HWND			theWindow;
	WindowData		*wd;
	MSG				theMessage;
	IdleTimeData	theIdleTimeData;

	UNUSED_PARAMETER(anInstance);
	UNUSED_PARAMETER(aPreviousInstance);
	UNUSED_PARAMETER(aCommandLine);
	UNUSED_PARAMETER(aCommandShow);

	//	Make sure this executable hasn't gotten
	//	separated from its supporting files.
	TestSupportingFiles(PRIMARYLANGID(GetUserDefaultLangID()));
	
	//	Set fallback user preferences.
	SetFallbackUserPrefFloat(u"characteristic size iu",	0.5		);	//	in intrinsic units;  width of view when square
	SetFallbackUserPrefFloat(u"viewing distance iu",	0.25	);	//	in intrinsic units;  from bridge of nose to center of display
	SetFallbackUserPrefFloat(u"eye offset iu",			0.005	);	//	in intrinsic units;  from bridge of nose to eye

	//	Load a dictionary corresponding to the user's preferred language.
	InitLocalization(GetUserDefaultLangID());

	//	Set up the window classes.
	if ( ! RegisterGeometryGamesWindowClasses() )
		goto CleanUpAndExit;

	//	Create a window.
	theWindow = CreateGeometryGamesWindow(1, 1, 0);
	if (theWindow == NULL)
		goto CleanUpAndExit;
	wd = (WindowData *) GetWindowLongPtr(theWindow, GWLP_USERDATA);
	if (wd == NULL)
		goto CleanUpAndExit;
	DoFileOpen(wd);

	//	Process messages.
	do
	{
		//	Fetch and dispatch all available messages.
		while (PeekMessage(&theMessage, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&theMessage);
			DispatchMessage(&theMessage);
		}

		//	No more messages are available,
		//	so draw the next frame in each active animation.
		//	Note whether windows are present and whether animations are active.
		theIdleTimeData.itsFramePeriod		= MeasureFramePeriod();
		theIdleTimeData.itsKeepGoingFlag	= false;
		theIdleTimeData.itsAnimationFlag	= false;
		EnumThreadWindows(GetCurrentThreadId(), DoIdleTime, (LPARAM) &theIdleTimeData);

		//	Display the frame rate if animations are active,
		//	or "0 fps" otherwise.
		DisplayFrameRate(&theIdleTimeData);

		//	If no animations are active, sleep to avoid hogging CPU cycles.
		if ( ! theIdleTimeData.itsAnimationFlag )
			Sleep(10);

		//	If no windows remain open at all, exit the message loop.
	} while (theIdleTimeData.itsKeepGoingFlag);

CleanUpAndExit:

	//	There's not much to clean up.
	//	Even UnregisterClass() isn't really necessary, because
	//	the classes would get unregistered automatically anyhow.
	(void) UnregisterClass(MAIN_WINDOW_CLASS_NAME,    GetModuleHandle(NULL));
	(void) UnregisterClass(DRAWING_WINDOW_CLASS_NAME, GetModuleHandle(NULL));

	//	Free language-specific resources.
	SetCurrentLanguage(u"--");

	//	Test for memory leaks.
	if (gMemCount != 0)
		ErrorMessage(u"Memory allocated does not equal memory freed.\r\nPlease report this error to\r\n\twww.geometrygames.org/contact.html", u"Memory Leak");

	return 0;
}
