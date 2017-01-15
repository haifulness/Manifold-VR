//	GeometryGamesUtilities-Mac-iOS.m
//
//	Implements
//
//		functions declared in GeometryGamesUtilities-Common.h that have 
//		the same implementation on Mac OS and iOS, but a different Win32 implementation
//	and
//		all functions declared in GeometryGamesUtilities-Mac-iOS.h.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesUtilities-Mac-iOS.h"
#import "GeometryGamesLocalization.h"

#if defined(SUPPORT_METAL) && defined(SUPPORT_OPENGL)
#import <Metal/MTLDevice.h>
#endif

#ifdef SUPPORT_OPENGL
#import "GeometryGames-OpenGL.h"
#endif

#import <mach/mach.h>

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#import <CoreText/CoreText.h>
#endif


static ErrorText	GetPathContents(NSString *aPathName, unsigned int *aNumRawBytes, Byte **someRawBytes);
static size_t		GetBevelBytesAtPosition(void *info, void *buffer, off_t position, size_t count);
static Byte			ColorComponentAsByte(float aColorComponent);
static void			ReleaseBevelInfo(void *info);


bool MetalIsAvailable(void)
{
#if defined(SUPPORT_METAL) && defined(SUPPORT_OPENGL)

	id<MTLDevice>	theDevice;
		
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED

#ifdef REQUIRE_IOS_10_OR_NEWER
	//	In iOS 8 and 9, Metal had a bug that let it execute
	//	a render command encoder and a compute command encoder
	//	in the wrong order.  Apple fixed the bug in iOS 10,
	//	so KaleidoPaint (which uses compute command encoders
	//	for flood fills) requires iOS 10 or later.
	//
	//		Technical note:  Instead of NSFoundationVersionNumber we could use
	//
	//			[[NSProcessInfo processInfo] operatingSystemVersion].majorVersion
	//
	//		for greater clarity, but checking NSFoundationVersionNumber
	//		would be more efficient if we ever need to call MetalIsAvailable()
	//		in time-critical places.
	//
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0)
#error No longer need to test NSFoundationVersionNumber at run time on iOS. \
		Can now remove the following line of code. \
		(But keep the test for theDevice != nil, of course!)
#endif
	if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max)
		//	Note:  Can use
		//		>= NSFoundationVersionNumber_iOS_10_0
		//	when that constant becomes available.
#else
	//	All the Geometry Games apps except KaleidoPaint
	//	should run fine on iOS 8 and up.
	//
	if (true)	//	iOS 8 and up is OK
#endif	//	REQUIRE_IOS_10_OR_NEWER

#endif	//	__IPHONE_OS_VERSION_MIN_REQUIRED

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_11)
#error No longer need to test NSFoundationVersionNumber at run time on Mac OS X. \
		Can now remove the following line of code. \
		(But keep the test for theDevice != nil, of course!)
#endif
	if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber10_10_3)	//	Mac OS X 10.11 or later
#endif	//	__MAC_OS_X_VERSION_MIN_REQUIRED

	{
		theDevice = MTLCreateSystemDefaultDevice();
		return (theDevice != nil);
	}
	else
	{
		return false;
	}

#elif defined(SUPPORT_METAL)

	return true;

#elif defined(SUPPORT_OPENGL)

	return false;

#else

#error Neither SUPPORT_METAL nor SUPPORT_OPENGL is defined

#endif
}

matrix_float4x4	ConvertMatrixToSIMD(double aMatrix[4][4])
{
	matrix_float4x4	theMatrix	= matrix_identity_float4x4;	//	initialize to suppress compiler warning
	unsigned int	i,
					j;
	
	//	aMatrix is stored in row-major order and acts as
	//
	//		(row vector)(aMatrix)
	//
	//	while theMatrix is stored in column-major order and acts as
	//
	//		(theMatrix)(column vector)
	//
	//	Thus we can simply copy the [i][j] element of one
	//	to the [i][j] element of the other, without transposing.
	
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			theMatrix.columns[i][j] = (float) aMatrix[i][j];

	return theMatrix;
}


#ifdef SUPPORT_OPENGL

void SetAlphaTextureFromString(
	GLuint			aTextureName,
	const Char16	*aString,		//	null-terminated UTF-16 string
	unsigned int	aWidthPx,		//	in pixels (not points);  must be a power of two
	unsigned int	aHeightPx,		//	in pixels (not points);  must be a power of two
	const Char16	*aFontName,		//	null-terminated UTF-16 string
	unsigned int	aFontSize,		//	height in pixels, excluding descent
	unsigned int	aFontDescent,	//	vertical space below baseline, in pixels
	bool			aCenteringFlag,	//	Center the text horizontally in the texture image?
	unsigned int	aMargin,		//	If aCenteringFlag == false, what horizontal inset to use?
	ErrorText		*aFirstError)
{
	ErrorText				theErrorMessage		= NULL;
	Byte					*theAlphaBytes		= NULL;
	CGContextRef			theBitmapContext	= NULL;
	CFStringRef				theFontName			= NULL;
	CTFontDescriptorRef		theFontDescriptor	= NULL;
	CTFontRef				theFont				= NULL;
	CFStringRef				theString			= NULL;
	CFDictionaryRef			theAttributes		= NULL;
	CFAttributedStringRef	theAttributedString	= NULL;
	CTLineRef				theLine				= NULL;
	CGFloat					theTextWidth		= 0.0,
							theOffset			= 0.0;

	//	Assumes desired OpenGL(ES) context is already active.

	//	OpenGL (and the underlying GPU) might 
	//	prefer textures with power-of-two dimensions.
	if ( ! IsPowerOfTwo(aWidthPx)
	 ||  ! IsPowerOfTwo(aHeightPx) )
	{
		theErrorMessage = u"String texture dimensions must be powers of two.";
		goto CleanUpSetAlphaTextureFromString;
	}

	//	Allocate temporary memory for the bitmapped image.
	//	The image will be an alpha-only image with 1 byte per pixel.
	theAlphaBytes = (Byte *) GET_MEMORY( aWidthPx * aHeightPx * 1 );
	if (theAlphaBytes == NULL)
	{
		theErrorMessage = u"Couldn't allocate theAlphaBytes.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Create a Core Graphics bitmap context.
	theBitmapContext = CGBitmapContextCreate(
							theAlphaBytes,
							aWidthPx,
							aHeightPx,
							8,
							aWidthPx * 1,
							NULL,
							(CGBitmapInfo) (kCGBitmapByteOrderDefault | kCGImageAlphaOnly) );
	if (theBitmapContext == NULL)
	{
		theErrorMessage = u"Couldn't create theBitmapContext.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Even though Core Graphics and Core Text use a consistent coordinate system
	//	with the y-coordinate running bottom-to-top, it seems that CGBitmapContextCreate()
	//	runs the rows of the buffer (theAlphaBytes) top-to-bottom.
	//	To simulate a bottom-to-top image, let's flip the coordinates.
	//	(The other alternative, if this strategy ever becomes problematic,
	//	would be to manually swap the rows of the image after all drawing is complete.)
	//
	//	Note on coordinate systems.
	//	In Mac OS, an AppKit view normally has its origin at the lower left
	//	with the vertical axis directed upwards, while
	//	in iOS, a UIKit view normally as its origin at the upper left,
	//	with the vertical axis directed downward.
	//	However, a bitmap context is an exception: in iOS as well as in Mac OS, 
	//	a bitmap context's default origin sits at the bottom left,
	//	with the vertical axis directed upwards.  
	//	So we must flip the image on both iOS and Mac OS.
	//
	CGContextScaleCTM(theBitmapContext, 1, -1);
	CGContextTranslateCTM(theBitmapContext, 0, -((float)aHeightPx));

	//	Clear the frame.
	CGContextClearRect(theBitmapContext, (CGRect) { {0, 0}, {aWidthPx, aHeightPx} } );

	//	Fill the frame with partial transparency, so we can see where we're drawing
	//	(for use during development only!).
//	CGContextSetRGBFillColor(theBitmapContext, 0.0, 0.0, 0.0, 0.25);
//	CGContextFillRect(theBitmapContext, (CGRect) { {0, 0}, {aWidth, aHeight} } );
	
	//	Set full opacity for drawing the text.
	CGContextSetRGBFillColor(theBitmapContext, 0.0, 0.0, 0.0, 1.0);
	
	//	Create a CFStringRef containing the font name.
	theFontName = CFStringCreateWithCharacters(NULL, aFontName, Strlen16(aFontName));
	if (theFontName == NULL)
	{
		theErrorMessage = u"Couldn't create theFontName.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Create a font descriptor.
	theFontDescriptor = CTFontDescriptorCreateWithNameAndSize(theFontName, 0.0);
	if (theFontDescriptor == NULL)
	{
		theErrorMessage = u"Couldn't create theFontDescriptor.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Create a font.
	//	(I'm hoping that Core Text caches fonts, so it will be fast
	//	to create and release the same font over and over.)
	theFont = CTFontCreateWithFontDescriptor(theFontDescriptor, aFontSize, NULL);
	if (theFont == NULL)
	{
		theErrorMessage = u"Couldn't create theFont.";
		goto CleanUpSetAlphaTextureFromString;
	}

	//	Create a CFStringRef containing the text to be rendered.
	theString = CFStringCreateWithCharacters(NULL, aString, Strlen16(aString));
	if (theString == NULL)
	{
		theErrorMessage = u"Couldn't create theString.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Create a CFDictionaryRef containing a single attribute, namely the font.
	theAttributes = CFDictionaryCreate(NULL,
		(const void * [1]){kCTFontAttributeName	},
		(const void * [1]){theFont				},
		1,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
	if (theAttributes == NULL)
	{
		theErrorMessage = u"Couldn't create theAttributes.";
		goto CleanUpSetAlphaTextureFromString;
	}

	//	Create a CFAttributedStringRef containing the text to be rendered.
	theAttributedString = CFAttributedStringCreate(NULL, theString, theAttributes);
	if (theAttributedString == NULL)
	{
		theErrorMessage = u"Couldn't create theAttributedString.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Create a line of text.
	theLine = CTLineCreateWithAttributedString(theAttributedString);
	if (theLine == NULL)
	{
		theErrorMessage = u"Couldn't create theLine.";
		goto CleanUpSetAlphaTextureFromString;
	}
	
	//	Center the text?  Or use a predetermined margin?
	if (aCenteringFlag)
	{
		theTextWidth = CTLineGetTypographicBounds(theLine, NULL, NULL, NULL);
		if (theTextWidth <= aWidthPx)
			theOffset = 0.5 * (aWidthPx - theTextWidth);
		else
			theOffset = 0.0;
	}
	else
	{
		theOffset = aMargin;
	}
	
	//	Draw theLine into theBitmapContext.
	CGContextSetTextPosition(theBitmapContext, theOffset, aFontDescent);
	CTLineDraw(theLine, theBitmapContext);

	//	Copy theAlphaBytes, which should now contain 
	//	an image of the desired text, into the alpha-only texture.
	SetTextureImage(aTextureName, aWidthPx, aHeightPx, 1, theAlphaBytes);

CleanUpSetAlphaTextureFromString:

	if (theLine != NULL)
		CFRelease(theLine);

	if (theAttributedString != NULL)
		CFRelease(theAttributedString);

	if (theAttributes != NULL)
		CFRelease(theAttributes);

	if (theString != NULL)
		CFRelease(theString);

	if (theFont != NULL)
		CFRelease(theFont);

	if (theFontDescriptor != NULL)
		CFRelease(theFontDescriptor);

	if (theFontName != NULL)
		CFRelease(theFontName);

	if (theBitmapContext != NULL)
		CGContextRelease(theBitmapContext);

	FREE_MEMORY_SAFELY(theAlphaBytes);

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

#endif	//	SUPPORT_OPENGL


ErrorText GetFileContents(
	const Char16	*aDirectory,	//	input,  zero-terminated UTF-16 string, may be NULL
	const Char16	*aFileName,		//	input,  zero-terminated UTF-16 string, may be NULL (?!)
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes;
									//		call FreeFileContents() when no longer needed
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
	//			for example "TorusGames-ja.txt" or "Clouds.rgba".

	NSString	*thePath;
	
	thePath = [[NSBundle mainBundle] resourcePath];
	if (aDirectory != NULL)
	{
		thePath = [[thePath
					stringByAppendingString:
						@"/"]
					stringByAppendingString:
						GetNSStringFromZeroTerminatedString(aDirectory)];
	}
	if (aFileName != NULL)
	{
		thePath = [[thePath
					stringByAppendingString:
						@"/"]
					stringByAppendingString:
						GetNSStringFromZeroTerminatedString(aFileName)];
	}
	
	return GetPathContents(thePath, aNumRawBytes, someRawBytes);
}

static ErrorText GetPathContents(
	NSString		*aPathName,		//	input,  absolute path name
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes;
									//		call FreeFileContents() when no longer needed
{
	//	Technical Note:  I had hoped that setlocale() + fopen() would
	//	let me read the file within the platform-independent part
	//	of the code, but alas on WinXP it works when aPathName 
	//	contains only ASCII characters but fails when aPathName 
	//	contains non-trivial Unicode characters, for example when
	//	I temporarily change "Textures" to "Textures日本".
	//	So I reluctantly resorted to platform-dependent code
	//	to read the file.

	NSData	*theFileContents;
	
	//	Just to be safe...
	if (aNumRawBytes == NULL || *aNumRawBytes != 0
	 || someRawBytes == NULL || *someRawBytes != NULL)
		return u"Bad input in GetPathContents()";
	
	//	Read the file.
	theFileContents = [NSData dataWithContentsOfFile:aPathName];
	if (theFileContents == nil)
		return u"Couldn't get file contents as NSData in GetPathContents()";
	
	//	Note the number of bytes.
	*aNumRawBytes = (unsigned int) [theFileContents length];
	if (*aNumRawBytes == 0)
		return u"File contains 0 bytes in GetPathContents()";

	//	Copy the data.  The caller will own this copy, and must
	//	eventually free it with a call to FreeFileContents().
	*someRawBytes = GET_MEMORY(*aNumRawBytes);
	if (*someRawBytes == NULL)
	{
		*aNumRawBytes = 0;
		return u"Couldn't allocate buffer in GetPathContents()";
	}
	[theFileContents getBytes:(void *)(*someRawBytes) length:(*aNumRawBytes)];
	
	//	Success!
	return NULL;
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
	const Char16	*aKey)
{
	//	The program's app delegate should have already 
	//	set a default value, so aKey's value should always exist.
	return [[NSUserDefaults standardUserDefaults]
			boolForKey:GetNSStringFromZeroTerminatedString(aKey)];
}

void SetUserPrefBool(
	const Char16	*aKey,
	bool			aValue)
{
	[[NSUserDefaults standardUserDefaults]
		setBool:	aValue
		forKey:		GetNSStringFromZeroTerminatedString(aKey)];
}

int GetUserPrefInt(
	const Char16	*aKey)
{
	//	The program's app delegate should have already 
	//	set a default value, so aKey's value should always exist.
	return (int) [[NSUserDefaults standardUserDefaults]
					integerForKey:GetNSStringFromZeroTerminatedString(aKey)];
}

void SetUserPrefInt(
	const Char16	*aKey,
	int				aValue)
{
	[[NSUserDefaults standardUserDefaults]
		setInteger:	aValue
		forKey:		GetNSStringFromZeroTerminatedString(aKey)];
}

float GetUserPrefFloat(
	const Char16	*aKey)
{
	//	The program's app delegate should have already 
	//	set a default value, so aKey's value should always exist.
	return [[NSUserDefaults standardUserDefaults]
			floatForKey:GetNSStringFromZeroTerminatedString(aKey)];
}

void SetUserPrefFloat(
	const Char16	*aKey,
	float			aValue)
{
	[[NSUserDefaults standardUserDefaults]
		setFloat:	aValue
		forKey:		GetNSStringFromZeroTerminatedString(aKey)];
}

const Char16 *GetUserPrefString(	//	returns aBuffer as a convenience to the caller
	const Char16	*aKey,
	Char16			*aBuffer,
	unsigned int	aBufferLength)
{
	NSString	*theValueAsNSString;
	
	//	The program's app delegate should have already 
	//	set a default value, so aKey's value should always exist.

	theValueAsNSString = [[NSUserDefaults standardUserDefaults]
		stringForKey:GetNSStringFromZeroTerminatedString(aKey)];
	
	return GetZeroTerminatedStringFromNSString(theValueAsNSString, aBuffer, aBufferLength);
}

void SetUserPrefString(
	const Char16	*aKey,
	const Char16	*aString)	//	zero-terminated UTF-16 string
{
	[[NSUserDefaults standardUserDefaults]
		setObject:	GetNSStringFromZeroTerminatedString(aString)
		forKey:		GetNSStringFromZeroTerminatedString(aKey)	];
}


void RandomInit(void)
{
	//	Be careful with srandomdev().  According to
	//
	//		http://kqueue.org/blog/2012/06/25/more-randomness-or-less/
	//
	//	srandomdev() will first try /dev/random, but if that fails
	//	it falls back to
	//
	//		struct timeval tv;
	//		unsigned long junk;
	//
	//		gettimeofday(&tv, NULL);
	//		srandom((getpid() << 16) ^ tv.tv_sec ^ tv.tv_usec ^ junk);
	//
	//	The kqueue.org article then goes on to explain
	//	why using uninitialized memory is a bad idea,
	//	and how in fact the LLVM compiler will optimize away that whole line!
	//	Presumably Apple has fixed this bug by now???
	
	srandomdev();
}

void RandomInitWithSeed(unsigned int aSeed)
{
	//	RandomInitWithSeed() is useful when one wants to be able
	//	to reliably reproduce a given series of random numbers.
	//	Otherwise it may be better to use RandomInit() instead.
	
	srandom(aSeed);
}

bool RandomBoolean(void)
{
	return (bool)( random() & 0x00000001 );
}

unsigned int RandomInteger(void)
{
	return (unsigned int)random();	//	in range 0 to RAND_MAX = 0x7FFFFFFF
}

float RandomFloat(void)
{
	return (float)( (double)random() / (double)RAND_MAX );
}


//	Functions with iOS/Mac-specific declarations.


NSString *GetPreferredLanguage(void)	//	Returns two-letter code without modifiers,
										//	for example "de", "en" or "vi".
										//	Uses an ad hoc convention for Chinese:
										//	"zs" = Mandarin in simplified  characters
										//	"zt" = Mandarin in traditional characters
{
	NSArray<NSString *>	*thePreferredLanguages;
	NSString			*thePreferredLanguage;

	//	Determine the user's default language.
	//
	//	Note #1.  preferredLocalizations determines language availability
	//	based on what localizations it finds in our program's application bundle.
	//	Thus, as a practical matter, localizing InfoPList.strings to a new language
	//	will bring that language to the system's attention.
	//	
	//	Note #2.  On Mac OS, one would expect preferredLocalizations to return 
	//	an NSArray<NSString *> containing all localizations supported by our mainBundle
	//	and acceptable to the user, listed in order of the user's preferences.
	//	In practice, however, preferredLocalizations always returns a 1-element array,
	//	even when several acceptable languages are available.  Apple's documentation 
	//	is evasive on this question, saying only that preferredLocalizations
	//
	//		Returns one or more localizations contained in the receiver’s bundle 
	//		that the receiver uses to locate resources based on the user’s preferences.
	//
	//	Fortunately preferredLocalizations reliably returns the user's most preferred
	//	localization, from among those supported.
	//
	thePreferredLanguages	= [[NSBundle mainBundle] preferredLocalizations];
	thePreferredLanguage	= ([thePreferredLanguages count] > 0 ?			//	If a preferred language exists...
								[thePreferredLanguages objectAtIndex:0] :	//	...use it.
								@"en");										//	Otherwise fall back to English.

	//	Note #3.  For Chinese, the infoPlist.strings file must
	//	contain a localization for "zh-Hans" and/or "zh-Hant".
	//	If the infoPlist.strings file contained only "zh",
	//	then -preferredLocalizations would report "en", not "zh".
	//	In general, -preferredLocalizations treats "zh-Hans" and "zh-Hant"
	//	as separate languages.  If the infoPlist.strings file contained
	//	"zh-Hans" without "zh-Hant", and the user had preferred languages
	//	{zh-Hant, en, … }, then -preferredLocalizations would report "en",
	//	not "zh-Hant".  For best results, infoPlist.strings must contain
	//	localizations for both "zh-Hans" and "zh-Hant".
	//
	//		Special language codes:  To accommodate "zh-Hans" and "zh-Hant",
	//		the Geometry Games software uses the non-standard 2-letter language codes
	//
	//			zs = zh-Hans = Mandarin Chinese in simplified characters
	//			zt = zh-Hant = Mandarin Chinese in traditional characters
	//
	if ([thePreferredLanguage hasPrefix:@"zh-Hans"])
		thePreferredLanguage = @"zs";
	if ([thePreferredLanguage hasPrefix:@"zh-Hant"])
		thePreferredLanguage = @"zt";

	//	Note #4.  -preferredLocalizations seems to automatically strip off country codes.
	//	If the app contains a "pt" localization, then whenever the user prefers
	//	either "pt-BR" or "pt-PT", the system will report plain "pt".
	//
	//	Ignore any unexpected suffixes. For example, if -preferredLocalizations
	//	reported "sr-Cyrl" or "sr-Latn", at present we'd keep only "sr".
	//	Support for alternative scripts could be added later if the need arises.
	//
	if ([thePreferredLanguage length] > 2)
		thePreferredLanguage = [thePreferredLanguage substringToIndex:2];
	
	return thePreferredLanguage;
}


NSString *GetLocalizedTextAsNSString(
	const Char16	*aKey)
{
	const Char16	*theLocalizedText;
	
	theLocalizedText = GetLocalizedText(aKey);

	return GetNSStringFromZeroTerminatedString(theLocalizedText);
}

NSString *GetNSStringFromZeroTerminatedString(
	const Char16	*anInputString)	//	zero-terminated UTF-16 string;  may be NULL
{
	if (anInputString != NULL)
		return [NSString stringWithCharacters:anInputString length:Strlen16(anInputString)];
	else
		return nil;
}

Char16 *GetZeroTerminatedStringFromNSString(	//	returns anOutputBuffer as a convenience to the caller
	NSString		*anInputString,
	Char16			*anOutputBuffer,		//	will include a terminating zero
	unsigned int	anOutputBufferLength)	//	maximum number of characters (including terminating zero),
											//		not number of bytes
{
	NSUInteger	theNumCharacters;
	
	//	Insist on enough buffer space for at least one regular character plus a terminating zero.
	GEOMETRY_GAMES_ASSERT(anOutputBufferLength >= 2, "output buffer too small");
	
	//	We hope to copy all of anInputString's characters, but...
	theNumCharacters = [anInputString length];
	
	//	... if anOutputBufferLength isn't big enough,
	//	then we'll copy only as many characters as will fit,
	//	while still leaving room for a terminating zero.
	if (theNumCharacters > anOutputBufferLength - 1)
		theNumCharacters = anOutputBufferLength - 1;
	
	//	Copy the regular characters
	[anInputString getCharacters:anOutputBuffer range:(NSRange){0, theNumCharacters}];
	
	//	Append the terminating zero.
	anOutputBuffer[theNumCharacters] = 0;
	
	//	Return anOutputBuffer as a convenience to the caller.
	return anOutputBuffer;
}


CGImageRef BevelImageCreate(
	BevelInfo *aBevelRequest)
{
	CGColorSpaceRef		theColorSpace	= NULL;
	BevelInfo			*theBevelInfo	= NULL;
	CGDataProviderRef	theDataProvider	= NULL;
	CGImageRef			theCGImage		= NULL;

	CGDataProviderDirectCallbacks	theCallbackFunctions =
									{
										0,
										NULL,
										NULL,
										&GetBevelBytesAtPosition,
										&ReleaseBevelInfo
									};

	//	aScale should be 1.0, 2.0 or 3.0.  Never a non-integer "native scale".
	if (aBevelRequest->itsScale != floor(aBevelRequest->itsScale))
		return nil;

	//	Create an RGB color space.
	theColorSpace = CGColorSpaceCreateDeviceRGB();
	if (theColorSpace == NULL)
		goto CleanUpBevelImageCreate;
		
	//	Make a copy of the BevelInfo which theDataProvider will own.
	theBevelInfo = (BevelInfo *)malloc(sizeof(BevelInfo));
	if (theBevelInfo == NULL)
		goto CleanUpBevelImageCreate;
	*theBevelInfo = *aBevelRequest;
	
	//	From this moment on, theDataProvider owns theBevelInfo.
	//	When theDataProvider gets released, 
	//	it will call ReleaseBevelInfo(theBevelInfo).
	theDataProvider = CGDataProviderCreateDirect(
		theBevelInfo,
		aBevelRequest->itsImageWidthPx * aBevelRequest->itsImageHeightPx * 4,
		&theCallbackFunctions);
	theBevelInfo = NULL;

	//	Create the image.
	theCGImage = CGImageCreate(
		aBevelRequest->itsImageWidthPx,
		aBevelRequest->itsImageHeightPx,
		8,
		32,
		aBevelRequest->itsImageWidthPx * 4,
		theColorSpace,
	//	kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault,
		kCGImageAlphaNoneSkipLast | kCGBitmapByteOrderDefault,
		theDataProvider,
		NULL,
		false,
		kCGRenderingIntentDefault);

CleanUpBevelImageCreate:

	CGDataProviderRelease(theDataProvider);	
	if (theBevelInfo != NULL)
		free(theBevelInfo);
	CGColorSpaceRelease(theColorSpace);

	return theCGImage;
}


//	GetBevelBytesAtPosition() is a wrapper function,
//	written as a CGDataProviderGetBytesAtPositionCallback.
//	It unpacks its arguments and passes them
//	to the platform-independent function GetBevelBytes().
//
static size_t GetBevelBytesAtPosition(
	void	*info,
	void	*buffer,
	off_t	position,
	size_t	count)
{
	BevelInfo		*theBevelInfo;
	Byte			*theBuffer;
	unsigned int	thePosition;
	Byte			theBaseColor[3];
	unsigned int	theScaleFactor,
					theWidth,
					theHeight,
					theBevelThickness,
					theChannel,	//	= 0, 1, 2 or 3; for red, blue, green or alpha
					thePixelCount,
					theNumBytesRequested;

	theBevelInfo	= (BevelInfo *)info;
	theBuffer		= (Byte *)buffer;
	thePosition		= (unsigned int)position;

	theBaseColor[0] = ColorComponentAsByte(theBevelInfo->itsBaseColor[0]);
	theBaseColor[1] = ColorComponentAsByte(theBevelInfo->itsBaseColor[1]);
	theBaseColor[2] = ColorComponentAsByte(theBevelInfo->itsBaseColor[2]);

	theScaleFactor = (theBevelInfo->itsScale == 2.0 ? 1 : 2);
	
	//	For brevity...
	theWidth			= theBevelInfo->itsImageWidthPx;
	theHeight			= theBevelInfo->itsImageHeightPx;
	theBevelThickness	= theBevelInfo->itsBevelWidthPx;

	//	Be prepared to start in the middle of a row
	//	or even in the middle of a pixel.
	theChannel				= (thePosition & 0x00000003);	//	0, 1, 2 or 3
	thePixelCount			= (thePosition - theChannel) / 4;
	theNumBytesRequested	= (unsigned int)count;
	
	GetBevelBytes(theBaseColor, theWidth, theHeight, theBevelThickness,
		theScaleFactor, thePixelCount, theChannel, theNumBytesRequested, theBuffer);
	
	return count;
}

static Byte ColorComponentAsByte(float aColorComponent)
{
	//	Map
	//		0.00 → 0x00
	//		0.25 → 0x40
	//		0.50 → 0x80
	//		0.75 → 0xC0
	//	but map
	//		1.00 → 0xFF not to 0x100
	//
	if (aColorComponent >= 1.0)
		return 0xFF;
	else
	if (aColorComponent >= 0.0)
		return (Byte)(0x100 * aColorComponent);
	else
		return 0x00;	//	should never occur
}

static void ReleaseBevelInfo(void *info)
{
	if (info != NULL)
		free(info);
}


void LogMemoryUsage(void)
{
	struct task_basic_info	theInfo;
	mach_msg_type_number_t	theSize = sizeof(theInfo);
	kern_return_t			theKernelError;
	
	if ((theKernelError = task_info(
			mach_task_self(), TASK_BASIC_INFO, (task_info_t)&theInfo, &theSize))
		== KERN_SUCCESS)
	{
		NSLog(@"resident memory = %u, virtual memory = %u\n",
				(unsigned int)theInfo.resident_size,
				(unsigned int)theInfo.virtual_size);
	}
	else
	{
		NSLog(@"task_info() failed with error %s\n",
				mach_error_string(theKernelError));
	}
}

