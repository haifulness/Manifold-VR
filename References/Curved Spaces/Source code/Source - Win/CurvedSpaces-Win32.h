//	CurvedSpaces-Win32.h
//
//	Definitions for Curved Spaces' Win32 user interface.
//	The user interface knows about the ModelData,
//	but doesn't know anything about OpenGL.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include "GeometryGames-Win32.h"
#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"


//	All the information needed to render a scene
//	lives in a single struct attached to the window.
typedef struct
{
	//	The GeometryGamesWindowData structure contains data
	//	that is Windows-specific but application-independent.
	//	This structure must be the WindowData's first field,
	//	so we can typecast a pointer to the ggwd into a pointer
	//	to the whole WindowsData, and vice versa.
	GeometryGamesWindowData	ggwd;
		
	//	The ModelData struct contain data that fully describes
	//	the model's mathematical details, but knows nothing
	//	about the graphics system (OpenGL or maybe someday DX12 or later).
	//	See CurvedSpaces-Common.h for details.
	ModelData				md;

#ifdef SUPPORT_OPENGL
	//	The GraphicsDataGL struct contains references
	//	to necessary OpenGL resources (shaders, textures, etc.)
	GraphicsDataGL			gd;
#endif

#ifdef CURVED_SPACES_TOUCH_INTERFACE
	//	GetMouseMotion() needs to know the previous cursor position
	//	as well as the current one.
	POINT					itsPrevCursorPosition;	//	in drawing panel coordinates
#endif

} WindowData;


//	Win32-specific global variables


//	Win32-specific global functions

//	in CurvedSpaces-Win32-WinMain.c

//	in CurvedSpaces-Win32-WndProc.c
extern void				DoFileOpen(WindowData *wd);
