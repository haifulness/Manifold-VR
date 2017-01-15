//	GeometryGamesUtilities-Android.h
//
//	Declares Geometry Games functions with Android-specific declarations.
//	The file GeometryGamesUtilities-Android.c implements these functions.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include <jni.h>
#include "GeometryGames-Common.h"	//	for Char16 here


extern void		PlayTheSound(const Char16 *aRelativePath);

extern void		InitAssetManager(JNIEnv *env, jobject aJavaAssetManager);

extern Char16	*MakeZeroTerminatedString(JNIEnv *env, jstring aString);
extern void		FreeZeroTerminatedString(Char16 **aZeroTerminatedString);

extern void		GetAndClearGenericErrorMessage(Char16 *anErrorMessageBuffer, unsigned int anErrorMessageBufferLength);
