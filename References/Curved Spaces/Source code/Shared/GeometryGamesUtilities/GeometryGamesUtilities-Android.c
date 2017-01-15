//	GeometryGamesUtilities-Android.c
//
//	Implements
//
//		functions declared in GeometryGamesUtilities-Common.h that have 
//		platform-independent declarations but platform-dependent implementations
//	and
//		all functions declared in GeometryGamesUtilities-Android.h.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesUtilities-Common.h"
#include "GeometryGames-OpenGL.h"
#include "GeometryGamesUtilities-Android.h"
#include "GeometryGames-Android-JavaGlobals.h"
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <time.h>


static AAssetManager	*gAssetManager	= NULL;


static ErrorText	GetPathContents(const Char16 *aPath, unsigned int *aNumRawBytes, Byte **someRawBytes);


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
	jint		theErrorCode;
	JNIEnv		*env;
	jbyteArray	theAlphaBytesAsJavaArray;
	jbyte		*theAlphaBytesAsCArray;

	UNUSED_PARAMETER(aFirstError);
	
	//	We can access Bitmap, Canvas, Paint, etc. more easily from Java than from C,
	//	so let's call the static alphaTextureFromString() method
	//	in the class gGeometryGamesUtilitiesClass, and let it do the work.

	if (gJavaVM == 0
	 || gGeometryGamesUtilitiesClass == 0
	 || gAlphaTextureFromStringMethodID == 0)
	{
		return;	//	should never occur
	}

	theErrorCode = (*gJavaVM)->GetEnv(gJavaVM, (void **)&env, JNI_VERSION_1_6);
	if (theErrorCode != JNI_OK)
		return;

	theAlphaBytesAsJavaArray = (jbyteArray) (*env)->CallStaticObjectMethod(
									env,
									gGeometryGamesUtilitiesClass,
									gAlphaTextureFromStringMethodID,
									(*env)->NewString(env, aString, Strlen16(aString)),
									aWidthPx,
									aHeightPx,
									(*env)->NewString(env, aFontName, Strlen16(aFontName)),
									aFontSize,
									aFontDescent,
									aCenteringFlag,
									aMargin);
	if (theAlphaBytesAsJavaArray == 0)
		return;	//	should never occur

	theAlphaBytesAsCArray = (*env)->GetByteArrayElements(	env,
															theAlphaBytesAsJavaArray,
															NULL);
	if (theAlphaBytesAsCArray != NULL)
	{
		SetTextureImage(aTextureName, aWidthPx, aHeightPx, 1, (Byte *)theAlphaBytesAsCArray);

		(*env)->ReleaseByteArrayElements(	env,
											theAlphaBytesAsJavaArray,
											theAlphaBytesAsCArray,
											JNI_ABORT);	//	no changes to save
	}
}


void InitAssetManager(
	JNIEnv	*env,
	jobject	aJavaAssetManager)
{
	if (gAssetManager == NULL)
		gAssetManager = AAssetManager_fromJava(env, aJavaAssetManager);
}

ErrorText GetFileContents(
	const Char16	*aDirectory,	//	input,  zero-terminated UTF-16 string, may be NULL
	const Char16	*aFileName,		//	input,  zero-terminated UTF-16 string, may be NULL (?!)
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes
{
	unsigned int	theDirectoryNameLength,
					theFileNameLength,
					thePathBufferLength;
	Char16			*thePath;
	ErrorText		theError;

	//	The iOS, Mac OS X and Windows versions of GetFileContents()
	//	assemble an absolute path of the form
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
	//
	//	This Android version of GetFileContents() is a little different,
	//	because on Android an app's assets don't have a location in the file system.
	//	They live in the .apk file, which is like a .zip archive.
	//	So we must instead assemble a name of the form
	//
	//		<directory name>/<file name>
	//
	//	and ask gAssetManager to get the data for us.

	if (gAssetManager == NULL)
		return u"GetFileContents() was called with gAssetManager not yet initialized.";

	theDirectoryNameLength	= (aDirectory != NULL) ? Strlen16(aDirectory) : 0;
	theFileNameLength		= (aFileName  != NULL) ? Strlen16(aFileName ) : 0;
	
	//	Allow space for a possible path separator '/' and a terminating zero.
	thePathBufferLength = theDirectoryNameLength + 1 + theFileNameLength + 1;
	
	thePath = (Char16 *) GET_MEMORY(thePathBufferLength * sizeof(Char16));
	if (thePath != NULL)
	{
		thePath[0] = 0;
		
		if (aDirectory != NULL)
			Strcat16(thePath, thePathBufferLength, aDirectory);
		
		if (aDirectory != NULL && aFileName != NULL)
			Strcat16(thePath, thePathBufferLength, u"/");
		
		if (aFileName != NULL)
			Strcat16(thePath, thePathBufferLength, aFileName);
		
		theError = GetPathContents(thePath, aNumRawBytes, someRawBytes);

		FREE_MEMORY_SAFELY(thePath);
		
		return theError;
	}
	else
	{
		return u"GetFileContents() couldn't get memory for thePath.";
	}
}

static ErrorText GetPathContents(
	const Char16	*aPath,			//	input,  relative path name within assets directory, zero-terminated UTF-16 string
	unsigned int	*aNumRawBytes,	//	output, the file size in bytes
	Byte			**someRawBytes)	//	output, the file's contents as raw bytes
{
	ErrorText		theError			= NULL;
	unsigned int	theMaxNumUTF8Bytes;
	char			*aPathAsUTF8		= NULL;
	AAsset			*theFileContents	= NULL;
	Byte			*theAssetBuffer		= NULL;
	
	//	Just to be safe...
	if (aNumRawBytes == NULL || *aNumRawBytes != 0
	 || someRawBytes == NULL || *someRawBytes != NULL)
	{
		//	Return immediately.
		//	Don't jump to CleanUpGetPathContents
		//	with corrupt input parameters. 
		return u"Bad input in GetPathContents()";
	}
	
	theMaxNumUTF8Bytes	= 4 * Strlen16(aPath) + 1;	//	allow 4 bytes/character plus 1 terminating zero
	aPathAsUTF8			= (char *) GET_MEMORY(theMaxNumUTF8Bytes * sizeof(char));
	if (aPathAsUTF8 == NULL)
	{
		theError = u"GetPathContents() could not get memory to convert aPath to UTF-8.";
		goto CleanUpGetPathContents;
	}
	UTF16toUTF8(aPath, aPathAsUTF8, theMaxNumUTF8Bytes);
	
	theFileContents = AAssetManager_open(gAssetManager, aPathAsUTF8, AASSET_MODE_STREAMING);
	if (theFileContents == NULL)
	{
		theError = u"GetPathContents() could not open asset at given path.";
		goto CleanUpGetPathContents;
	}

	*aNumRawBytes	= (unsigned int)AAsset_getLength(theFileContents);
	theAssetBuffer	= (   Byte *   )AAsset_getBuffer(theFileContents);
	
	if (*aNumRawBytes == 0)	//	seems unlikely
	{
		theError = u"Asset has 0 bytes in GetPathContents().";
		goto CleanUpGetPathContents;
	}
	
	//	The documentation in android/asset_manager.h doesn't
	//	say explicitly what the status of the pointer
	//	returned by AAsset_getBuffer() is, but the implication
	//	is that it points to memory owned by the AAsset.
	//	When we're done using it, we free the AAsset (theFileContents)
	//	but we don't explicitly free the buffer (theAssetBuffer).
	
	//	Furthermore, the status of buffer (theAssetBuffer)
	//	is somewhat ambiguous.  It might simply be a pointer
	//	into the AAsset's internal structure.
	//	To keep things simple and robust,
	//	let's explicitly allocate our own buffer,
	//	copy in the data, and close the AAsset,
	//	so we're no longer dependent on the asset manager at all.
	
	*someRawBytes = (Byte *) GET_MEMORY(*aNumRawBytes);
	if (*someRawBytes == NULL)
	{
		theError = u"GetPathContents() couldn't get memory for theRawBytes.";
		goto CleanUpGetPathContents;
	}
	
	memcpy(*someRawBytes, theAssetBuffer, *aNumRawBytes);

CleanUpGetPathContents:

	if (theFileContents != NULL)
	{
		AAsset_close(theFileContents);
		theFileContents = NULL;
	}
	
	FREE_MEMORY_SAFELY(aPathAsUTF8);
	
	if (theError != NULL)
	{
		*aNumRawBytes = 0;
		FREE_MEMORY_SAFELY(*someRawBytes);
	}
	
	return theError;
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
	const Char16	*aKey)	//	ASCII
{
	UNUSED_PARAMETER(aKey);

	FatalError(u"getUserPrefBool() is currently implemented in the Java code.", u"Unimplemented function");

	//	Might also need to write SetFallbackUserPrefBool().

	return false;
}

void SetUserPrefBool(
	const Char16	*aKey,	//	ASCII
	bool			aValue)
{
	UNUSED_PARAMETER(aKey);
	UNUSED_PARAMETER(aValue);

	FatalError(u"setUserPrefBool() is currently implemented in the Java code.", u"Unimplemented function");
}

int GetUserPrefInt(
	const Char16	*aKey)	//	ASCII
{
	UNUSED_PARAMETER(aKey);

	FatalError(u"GetUserPrefInt() is currently unimplemented.  It might be easier to write it in the Java code.", u"Unimplemented function");

	//	Might also need to write SetFallbackUserPrefInt()

	return 0;
}

void SetUserPrefInt(
	const Char16	*aKey,	//	ASCII
	int				aValue)
{
	UNUSED_PARAMETER(aKey);
	UNUSED_PARAMETER(aValue);

	FatalError(u"SetUserPrefInt() is currently unimplemented.  It might be easier to write it in the Java code.", u"Unimplemented function");
}

float GetUserPrefFloat(
	const Char16	*aKey)	//	ASCII
{
	UNUSED_PARAMETER(aKey);

	FatalError(u"GetUserPrefFloat() is currently unimplemented.  It might be easier to write it in the Java code.", u"Unimplemented function");

	//	Might also need to write SetFallbackUserPrefFloat()

	return 0.0;
}

void SetUserPrefFloat(
	const Char16	*aKey,	//	ASCII
	float			aValue)
{
	UNUSED_PARAMETER(aKey);
	UNUSED_PARAMETER(aValue);

	FatalError(u"SetUserPrefFloat() is currently unimplemented.  It might be easier to write it in the Java code.", u"Unimplemented function");
}

const Char16 *GetUserPrefString(	//	returns aBuffer as a convenience to the caller
	const Char16	*aKey,
	Char16			*aBuffer,
	unsigned int	aBufferLength)
{
	//	At the moment, only Move & Turn uses GetUserPrefString().
	//	There's no Android version of Move & Turn, therefore
	//	there's no immediate need to write this function.

	FatalError(	u"Still need to write this, imitating GetUserPrefBool()",
				u"Missing code in GetUserPrefString()");

	//	Might also need to write SetFallbackUserPrefString()
	//	as in the Win32 version.

	UNUSED_PARAMETER(aKey);
	if (aBufferLength > 0)
		aBuffer[0] = 0;
	
	return aBuffer;
}

void SetUserPrefString(
	const Char16	*aKey,
	const Char16	*aString)	//	zero-terminated UTF-16 string
{
	//	At the moment, only Move & Turn uses GetUserPrefString().
	//	There's no Android version of Move & Turn, therefore
	//	there's no immediate need to write this function.
	FatalError(	u"Still need to write this, imitating SetUserPrefBool()",
				u"Missing code in SetUserPrefString()");

	UNUSED_PARAMETER(aKey);
	UNUSED_PARAMETER(aString);
}


void RandomInit(void)
{
	//	I couldn't find srandomdev() for Android,
	//	so use srand() instead.
	srand((int)time(NULL));

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

void RandomInitWithSeed(unsigned int aSeed)
{
	//	RandomInitWithSeed() is useful when one wants to be able
	//	to reliably reproduce a given series of random numbers.
	//	Otherwise it may be better to use RandomInit() instead.
	
	srand(aSeed);
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


Char16 *MakeZeroTerminatedString(	//	When done with the returned zero-terminated string,
									//		use FreeZeroTerminatedString() to free it.
	JNIEnv	*env,
	jstring	aString)
{
	const Char16	*theString;
	unsigned int	theStringLength;
	Char16			*theStringWithZeroTerminator;
	unsigned int	i;

	//	According to Oracle's JNI documentation
	//
	//		http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html
	//
	//	a jchar is an "unsigned 16 bit" type.  Just what we need!

	theString		= (*env)->GetStringChars(env, aString, 0);
	theStringLength	= (*env)->GetStringLength(env, aString);

	theStringWithZeroTerminator = (Char16 *) GET_MEMORY((theStringLength + 1) * sizeof(Char16));
	GEOMETRY_GAMES_ASSERT(theStringWithZeroTerminator != 0, "Memory allocation failure");
	
	//	Copy aStrings's characters.
	for (i = 0; i < theStringLength; i++)
		theStringWithZeroTerminator[i] = theString[i];

	//	Append a terminating zero.
	theStringWithZeroTerminator[theStringLength] = 0;

	(*env)->ReleaseStringChars(env, aString, theString);

	return theStringWithZeroTerminator;
}

void FreeZeroTerminatedString(
	Char16	**aZeroTerminatedString)
{
	FREE_MEMORY_SAFELY(*aZeroTerminatedString);
}


//	Note #1:  This method for displaying generic errors is far from ideal,
//	because the messages get fetched only at the end of onDrawFrame.
//	It would be better to display the error immediately when
//	ErrorMessage() gets called, but things get very messy 
//	when trying to call from the C code out to the Java code,
//	and then trying to locate the current renderer and itsCallbackHandler.
//	So for now let's let onDrawFrame check for errors.

//	Note #2:  FatalError() does not currently abort the program.
//	It simply calls ErrorMessage().

//	Note #3:  The current implementation of FatalError()/ErrorMessage()
//	is not thread-safe.

static Char16	gFirstErrorTitle[1024]		= {0},
				gFirstErrorMessage[1024]	= {0};

//	Use FatalError() for situations the user might conceivably encounter,
//	like an OpenGL setup or render error.  Otherwise use GeometryGamesAssertionFailed.
void FatalError(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	char	theMessage[1024],
			theTitle[1024];

#warning Still need to implement FatalError().
	ErrorMessage(aMessage, aTitle);
			
	UTF16toUTF8(aMessage, theMessage, BUFFER_LENGTH(theMessage));
	UTF16toUTF8(aTitle,   theTitle,   BUFFER_LENGTH(theTitle  ));
	__android_log_print(ANDROID_LOG_ERROR,
		"Geometry Games",
		"FatalError:  title=%s message=%s",
		aTitle   != NULL ? theTitle : "<none>",
		aMessage != NULL ? theMessage : "<none>");
}

void ErrorMessage(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
#warning The implementation of ErrorMessage() is less than ideal.
	char	theMessage[1024],
			theTitle[1024];
	
	//	Copy the message for display to the user
	//	iff no earlier message is already pending.
	if (gFirstErrorTitle[0] == 0 && gFirstErrorMessage[0] == 0)
	{
		if (aTitle != NULL)
			Strcpy16(gFirstErrorTitle, BUFFER_LENGTH(gFirstErrorTitle), aTitle);
		else
			gFirstErrorTitle[0] = 0;

		if (aMessage != NULL)
			Strcpy16(gFirstErrorMessage, BUFFER_LENGTH(gFirstErrorMessage), aMessage);
		else
			gFirstErrorMessage[0] = 0;
	}
			
	UTF16toUTF8(aMessage, theMessage, BUFFER_LENGTH(theMessage));
	UTF16toUTF8(aTitle,   theTitle,   BUFFER_LENGTH(theTitle  ));
	__android_log_print(ANDROID_LOG_ERROR,
		"Geometry Games",
		"ErrorMessage:  title=%s message=%s",
		aTitle   != NULL ? theTitle : "<none>",
		aMessage != NULL ? theMessage : "<none>");
}

void GetAndClearGenericErrorMessage(
	Char16			*anErrorMessageBuffer,		//	receives output as zero-terminated UTF-16 string
	unsigned int	anErrorMessageBufferLength)	//	in Char16's, not in bytes
{
	if (gFirstErrorTitle[0] != 0 || gFirstErrorMessage[0] != 0)
	{
		Strcpy16(anErrorMessageBuffer, anErrorMessageBufferLength, gFirstErrorTitle		);
		Strcat16(anErrorMessageBuffer, anErrorMessageBufferLength, u": "				);
		Strcat16(anErrorMessageBuffer, anErrorMessageBufferLength, gFirstErrorMessage	);
		
		gFirstErrorTitle[0]		= 0;
		gFirstErrorMessage[0]	= 0;
	}
	else
	{
		anErrorMessageBuffer[0] = 0;
	}
}
