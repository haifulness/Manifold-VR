//	CurvedSpacesView.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "CurvedSpacesView.h"
#import "CurvedSpaces-Common.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesGraphicsDataGL.h"
#import "CurvedSpacesGraphics-OpenGL.h"

#ifdef SAVE_ANIMATION
#import <QTKit/QTMovie.h>
#endif


//	Privately-declared properties and methods
@interface CurvedSpacesView()
@end


@implementation CurvedSpacesView
{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
	//	Have we commandeered the mouse for steering the spaceship?
	bool	itsMouseSteeringFlag;
#endif
}


- (id)initWithModel:(GeometryGamesModel *)aModel frame:(NSRect)aFrame
{
	self = [super	initWithModel:	aModel
					frame:			aFrame
					opaque:			true
					depthBuffer:	true	//	Depth buffer is essential.
					multisampling:	true	//	Curved Spaces looks best with multisampling.
					stereo:			false
					stencilBuffer:	false];
	if (self != nil)
	{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
		//	The mouse is still free.
		itsMouseSteeringFlag = false;
#endif
	}
	return self;
}


- (void)requestShaderUpdate
{
	GraphicsDataGL	*gd	= NULL;

	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	gd->itsPreparedShaders = false;
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
}

- (void)requestTextureUpdate
{
	GraphicsDataGL	*gd	= NULL;

	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	gd->itsPreparedTextures = false;
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
}

- (void)requestVBOUpdate
{
	GraphicsDataGL	*gd	= NULL;

	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	gd->itsPreparedVBOs = false;
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
}


#ifdef CURVED_SPACES_MOUSE_INTERFACE

- (void)handleApplicationWillResignActiveNotification:(NSNotification *)aNotification
{
	[self exitNavigationalMode];	//	Typically unnecessary, but safe and future-proof

	[super handleApplicationWillResignActiveNotification:aNotification];
}

- (void)handleApplicationDidBecomeActiveNotification:(NSNotification *)aNotification
{
	[super handleApplicationDidBecomeActiveNotification:aNotification];
}

- (void)handleWindowDidMiniaturizeNotification:(NSNotification *)aNotification
{
	[self exitNavigationalMode];	//	Typically unnecessary, but safe and future-proof

	[super handleWindowDidMiniaturizeNotification:aNotification];
}

- (void)handleWindowDidDeminiaturizeNotification:(NSNotification *)aNotification
{
	[super handleWindowDidDeminiaturizeNotification:aNotification];
}

#endif	//	CURVED_SPACES_MOUSE_INTERFACE


#ifdef SAVE_ANIMATION
- (CVReturn)updateAnimation	//	overrides superclass method
{
	ModelData			*md					= NULL;
	TitledErrorMessage	theDrawError		= {NULL, NULL};
	CVReturn			theReturnValue;
	id					theController		= nil;
	NSFileHandle		*theFlightPathFile	= nil;

	static unsigned int	theFrameCount = 0;

	//	When creating a QuickTime movie, skip every other frame,
	//	to run at ~30fps instead of the typical 60fps.
	if (theFrameCount++ & 0x00000001)
		return kCVReturnSuccess;

	//	Let the superclass update the simulation and draw the scene.
	theReturnValue = [super updateAnimation];

	//	Record the current parameters to theFlightPathFile.
	//
	//	If this were a standard Curved Spaces feature,
	//	we'd make theController an official CurvedSpacesViewDelegate
	//	and access it that way.  For for a private feature,
	//	let's just kludge a way to get theFlightPathFile,
	//	to keep the main code simple (and avoid ugly #ifdef's
	//	around protocol definitions).
	theController = [[self window] delegate];
	if ([theController respondsToSelector:@selector(itsFlightPathFile)])
	{
		theFlightPathFile = [theController itsFlightPathFile];
		if (theFlightPathFile != nil)
		{
			[itsModel lockModelData:&md];
			[theFlightPathFile writeData:[NSData dataWithBytes:&md->itsUserPlacement   length:sizeof(md->itsUserPlacement  ) ]];
			[theFlightPathFile writeData:[NSData dataWithBytes:&md->itsCurrentAperture length:sizeof(md->itsCurrentAperture) ]];
			[theFlightPathFile writeData:[NSData dataWithBytes:&md->itsRotationAngle   length:sizeof(md->itsRotationAngle  ) ]];
			[itsModel unlockModelData:&md];
		}
	}

	//	Return whatever value the superclass returned.
	return theReturnValue;
}
#endif	


- (void)keyDown:(NSEvent *)anEvent
{
	ModelData	*md	= NULL;

	switch ([anEvent keyCode])
	{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
		case ESCAPE_KEY:
			if (itsMouseSteeringFlag)
				[self exitNavigationalMode];
			break;
#endif
		
		case LEFT_ARROW_KEY:
			[itsModel lockModelData:&md];
			ChangeAperture(md, true);
			[itsModel unlockModelData:&md];
			break;

		case RIGHT_ARROW_KEY:
			[itsModel lockModelData:&md];
			ChangeAperture(md, false);
			[itsModel unlockModelData:&md];
			break;

		case DOWN_ARROW_KEY:
			[itsModel lockModelData:&md];
			md->itsUserSpeed -= USER_SPEED_INCREMENT;
			[itsModel unlockModelData:&md];
			break;

		case UP_ARROW_KEY:
			[itsModel lockModelData:&md];
			md->itsUserSpeed += USER_SPEED_INCREMENT;
			[itsModel unlockModelData:&md];
			break;

		case SPACEBAR_KEY:
			[itsModel lockModelData:&md];
			md->itsUserSpeed = 0.0;
			[itsModel unlockModelData:&md];
			break;

#ifdef START_OUTSIDE
		case ENTER_KEY:
			[itsModel lockModelData:&md];
			if (md->itsViewpoint == ViewpointExtrinsic)
				md->itsViewpoint = ViewpointEntering;
			[itsModel unlockModelData:&md];
			break;
#endif

		default:
		
			//	Ignore the keystoke.
			//	Let the superclass pass it down the responder chain.
			[super keyDown:anEvent];

			//	Process the keystroke normally.
		//	[self interpretKeyEvents:@[anEvent]];

			break;
	}
}


#ifdef CURVED_SPACES_MOUSE_INTERFACE

- (void)enterNavigationalMode
{
	//	Hide the cursor and disassociate it from mouse motion
	//	so the (invisible) cursor stays parked over our window.
	//	Mouse motions will nevertheless arrive in mouseMoved calls.

	if ( ! itsMouseSteeringFlag )
	{
		CGDisplayHideCursor(kCGDirectMainDisplay);	//	parameter is ignored
		CGAssociateMouseAndMouseCursorPosition(NO);
		[[self window] setAcceptsMouseMovedEvents:YES];
		itsMouseSteeringFlag = true;
	}
}

- (void)exitNavigationalMode
{
	//	Restore the usual cursor.

	if (itsMouseSteeringFlag)
	{
		CGDisplayShowCursor(kCGDirectMainDisplay);	//	parameter is ignored
		CGAssociateMouseAndMouseCursorPosition(YES);
		[[self window] setAcceptsMouseMovedEvents:NO];
		itsMouseSteeringFlag = false;
	}
}


- (void)mouseMoved:(NSEvent *)anEvent
{
	unsigned int	theModifierFlags;
	ModelData		*md				= NULL;

	//	In theory we shouldn't need to test itsMouseSteeringFlag 
	//	because we receive mouseMoved events only while in navigational mode.
	//	Nevertheless it seems prudent to double check, to avoid any danger 
	//	of a "stuck steering wheel".
	if (itsMouseSteeringFlag)
	{
		theModifierFlags = (unsigned int) [anEvent modifierFlags];

		[itsModel lockModelData:&md];
		MouseMoved(	md,
					[self mouseLocation:anEvent],
					[self mouseDisplacement:anEvent],
					theModifierFlags & NSShiftKeyMask,
					theModifierFlags & NSControlKeyMask,
					theModifierFlags & NSAlternateKeyMask);
		[itsModel unlockModelData:&md];
	}
}

- (void)mouseDown:(NSEvent *)anEvent
{
	//	For consistency with the Torus Games, we want a double-click
	//	to get us out of navigational mode. Nevertheless, for the convenience
	//	of CurvedSpaces-only users, a single-click should also do the job.
	//	The solution is to exit navigational mode on the first click, 
	//	and then ignore the second click (if any) so that it doesn't put us
	//	back into navigational mode.
	if ([anEvent clickCount] > 1)
		return;

	if ( ! itsMouseSteeringFlag )
		[self enterNavigationalMode];
	else
		[self exitNavigationalMode];
}

- (void)rightMouseDown:(NSEvent *)anEvent
{
	[self mouseDown:anEvent];
}

- (void)errorWithTitle:(ErrorText)aTitle message:(ErrorText)aMessage
{
	//	Call this method only with the ModelData unlocked.

	if (itsMouseSteeringFlag)
		[self exitNavigationalMode];
	
	ErrorMessage(aMessage, aTitle);
}

#endif	//	CURVED_SPACES_MOUSE_INTERFACE


#ifdef CURVED_SPACES_TOUCH_INTERFACE

- (void)mouseDragged:(NSEvent *)anEvent
{
	unsigned int	theModifierFlags;
	ModelData		*md				= NULL;

	theModifierFlags = (unsigned int) [anEvent modifierFlags];

	[itsModel lockModelData:&md];
	MouseMoved(	md,
				[self mouseLocation:anEvent],
				[self mouseDisplacement:anEvent],
				theModifierFlags & NSShiftKeyMask,
				theModifierFlags & NSControlKeyMask,
				theModifierFlags & NSAlternateKeyMask);
	[itsModel unlockModelData:&md];
}

#endif	//	CURVED_SPACES_TOUCH_INTERFACE


- (void)scrollWheel:(NSEvent *)anEvent
{
	CGFloat		theDeltaY;
	ModelData	*md			= NULL;
	
	theDeltaY = [anEvent deltaY];
	if ([anEvent isDirectionInvertedFromDevice])
		theDeltaY = - theDeltaY;

	[itsModel lockModelData:&md];
	md->itsUserSpeed += theDeltaY * 0.1 * USER_SPEED_INCREMENT;
	[itsModel unlockModelData:&md];
}


#ifdef SAVE_ANIMATION

- (void)qtPlayMovieToFile:(bool)aSaveToFileFlag
{
	NSOpenPanel			*theOpenPanel			= nil;
	NSString			*theFilePath			= nil;
	NSFileHandle		*theFlightPathFile		= nil;
	NSSavePanel			*theSavePanel			= nil;
	NSString			*theMovieFilePath		= nil;
	NSSize				theViewSize;
	ModelData			*md						= NULL;
	QTMovie				*theMovie				= nil;
	NSData				*theUserPlacement		= nil,
						*theCurrentAperature	= nil,
						*theRotationAngle		= nil;
	bool				theEndOfFileFlag;
	double				thePreviousAperture		= 0.0;
	ErrorText			theSetUpError			= NULL,
						theRenderError			= NULL;
	CFAbsoluteTime		thePreviousTime,
						theCurrentTime,
						theRemainingTime;
	NSBitmapImageRep	*theBitmap				= nil;
	NSImage				*theImage				= nil;

	//	Pause the usual animation.
	[self pauseAnimation];

	//	For some reason setTitle: gets ignored if we call it after the Open dialog.
	//	(Maybe because we've got exclusive control at that point???)
	[[self window] setTitle:aSaveToFileFlag ?
		@"Rendering QuickTime Movie.  Please be patient…" :
		@"Dry run only:  no movie is being created."];

	//	Let the user select an input file.
	theOpenPanel = [NSOpenPanel openPanel];
	[theOpenPanel setMessage:@"Select an input file defining the flight path."];
	if ([theOpenPanel runModal] != NSFileHandlingPanelOKButton)
		goto CleanUpQtPlayMovieToFile;
	
	//	Open the input file.
	theFilePath = [theOpenPanel filename];
	theFlightPathFile = [NSFileHandle fileHandleForReadingAtPath:theFilePath];
	if (theFlightPathFile == nil)
	{
		ErrorMessage("Couldn't open input file in qtPlayMovieToFile.", "I/O Error");
		goto CleanUpQtPlayMovieToFile;
	}
	
	//	Set up an output movie file.
	if (aSaveToFileFlag)
	{
		theSavePanel = [NSSavePanel savePanel];
		[theSavePanel setMessage:@"Select an output file for the QuickTime movie."];
		[theSavePanel setRequiredFileType:@"mov"];
		if ([theSavePanel runModal] != NSFileHandlingPanelOKButton)
			goto CleanUpQtPlayMovieToFile;
		theMovieFilePath = [theSavePanel filename];

		//	A QTMovie wants to initialize from a pre-existing file, it seems.
		//	So make sure the output file exists (even if it contains an empty movie).
		//	Note:  This will overwrite any previous version of the output file.
		[[QTMovie movie] writeToFile:theMovieFilePath withAttributes:@{}];

		//	Now that a (possibly empty) file exists, allocate and initialize the real movie.
		theMovie = [[QTMovie alloc] initWithFile:theMovieFilePath error:nil];
		[theMovie setAttribute:@YES forKey:QTMovieEditableAttribute];
	}

	theViewSize = [self convertSizeToBase:[self bounds].size];	//	converts points to pixels
	
	thePreviousTime = CFAbsoluteTimeGetCurrent();

	while (true)
	{
		//	Create a local autorelease pool.
		//	Each frame will require several megabytes of data,
		//	which we'll want to release before moving on to the next frame.
		@autoreleasepool
		{
			[itsModel lockModelData:&md];
			
			theUserPlacement	= [theFlightPathFile readDataOfLength:sizeof(md->itsUserPlacement  )];
			theCurrentAperature	= [theFlightPathFile readDataOfLength:sizeof(md->itsCurrentAperture)];
			theRotationAngle	= [theFlightPathFile readDataOfLength:sizeof(md->itsRotationAngle  )];

			theEndOfFileFlag = (
				[theUserPlacement    length] != sizeof(md->itsUserPlacement  )
			 || [theCurrentAperature length] != sizeof(md->itsCurrentAperture)
			 || [theRotationAngle    length] != sizeof(md->itsRotationAngle  ));

			[itsModel unlockModelData:&md];
			
			if (theEndOfFileFlag)	//	no more data points?
				break;	//SHOULD DRAIN THE AUTORELEASE POOL

			[itsModel lockModelData:&md];
			
			thePreviousAperture = md->itsCurrentAperture;
		
			[theUserPlacement    getBytes:&md->itsUserPlacement   length:sizeof(md->itsUserPlacement  )];
			[theCurrentAperature getBytes:&md->itsCurrentAperture length:sizeof(md->itsCurrentAperture)];
			[theRotationAngle    getBytes:&md->itsRotationAngle   length:sizeof(md->itsRotationAngle  )];
			
			//	Did the aperture change?
			if (md->itsCurrentAperture != thePreviousAperture)
			{
				//	Set itsDesiredAperture, just to keep things clean.
				md->itsDesiredAperture = md->itsCurrentAperture;

				//	We'll need a new VBO with the new aperture.
				gd->itsPreparedVBOs = false;
			}

			[itsModel unlockModelData:&md];


			[itsModel lockModelData:&md];
			[self lockAndSetContext];

			theSetUpError	= SetUpGraphicsAsNeeded(md);
			if (theSetUpError == NULL)
				theRenderError = Render(md, (unsigned int)theViewSize.width, (unsigned int)theViewSize.height, NULL);
			if (theSetUpError == NULL && theRenderError == NULL)
				[itsContext flushBuffer];

			[self unsetAndUnlockContext];
			[itsModel unlockModelData:&md];

			if (theSetUpError != NULL)
				break;	//SHOULD DRAIN THE AUTORELEASE POOL
			if (theRenderError != NULL)
				break;	//SHOULD DRAIN THE AUTORELEASE POOL
			
			if (aSaveToFileFlag)
			{
				//	Get a bitmap of the desired size...
#ifdef HIGH_DEF_ANIMATION
				theBitmap = [self imageAsBitmapImageRepOfSize:(NSSize){1920, 1080}];	//	high-def
#else
				theBitmap = [self imageAsBitmapImageRepOfSize:(NSSize){ 720,  480}];	//	low-def
#endif //HIGH_DEF_ANIMATION

				//	...and wrap it as an NSImage.
				//
				//	Note:  The documentation for addRepresentation makes the vague comment that
				//	"After invoking this method, you may need to explicitly set features
				//	of the new image representation, such as the size, number of colors, and so on."
				//	but in practice it seems to work OK.
				theImage = [[NSImage alloc] initWithSize:(NSSize){0.0, 0.0}];
				[theImage addRepresentation:theBitmap];

				[theMovie
					addImage:		theImage
					forDuration:	QTMakeTime((long long)1, (long)30)
					withAttributes:	@{QTAddImageCodecType : SAVE_ANIMATION_CODEC}];
				[theMovie updateMovieFile];
			}

			//	While recording a movie, we'll almost surely lag behind 30fps.
			//	But while previewing a movie, we'd probably exceed 30fps 
			//	if we didn't sleep between frames.
			theCurrentTime		= CFAbsoluteTimeGetCurrent();
			theRemainingTime	= thePreviousTime + 1.0/30.0 - theCurrentTime;
			if (theRemainingTime > 0.0 && theRemainingTime < 1.0)
				usleep( (useconds_t)(1000000.0 * theRemainingTime) );
			thePreviousTime = CFAbsoluteTimeGetCurrent();

		}
	}

CleanUpQtPlayMovieToFile:

	[theFlightPathFile closeFile];

	[[self window] setTitle:(aSaveToFileFlag ? @"Done rendering movie" : @"Done previewing movie")];

	[self resumeAnimation];

#ifdef CURVED_SPACES_MOUSE_INTERFACE
	if (theSetUpError != NULL)
		[self errorWithTitle:"OpenGL Setup Error during flight playback" message:theSetUpError];
	if (theRenderError != NULL)
		[self errorWithTitle:"OpenGL Rendering Error during flight playback" message:theRenderError];
#else
	if (theSetUpError != NULL)
		ErrorMessage(theSetUpError,  "OpenGL Setup Error during flight playback"	);
	if (theRenderError != NULL)
		ErrorMessage(theRenderError, "OpenGL Rendering Error during flight playback");
#endif
}

#endif


@end
