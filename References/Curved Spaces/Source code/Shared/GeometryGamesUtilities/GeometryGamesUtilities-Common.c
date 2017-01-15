//	GeometryGamesUtilities-Common.c
//
//	Implements functions declared in GeometryGamesUtilities-Common.h 
//	that have platform-independent implementations.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesUtilities-Common.h"
#include "GeometryGamesLocalization.h"
#include <stdio.h>
#include <string.h>
#include <math.h>	//	for floor()


//	Test for memory leaks.
signed int		gMemCount		= 0;
#ifdef THREADSAFE_MEM_COUNT
pthread_mutex_t	gMemCountMutex	= PTHREAD_MUTEX_INITIALIZER;
#endif


static char	*CharacterAsUTF8String(Char16 aCharacter, char aBuffer[5]);


bool IsPowerOfTwo(unsigned int n)
{
	if (n == 0)
		return false;

	while ((n & 0x00000001) == 0)
		n >>= 1;

	return (n == 0x00000001);
}


bool UTF8toUTF16(	//	returns "true" upon success, "false" upon failure
	const char		*anInputStringUTF8,		//	zero-terminated UTF-8 string (BMP only -- code points 0x0000 - 0xFFFF)
	Char16			*anOutputBufferUTF16,	//	pre-allocated buffer for zero-terminated UTF-16 string
	unsigned int	anOutputBufferLength)	//	in Char16's, not bytes;  includes space for terminating zero
{
	const char		*r;	//	reading locating
	Char16			*w,	//	writing location
					c;	//	most recent character copied
	unsigned int	aNumCharactersAvailable,
					i;

	r						= anInputStringUTF8;
	w						= anOutputBufferUTF16;
	aNumCharactersAvailable	= anOutputBufferLength;
	
	do
	{
		if (*r & 0x80)	//	1-------    (part of multi-byte sequence)
		{
			if (*r & 0x40)	//	11------	(lead byte in multi-byte sequence)
			{
				if (*r & 0x20)	//	111-----	(at least 3 bytes in sequence)
				{
					if (*r & 0x10)	//	1111----	(at least 4 bytes in sequence)
						goto UTF8toUTF16Failed;	//	should never occur for 16-bit Unicode (BMP)

					//	leading byte is 1110xxxx
					//	so it's a three-byte sequence
					//	read the leading byte
					c = (*r << 12) & 0xF000;
					
					//	read the first follow-up byte
					r++;
					if ((*r & 0xC0) != 0x80)	//	must be 10xxxxxx
						goto UTF8toUTF16Failed;
					c |= ((*r << 6) & 0x0FC0);
					
					//	read the second follow-up byte
					r++;
					if ((*r & 0xC0) != 0x80)	//	must be 10xxxxxx
						goto UTF8toUTF16Failed;
					c |= (*r & 0x003F);
				}
				else	//	110-----	(lead byte in 2-byte sequence)
				{
					//	read the leading byte
					c = (*r << 6) & 0x07C0;
					
					//	read the follow-up byte as well
					r++;
					if ((*r & 0xC0) != 0x80)	//	must be 10xxxxxx
						goto UTF8toUTF16Failed;
					c |= (*r & 0x3F);
				}
				
				//	move on
				r++;
			}
			else	//	10------
			{
				//	error: unexpected trailing byte
				goto UTF8toUTF16Failed;
			}
		}
		else	//	0-------    (plain 7-bit character)
		{
			c = *r++;
		}

		if (aNumCharactersAvailable-- > 0)
			*w++ = c;
		else
			goto UTF8toUTF16Failed;

	} while (c);	//	keep going while copied character is nonzero
	
	return true;

UTF8toUTF16Failed:
	
	for (i = 0; i < anOutputBufferLength; i++)
		anOutputBufferUTF16[i] = 0;
	
	return false;
}

bool UTF16toUTF8(	//	returns "true" upon success, "false" upon failure
	const Char16	*anInputStringUTF16,	//	zero-terminated UTF-16 string (BMP only -- surrogate pairs not supported)
	char			*anOutputBufferUTF8,	//	pre-allocated buffer for zero-terminated UTF-8 string
	unsigned int	anOutputBufferLength)	//	in bytes;  includes space for terminating zero
{
	const Char16	*r;		//	reading locating
	char			*w,		//	writing location
					u[5];	//	UTF-8 encoding of current character
	unsigned int	aNumBytesAvailable,
					theNumNewBytes,
					i;

	r					= anInputStringUTF16;
	w					= anOutputBufferUTF8;
	aNumBytesAvailable	= anOutputBufferLength;
	
	while (*r != 0x0000)
	{
		if (*r >= 0xD800 && *r <= 0xDFFF)	//	part of a surrogate pair?
			goto UTF16toUTF8Failed;

		CharacterAsUTF8String(*r, u);

		theNumNewBytes = (unsigned int) strlen(u);
		if (aNumBytesAvailable < theNumNewBytes + 1)
			goto UTF16toUTF8Failed;
		strcpy(w, u);
		w					+= theNumNewBytes;
		aNumBytesAvailable	-= theNumNewBytes;

		r++;
	}
	
	if (aNumBytesAvailable < 1)
		goto UTF16toUTF8Failed;
	*w = 0;	//	terminating zero
	
	return true;

UTF16toUTF8Failed:
	
	for (i = 0; i < anOutputBufferLength; i++)
		anOutputBufferUTF8[i] = 0;

	return false;
}

static char *CharacterAsUTF8String(
	Char16	aCharacter,	//	input, a unicode character as UTF-16
	char	aBuffer[5])	//	output, the same character as UTF-8
{
	//	This cascade of cases allows for future UTF-32 compatibility.
	
	if ((aCharacter & 0x0000007F) == aCharacter)
	{
		//	aCharacter has length 0-7 bits.
		//	Write it directly as a single byte.
		aBuffer[0] = (char) aCharacter;
		aBuffer[1] = 0;
	}
	else
	if ((aCharacter & 0x000007FF) == aCharacter)
	{
		//	aCharacter has length 8-11 bits.
		//	Write it as a 2-byte sequence.
		aBuffer[0] = (char) (0xC0 | ((aCharacter >> 6) & 0x1F));
		aBuffer[1] = (char) (0x80 | ((aCharacter >> 0) & 0x3F));
		aBuffer[2] = 0;
	}
	else
	if ((aCharacter & 0x0000FFFF) == aCharacter)
	{
		//	aCharacter has length 12-16 bits.
		//	Write it as a 3-byte sequence.
		aBuffer[0] = (char) (0xE0 | ((aCharacter >> 12) & 0x0F));
		aBuffer[1] = (char) (0x80 | ((aCharacter >>  6) & 0x3F));
		aBuffer[2] = (char) (0x80 | ((aCharacter >>  0) & 0x3F));
		aBuffer[3] = 0;
	}
	else
	if ((aCharacter & 0x001FFFFF) == aCharacter)
	{
		//	aCharacter has length 17-21 bits.
		//	Write it as a 4-byte sequence.
		aBuffer[0] = (char) (0xF0 | ((aCharacter >> 18) & 0x07));
		aBuffer[1] = (char) (0x80 | ((aCharacter >> 12) & 0x3F));
		aBuffer[2] = (char) (0x80 | ((aCharacter >>  6) & 0x3F));
		aBuffer[3] = (char) (0x80 | ((aCharacter >>  0) & 0x3F));
		aBuffer[4] = 0;
	}
	else
	{
		//	aCharacter has length 22 or more bits,
		//	which is not valid Unicode.
		GEOMETRY_GAMES_ASSERT(0, "CharacterAsUTF8String() received an invalid Unicode character.  Unicode characters may not exceed 21 bits.");
	}
	
	return aBuffer;
}


size_t Strlen16(	//	returns number of Char16s, excluding terminating zero
	const Char16	*aString)	//	zero-terminated UTF-16 string
{
	const Char16	*theTerminatingZero;
	
	if (aString != NULL)
	{
		theTerminatingZero = aString;

		while (*theTerminatingZero != 0)
			theTerminatingZero++;
		
		return theTerminatingZero - aString;
	}
	else
	{
		return 0;
	}
}

bool Strcpy16(	//	Returns true if successful, false if aDstBuffer is too small.
	Char16			*aDstBuffer,	//	initial contents are ignored
	size_t			aDstBufferSize,	//	in Char16's, not in bytes
	const Char16	*aSrcString)	//	zero-terminated UTF-16 string
{
	size_t			theNumCharsAvailable;
	Char16			*w;	//	write location
	const Char16	*r;	//	read  location
	
	theNumCharsAvailable	= aDstBufferSize;
	w						= aDstBuffer;
	r						= aSrcString;
	
	while (theNumCharsAvailable-- > 0)
	{
		if ((*w++ = *r++) == 0)
			return true;
	}
	
	//	aDstBuffer wasn't big enough to accomodate aSrcString.
	//	Write a terminating zero into aDstBuffer's last position.
	if (aDstBufferSize >= 1)
		aDstBuffer[aDstBufferSize - 1] = 0;
	
	//	The Geometry Games don't pretend to fully support UTF-16 surrogate pairs,
	//	but for future robustness, let's make sure we didn't truncate
	//	the output string in the middle of a surrogate pair.
	if (aDstBufferSize >= 2							//	Buffer contains at least two Char16's?
	 && aDstBuffer[aDstBufferSize - 2] >= 0xD800	//	Last non-zero Char16 is
	 && aDstBuffer[aDstBufferSize - 2] <= 0xDBFF)	//		a widowed leading surrogate?
	{
		aDstBuffer[aDstBufferSize - 2] = 0;			//	Suppress widowed leading surrogate.
	}
	
	return false;
}

bool Strcat16(	//	Returns true if successful, false if aDstBuffer is too small.
	Char16			*aDstBuffer,	//	contains the first zero-terminated UTF-16 string
									//		plus empty space for the second
	size_t			aDstBufferSize,	//	in Char16's, not in bytes
	const Char16	*aSrcString)	//	the second zero-terminated UTF-16 string
{
	Char16	*theSubBuffer;
	size_t	theSubBufferSize;
	
	//	Locate aDstBuffer's terminating zero.
	theSubBuffer		= aDstBuffer;
	theSubBufferSize	= aDstBufferSize;
	while (theSubBufferSize > 0	//	Still something left to read?
		&& *theSubBuffer != 0)	//	If so, is it a non-trivial Char16?
	{
		theSubBufferSize--;		//	Move on to the next Char16.
		theSubBuffer++;			//
	}
	
	//	The above while() loop will terminate when either
	//
	//		theSubBufferSize == 0	(bad news)
	//	or
	//		*theSubBuffer == 0		(good news)
	//
	if (theSubBufferSize == 0)
		FatalError(u"Strcat16() received aDstBuffer with no terminating zero.", u"Internal Error");

	//	We've found aDstBuffer's terminating zero.
	//	Ask Strcpy16() to copy aSrcString beginning there,
	//	and return the result.
	return Strcpy16(theSubBuffer, theSubBufferSize, aSrcString);
}

bool SameString16(
	const Char16	*aStringA,
	const Char16	*aStringB)
{
	while (true)
	{
		if (*aStringA != *aStringB)
			return false;
		
		if (*aStringA == 0)
			return true;
		
		aStringA++;
		aStringB++;
	}
}


Char16 *AdjustKeyForNumber(	//	Returns aKey (for convenience).
	Char16			*aKey,	//	Zero-terminated ASCII string to be adjusted in situ
	unsigned int	aNumber)
{
	const Char16	*theSuffix = u"--";
	size_t			theLength;
	
	//	The English language uses only singular and plural forms for nouns,
	//	but Russian treats numbers ending in 2, 3 and 4 differently
	//	from those ending in 5, 6, 7, 8, 9 and 0.  (The 2,3,4 form might
	//	be a remnant of the proto-indoeuropean "dual" form.)
	//	On the other hand, Japanese uses a single form for almost all nouns,
	//	with no singular/plural distinction at all.
	//
	//	The present function takes a generic key, for example "DeleteNDrawings**",
	//	and overwrites its last two characters to give a key like "DeleteNDrawingsSG",
	//	"DeleteNDrawingsDU", "DeleteNDrawingsPL" or "DeleteNDrawingsTN", suitable
	//	for a number that requires a singular, dual, plural or transnumeral noun,
	//	respectively.
	
	if (IsCurrentLanguage(u"ru"))
	{
		switch (aNumber % 10)
		{
			case 1:
				if ( (aNumber / 10) % 10 == 1 )
					theSuffix = u"PL";	//	11, 111, 211, etc.
				else
					theSuffix = u"SG";	//	1, 21, 31, etc.
				break;
			
			case 2:
			case 3:
			case 4:
				if ( (aNumber / 10) % 10 == 1 )
					theSuffix = u"PL";	//	12, 13, 14; 112, 113, 114; etc.
				else
					theSuffix = u"DU";	//	2, 3, 4; 22, 23, 24; etc.
				break;
				
			case 0:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				theSuffix = u"PL";
				break;
		}
	}
	else
	if (IsCurrentLanguage(u"ja")
	 || IsCurrentLanguage(u"ko")	//	In Korean a plural marker is possible, but usually unnecessary.
	 || IsCurrentLanguage(u"zs")	//	In Chinese only nouns referring to people get a plural marker.
	 || IsCurrentLanguage(u"zt"))
	{
		theSuffix = u"TN";	//	"transnumeral"
	}
	else
	if (IsCurrentLanguage(u"ar"))
	{
		switch (aNumber)
		{
			//	This code implicitly assumes
			//	that "0 objects" takes the plural form in Arabic,
			//	but I haven't checked that assumption
			//	with Hamza or any other native speaker.
			
			case 1:		theSuffix = u"SG";	break;
			case 2:		theSuffix = u"DU";	break;
			default:	theSuffix = u"PL";	break;
		}
	}
	else	//	generic case
	{
		if (aNumber == 1)
			theSuffix = u"SG";
		else
			theSuffix = u"PL";
	}
	
	theLength = Strlen16(aKey);
	GEOMETRY_GAMES_ASSERT(theLength >= 2, "aKey is too small for suffix");
	
	aKey[theLength - 2] = theSuffix[0];
	aKey[theLength - 1] = theSuffix[1];
	
	return aKey;
}


ErrorText ReadImageRGBA(
	const Char16	*aTextureFileName,	//	input, zero-terminated UTF-16 string
	GreyscaleMode	aGreyscaleMode,		//	input
	ImageRGBA		**anImageRGBA)		//	output
{
	ErrorText		theErrorMessage		= NULL;
	ImageRGBA		*theImage			= NULL;
	unsigned int	theNumRawBytes		= 0;
	float			theAlphaFraction;
	Byte			theLuminance;
	unsigned int	i;
	
	static bool		theFractionsAreInitialized	= false;
	static float	theFractions[256];
	
	//	Check the output variable.
	if (anImageRGBA == NULL)
	{
		theErrorMessage = u"anImageRGBA == NULL in ReadImageRGBA()";
		goto CleanUpReadImageRGBA;
	}
	if (*anImageRGBA != NULL)
	{
		theErrorMessage = u"*anImageRGBA != NULL in ReadImageRGBA()";
		goto CleanUpReadImageRGBA;
	}

	//	Allocate and initialize an empty ImageRGBA.
	theImage = GET_MEMORY(sizeof(ImageRGBA));
	if (theImage == NULL)
		goto CleanUpReadImageRGBA;
	theImage->itsWidth		= 0;
	theImage->itsHeight		= 0;
	theImage->itsPixels		= NULL;
	theImage->itsRawBytes	= NULL;

	//	Read the texture file's bytes.
	if ((theErrorMessage = GetFileContents(u"Textures", aTextureFileName, &theNumRawBytes, &theImage->itsRawBytes)) != NULL)
		goto CleanUpReadImageRGBA;

	//	Did we get a header?
	if (theNumRawBytes < 8)
	{
		theErrorMessage = u"Texture file lacks width and/or height specification.";
		goto CleanUpReadImageRGBA;
	}

	//	Parse the image width and height as big-endian 4-byte integers.
	theImage->itsWidth  = ((unsigned int)theImage->itsRawBytes[0] << 24)
						+ ((unsigned int)theImage->itsRawBytes[1] << 16)
						+ ((unsigned int)theImage->itsRawBytes[2] <<  8)
						+ ((unsigned int)theImage->itsRawBytes[3] <<  0);
	theImage->itsHeight = ((unsigned int)theImage->itsRawBytes[4] << 24)
						+ ((unsigned int)theImage->itsRawBytes[5] << 16)
						+ ((unsigned int)theImage->itsRawBytes[6] <<  8)
						+ ((unsigned int)theImage->itsRawBytes[7] <<  0);

	//	In OpenGL ES 3, textures no longer need to have power-of-two dimensions.
	//	In OpenGL ES 2, they must have power-of-two dimensions
	//	if mipmapping and/or coordinate wrapping are desired.
	//	In any case, I plan to keep the Geometry Games textures
	//	at power-of-two dimensions for the long-term future,
	//	if only for whatever small efficiency that gains when mipmapping.
	//	This requirement could be dropped if there were ever a reason to do so.
	//
	if ( ! IsPowerOfTwo(theImage->itsWidth )
	 ||  ! IsPowerOfTwo(theImage->itsHeight) )
	{
		theErrorMessage = u"OpenGL ES 2 requires that each texture's width and height be powers of two, if mipmapping and/or coordinate wrapping are desired.";
		goto CleanUpReadImageRGBA;
	}

	//	Is the total file length correct?
	if (theNumRawBytes !=  8  +  4 * theImage->itsWidth * theImage->itsHeight)
	{
		theErrorMessage = u"Number of pixels in texture file does not match stated width and height.";
		goto CleanUpReadImageRGBA;
	}

	//	Locate the pixels.
	theImage->itsPixels = (PixelRGBA *) (theImage->itsRawBytes + 8);
	
	//	Premultiply each pixel's red, green and blue components by its alpha component.
	//
	//	Note:  The RGBA files store un-premultiplied values for greater flexibility.
	//	For example, we want to be able to edit an image's alpha mask without losing RGB values.
	if ( ! theFractionsAreInitialized )
	{
		for (i = 0; i < 256; i++)
			theFractions[i] = i / 255.0;
		theFractionsAreInitialized = true;
	}
	for (i = 0; i < theImage->itsWidth * theImage->itsHeight; i++)
	{
		if (theImage->itsPixels[i].a != 0xFF)
		{
			theAlphaFraction = theFractions[theImage->itsPixels[i].a];
			theImage->itsPixels[i].r *= theAlphaFraction;
			theImage->itsPixels[i].g *= theAlphaFraction;
			theImage->itsPixels[i].b *= theAlphaFraction;
		}
	}

	//	Convert to greyscale if desired.
	//	The formula
	//
	//		luminance = 30% red + 59% green + 11% blue
	//
	//	appears widely on the internet, but with little explanation.
	//	Presumably its origins lie in human color perception.
	if (aGreyscaleMode == GreyscaleOn)
	{
		for (i = 0; i < theImage->itsWidth * theImage->itsHeight; i++)
		{
			theLuminance = (Byte) floor(0.5 + (
				0.30 * theImage->itsPixels[i].r
			  + 0.59 * theImage->itsPixels[i].g
			  + 0.11 * theImage->itsPixels[i].b));

			theImage->itsPixels[i].r = theLuminance;
			theImage->itsPixels[i].g = theLuminance;
			theImage->itsPixels[i].b = theLuminance;
		}
	}
	
	//	Assign theImage to the output variable.
	*anImageRGBA = theImage;

CleanUpReadImageRGBA:
	
	if (theErrorMessage != NULL)
	{
		FreeImageRGBA(&theImage);
		if (anImageRGBA != NULL)
			*anImageRGBA = NULL;
	}
	
	return theErrorMessage;
}

void FreeImageRGBA(ImageRGBA **anImage)
{
	if (anImage != NULL && *anImage != NULL)
	{
		FreeFileContents(NULL, &(*anImage)->itsRawBytes);

		FREE_MEMORY_SAFELY(*anImage);
	}
}


void InvertRawImage(
	unsigned int	aWidth,
	unsigned int	aHeight,
	PixelRGBA		*aPixelBuffer)
{
	unsigned int	theLowerRow,
					theUpperRow,
					theCount;
	PixelRGBA		*theLowerPixel,
					*theUpperPixel,
					theSwapPixel;

	theLowerRow	= 0;
	theUpperRow	= aHeight - 1;

	while (theUpperRow > theLowerRow)
	{
		theLowerPixel = aPixelBuffer + theLowerRow*aWidth;
		theUpperPixel = aPixelBuffer + theUpperRow*aWidth;

		for (theCount = aWidth; theCount-- > 0; )
		{
			theSwapPixel		= *theLowerPixel;
			*theLowerPixel++	= *theUpperPixel;
			*theUpperPixel++	= theSwapPixel;
		}

		theLowerRow++;
		theUpperRow--;
	}
}


//	Package up information for the new-thread wrapper function.

#ifdef __WIN32__
#include <process.h>	//	use _beginthread() on Windows
#else
#include <pthread.h>	//	use pthread_create() on all other platforms
#endif

typedef struct
{
	ModelData	*md;
	void		(*itsStartFuction)(ModelData *);
	bool		itsArgumentsHaveBeenCopied;
} ThreadStartData;

#ifdef __WIN32__
static void __cdecl	StartFunctionWrapper(void *aParam);
#else
static void			*StartFunctionWrapper(void *aParam);
#endif

void StartNewThread(
	ModelData	*md,
	void		(*aStartFuction)(ModelData *))
{
	//	Put a wrapper around aStartFuction to isolate details
	//	like _cdecl vs. _stdcall from the rest of the program.
	
	ThreadStartData	theThreadStartData;
#ifdef __WIN32__
#else
	pthread_t		theNewThreadID;	//	We'll ignore the new thread's ID.
#endif
	
	theThreadStartData.md							= md;
	theThreadStartData.itsStartFuction				= aStartFuction;
	theThreadStartData.itsArgumentsHaveBeenCopied	= false;

#ifdef __WIN32__
	_beginthread(StartFunctionWrapper, 0, (void *)&theThreadStartData);
#else
	pthread_create(&theNewThreadID, NULL, StartFunctionWrapper, (void *)&theThreadStartData);
#endif
	
	while ( ! theThreadStartData.itsArgumentsHaveBeenCopied )
		SleepBriefly();
}

#ifdef __WIN32__	//	for _beginthread() on Windows
static void __cdecl	StartFunctionWrapper(void *aParam)
#else				//	for pthread_create() on all platforms except Windows
static void			*StartFunctionWrapper(void *aParam)
#endif
{
	ThreadStartData	theThreadStartData;
	
	//	Copy the input parameters to our own local memory.
	theThreadStartData = *(ThreadStartData *)aParam;
	
	//	Let the caller know we've made a copy, so it can return safely.
	((ThreadStartData *)aParam)->itsArgumentsHaveBeenCopied = true;
	
	//	Call itsStartFunction.
	//	Whatever calling convention the compiler prefers is fine by us.
	(*theThreadStartData.itsStartFuction)(theThreadStartData.md);

#ifdef __WIN32__
	return;
#else
	return NULL;
#endif
}


#ifdef __WIN32__
#include <Windows.h>	//	for Sleep()
#else
#include <unistd.h>		//	for usleep()
#endif

void SleepBriefly(void)
{
	//	Normally the platform-independent code doesn't sleep;
	//	it returns control to the platform-dependent UI code
	//	which manages idle time as it sees fit.  However, when
	//	the Torus Games Chess waits for its secondary thread to terminate,
	//	it needs to do something to avoid hogging CPU cycles while it waits.
	
	//	NOTE:  I'M NOT THRILLED WITH THE IDEA OF USING SleepBriefly().
	//	SOMEDAY I MAY WANT TO RE-THINK THESE QUESTIONS
	//	AND PERHAPS FIND A BETTER SOLUTION.
	
#ifdef __WIN32__
	Sleep(10);		//	10 milliseconds
#else
	usleep(10000);	//	10000 µs = 10 ms
#endif
}


void GetBevelBytes(
	Byte			aBaseColor[3],		//	{R,G,B}
	unsigned int	anImageWidthPx,		//	in pixels, not points
	unsigned int	anImageHeightPx,	//	in pixels, not points
	unsigned int	aBevelThicknessPx,	//	in pixels, not points
	unsigned int	aScaleFactor,		//	1 (normal) or 2 (double resolution)
#ifdef __APPLE__
	//	The Android code creates the whole image in one call,
	//		so it always starts with channel 0 of pixel 0.
	//
	//	The Mac and iOS versions may create the image in pieces,
	//		so they may in principle ask to to begin
	//		in the middle of a row or even in the middle of a pixel.
	//
	unsigned int	anInitialPixelCount,
	unsigned int	anInitialChannel,		//	= 0, 1, 2 or 3; for red, blue, green or alpha
	unsigned int	theNumBytesRequested,	//	typically 4 * anImageWidthPx * anImageHeightPx
#endif
	Byte			*aBuffer)				//	output as {R,G,B,A} pixels (Mac and iOS)
											//		   or {B,G,R,A} pixels (Android),
											//		with one byte per component
{
	unsigned int	theCol,
					theRow,
					theChannel,
					theNumBytesRemaining;
	Byte			theBaseColor[3];
	unsigned int	theColRev,
					theRowRev,
					t,			//	integer weighting factor, as parts in 32
					theBlend;	//	color to blend with:  black (0x00) or white (0xFF)

#ifdef __APPLE__
	theRow					= anInitialPixelCount / anImageWidthPx;
	theCol					= anInitialPixelCount % anImageWidthPx;
	theChannel				= anInitialChannel;
	theNumBytesRemaining	= theNumBytesRequested;
#else
	theRow					= 0;
	theCol					= 0;
	theChannel				= 0;
	theNumBytesRemaining	= 4 * anImageWidthPx * anImageHeightPx;
#endif

#ifdef __ANDROID__
	//	On Android, use colors {B,G,R}.
	theBaseColor[0] = aBaseColor[2];
	theBaseColor[1] = aBaseColor[1];
	theBaseColor[2] = aBaseColor[0];
#else
	//	On Mac and iOS, use colors {R,G,B}.
	theBaseColor[0] = aBaseColor[0];
	theBaseColor[1] = aBaseColor[1];
	theBaseColor[2] = aBaseColor[2];
#endif

	//	At normal resolution (theScale == 1), 
	//		rows 0 and 1 form a "transition region",
	//		while rows 2, 3, 4, … all get colored the same.
	//	At double resolution (theScale == 2),
	//		rows 0, 1, 2 and 3 form a "transition region",
	//		while rows 4, 5, 6, … all get colored the same.
	//	To handle both cases in a uniform way,
	//		multiply the row number by 2 in the normal resolution case,
	//		so rows 0 and 1 get colored the same as
	//		rows 0 and 2 of the double resolution case.

	do
	{
		GEOMETRY_GAMES_ASSERT(theRow < anImageHeightPx, "theRow >= anImageHeightPx");
		
		theRowRev = (anImageHeightPx - 1) - theRow;
		theColRev = (anImageWidthPx  - 1) - theCol;

		//	Blend t parts of black (0x00) or white (0xFF)
		//	with 32-t parts of theBaseColor.
		if (theRow >= aBevelThicknessPx && theRowRev >= aBevelThicknessPx
		 && theCol >= aBevelThicknessPx && theColRev >= aBevelThicknessPx)
		{
			//	generic center region
			t			= 0;
			theBlend	= 0x00;
		}
		else
		if (theCol >= theRow && theColRev >= theRow)
		{
			//	northern quadrant, including pixels on the diagonal
			
			if (theCol == theRow || theColRev == theRow)
			{	//	diagonal
				t			= 0;
				theBlend	= 0x00;
			}
			else
			{	//	non-diagonal
				switch (theRow * aScaleFactor)
				{
					case 0:		t = 1;	break;
					case 1:		t = 2;	break;
					case 2:		t = 4;	break;
					case 3:		t = 6;	break;
					default:	t = 8;	break;
				}
				theBlend = 0x00;
			}
		}
		else
		if (theCol >= theRowRev && theColRev >= theRowRev)
		{
			//	southern quadrant, including pixels on the diagonal

			if (theCol == theRowRev || theColRev == theRowRev)
			{	//	diagonal
				switch (theRowRev * aScaleFactor)
				{
					case 0:		t =  2;	break;
					case 1:		t =  4;	break;
					case 2:		t =  6;	break;
					case 3:		t =  9;	break;
					default:	t = 12;	break;
				}
			}
			else
			{	//	non-diagonal
				switch (theRowRev * aScaleFactor)
				{
					case 0:		t =  2;	break;
					case 1:		t =  4;	break;
					case 2:		t =  8;	break;
					case 3:		t = 12;	break;
					default:	t = 16;	break;
				}
			}
			theBlend = 0xFF;
		}
		else
		if (theCol < aBevelThicknessPx)
		{
			//	western quadrant, excluding pixels on the diagonal
			switch (theCol * aScaleFactor)
			{
				case 0:		t = 1;	break;
				case 1:		t = 2;	break;
				case 2:		t = 4;	break;
				case 3:		t = 6;	break;
				default:	t = 8;	break;
			}
			theBlend = 0xFF;
		}
		else
		if (theColRev < aBevelThicknessPx)
		{
			//	eastern quadrant, excluding pixels on the diagonal
			switch (theColRev * aScaleFactor)
			{
				case 0:		t = 1;	break;
				case 1:		t = 2;	break;
				case 2:		t = 4;	break;
				case 3:		t = 6;	break;
				default:	t = 8;	break;
			}
			theBlend = 0xFF;
		}
		else
		{
			//	should never reach this point
			t			= 32;
			theBlend	= 0x00;
		}
		
		//	Install the blended color.

		//	On the first pass through the main loop
		//	we could be starting in any row, column or channel.
		//	On subsequent passes we'll be at channel 0.
		//	Either way, set values for whatever channels remain
		//	in the current pixel...
		
		//	RGB or BGR channels
		while (theNumBytesRemaining > 0 && theChannel < 3)
		{
			*aBuffer++ = ((32 - t) * theBaseColor[theChannel]
						 +    t    * theBlend)
						>> 5;	//	same as /32, but maybe quicker

			theChannel++;
			theNumBytesRemaining--;
		}
		
		//	α channel
		if (theNumBytesRemaining > 0)
		{
			*aBuffer++ = 0xFF;	//	current implementation may ignore this value
			theNumBytesRemaining--;
		}
		
		theChannel = 0;
		theCol++;
		if (theCol == anImageWidthPx)
		{
			theCol = 0;
			theRow++;
		}

	} while (theNumBytesRemaining > 0);
}


//	Use GeometryGamesAssertionFailed for "impossible" situations that
//	the user will almost surely never encounter.  Otherwise use FatalError().
void GeometryGamesAssertionFailed(
	const char		*aPathName,		//	UTF-8, but assume file name (without path) is ASCII
	unsigned int	aLineNumber,
	const char		*aFunctionName,	//	in principle UTF-8, but assume ASCII
	const char		*aDescription)	//	in principle UTF-8, but assume ASCII
{
	const char	*theLastSeparator,
				*theCharacter,
				*theFileName;
	
	//	The file name itself (without the full path) appears
	//	just after the last '/'.
	theLastSeparator = NULL;
	for (theCharacter = aPathName; *theCharacter != 0; theCharacter++)
		if (*theCharacter == '/')
			theLastSeparator = theCharacter;
	if (theLastSeparator != NULL)	//	did we find a '/' ?
		theFileName = theLastSeparator + 1;
	else
		theFileName = aPathName;
	
	//	Assume the strings are all ASCII.
	printf("\n\nGeometry Games assertion failed\n    File:      %s\n    Line:      %d\n    Function:  %s\n    Reason:    %s\n\n\n",
		theFileName, aLineNumber, aFunctionName, aDescription);
	exit(1);
}
