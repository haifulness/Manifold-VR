//	GeometryGamesUtilities-Win.c
//
//	Implements
//
//		functions declared in GeometryGamesUtilities-Common.h that have 
//		platform-independent declarations but platform-dependent implementations
//	and
//		all functions declared in GeometryGamesUtilities-Win.h.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesUtilities-Common.h"
#include "GeometryGamesUtilities-Win.h"
#include "GeometryGames-OpenGL.h"
#include "GeometryGamesLocalization.h"
#include <time.h>	//	for seeding srand()
#include <stdio.h>	//	for snprintf()


static ErrorText	GetPathContents(Char16 *aPathName, unsigned int *aNumRawBytes, Byte **someRawBytes);
static bool			GetBasePath(Char16 *aPathBuffer, unsigned int aPathBufferLength);
static void			MakeUserPrefKeyName(Char16 *aKeyNameBuffer, unsigned int aKeyNameBufferSize);


//	Functions with platform-independent declarations.


void SetAlphaTextureFromString(
	GLuint			aTextureName,
	const Char16	*aString,		//	null-terminated UTF-16 string
	unsigned int	aWidthPx,		//	must be a power of two
	unsigned int	aHeightPx,		//	must be a power of two
	const Char16	*aFontName,		//	null-terminated UTF-16 string
	unsigned int	aFontSize,		//	height in pixels, excluding descent
	unsigned int	aFontDescent,	//	vertical space below baseline, in pixels
	bool			aCenteringFlag,	//	Center the text horizontally in the texture image?
	unsigned int	aMargin,		//	If aCenteringFlag == false, what horizontal inset to use?
	ErrorText		*aFirstError)
{
	ErrorText		theErrorMessage	= NULL;
	HDC				theScreenDC		= NULL,
					theOffscreenDC	= NULL;
	HBITMAP			theBitmap		= NULL;
	LOGFONT			theLogicalFont	= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, u""};
	HFONT			theFont			= NULL;
	TEXTMETRIC		theTextMetrics;
	Byte			*theRGBABytes	= NULL;
	BITMAPINFO		theBitmapInfo	=	{
											{sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0},
											{{0,0,0,0}}
										};
	Byte			*theAlphaBytes	= NULL;
	unsigned int	i;

	//	Assumes desired OpenGL context is already active.

	if ( ! IsPowerOfTwo(aWidthPx)
	 ||  ! IsPowerOfTwo(aHeightPx) )
	{
		theErrorMessage = u"String texture dimensions must be powers of two.";
		goto CleanUpSetAlphaTextureFromString;
	}

	theScreenDC = CreateDC(u"DISPLAY", NULL, NULL, NULL);
	//	Don't panic if theScreenDC == NULL.
	//	Even without it we can still get a default monochrome bitmap.

	theOffscreenDC = CreateCompatibleDC(theScreenDC);	//	OK even if theScreenDC == NULL
	if (theOffscreenDC == NULL)
	{
		theErrorMessage = u"Couldn't create device context.";
		goto CleanUpSetAlphaTextureFromString;
	}

	//	Unfortunately SelectObject() will insist on either a monochrome bitmap
	//	or a bitmap with the same color organization as the device
	//	(see Petzold page 666), so we can't use an obvious greyscale bitmap.
	//	Normally we'll use a color bitmap (for good antialiasing), but
	//	if for any reason theScreenDC == NULL we'll settle for monochrome
	//	(no antialiasing) instead.
	if (theScreenDC != NULL)
		theBitmap = CreateCompatibleBitmap(theScreenDC,    aWidthPx, aHeightPx);	//	same pixel format as screen
	else
		theBitmap = CreateCompatibleBitmap(theOffscreenDC, aWidthPx, aHeightPx);	//	monochrome
	if (theBitmap == NULL)
	{
		theErrorMessage = u"Couldn't create bitmap.";
		goto CleanUpSetAlphaTextureFromString;
	}
	SelectObject(theOffscreenDC, theBitmap);

	SelectObject(theOffscreenDC, (HBRUSH)GetStockObject(BLACK_BRUSH));	//	makes background transparent

	theLogicalFont.lfHeight		= -aFontSize;	//	negative value indicates character height minus internal leading
	theLogicalFont.lfCharSet	= GetWin32CharSetForCurrentLanguage();
	Strcpy16(theLogicalFont.lfFaceName, LF_FACESIZE, aFontName);
	
	//	Microsoft's documentation for CreateFontIndirect says
	//
	//		The fonts for many East Asian languages have two typeface names: 
	//		an English name and a localized name. CreateFont and CreateFontIndirect 
	//		take the localized typeface name only on a system locale that 
	//		matches the language, while they take the English typeface name 
	//		on all other system locales. The best method is to try one name and, 
	//		on failure, try the other. Note that EnumFonts, EnumFontFamilies, 
	//		and EnumFontFamiliesEx return the English typeface name 
	//		if the system locale does not match the language of the font.
	//
	//	Surely by now (2011) all versions of Windows XP and up will accept 
	//	the English typeface name?  Indeed, now (2013) a newer version
	//	of Microsoft's documentation adds the sentence
	//
	//		The font mapper for CreateFont, CreateFontIndirect,
	///		and CreateFontIndirectEx recognizes both the English
	//		and the localized typeface name, regardless of locale.
	//
	theFont = CreateFontIndirect(&theLogicalFont);
	if (theFont == NULL)
	{
		theErrorMessage = u"Couldn't create theFont.";
		goto CleanUpSetAlphaTextureFromString;
	}
	SelectObject(theOffscreenDC, theFont);

	GetTextMetrics(theOffscreenDC, &theTextMetrics);

	SetTextColor(theOffscreenDC, 0x00FFFFFF);
	SetBkMode(theOffscreenDC, TRANSPARENT);
	
	//	For testing purposes only.
//	FillRect(theOffscreenDC, &(RECT){0,0,aWidthPx,aHeightPx}, (HBRUSH)GetStockObject(GRAY_BRUSH));

	//	Let's set the text alignment to TA_BOTTOM (rather than TA_BASELINE)
	//	and ignore aFontDescent.
	UNUSED_PARAMETER(aFontDescent);
	if (aCenteringFlag)
	{
		SetTextAlign(theOffscreenDC, TA_CENTER | TA_BOTTOM);
		TextOut(theOffscreenDC, aWidthPx/2, (aHeightPx + theTextMetrics.tmHeight)/2, aString, Strlen16(aString));
		//	y = (aHeightPx + theTextMetrics.tmHeight)/2 works well in crossword and word search cells.
		//	KaleidoTile's Archimedean name used to use y = aHeightPx, so I may need to review that.
	}
	else
	{
		SetTextAlign(theOffscreenDC, TA_LEFT | TA_BOTTOM);
		TextOut(theOffscreenDC, aMargin, aHeightPx, aString, Strlen16(aString));
	}

	theRGBABytes = (Byte *) GET_MEMORY( aWidthPx * aHeightPx * 4 );
	if (theRGBABytes == NULL)
	{
		theErrorMessage = u"Couldn't allocate theRGBABytes.";
		goto CleanUpSetAlphaTextureFromString;
	}

	theBitmapInfo.bmiHeader.biWidth  = aWidthPx;
	theBitmapInfo.bmiHeader.biHeight = aHeightPx;
	if (aHeightPx != (unsigned int) GetDIBits(theOffscreenDC, theBitmap, 0, aHeightPx, theRGBABytes, &theBitmapInfo, DIB_RGB_COLORS))
	{
		theErrorMessage = u"Couldn't read bitmap bits.";
		goto CleanUpSetAlphaTextureFromString;
	}

	theAlphaBytes = (Byte *) GET_MEMORY( aWidthPx * aHeightPx * 1 );
	if (theAlphaBytes == NULL)
	{
		theErrorMessage = u"Couldn't allocate theAlphaBytes.";
		goto CleanUpSetAlphaTextureFromString;
	}

	//	Use the first component (red? blue?) of each pixel in theBitmap
	//	as the alpha value for the texture.
	for (i = 0; i < aWidthPx * aHeightPx; i++)
		theAlphaBytes[i] = theRGBABytes[4*i + 0];

	SetTextureImage(aTextureName, aWidthPx, aHeightPx, 1, theAlphaBytes);

CleanUpSetAlphaTextureFromString:

	FREE_MEMORY_SAFELY(theAlphaBytes);

	FREE_MEMORY_SAFELY(theRGBABytes);

	if (theScreenDC != NULL)
		DeleteDC(theScreenDC);

	if (theOffscreenDC != NULL)		//	Delete theOffscreenDC before deleting objects selected into it.
		DeleteDC(theOffscreenDC);

	if (theBitmap != NULL)
		DeleteObject(theBitmap);
		
	if (theFont != NULL)
		DeleteObject(theFont);

	if (theErrorMessage != NULL)
	{
		//	Substitute a transparent texture for the missing one.
		SetTextureImage(aTextureName, 1, 1, 1, (Byte [1]){0x00});
		
		//	Report this error iff an error report is desired
		//	and no earlier error has occurred.
		if (aFirstError != NULL
		 && *aFirstError == NULL)
		{
			*aFirstError = theErrorMessage;
		}

		//	Let the caller push on without the desired texture...
	}
}


ErrorText GetFileContents(
	const Char16	*aDirectory,	//	input,  zero-terminated UTF-16 string, may be NULL
	const Char16	*aFileName,		//	input,  zero-terminated UTF-16 string, may be NULL (?!)
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes
{
	Char16	thePath[4096];
			
	//	Assemble an absolute path of the form
	//
	//		<base path>/<directory name>/<file name>
	//
	GetAbsolutePath(aDirectory, aFileName, thePath, BUFFER_LENGTH(thePath));
	
	//	Get the file's contents.
	return GetPathContents(thePath, aNumRawBytes, someRawBytes);
}

ErrorText GetAbsolutePath(
	const Char16	*aDirectory,		//	input,  zero-terminated UTF-16 string, may be NULL
	const Char16	*aFileName,			//	input,  zero-terminated UTF-16 string, may be NULL
	Char16			*aPathBuffer,		//	output, zero-terminated UTF-16 string
	unsigned int	aPathBufferLength)	//	output buffer size in Char16's (not bytes), including space for terminating zero
{
	//	Assemble an absolute path of the form
	//
	//		<base path>/<directory name>/<file name>
	//
	//	where
	//
	//		<base path> says where the application lives,
	//			and won't be known until runtime,
	//
	//		<directory name> specifies the directory of interest,
	//			for example "Languages", "Sounds" or "Textures".
	//
	//		<file name> specifies the particular file,
	//			for example "StarterCode-ja.txt" or "Clouds.rgba".

	if (aPathBuffer == NULL || aPathBufferLength == 0)
		return u"Internal error: missing path buffer in GetAbsolutePath()";
	
	//	Get the base path, with no final '/', but with a terminating 0.
	if ( ! GetBasePath(aPathBuffer, aPathBufferLength) )
	{
		aPathBuffer[0] = 0;
		return u"Couldn't get base path in GetAbsolutePath()";
	}

	if (aDirectory != NULL)
	{
		//	Append a path separator.
		if ( ! Strcat16(aPathBuffer, aPathBufferLength, u"/") )
			return u"Path name too long (1) in GetAbsolutePath()";

		//	Append the directory name.
		if ( ! Strcat16(aPathBuffer, aPathBufferLength, aDirectory) )
			return u"Path name too long (2) in GetAbsolutePath()";
	}

	if (aFileName != NULL)
	{
		//	Append another path separator.
		if ( ! Strcat16(aPathBuffer, aPathBufferLength, u"/") )
			return u"Path name too long (3) in GetAbsolutePath()";

		//	Append the file name.
		if ( ! Strcat16(aPathBuffer, aPathBufferLength, aFileName) )
			return u"Path name too long (4) in GetAbsolutePath()";
	}
	
	return NULL;
}

static bool GetBasePath(
	Char16			*aPathBuffer,		//	output buffer, zero-terminated UTF-16 string
	unsigned int	aPathBufferLength)	//	output buffer size in Char16's (not bytes),
										//		including space for terminating zero
{
	bool			theSuccessFlag	= false;
	unsigned int	theTerminator,
					i;

	//	Get an absolute path for our executable file.
	//	GetModuleFileName() will return the length of the string,
	//	which we interpret as the position of the terminating zero.
	//	If the true path is too long for the buffer, presumably
	//	GetModuleFileName() allows room for the terminating zero,
	//	so assume a path that just barely fits has been truncated.
	//	If GetModuleFileName() fails for any other reason, it returns 0.
	theTerminator = GetModuleFileName(NULL, aPathBuffer, aPathBufferLength);
	if (theTerminator == 0
	 || theTerminator >= aPathBufferLength - 1)
	{
		goto CleanUpGetBasePath;
	}

	//	Remove the name of executable file itself,
	//	leaving only the path to the directory,
	//	including the trailing backslash.
	while (theTerminator > 0 && aPathBuffer[theTerminator - 1] != u'\\')
		aPathBuffer[--theTerminator] = 0;
	
	//	Remove the trailing backslash.
	if (theTerminator > 0)
		aPathBuffer[--theTerminator] = 0;
	
	//	Convert the path separator from '\' to '/'.
	for (i = 0; i < theTerminator; i++)
		if (aPathBuffer[i] == u'\\')
			aPathBuffer[i] = u'/';

	//	Success!
	theSuccessFlag = true;

CleanUpGetBasePath:
	
	if ( ! theSuccessFlag )
		aPathBuffer[0] = 0;
	
	return theSuccessFlag;
}

static ErrorText GetPathContents(
	Char16			*aPathName,		//	input,  absolute path name as zero-terminated UTF-16 string
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes
{
	//	Technical Note:  I had hoped that setlocale() + fopen() would
	//	let me read the file within the platform-independent part
	//	of the code, but alas on WinXP it works when aPathName 
	//	contains only ASCII characters but fails when aPathName 
	//	contains non-trivial Unicode characters, for example when
	//	I temporarily change "Textures" to "Textures日本".
	//	So I reluctantly resorted to platform-dependent code
	//	to read the file.
	//
	//	Note:  It seems that _wfopen() might succeed where plain fopen() fails.
	//	See
	//		http://www.codeproject.com/Questions/544291/fopenplusfailedpluswhenplusjapaneseplusfolderplusp
	//	But _wfopen() is a Windows-only function, so we might as well
	//	stick with the code below.  If I ever used _wfopen() for anything,
	//	I'd need to carefully check whether it's compatible
	//	with plain fprintf()/fscanf(), or whether the wide-character
	//	equivalents fwprintf()/fwscanf() would be required.

	ErrorText	theErrorMessage	= NULL;
	HANDLE		theFile			= INVALID_HANDLE_VALUE;
	DWORD		theNumBytesRead	= 0;
	
	*aNumRawBytes	= 0;
	*someRawBytes	= NULL;

	//	Open the file.
	theFile = CreateFile(aPathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (theFile == INVALID_HANDLE_VALUE)
	{
		theErrorMessage = u"Couldn't open file in GetPathContents().";
		goto CleanUpGetPathContents;
	}

	//	Get the file size.
	*aNumRawBytes = GetFileSize(theFile, NULL);
	if (*aNumRawBytes == INVALID_FILE_SIZE)
	{
		theErrorMessage = u"Couldn't get file size in GetPathContents().";
		goto CleanUpGetPathContents;
	}

	//	Allocate a buffer to read the file contents into.
	*someRawBytes = (Byte *) GET_MEMORY(*aNumRawBytes);
	if (*someRawBytes == NULL)
	{
		theErrorMessage = u"Couldn't allocate memory for file's contents in GetPathContents().";
		goto CleanUpGetPathContents;
	}

	//	Read the file contents into the buffer.
	if (ReadFile(theFile, *someRawBytes, *aNumRawBytes, &theNumBytesRead, NULL) == 0
	 || theNumBytesRead != *aNumRawBytes)
	{
		theErrorMessage = u"Couldn't read file's contents in GetPathContents().";
		goto CleanUpGetPathContents;
	}

CleanUpGetPathContents:
	
	if (theFile != INVALID_HANDLE_VALUE)
		CloseHandle(theFile);
	
	if (theErrorMessage != NULL)
	{
		*aNumRawBytes = 0;
		FREE_MEMORY_SAFELY(*someRawBytes);
	}

	return theErrorMessage;
}

void FreeFileContents(
	unsigned int	*aNumRawBytes,	//	may be NULL
	Byte			**someRawBytes)
{
	if (aNumRawBytes != NULL)
		*aNumRawBytes = 0;

	FREE_MEMORY_SAFELY(*someRawBytes);
}


bool GetUserPrefBool(
	const Char16	*aKey)	//	zero-terminated UTF-16 string
{
	return (GetUserPrefInt(aKey) != 0);
}

void SetUserPrefBool(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	bool			aValue)
{
	SetUserPrefInt(aKey, aValue ? 1 : 0);
}

void SetFallbackUserPrefBool(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	bool			aFallbackValue)
{
	SetFallbackUserPrefInt(aKey, aFallbackValue ? 1 : 0);
}

int GetUserPrefInt(
	const Char16	*aKey)	//	zero-terminated UTF-16 string
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;
	LONG	theQueryError;
	DWORD	theValue,
			theNumBytes,
			theType;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	//	Try to read a previously saved user pref from the Windows registry.
	if (RegOpenKeyEx(HKEY_CURRENT_USER, theKeyName, 0, KEY_READ, &theRegistryKey) == ERROR_SUCCESS)
	{
		theNumBytes = sizeof(DWORD);
		theQueryError = RegQueryValueEx(theRegistryKey, aKey, NULL, &theType, (BYTE *)&theValue, &theNumBytes);
		RegCloseKey(theRegistryKey);
		if (theQueryError == ERROR_SUCCESS
		 && theType == REG_DWORD
		 && theNumBytes == sizeof(DWORD))
		{
			return (int)theValue;
		}
	}

	//	No previously saved user pref was found.
	//	Return a reasonable default value and hope for the best.
	return 0;
}

void SetUserPrefInt(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	int				aValue)
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;
	DWORD	theValue;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	if (RegCreateKeyEx(HKEY_CURRENT_USER, theKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &theRegistryKey, NULL)
		== ERROR_SUCCESS)
	{
		theValue = (DWORD)aValue;
		RegSetValueEx(theRegistryKey, aKey, 0, REG_DWORD, (BYTE *)&theValue, sizeof(DWORD));
		RegCloseKey(theRegistryKey);
	}
}

void SetFallbackUserPrefInt(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	int				aFallbackValue)
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;
	LONG	theQueryError;
	DWORD	theValue,
			theNumBytes;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	//	Try to read a previously saved user pref from the Windows registry.
	if (RegOpenKeyEx(HKEY_CURRENT_USER, theKeyName, 0, KEY_READ, &theRegistryKey) == ERROR_SUCCESS)
	{
		theNumBytes = sizeof(DWORD);
		theQueryError = RegQueryValueEx(theRegistryKey, aKey, NULL, NULL, (BYTE *)&theValue, &theNumBytes);
		RegCloseKey(theRegistryKey);
		if (theQueryError == ERROR_SUCCESS && theNumBytes == sizeof(DWORD))
		{
			//	The registry already contains a value for aKey.  Don't disturb it.
			return;
		}
	}

	//	The registry does not yet contain a value for aKey.  Set aFallbackValue.
	if (RegCreateKeyEx(HKEY_CURRENT_USER, theKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &theRegistryKey, NULL)
		== ERROR_SUCCESS)
	{
		theValue = (DWORD)aFallbackValue;
		RegSetValueEx(theRegistryKey, aKey, 0, REG_DWORD, (BYTE *)&theValue, sizeof(DWORD));
		RegCloseKey(theRegistryKey);
	}
}

float GetUserPrefFloat(
	const Char16	*aKey)	//	zero-terminated UTF-16 string
{
	Char16	theValueAsUTF16String[64];	//	zero-terminated UTF-16 string
	char	theValueAsASCIIString[64];	//	zero-terminated ASCII string
	float	theValueAsFloat;

	//	The Win32 registry doesn't support floats, so instead we store each float as a string.
	//	For example, we store the float 3.1415927 as the ASCII string "3.1415927".
	//
	GetUserPrefString(aKey, theValueAsUTF16String, BUFFER_LENGTH(theValueAsUTF16String));
	UTF16toUTF8(theValueAsUTF16String, theValueAsASCIIString, BUFFER_LENGTH(theValueAsASCIIString));
	theValueAsFloat = atof(theValueAsASCIIString);	//	returns 0.0 if theValueAsString is invalid

	return theValueAsFloat;
}

void SetUserPrefFloat(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	float			aValue)
{
	char	theValueAsASCIIString[64];	//	zero-terminated ASCII string
	Char16	theValueAsUTF16String[64];	//	zero-terminated UTF-16 string
	
	//	Please see comment in GetUserPrefFloat() above.
	snprintf(theValueAsASCIIString, sizeof(theValueAsASCIIString), "%f", aValue);
	UTF8toUTF16(theValueAsASCIIString, theValueAsUTF16String, BUFFER_LENGTH(theValueAsUTF16String));
	SetUserPrefString(aKey, theValueAsUTF16String);
}

void SetFallbackUserPrefFloat(
	const Char16	*aKey,	//	zero-terminated UTF-16 string
	float			aFallbackValue)
{
	char	theFallbackValueAsASCIIString[64];	//	zero-terminated ASCII string
	Char16	theFallbackValueAsUTF16String[64];	//	zero-terminated UTF-16 string
	
	//	Please see comment in GetUserPrefFloat() above.
	snprintf(theFallbackValueAsASCIIString, sizeof(theFallbackValueAsASCIIString), "%f", aFallbackValue);
	UTF8toUTF16(theFallbackValueAsASCIIString, theFallbackValueAsUTF16String, BUFFER_LENGTH(theFallbackValueAsUTF16String));
	SetFallbackUserPrefString(aKey, theFallbackValueAsUTF16String);
}

const Char16 *GetUserPrefString(	//	returns aBuffer as a convenience to the caller
	const Char16	*aKey,			//	zero-terminated UTF-16 string
	Char16			*aBuffer,		//	output buffer, zero-terminated UTF-16 string
	unsigned int	aBufferLength)	//	max number of characters, including a terminating zero
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;
	LONG	theQueryError;
	DWORD	theNumBytes,
			theNumCharacters,
			theType;
	
	if (aBufferLength == 0)	//	should never occur
		return NULL;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	//	Try to read a previously saved user pref from the Windows registry.
	if (RegOpenKeyEx(HKEY_CURRENT_USER, theKeyName, 0, KEY_READ, &theRegistryKey) == ERROR_SUCCESS)
	{
		theNumBytes = (DWORD)(aBufferLength * sizeof(Char16));
		theQueryError = RegQueryValueEx(theRegistryKey, aKey, NULL, &theType, (BYTE *)aBuffer, &theNumBytes);
		if (theQueryError == ERROR_SUCCESS
		 && theType == REG_SZ)
		{
			//	Microsoft's page
			//
			//		http://msdn.microsoft.com/en-us/library/windows/desktop/ms724884(v=vs.85).aspx
			//
			//	says that
			//
			//		If data has the REG_SZ, REG_MULTI_SZ, or REG_EXPAND_SZ type,
			//		the string may not have been stored with the proper terminating
			//		null characters. Therefore, when reading a string
			//		from the registry, you must ensure that the string
			//		is properly terminated before using it;
			//		otherwise, it may overwrite a buffer.
			//
			theNumCharacters = theNumBytes / sizeof(Char16);
			if (theNumCharacters > 0)
				aBuffer[theNumCharacters - 1] = 0;	//	should always be redundant
		}
		else
		{
			aBuffer[0] = 0;
		}
		RegCloseKey(theRegistryKey);
	}
	else
	{
		//	No previously saved user pref was found.
		//	Return an empty string and hope for the best.
		aBuffer[0] = 0;
	}
	
	return aBuffer;
}

void SetUserPrefString(
	const Char16	*aKey,		//	zero-terminated UTF-16 string
	const Char16	*aString)	//	zero-terminated UTF-16 string
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	if (RegCreateKeyEx(HKEY_CURRENT_USER, theKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &theRegistryKey, NULL)
		== ERROR_SUCCESS)
	{
		//	Be sure to write the terminating zero.
		RegSetValueEx(theRegistryKey, aKey, 0, REG_SZ, (BYTE *)aString, (Strlen16(aString) + 1) * sizeof(Char16));
		RegCloseKey(theRegistryKey);
	}
}

void SetFallbackUserPrefString(
	const Char16	*aKey,				//	zero-terminated UTF-16 string
	const Char16	*aFallbackValue)	//	zero-terminated UTF-16 string
{
	Char16	theKeyName[128]	= u"0";
	HKEY	theRegistryKey;
	LONG	theQueryError;
	DWORD	theNumBytes;

	MakeUserPrefKeyName(theKeyName, sizeof(theKeyName));

	//	Try to read a previously saved user pref from the Windows registry.
	if (RegOpenKeyEx(HKEY_CURRENT_USER, theKeyName, 0, KEY_READ, &theRegistryKey) == ERROR_SUCCESS)
	{
		theNumBytes = sizeof(DWORD);
		theQueryError = RegQueryValueEx(theRegistryKey, aKey, NULL, NULL, NULL, &theNumBytes);
		RegCloseKey(theRegistryKey);
		if (theQueryError == ERROR_SUCCESS && theNumBytes > 0)
		{
			//	The registry already contains a value for aKey.  Don't disturb it.
			return;
		}
	}

	//	The registry does not yet contain a value for aKey.  Set aFallbackValue.
	if (RegCreateKeyEx(HKEY_CURRENT_USER, theKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &theRegistryKey, NULL)
		== ERROR_SUCCESS)
	{
		//	Be sure to write the terminating zero.
		RegSetValueEx(theRegistryKey, aKey, 0, REG_SZ, (BYTE *)aFallbackValue, (Strlen16(aFallbackValue) + 1) * sizeof(Char16));
		RegCloseKey(theRegistryKey);
	}
}

static void MakeUserPrefKeyName(
	Char16			*aKeyNameBuffer,
	unsigned int	aKeyNameBufferSize)
{
	Strcpy16(aKeyNameBuffer, aKeyNameBufferSize, u"Software\\Geometry Games\\");
	Strcat16(aKeyNameBuffer, aKeyNameBufferSize, gLanguageFileBaseName);
}


extern void RandomInit(void)
{
	//	Initialize the random number generator.
	srand( (unsigned int) time(NULL) );

	//	If one runs the program several times within the course
	//	of, say, a half-hour, time(NULL) will of course vary from
	//	one run to the next.  However, the first random number 
	//	one subsequently gets from rand() is decidedly non-random.
	//	For example, in the Torus Games, the horizontal position 
	//	of the first jigsaw puzzle piece is always the same.  
	//	To avoid this problem, get a random number and throw it away
	//	before proceeding.
	(void) rand();	//	Discard first not-so-random number.
}

extern bool RandomBoolean(void)
{
	return (bool)( rand() & 0x00000001 );
}

extern unsigned int RandomInteger(void)
{
	return (unsigned int)rand();	//	in range 0 to RAND_MAX = 0x7FFFFFFF
}

extern float RandomFloat(void)
{
	return (float)( (double)rand() / (double)RAND_MAX );
}


void FatalError(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ErrorMessage(aMessage, aTitle);
	ExitProcess(1);
}

void ErrorMessage(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	DWORD	theTextDirection;

	if (aTitle == NULL)
		aTitle = u" ";
	if (aMessage == NULL)
	{
		aMessage	= aTitle;
		aTitle		= u" ";
	}

	theTextDirection = CurrentLanguageReadsRightToLeft() ? (MB_RTLREADING | MB_RIGHT) : 0;

	MessageBoxW(NULL, aMessage, aTitle, MB_OK | MB_TASKMODAL | theTextDirection);
}


//	Functions with Win-specific declarations.


bool ConvertEndOfLineMarkers(
	Char16			*aTextBuffer,		//	zero-terminated Char16 string
	unsigned int	aTextBufferLength)	//	length of aTextBuffer in Char16s (not bytes),
										//		including space for the terminating zero;
										//		typically greater than the space occupied
										//		by the string itself
{
	//	Convert "\n" to "\r\n" if aValueBuffer is big enough to hold it.

	Char16	*r,	//	read  position
			*w;	//	write position

	//	Set r to the terminating zero.
	//	Set w a little bit further on, to allow room to insert '\r' characters.
	r = w = aTextBuffer;
	while (*r != 0)
	{
		if (*r == u'\n')
			w++;
		r++;
		w++;
	}

	//	In theory w could wrap around past the end of the 32-bit or 64-bit address space,
	//	but this seems unlikely in practice.
	GEOMETRY_GAMES_ASSERT(w >= r, "pointer wrapped past end of address space");
	
	//	If we've got room to insert the '\r' characters...
	if (w < aTextBuffer + aTextBufferLength)
	{
		//	then read the string backwards...
		do
		{
			//	copying all characters...
			*w = *r;

			//	and inserting '\r' characters as necessary.
			if (*r == u'\n'
			 && --w >= aTextBuffer)	//	error check is logically unnecessary
			{
				*w = u'\r';
			}

		} while (--r >= aTextBuffer && --w >= aTextBuffer);
			
		return true;
	}
	else
	{
		//	Provide the most informative error message we can in the space available.
		if ( ! Strcpy16(aTextBuffer, aTextBufferLength, u"<buffer too short for \\r\\n markers>") )
		{
			if ( ! Strcpy16(aTextBuffer, aTextBufferLength, u"?") )
			{
				(void) Strcpy16(aTextBuffer, aTextBufferLength, u"");
			}
		}

		return false;
	}
}


LANGID GetWin32LangID(
	const Char16	aTwoLetterLanguageCode[3])	//	For example "en" or "ja"
												//	Special case:
												//		Chinese uses not "zh",
												//		but "zs" (simplified characters)
												//		or  "zt" (traditional characters)
{
	const Char16	*c;
	USHORT			thePrimaryLanguageID	= LANG_NEUTRAL,
					theSublanguageID		= SUBLANG_DEFAULT;	//	will get overwritten for Chinese
	
	c = aTwoLetterLanguageCode;	//	for brevity

	if (SameTwoLetterLanguageCode(c, u"ar")) thePrimaryLanguageID = LANG_ARABIC;
	if (SameTwoLetterLanguageCode(c, u"cy")) thePrimaryLanguageID = LANG_WELSH;
	if (SameTwoLetterLanguageCode(c, u"de")) thePrimaryLanguageID = LANG_GERMAN;
	if (SameTwoLetterLanguageCode(c, u"el")) thePrimaryLanguageID = LANG_GREEK;
	if (SameTwoLetterLanguageCode(c, u"en")) thePrimaryLanguageID = LANG_ENGLISH;
	if (SameTwoLetterLanguageCode(c, u"es")) thePrimaryLanguageID = LANG_SPANISH;
	if (SameTwoLetterLanguageCode(c, u"et")) thePrimaryLanguageID = LANG_ESTONIAN;
	if (SameTwoLetterLanguageCode(c, u"fi")) thePrimaryLanguageID = LANG_FINNISH;
	if (SameTwoLetterLanguageCode(c, u"fr")) thePrimaryLanguageID = LANG_FRENCH;
	if (SameTwoLetterLanguageCode(c, u"it")) thePrimaryLanguageID = LANG_ITALIAN;
	if (SameTwoLetterLanguageCode(c, u"ja")) thePrimaryLanguageID = LANG_JAPANESE;
	if (SameTwoLetterLanguageCode(c, u"ko")) thePrimaryLanguageID = LANG_KOREAN;
	if (SameTwoLetterLanguageCode(c, u"nl")) thePrimaryLanguageID = LANG_DUTCH;
	if (SameTwoLetterLanguageCode(c, u"pt")) thePrimaryLanguageID = LANG_PORTUGUESE;
	if (SameTwoLetterLanguageCode(c, u"ru")) thePrimaryLanguageID = LANG_RUSSIAN;
	if (SameTwoLetterLanguageCode(c, u"sv")) thePrimaryLanguageID = LANG_SWEDISH;
	if (SameTwoLetterLanguageCode(c, u"vi")) thePrimaryLanguageID = LANG_VIETNAMESE;
	if (SameTwoLetterLanguageCode(c, u"zh"))
		FatalError(u"Please replace generic Chinese “zh” with simplified Chinese “zs” and traditional Chinese “zt”.", u"Internal Error");
	if (SameTwoLetterLanguageCode(c, u"zs"))
	{
		thePrimaryLanguageID	= LANG_CHINESE;
		theSublanguageID		= SUBLANG_CHINESE_SIMPLIFIED;
	}
	if (SameTwoLetterLanguageCode(c, u"zt"))
	{
		thePrimaryLanguageID	= LANG_CHINESE;
		theSublanguageID		= SUBLANG_CHINESE_TRADITIONAL;
	}

	if (thePrimaryLanguageID == LANG_NEUTRAL)
		FatalError(u"GetWin32LangID() received an unexpected language code.", u"Internal Error");
	
	return MAKELANGID(thePrimaryLanguageID, theSublanguageID);
}


BYTE GetWin32CharSetForCurrentLanguage(void)
{
	//	Win32 accepts a LOGFONT structure to request a font.
	//	If the exact font isn't available, CreateFontIndirect()
	//	finds the best match it can.  The lfCharSet parameter
	//	is somewhat misleading:  it specifies the glyphs
	//	that we'd like the font to provide.  For example,
	//	lfCharSet = SHIFTJIS_CHARSET means we'd like a font
	//	that supports Japanese.  It has nothing to do with
	//	the Shift JIS chacter encoding itself.
	//	The actual character set is of course always unicode.

	if (IsCurrentLanguage(u"ar"))
		return ARABIC_CHARSET;
	else
	if (IsCurrentLanguage(u"el"))
		return GREEK_CHARSET;
	else
	if (IsCurrentLanguage(u"he"))
		return HEBREW_CHARSET;
	else
	if (IsCurrentLanguage(u"ja"))
		return SHIFTJIS_CHARSET;
	else
	if (IsCurrentLanguage(u"ko"))
		return HANGUL_CHARSET;
	else
	if (IsCurrentLanguage(u"ru"))
		return RUSSIAN_CHARSET;
	else
	if (IsCurrentLanguage(u"th"))
		return THAI_CHARSET;
	else
	if (IsCurrentLanguage(u"tr"))
		return TURKISH_CHARSET;
	else
	if (IsCurrentLanguage(u"vi"))
		return VIETNAMESE_CHARSET;
	else
	if (IsCurrentLanguage(u"zs"))
		return GB2312_CHARSET;
	else
	if (IsCurrentLanguage(u"zt"))
		return CHINESEBIG5_CHARSET;
	else
		return ANSI_CHARSET;
}
