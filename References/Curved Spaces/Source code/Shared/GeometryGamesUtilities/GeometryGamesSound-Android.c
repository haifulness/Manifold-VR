//	GeometryGamesSound-Android.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesSound.h"
#include "GeometryGamesUtilities-Common.h"
#include "GeometryGames-Android-JavaGlobals.h"


//	Play sounds?
bool	gPlaySounds	= true;	//	<AppName>Application.onCreate() may override
							//		this default value with a user pref


void PlayTheSound(const Char16 *aRelativePath)	//	zero-terminated UTF-16 string with path separator '/'
{
	jint		theErrorCode;
	JNIEnv		*env;
	jstring		theRelativePath;
	
	//	Are sounds disabled?
	if ( ! gPlaySounds )
		return;

	//	We can access the Android MediaPlayer more easily from Java than from C,
	//	so let's call the static playSound() method
	//	in the class gGeometryGamesUtilitiesClass, and let it do the work.
	
	if (gJavaVM == 0
	 || gGeometryGamesUtilitiesClass == 0
	 || gPlaySoundMethodID == 0)
	{
		return;	//	should never occur
	}
	
	//	It seems odd that in the GetEnv() call I need to tell
	//	the JavaVM what version of Java the JNIEnv should have.
	//	It seems like that's something that it should be telling me.
	//	For now I'll assume that I'm merely stating my requirement,
	//	so that GetEnv() may fail with error code JNI_EVERSION
	//	if it can't meet my request.
	theErrorCode = (*gJavaVM)->GetEnv(gJavaVM, (void **)&env, JNI_VERSION_1_6);
	if (theErrorCode != JNI_OK)
		return;
	
	//	Create a Java String containing the relative path name.
	theRelativePath = (*env)->NewString(env, aRelativePath, Strlen16(aRelativePath));
	
	//	Call gGeometryGamesUtilitiesClass.playSound().
	(*env)->CallStaticVoidMethod(	env,
									gGeometryGamesUtilitiesClass,
									gPlaySoundMethodID,
									theRelativePath);
	
	//	Calling DeleteLocalRef() isn't really necessary, because
	//
	//		"When a native method returns,
	//		all local references are automatically deleted."
	//
	//	If we were creating a lot of objects (for example in a loop)
	//	then it would be important to free them to avoid hitting
	//	the local reference limit.
	//
//	(*env)->DeleteLocalRef(env, theRelativePath);
}
