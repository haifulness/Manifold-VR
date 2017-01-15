//	GeometryGamesSound-iOS.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesSound.h"
#import "GeometryGamesUtilities-Mac-iOS.h"
#import "GeometryGamesUtilities-Common.h"
#import <AudioToolbox/AudioServices.h>
#import <AVFoundation/AVFoundation.h>


//	Play sounds?
bool	gPlaySounds	= true;	//	App delegate will override
							//		using "sound effects" user pref

//	Each sound cache entry associates an AVAudioPlayer* or SystemSoundID to a file name.
//	The file name includes the file extension, for example "Foo.m4a" or "Foo.wav".
static NSMutableDictionary	*gCachedSoundsM4A,	//	each value is an AVAudioPlayer*
							*gCachedSoundsWAV;	//	each value is an NSNumber containing a SystemSoundID


static void	PlayMIDI(NSString *aSoundFileName);
static void	PlayWAV(NSString *aSoundFileName);


void SetUpAudio(void)
{
	AVAudioSession	*theAudioSession;
	
	//	SetUpAudio() presently sets up an M4A player, not a MIDI player.
	//	If iOS ever supports MIDI, we can switch over.

	theAudioSession = [AVAudioSession sharedInstance];	//	Implicitly initializes the audio session.
	[theAudioSession setCategory:AVAudioSessionCategoryAmbient error:nil];	//	Never interrupt background music.
	[theAudioSession setActive:YES error:nil];			//	Unnecessary but recommended.

	gCachedSoundsM4A = [NSMutableDictionary dictionaryWithCapacity:32];	//	will grow if necessary
	gCachedSoundsWAV = [NSMutableDictionary dictionaryWithCapacity:32];	//	will grow if necessary
}

void ShutDownAudio(void)
{
	//	Never gets called (by design).
	
	//	But if it did get called, we'd want to clear the sound caches
	//	and release the dictionaries.

	ClearSoundCaches();

	gCachedSoundsM4A = nil;
	gCachedSoundsWAV = nil;
}

void ClearSoundCaches(void)
{
	NSString		*theKey;
	AVAudioPlayer	*theAudioPlayer;
	NSNumber		*theSoundID;
	
	for (theKey in gCachedSoundsM4A)
	{
		theAudioPlayer = [gCachedSoundsM4A objectForKey:theKey];
		[theAudioPlayer stop];
		theAudioPlayer = nil;
	}
	[gCachedSoundsM4A removeAllObjects];
	
	for (theKey in gCachedSoundsWAV)
	{
		theSoundID = [gCachedSoundsWAV objectForKey:theKey];
		AudioServicesDisposeSystemSoundID([theSoundID unsignedIntValue]);
	}
	[gCachedSoundsWAV removeAllObjects];
}


void PlayTheSound(const Char16 *aSoundFileName)	//	zero-terminated UTF-16 string
{
	NSString	*theSoundFileName;

	if (
		//	Has the user enabled sound effects?
		gPlaySounds
	 &&
	 	//	Is background music playing?
		//	If so, suppress sound effects regardless of the value of gPlaySounds.
	 	! [[AVAudioSession sharedInstance] secondaryAudioShouldBeSilencedHint])
	{
		theSoundFileName = GetNSStringFromZeroTerminatedString(aSoundFileName);
		
		if ([theSoundFileName hasSuffix:@".mid"])
			PlayMIDI(theSoundFileName);
		else
		if ([theSoundFileName hasSuffix:@".wav"])
			PlayWAV(theSoundFileName);
		else
			ErrorMessage(u"The Geometry Games apps support only .mid and .wav files", u"Internal Error");
	}
}

static void PlayMIDI(NSString *aSoundFileName)	//	for example "Foo.mid"
{
	NSString		*theM4AFileName;
	AVAudioPlayer	*theAudioPlayer;
	NSString		*theFullPath;
	NSURL			*theFileURL;

	//	iOS 5.0 supports MIDI files, but with no built-in instruments
	//	(we'd need to provide our own DLS or SoundFont file,
	//	and also set up something called an "AUGraph").
	//	[OR, IN iOS 8 AND UP, USE AN AVMIDIPlayer,
	//	BUT STILL WITH OUR OWN SOUND FONT.]
	//
	//	Instead, let's stick with the iOS 4 version of this function
	//	and simply play an equivalent M4A file, prepared in advance.
	theM4AFileName	= [[aSoundFileName
						stringByDeletingPathExtension]		//	remove .mid
						stringByAppendingString:@".m4a"];	//	append .m4a

	//	Does  already contain the requested sound?
	theAudioPlayer = [gCachedSoundsM4A objectForKey:theM4AFileName];
	
	//	If not, load the sound now, and cache it for future use.
	if (theAudioPlayer == nil)
	{
		theFullPath = [[[[NSBundle mainBundle] resourcePath]
						stringByAppendingString:@"/Sounds - iOS/"]
						stringByAppendingString:theM4AFileName];
		
		//	Technical note:  Be careful here -- if the requested file is not present,
		//	fileURLWithPath:isDirectory: will throw an exception and crash!
		theFileURL		= [NSURL fileURLWithPath:theFullPath isDirectory:NO];
		theAudioPlayer	= [[AVAudioPlayer alloc] initWithContentsOfURL:theFileURL error:NULL];
		if (theAudioPlayer != nil)
			[gCachedSoundsM4A setObject:theAudioPlayer forKey:theM4AFileName];
	}
	
	if (theAudioPlayer != nil)	//	should never fail
	{
		//	If the requested sound is already playing, stop it.
		//	Otherwise the "play" command (see below) will likely have no effect.
		if ([theAudioPlayer isPlaying])
			[theAudioPlayer stop];
		
		//	Restart the sound from the beginning each time.
		//	(Let's hope the AVAudioPlayer caches the decoded sound,
		//	so it doesn't have to decode the M4A file over and over.)
		[theAudioPlayer setCurrentTime:0.0];

		//	There's no need to call [theAudioPlayer prepareToPlay],
		//	given that we'll be calling [theAudioPlayer play] immediately anyhow.

		//	Start the sound playing.
		//
		//		Note:  AVAudioPlayer has the well-known bug that it throws
		//		internal C++ exceptions as part of its normal operation, at least
		//		when running in the iOS Simulator (version 8.3 and maybe others).
		//		If the debugger is to set to break on "All Exceptions",
		//		it will break on the following line (over and over and over...).
		//		The workaround is to break on "All Objective-C Exceptions"
		//		rather than "All Exceptions" (thus ignoring C++ exceptions).
		//		If some future version of AVAudioPlayer and/or some future version
		//		of the simulator fixes this bug, then I can go back to breaking
		//		on all exceptions.
		//
		[theAudioPlayer play];
	}
}

static void PlayWAV(NSString *aSoundFileName)	//	for example "Foo.wav"
{
	NSNumber		*theSoundID;
	NSString		*theFullPath;
	NSURL			*theFileURL;
	SystemSoundID	theRawSoundID	= 0;

	//	Technical Note:  I tried funnelling PlayWAV() and PlayMIDI() into
	//	a single unified PlaySoundFile() function, which played all sounds
	//	with an AVAudioPlayer, exactly as in PlayMIDI() above.
	//	This basically worked, except that each call to PlayWAV()
	//	would block the main thread for a brief but perceptible amount of time.
	//	The web page 
	//
	//		http://www.blumtnwerx.com/blog/2009/05/adventures-in-sound-land/
	//
	//	observes that each call to [anAVAudioPlayer play] blocks the main thread
	//	for 0.04-0.05 sec, even though the sound itself plays asynchronously
	//	in a separate thread.  The problem was most noticeable in the Pool game,
	//	where the motion of the balls would hesitate slightly each time two balls
	//	collided and made a sound.  The initial break (of the pool balls) was
	//	disasterous, because it generated a great many sounds and thus a long delay.
	//	(The user doesn't hear all those sounds, because each succeeding one cancels
	//	the previous one almost immediately.)  Moral of the story:  use AudioServices,
	//	not AVAudioPlayer, for time-critical sounds.
	//
	//	Afterthought, for future reference:  If absolutely necessary, I could
	//	call [anAVAudioPlayer play] from a separate thread, but that hardly
	//	advances the present goal of simplifying the code.  So for now I'm happy
	//	to leave well enough alone, and let AudioServices play the pool sounds.

	//	Does  already contain the requested sound?
	theSoundID = [gCachedSoundsWAV objectForKey:aSoundFileName];
	
	//	If not, load the sound now, and cache it for future use.
	if (theSoundID == nil)
	{
		theFullPath = [[[[NSBundle mainBundle] resourcePath]
						stringByAppendingString:@"/Sounds - iOS/"]
						stringByAppendingString:aSoundFileName];
		
		//	Technical note:  Be careful here -- if the requested file is not present,
		//	fileURLWithPath:isDirectory: will throw an exception and crash!
		theFileURL = [NSURL fileURLWithPath:theFullPath isDirectory:NO];

		AudioServicesCreateSystemSoundID(
			//Old code (just for reference, may eventually be deleted):
			//CFBundleCopyResourceURL(CFBundleGetMainBundle(),
			//						(CFStringRef) aSoundFileName,
			//						(CFStringRef) nil,
			//						(CFStringRef) @"Sounds - iOS"),
			(__bridge CFURLRef) theFileURL,
			&theRawSoundID);
		
		//	Warning:  I'm assuming that a valid SystemSoundID is never zero.
		//	I haven't seen that assumption documented anywhere,
		//	but the fact that AudioServicesCreateSystemSoundID() sets
		//	the SystemSoundID value to zero when it can't find the requested file
		//	is a good sign.
		if (theRawSoundID != 0)
		{
			theSoundID = [NSNumber numberWithUnsignedInt:theRawSoundID];
			[gCachedSoundsWAV setObject:theSoundID forKey:aSoundFileName];
		}
	}
	
	if (theSoundID != nil)	//	should never fail
	{
		AudioServicesPlaySystemSound([theSoundID unsignedIntValue]);
	}
}

