//	GeometryGames-Android-JavaGlobals.c
//
//	© 2013 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGames-Android-JavaGlobals.h"

JavaVM		*gJavaVM						= 0;
jclass		gGeometryGamesUtilitiesClass	= 0;
jmethodID	gPlaySoundMethodID				= 0,
			gAlphaTextureFromStringMethodID	= 0;
