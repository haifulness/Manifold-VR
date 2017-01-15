#include "GeometryGamesJNI.h"
#include "GeometryGames-Android-JavaGlobals.h"
#include "GeometryGamesUtilities-Android.h"
#include "GeometryGames-Common.h"
#include "GeometryGames-OpenGL.h"
#include "GeometryGamesLocalization.h"
#include <GLES3/gl3.h>
#include <android/log.h>


//	Functions that return a jstring use code like
//
//		return (*env)->NewString(env, theString, Strlen16(theString));
//
//	which raises the question of whether the newly created jstring is retained or not.
//	If it's not retained, it would seem that it could get garbage-collected at any moment.
//	If it is retained, it would seem that after we return it would never get fully released,
//		and would therefore leak memory.
//
//	The answer seems to be that it's retained until this C function returns,
//	at which point Java automatically releases it.  More precisely, the blog
//
//		android-developers.blogspot.ie/2011/11/jni-local-reference-changes-in-ics.html
//
//	says
//
//		A local is only usable from the thread it was originally handed to,
//		and is valid until either an explicit call to DeleteLocalRef() or,
//		more commonly, until you return from your native method.
//		When a native method returns, all local references are automatically deleted.
//


//	cache_Java_objects
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_cache_1Java_1objects(
	JNIEnv	*env,
	jclass	cls)
{
	int		theErrorCode;
	jclass	theGeometryGamesUtilitiesClass;

	(void)cls;

	//	On the one hand, we can't cache the JNIEnv,
	//	because each thread will have its own.
	//
	//	On the other hand, we can cache the following items,
	//	which are the same for all threads.

	//	gJavaVM
	theErrorCode = (*env)->GetJavaVM(env, &gJavaVM);
	if (theErrorCode != JNI_OK)	//	should never occur
		gJavaVM = 0;

	//	Class(es)
	//
	//	FindClass() returns a local reference which will be released
	//	as soon as the present C function returns control to Java.
	//	To cache the Class reference, we must convert
	//	the local reference to a global reference.

	theGeometryGamesUtilitiesClass	= (*env)->FindClass(env, "org/geometrygames/geometrygamesshared/GeometryGamesUtilities");
	gGeometryGamesUtilitiesClass	= (jclass) (*env)->NewGlobalRef(env, theGeometryGamesUtilitiesClass);

	//	Method IDs
	//
	//	A jmethodID is not a jobject, but rather a pointer to an opaque struct.
	//	It doesn't do reference counting, so no global reference is needed (or even possible).
	//
	//	On the one hand, for methods like playSound that get called only occasionally,
	//	there's really no point in caching its method ID.
	//	The C code could simply call GetStaticMethodID() each time it's needed.
	//	On the other hand, if we ever need to make performance-critical calls
	//	from the C code to the Java code, we'll want cached method IDs.
	//	Having some methods IDs cached and others not cached would be ugly,
	//	so for now let's go ahead and cache all method IDs.

	gPlaySoundMethodID
		= (*env)->GetStaticMethodID(
				env,
				gGeometryGamesUtilitiesClass,
				"playSound",
				"(Ljava/lang/String;)V");

	gAlphaTextureFromStringMethodID
		= (*env)->GetStaticMethodID(
				env,
				gGeometryGamesUtilitiesClass,
				"alphaTextureFromString",
				"(Ljava/lang/String;IILjava/lang/String;IIZI)[B");
}


//	init_asset_manager
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_init_1asset_1manager(
	JNIEnv	*env,
	jclass	cls,
	jobject	anAssetManager)
{
	(void)cls;

	InitAssetManager(env, anAssetManager);
}


//	set_current_language
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_set_1current_1language(
	JNIEnv	*env,
	jclass	cls,
	jstring	aLanguage)	//	for example en_US or zh_CN (but not zh_Hans!)
{
	Char16			*theLanguage,
					theTwoLetterCode[3];
	bool			theLanguageIsSupported;
	unsigned int	i;

	(void)cls;


	theLanguage = MakeZeroTerminatedString(env, aLanguage);

	if (theLanguage[0] == 0 || theLanguage[1] == 0)	//	should never occur
		return;

	theTwoLetterCode[0] = theLanguage[0];
	theTwoLetterCode[1] = theLanguage[1];
	theTwoLetterCode[2] = 0;

	//	Special case for Chinese, where we want to distinguish
	//	zh-Hans (which Android reports as zh_CN) from
	//	zh-Hant (which Android reports as zh_TW).
	if (theLanguage[0] == u'z'
	 && theLanguage[1] == u'h'
	 && theLanguage[2] == u'_'
	 && theLanguage[3] !=  0
	 && theLanguage[4] !=  0)
	{
		if ((theLanguage[3] == u'C' && theLanguage[4] == u'N')	//	zh_CN
		 || (theLanguage[3] == u'S' && theLanguage[4] == u'G'))	//	zh_SG
		{
			theTwoLetterCode[1] = u's';	//	converts "zh" -> "zs" (nonstandard!)
		}
		else
		if ((theLanguage[3] == u'T' && theLanguage[4] == u'W')	//	zh_TW
		 || (theLanguage[3] == u'H' && theLanguage[4] == u'K'))	//	zh_HK
		{
			theTwoLetterCode[1] = u't';	//	converts "zh" -> "zt" (nonstandard!)
		}
		else
		{
			//	unknown Chinese region
			theTwoLetterCode[1] = u's';	//	converts "zh" -> "zs" (nonstandard!)
		}
	}

	FreeZeroTerminatedString(&theLanguage);

	//	Does the app support the requested language?
	theLanguageIsSupported = false;
	for (i = 0; i < gNumLanguages; i++)
	{
		if (theTwoLetterCode[0] == gLanguages[i][0]
		 && theTwoLetterCode[1] == gLanguages[i][1])
		{
			theLanguageIsSupported = true;
		}
	}

	//	Set the requested language if it's supported.
	if (theLanguageIsSupported)
	{
		SetCurrentLanguage(theTwoLetterCode);
	}
	else
	if ( ! IsCurrentLanguage(u"--") )
	{
		//	Keep the previously set language.
	}
	else
	{
		//	Fall back to English.
		SetCurrentLanguage(u"en");
	}
}

//	get_num_languages
JNIEXPORT jint JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1num_1languages(
	JNIEnv	*env,
	jclass	cls)
{
	(void)env;
	(void)cls;

	return gNumLanguages;
}

//	get_language_code
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1language_1code(
	JNIEnv	*env,
	jclass	cls,
	jint	anIndex)
{
	(void)cls;

	GEOMETRY_GAMES_ASSERT(	(unsigned int)anIndex < gNumLanguages,
							"get_language_code() received an invalid index.");

	return (*env)->NewString(env, gLanguages[anIndex], Strlen16(gLanguages[anIndex]));
}

//	get_language_endonym
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1language_1endonym(
	JNIEnv	*env,
	jclass	cls,
	jint	anIndex)
{
	const Char16	*theEndonym;

	(void)cls;

	GEOMETRY_GAMES_ASSERT(	(unsigned int)anIndex < gNumLanguages,
							"get_language_endonym() received an invalid index.");

	theEndonym = GetEndonym(gLanguages[anIndex]);

	return (*env)->NewString(env, theEndonym, Strlen16(theEndonym));
}

//	is_current_language_index
JNIEXPORT jboolean JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_is_1current_1language_1index(
	JNIEnv	*env,
	jclass	cls,
	jint	anIndex)
{
	(void)env;
	(void)cls;

	GEOMETRY_GAMES_ASSERT(	(unsigned int)anIndex < gNumLanguages,
							"is_current_language_index() received an invalid index.");

	return (jboolean) IsCurrentLanguage(gLanguages[anIndex]);
}

//	is_current_language_code
JNIEXPORT jboolean JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_is_1current_1language_1code(
	JNIEnv	*env,
	jclass	cls,
	jstring	aTwoLetterLanguageCode)
{
	Char16		*theTwoLetterLanguageCode;
	jboolean	theLanguageIsCurrent;

	(void)cls;

	theTwoLetterLanguageCode	= MakeZeroTerminatedString(env, aTwoLetterLanguageCode);
	theLanguageIsCurrent		= (jboolean) IsCurrentLanguage(theTwoLetterLanguageCode);
	FreeZeroTerminatedString(&theTwoLetterLanguageCode);

	return theLanguageIsCurrent;
}

//	get_current_two_letter_language_code
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1current_1two_1letter_1language_1code(
	JNIEnv	*env,
	jclass	cls)
{
	(void)cls;

	return (*env)->NewString(env, GetCurrentLanguage(), Strlen16(GetCurrentLanguage()));
}

//	current_language_reads_left_to_right
JNIEXPORT jboolean JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_current_1language_1reads_1left_1to_1right(
		JNIEnv	*env,
		jclass	cls)
{
	(void)env;
	(void)cls;

	return (jboolean) CurrentLanguageReadsLeftToRight();
}

//	current_language_reads_right_to_left
JNIEXPORT jboolean JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_current_1language_1reads_1right_1to_1left(
		JNIEnv	*env,
		jclass	cls)
{
	(void)env;
	(void)cls;

	return (jboolean) CurrentLanguageReadsRightToLeft();
}

//	get_localized_text_as_java_string
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1localized_1text_1as_1java_1string(
	JNIEnv	*env,
	jclass	cls,
	jstring	aKey)
{
	Char16			*theKeyWithZeroTerminator;
	const Char16	*theValue;

	(void)cls;

	theKeyWithZeroTerminator	= MakeZeroTerminatedString(env, aKey);
	theValue					= GetLocalizedText(theKeyWithZeroTerminator);
	FreeZeroTerminatedString(&theKeyWithZeroTerminator);

	return (*env)->NewString(env, theValue, Strlen16(theValue));
}

//	adjust_key_for_number
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_adjust_1key_1for_1number(
	JNIEnv	*env,
	jclass	cls,
	jstring	aKey,
	jint	aNumber)	//	must be non-negative
{
	Char16			*theNegativeNumberErrorMessage	= u"Internal error:  adjust_key_for_number() received a negative number";
	unsigned int	theStringLength;
	Char16			*theRawKeyWithZeroTerminator,
					*theModifiedKeyWithZeroTerminator;
	jstring			theModifiedKeyAsJstring;

	(void)cls;

	if (aNumber < 0)
		(*env)->NewString(env, theNegativeNumberErrorMessage, Strlen16(theNegativeNumberErrorMessage));

	theStringLength = (*env)->GetStringLength(env, aKey);

	theRawKeyWithZeroTerminator	= MakeZeroTerminatedString(env, aKey);
	theModifiedKeyWithZeroTerminator = (Char16 *) GET_MEMORY((theStringLength + 1) * sizeof(Char16));
	Strcpy16(theModifiedKeyWithZeroTerminator, theStringLength + 1, theRawKeyWithZeroTerminator);
	FreeZeroTerminatedString(&theRawKeyWithZeroTerminator);

	AdjustKeyForNumber(theModifiedKeyWithZeroTerminator, aNumber);

	theModifiedKeyAsJstring = (*env)->NewString(env, theModifiedKeyWithZeroTerminator, Strlen16(theModifiedKeyWithZeroTerminator));

	FREE_MEMORY_SAFELY(theModifiedKeyWithZeroTerminator);

	return theModifiedKeyAsJstring;
}


//	alloc_model_data
JNIEXPORT jlong JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_alloc_1model_1data(
	JNIEnv	*env,
	jclass	cls)
{
	jlong	theModelDataPtr;	//	64-bit integer to hold C pointer

	(void)env;
	(void)cls;

	theModelDataPtr = (jlong) (uintptr_t) malloc(SizeOfModelData());

	return theModelDataPtr;
}

//	free_model_data
JNIEXPORT jlong JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_free_1model_1data(
	JNIEnv	*env,
	jclass	cls,
	jlong	mdAsLong)
{
	(void)env;
	(void)cls;

	free( (ModelData *) (uintptr_t) mdAsLong);

	return 0;	//	Return 0 as a convenience, so caller may clear its variable.
}

//	set_up_model_data
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_set_1up_1model_1data(
		JNIEnv	*env,
		jclass	cls,
		jlong	mdAsLong)
{
	(void)env;
	(void)cls;

	SetUpModelData( (ModelData *) (uintptr_t) mdAsLong );
}

//	shut_down_model_data
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_shut_1down_1model_1data(
		JNIEnv	*env,
		jclass	cls,
		jlong	mdAsLong)
{
	(void)env;
	(void)cls;

	ShutDownModelData( (ModelData *) (uintptr_t) mdAsLong );
}


//	alloc_graphics_data_gl
JNIEXPORT jlong JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_alloc_1graphics_1data_1gl(
		JNIEnv	*env,
		jclass	cls)
{
	jlong	theGraphicsDataGLPtr;	//	64-bit integer to hold C pointer

	(void)env;
	(void)cls;

	theGraphicsDataGLPtr = (jlong) (uintptr_t) malloc(SizeOfGraphicsDataGL());

	return theGraphicsDataGLPtr;
}

//	free_graphics_data_gl
JNIEXPORT jlong JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_free_1graphics_1data_1gl(
		JNIEnv	*env,
		jclass	cls,
		jlong	gdAsLong)
{
	(void)env;
	(void)cls;

	free( (GraphicsDataGL *) (uintptr_t) gdAsLong);

	return 0;	//	Return 0 as a convenience, so caller may clear its variable.
}

//	zero_graphics_data_gl
JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_zero_1graphics_1data_1gl(
		JNIEnv	*env,
		jclass	cls,
		jlong	gdAsLong)
{
	(void)env;
	(void)cls;

	ZeroGraphicsDataGL( (GraphicsDataGL *) (uintptr_t) gdAsLong );
}

//	on_draw_frame
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_on_1draw_1frame(
	JNIEnv	*env,
	jclass	cls,
	jlong	mdAsLong,
	jlong	gdAsLong,
	jint	width,	//	in true pixels (not density-independent pixels)
	jint	height,	//	in true pixels (not density-independent pixels)
	jdouble	time)	//	time since system boot, in seconds
{
	ModelData		*md;
	GraphicsDataGL	*gd;
	double			theElapsedTime;
	ErrorText		theSetUpError				= NULL,
					theRenderError				= NULL,
					theErrorPrefix				= NULL,
					theErrorMessage				= NULL;
	Char16			*thePrefixedErrorMessage	= NULL;
	unsigned int	theBufferLength				= 0;
	jstring			theErrorAsJString;

	static jdouble	thePreviousFrameTime = 0.0;

	(void)env;
	(void)cls;

	if (mdAsLong == 0)	//	should never occur
	{
		theErrorPrefix	= u"internal error: ";
		theErrorMessage	= u"on_draw_frame received mdAsLong == 0";
		goto CleanUpOnDrawFrame;
	}
	if (gdAsLong == 0)	//	should never occur
	{
		theErrorPrefix	= u"internal error: ";
		theErrorMessage	= u"on_draw_frame received gdAsLong == 0";
		goto CleanUpOnDrawFrame;
	}

	md = (  ModelData *   ) (uintptr_t) mdAsLong;
	gd = (GraphicsDataGL *) (uintptr_t) gdAsLong;

	if (thePreviousFrameTime == 0.0)
		thePreviousFrameTime = time;

	//	Note:  There's no need to manually throttle the frame rate.
	//	The GLSurfaceView will call on_draw_frame() at most 60 times per seconds,
	//	no matter how fast we render the scene.
	theElapsedTime			= time - thePreviousFrameTime;
	thePreviousFrameTime	= time;

	//	<AppName>Renderer.onDrawFrame() might call on_draw_frame() because either
	//
	//		1.	a continuous animation is running,
	//		2.	the platform-independent code requested a one-time redraw
	//				in response to some user interaction, or
	//		3.	the operating system requested a redraw,
	//				perhaps because the app had been hidden.
	//
	//	The existence of Possibility #3 means that we must redraw the scene now.
	//	There's no need to check
	//
	//		SimulationWantsUpdates(md)
	//
	//	In other words, while
	//
	//		the iOS, Mac and Windows versions of the app keep a timer running constantly,
	//			and call SimulationWantsUpdates() to decide whether a redraw is needed,
	//
	//		this Android version of the app relies on the user interface
	//			to turn the animation on and off, and to request single-frame
	//			redraws as necessary.
	//
	SimulationUpdate(md, theElapsedTime);

	theSetUpError = SetUpGraphicsAsNeeded(md, gd);
	if (theSetUpError == NULL)
	{
		theRenderError = Render(md, gd, width, height, NULL);
		if (theRenderError != NULL)
		{
			theErrorPrefix	= u"render error: ";
			theErrorMessage	= theRenderError;
			glClearColor(0.0, 0.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}
	else
	{
		theErrorPrefix	= u"set-up error: ";
		theErrorMessage	= theSetUpError;
		glClearColor(1.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

CleanUpOnDrawFrame:
	if (theErrorPrefix == NULL && theErrorMessage == NULL)
	{
		//	I'd like to return NULL here, but it's not 100% obvious that
		//	the C value NULL ( = 0 ) will map to Java's null object.
		//	Probably it would, but I've found no documentation saying so.
		//	In fact, one person wrote in an online forum that returning 0
		//	slowed his app down horribly, perhaps because some sort
		//	of exception processing was going on.
		//	To be safe, let's instead return an empty string here
		//	to indicate that no error occurred.
		theErrorAsJString = (*env)->NewStringUTF(env, "");
	}
	else
	{
		if (theErrorPrefix  == NULL)	//	should never occur
			theErrorPrefix  = u"<missing prefix>: ";
		if (theErrorMessage == NULL)	//	should never occur
			theErrorMessage = u"<missing message>";

		theBufferLength = Strlen16(theErrorPrefix) + Strlen16(theErrorMessage) + 1;
		thePrefixedErrorMessage = (Char16 *)GET_MEMORY(theBufferLength * sizeof(Char16));
		GEOMETRY_GAMES_ASSERT(thePrefixedErrorMessage != NULL, "Couldn't get memory to report on_draw_frame() error");

		Strcpy16(thePrefixedErrorMessage, theBufferLength, theErrorPrefix );
		Strcat16(thePrefixedErrorMessage, theBufferLength, theErrorMessage);

		theErrorAsJString = (*env)->NewString(env, thePrefixedErrorMessage, Strlen16(thePrefixedErrorMessage));

		FREE_MEMORY_SAFELY(thePrefixedErrorMessage);
	}

	return theErrorAsJString;
}

//	simulation_wants_update
JNIEXPORT jboolean JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_simulation_1wants_1update(
	JNIEnv	*env,
	jclass	cls,
	jlong	mdAsLong)
{
	ModelData	*md;

	(void)env;
	(void)cls;

	if (mdAsLong == 0)	//	should never occur
		return false;

	md = (ModelData *) (uintptr_t) mdAsLong;

	return SimulationWantsUpdates(md);
}


//	get_and_clear_generic_error_message
JNIEXPORT jstring JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_get_1and_1clear_1generic_1error_1message(
	JNIEnv	*env,
	jclass	cls)
{
	(void)cls;

	Char16	thePrefixedErrorMessage[2048];

	GetAndClearGenericErrorMessage(thePrefixedErrorMessage, BUFFER_LENGTH(thePrefixedErrorMessage));

	return (*env)->NewString(env, thePrefixedErrorMessage, Strlen16(thePrefixedErrorMessage));
}
