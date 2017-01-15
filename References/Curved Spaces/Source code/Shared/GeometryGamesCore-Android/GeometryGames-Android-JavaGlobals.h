//	GeometryGames-Android-JavaGlobals.h
//
//	© 2013 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include <jni.h>

//	The web page
//
//		developer.android.com/training/articles/perf-jni.html
//
//	says that
//
//		"In theory you can have multiple JavaVMs per process, 
//		but Android only allows one."
//
//	so we can cache a pointer to it, for use by Android-specific C code
//	that gets called from platform-independent C code, and therefore
//	gets passed no information at all about the Java context.
//
extern JavaVM		*gJavaVM;

//	Classes and method IDs are the same for all threads,
//	so we may safely cache them.
extern jclass		gGeometryGamesUtilitiesClass;
extern jmethodID	gPlaySoundMethodID,
					gAlphaTextureFromStringMethodID;
