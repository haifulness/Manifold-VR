//	GeometryGamesSound-Mac.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Cocoa/Cocoa.h>
#include <AudioToolbox/AudioToolbox.h>	//	for MIDI player
#include "GeometryGamesSound.h"
#include "GeometryGamesUtilities-Mac-iOS.h"
#include "GeometryGamesUtilities-Common.h"


//	Maintain a global MIDI player.
MusicPlayer		gMIDIPlayer		= NULL;
MusicSequence	gMIDISequence	= NULL;

//	Play sounds?
bool			gPlaySounds		= true;	//	App delegate will override
										//		using "sound effects" user pref


static void	SetUpMIDI(void);
static void	SetUpWAV(void);
static void	ShutDownMIDI(void);
static void	ShutDownWAV(void);
static void	PlayMIDI(NSString *aFullPath);
static void	PlayWAV(NSString *aFullPath);


//	Define a SoundKeeper object to maintain a strong reference
//	to each NSSound object that's currently playing.
//	Without a strong reference, the NSSound would get deallocated
//	and playback would stop.  When the NSSound finishes playing,
//	it calls -sound:didFinishPlaying: and the SoundKeeper
//	deletes its strong reference to it, thereby deallocating the NSSound.
@interface SoundKeeper : NSObject <NSSoundDelegate>
{
}
- (id)init;
- (void)addSound:(NSSound *)aSound;
- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)finishedPlaying;
@end
@implementation SoundKeeper
{
	NSMutableArray<NSSound *>	*itsSounds;
}
- (id)init
{
	self = [super init];
	if (self != nil)
	{
		itsSounds = [[NSMutableArray<NSSound *> alloc] initWithCapacity:4];
	}
	return self;
}
- (void)addSound:(NSSound *)aSound
{
	[itsSounds addObject:aSound];
}
- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)finishedPlaying
{
	UNUSED_PARAMETER(finishedPlaying);
	[sound setDelegate:nil];	//	safe but probably unnecessary
	[itsSounds removeObject:sound];
}
@end

static SoundKeeper	*gSoundKeeper	= nil;


void SetUpAudio(void)
{
	SetUpMIDI();
	SetUpWAV();
}

static void SetUpMIDI(void)
{
	if (NewMusicPlayer(&gMIDIPlayer) != noErr)
		gMIDIPlayer = NULL;

	gMIDISequence = NULL;
	
	//	The first time a sound was played, there'd be
	//	a noticeable delay while the MIDI player got up to speed.
	//	To avoid, say, the first Maze victory sound arriving late,
	//	let's play a silent sound now to get the MIDI player warmed up.
	PlayTheSound(u"SilenceToPrestartMIDI.mid");
}

static void SetUpWAV(void)
{
	gSoundKeeper = [[SoundKeeper alloc] init];
}

void ShutDownAudio(void)
{
	ShutDownMIDI();
	ShutDownWAV();
}

static void ShutDownMIDI(void)
{
	if (gMIDIPlayer != NULL)
	{
		MusicPlayerStop(gMIDIPlayer);
		DisposeMusicPlayer(gMIDIPlayer);
		gMIDIPlayer = NULL;
	}
	
	if (gMIDISequence != NULL)
	{
		DisposeMusicSequence(gMIDISequence);
		gMIDISequence = NULL;
	}
}

static void ShutDownWAV(void)
{
	gSoundKeeper = nil;	//	releases any currently playing sounds
}


void PlayTheSound(const Char16 *aSoundFileName)	//	zero-terminated UTF-16 string
{
	NSString	*theSoundFileName,
				*theFullPath;
	
	if (gPlaySounds)
	{
		theSoundFileName	= GetNSStringFromZeroTerminatedString(aSoundFileName);
		theFullPath			= [[[[NSBundle mainBundle] resourcePath]
								stringByAppendingString:@"/Sounds/"]
								stringByAppendingString:theSoundFileName];
		
		if ([theSoundFileName hasSuffix:@".mid"])
			PlayMIDI(theFullPath);
		else
		if ([theSoundFileName hasSuffix:@".wav"])
			PlayWAV(theFullPath);
		else
			ErrorMessage(u"The Geometry Games apps support only .mid and .wav files", u"Internal Error");
	}
}

static void PlayMIDI(NSString *aFullPath)
{
	MusicSequence	theNewSequence	= NULL;

	if (gMIDIPlayer != NULL)
	{
		//	Stop any music that might already be playing.
		MusicPlayerStop(gMIDIPlayer);

		//	Create a new sequence, start it playing,
		//	and remember it so we can free it later.
		if (NewMusicSequence(&theNewSequence) == noErr)
		{
			if
			(
				MusicSequenceFileLoadData(
					theNewSequence,
					(__bridge CFDataRef) [NSData dataWithContentsOfFile:aFullPath],
					kMusicSequenceFile_MIDIType,
					0
					) == noErr
			 &&
				MusicPlayerSetSequence(gMIDIPlayer, theNewSequence) == noErr
			)
			{
				//	Start the new music playing.
				MusicPlayerStart(gMIDIPlayer);
				
				//	Destroy the old sequence and remember the new one.
				if (gMIDISequence != NULL)
					DisposeMusicSequence(gMIDISequence);
				gMIDISequence	= theNewSequence;
				theNewSequence	= NULL;
			}
			else
			{
				if (theNewSequence != NULL)
					DisposeMusicSequence(theNewSequence);
				theNewSequence = NULL;
			}
		}
	}
}

static void PlayWAV(NSString *aFullPath)
{
	NSSound	*theSound;

	theSound = [[NSSound alloc] initWithContentsOfFile:aFullPath byReference:YES];
	[gSoundKeeper addSound:theSound];
	[theSound play];
}
