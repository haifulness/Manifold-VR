//	GeometryGamesUtilities-Win.h
//
//	Declares Geometry Games functions with Win-specific declarations.
//	The file GeometryGamesUtilities-Win.c implements these functions.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

//	To get the definition of WCHAR, include the Win32 headers.
//	Request the WIN32_LEAN_AND_MEAN headers using 16-bit UNICODE characters.
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include "GeometryGames-Common.h"	//	for Char16


extern ErrorText	GetAbsolutePath(const Char16 *aDirectory, const Char16 *aFileName, Char16 *aPathBuffer, unsigned int aPathBufferLength);
extern void			SetFallbackUserPrefBool  (const Char16 *aKey, bool  aFallbackValue);
extern void			SetFallbackUserPrefInt   (const Char16 *aKey, int   aFallbackValue);
extern void			SetFallbackUserPrefFloat (const Char16 *aKey, float aFallbackValue);
extern void			SetFallbackUserPrefString(const Char16 *aKey, const Char16 *aFallbackValue);
extern bool			ConvertEndOfLineMarkers(Char16 *aTextBuffer, unsigned int aTextBufferLength);
extern LANGID		GetWin32LangID(const Char16 aTwoLetterLanguageCode[3]);
extern BYTE			GetWin32CharSetForCurrentLanguage(void);
