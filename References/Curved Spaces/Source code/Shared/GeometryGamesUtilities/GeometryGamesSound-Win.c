//	GeometryGamesSound-Win.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesSound.h"
#include "GeometryGamesUtilities-Common.h"
#include "GeometryGamesUtilities-Win.h"
#include <mmsystem.h>	//	comment out:  typedef UINT FAR   *LPUINT;
#include <process.h>

#define FILE_PATH_BUFFER_SIZE	2048


//	Play sounds?
bool	gPlaySounds = true;	//	User interface may override using "sound effects" user pref


static bool StringHasSuffix(const Char16 *aString, const Char16 *aSuffix);
static void	PlayMIDI(const Char16 aFullPath[FILE_PATH_BUFFER_SIZE]);
static void	PlayWAV(const Char16 aFullPath[FILE_PATH_BUFFER_SIZE]);


void SetUpAudio(void)
{
}

void ShutDownAudio(void)
{
}


void PlayTheSound(const Char16 *aSoundFileName)	//	zero-terminated UTF-16 string
{
	Char16	theFullPath[FILE_PATH_BUFFER_SIZE]	= u"";

	if (gPlaySounds
	 && GetAbsolutePath(u"Sounds", aSoundFileName, theFullPath, BUFFER_LENGTH(theFullPath)) == NULL)
	{
		if (StringHasSuffix(aSoundFileName, u".mid"))
			PlayMIDI(theFullPath);
		else
		if (StringHasSuffix(aSoundFileName, u".wav"))
			PlayWAV(theFullPath);
		else
			ErrorMessage(u"The Geometry Games apps support only .mid and .wav files", u"Internal Error");
	}
}

static bool StringHasSuffix(
	const Char16	*aString,
	const Char16	*aSuffix)
{
	unsigned int	theStringLength,
					theSuffixLength;
	
	theStringLength = Strlen16(aString);
	theSuffixLength = Strlen16(aSuffix);
	
	if (theStringLength >= theSuffixLength)
		return SameString16(aString + (theStringLength - theSuffixLength), aSuffix);
	else
		return false;
}

static void PlayMIDI(
	const Char16	aFullPath[FILE_PATH_BUFFER_SIZE])	//	zero-terminated UTF-16 string with path separator '/'
{
	//	Very short MIDI files (sound effects) open acceptably fast.
	//	Longer MIDI files (songs) load fast on Win95/98 but take tens of seconds
	//	to load on WinXP (!) and must therefore be pre-loaded in a separate thread
	//	(see "MIDI thread - old/TorusGamesMusic.c" for working code).

	Char16	theOpenCommand[FILE_PATH_BUFFER_SIZE + 128]	= u"";

	//	Format theOpenCommand as
	//
	//		open \"<aFullPath>\" type sequencer alias TheSound
	//
	//
	//	Notes from previous versions:
	//
	//		-	wsprintf() is dangerous:  no protection against buffer overruns!
	//
	//		-	wsnprintf() would be worse:  it doesn't guarantee zero-termination.
	//
	//		-	StringCbPrintf() would work fine if the user is running WindowsXP SP2 or later
	//				(and if the CodeBlocks compiler knows about it, which by now it should).
	//
	Strcpy16(theOpenCommand, BUFFER_LENGTH(theOpenCommand), u"open \""							);
	Strcat16(theOpenCommand, BUFFER_LENGTH(theOpenCommand), aFullPath							);
	Strcat16(theOpenCommand, BUFFER_LENGTH(theOpenCommand), u"\" type sequencer alias TheSound"	);

	mciSendString(u"stop all",				0, 0, 0);
	mciSendString(u"close all",				0, 0, 0);
	mciSendString(theOpenCommand,			0, 0, 0);
	mciSendString(u"play TheSound from 0",	0, 0, 0);
}

static void PlayWAV(
	const Char16	aFullPath[FILE_PATH_BUFFER_SIZE])	//	zero-terminated UTF-16 string with path separator '/'
{
	PlaySound(aFullPath, NULL, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
}
