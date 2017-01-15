//	GeometryGamesViewMac.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesViewMac.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesGraphicsDataGL.h"
#import "GeometryGamesWindowController.h"
#import "GeometryGamesUtilities-Common.h"
#import "GeometryGamesUtilities-Mac.h"
#import "GeometryGamesLocalization.h"
#ifdef SUPPORT_OPENGL
#import "GeometryGames-OpenGL.h"
#endif


static CVReturn WrapperForDrawOneFrame(CVDisplayLinkRef displayLink,
	const CVTimeStamp *now, const CVTimeStamp *outputTime, 
	CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext);


//	Privately-declared methods
@interface GeometryGamesViewMac()
- (void)setUpDisplayLink;
- (void)shutDownDisplayLink;
- (TitledErrorMessage)drawWithModelData:(ModelData *)md graphicsData:(GraphicsDataGL *)gd;
@end


@implementation GeometryGamesViewMac


- (id)initWithModel:(GeometryGamesModel *)aModel frame:(NSRect)aFrame opaque:(bool)anOpacityFlag
	depthBuffer:(bool)aDepthBufferFlag multisampling:(bool)aMultisamplingFlag
	stereo:(bool)aStereoBuffersFlag stencilBuffer:(bool)aStencilBufferFlag
{
	GLint	theOpacity = 0;

	self = [super initWithFrame:aFrame];
	if (self != nil)
	{
		itsOpacityFlag			= anOpacityFlag;
		itsDepthBufferFlag		= aDepthBufferFlag;
		itsMultisamplingFlag	= aMultisamplingFlag;
		itsStereoBuffersFlag	= aStereoBuffersFlag;
		itsStencilBufferFlag	= aStencilBufferFlag;

		itsModel = aModel;
		if (itsModel == nil)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}

		itsGraphicsDataGL = [[GeometryGamesGraphicsDataGL alloc] init];
		if (itsGraphicsDataGL == nil)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		
		//	Set up an OpenGL context.
		itsPixelFormat = GetPixelFormat(itsDepthBufferFlag,
										itsMultisamplingFlag,
										itsStereoBuffersFlag,
										itsStencilBufferFlag);
		if (itsPixelFormat == nil)
		{
			FatalError(	GetLocalizedText(u"Failed to get pixel format"),
						GetLocalizedText(u"WindowTitle"));
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		itsContext = [[NSOpenGLContext alloc] initWithFormat:itsPixelFormat shareContext:nil];
		if (itsContext == nil)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		theOpacity = (itsOpacityFlag ? 1 : 0);
		[itsContext setValues:&theOpacity forParameter:NSOpenGLCPSurfaceOpacity];
		
		//	Rather than routing messages from
		//
		//		GeometryGamesAppDelegate
		//			-applicationWillResignActive:
		//			-applicationDidBecomeActive:
		//
		//		GeometryGamesWindowController
		//			-windowDidMiniaturize:
		//			-windowDidDeminiaturize:
		//
		//	let's receive the corresponding notifications directly.

		[[NSNotificationCenter defaultCenter]
			addObserver:	self
			selector:		@selector(handleApplicationWillResignActiveNotification:)
			name:			NSApplicationWillResignActiveNotification
			object:			nil];

		[[NSNotificationCenter defaultCenter]
			addObserver:	self
			selector:		@selector(handleApplicationDidBecomeActiveNotification:)
			name:			NSApplicationDidBecomeActiveNotification
			object:			nil];

		[[NSNotificationCenter defaultCenter]
			addObserver:	self
			selector:		@selector(handleWindowDidMiniaturizeNotification:)
			name:			NSWindowDidMiniaturizeNotification
			object:			nil];	//	See comment in -handleWindowDidMiniaturizeNotification: .

		[[NSNotificationCenter defaultCenter] 
			addObserver:	self
			selector:		@selector(handleWindowDidDeminiaturizeNotification:)
			name:			NSWindowDidDeminiaturizeNotification
			object:			nil];	//	See comment in -handleWindowDidDeminiaturizeNotification: .

		//	GeometryGamesViewMac is a subclass of NSView, not NSOpenGLView,
		//	so we must manually request frame change notifications.
		[[NSNotificationCenter defaultCenter] 
			addObserver:	self
			selector:		@selector(handleViewGlobalFrameDidChangeNotification:)
			name:			NSViewGlobalFrameDidChangeNotification
			object:			self];
		
		//	If a draw error has occurred, suppress further drawing.
		//	Even with a fatal error, the AppKit will try to redraw
		//	the window underneath the fatal error message.
		itsDrawErrorHasOccurred = false;

		//	Set up a CVDisplayLink, but don't start it.
		itsDisplayLink		= NULL;
		itsLastUpdateTime	= CFAbsoluteTimeGetCurrent();
		[self setUpDisplayLink];
	}
	return self;
}

- (void)dealloc
{
	//	-shutDownDisplayLink calls -pauseAnimation, which should block 
	//	until any currently executing callback function (WrapperForDrawOneFrame)
	//	has returned.
	[self shutDownDisplayLink];

	//	Unsubscribe from all notifications.
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}


- (void)viewDidMoveToWindow
{
	if ([self window] != nil)
	{
		//	This view has been added to a Window.
		//	Start the animation running.
		[self resumeAnimation];
	}
	else
	{
		//	Apple's documentation for NSView's -viewDidMoveToWindow says that
		//
		//		If you call the window method and it returns nil,
		//		that result signifies that the view was removed from its window
		//		and does not currently reside in any window.
		//
		[self pauseAnimation];
	}
}


- (void)handleApplicationWillResignActiveNotification:(NSNotification *)aNotification
{
	//	According to Apple's NSApplication Class Reference,
	//	NSApplicationWillResignActiveNotification is
	//
	//		Posted immediately before the application gives up
	//		its active status to another application.
	//		The notification object is sharedApplication.
	//		This notification does not contain a userInfo dictionary.
	//

	UNUSED_PARAMETER(aNotification);
	
	[self pauseAnimation];
}

- (void)handleApplicationDidBecomeActiveNotification:(NSNotification *)aNotification
{
	//	According to Apple's NSApplication Class Reference,
	//	NSApplicationDidBecomeActiveNotification is
	//
	//		Posted immediately after the application becomes active.
	//		The notification object is sharedApplication.
	//		This notification does not contain a userInfo dictionary.
	//

	UNUSED_PARAMETER(aNotification);

	if ( ! [[self window] isMiniaturized] )
		[self resumeAnimation];
}

- (void)handleWindowDidMiniaturizeNotification:(NSNotification *)aNotification
{
	//	According to Apple's NSWindow Class Reference,
	//	NSWindowDidMiniaturizeNotification is
	//
	//		Posted whenever an NSWindow object is minimized.
	//		The notification object is the NSWindow object that has been minimized.
	//		This notification does not contain a userInfo dictionary.
	//

	//	NSNotificationCenter's -addObserver:selector:name:object: would let us
	//	pass a non-nil object for the notificationSender.  However, there's
	//	no guarantee that the sender is the NSWindow being minimized.
	//	Apple's documentation for NSNotification's -object method says that
	//
	//		[the returned object] is often the object that posted this notification
	//
	//	but allows for the possibility that the sender might be something
	//	other than [aNotification object].
	//	So to be safe, let's receive all NSWindowDidMiniaturizeNotifications
	//	and test each one to see whether it's our own window that's getting miniaturized.
	//
	if ([aNotification object] == [self window])
		[self pauseAnimation];
}

- (void)handleWindowDidDeminiaturizeNotification:(NSNotification *)aNotification
{
	//	According to Apple's NSWindow Class Reference,
	//	NSWindowDidDeminiaturizeNotification is
	//
	//		Posted whenever an NSWindow object is deminimized.
	//		The notification object is the NSWindow object that has been deminimized.
	//		This notification does not contain a userInfo dictionary.
	//

	//	NSNotificationCenter's -addObserver:selector:name:object: would let us
	//	pass a non-nil object for the notificationSender.  However, there's
	//	no guarantee that the sender is the NSWindow being de-minimized.
	//	Apple's documentation for NSNotification's -object method says that
	//
	//		[the returned object] is often the object that posted this notification
	//
	//	but allows for the possibility that the sender might be something
	//	other than [aNotification object].
	//	So to be safe, let's receive all NSWindowDidMiniaturizeNotifications
	//	and test each one to see whether it's our own window that's getting de-miniaturized.
	//
	if ([aNotification object] == [self window]			//	essential
	 && [[NSApplication sharedApplication] isActive])	//	unnecessary but harmless
	{
		[self resumeAnimation];
	}
}


- (BOOL)acceptsFirstResponder
{
	return YES;	//	accept keystrokes and action messages
}

- (void)keyDown:(NSEvent *)anEvent
{
	switch ([anEvent keyCode])
	{
		//	Handle special keys, like the esc key
		//	and arrow keys, here in keyDown.
	//	case ESCAPE_KEY:
	//		break;

		default:
		
			//	Ignore the keystoke.
			//	Let the superclass pass it down the responder chain.
			[super keyDown:anEvent];

			//	Process the keystroke normally.
		//	[self interpretKeyEvents:@[anEvent]];

			break;
	}
}


- (void)lockAndSetContext
{
	//	Note #1.  Assume the ModelData has already been locked,
	//	because by convention we always lock the ModelData
	//	before locking the context, and then later
	//	unlock them in the reverse order.
	//	By following a consistent convention
	//	we avoid all risk of deadlock.

	//	Note #2.  In the purest case, the window controller
	//	would handle menu commands and update the ModelData,
	//	with no need to interact with the view.
	//	The flaw in that plan is that the ModelData may want
	//	to cache OpenGL objects.  For example, KaleidoPaint
	//	caches an OpenGL texture for each drawing's unit cell, 
	//	so thumbnails may be drawn quickly.  Such optimizations
	//	break the otherwise strict separation between the abstract
	//	representation of the drawing and its realization in this view.  
	//	Fortunately such optimizations are easy to accommodate:
	//	the window controller need only set and unset the graphics
	//	context whenever it creates or deletes OpenGL objects.

	CGLLockContext([itsContext CGLContextObj]);
	[itsContext makeCurrentContext];
	//	itsContext is ready to use...
}

- (void)unsetAndUnlockContext
{
	//	...done using itsContext.
	[NSOpenGLContext clearCurrentContext];
	CGLUnlockContext([itsContext CGLContextObj]);
}


- (void)lockFocus
{
	[super lockFocus];

	//	The right place to call -setView: seems to be here in -lockFocus.
	//	I don't know the full reasoning on that, but empirically it works great.
	if ([itsContext view] != self)
		[itsContext setView:self];
}

- (BOOL)isOpaque
{
	return itsOpacityFlag;
}

- (void)handleViewGlobalFrameDidChangeNotification:(NSNotification *)aNotification
{
	UNUSED_PARAMETER(aNotification);

	//	Use a mutex to prevent the main thread and the CVDisplayLink thread 
	//	from accessing the context simultaneously.
	//
	//	Technical Note:  We don't need to use the ModelData here, 
	//	but if we did we'd want to lock it *before* locking the context,
	//	because that's the order in which -updateAnimationAtTime:elapsedTime:
	//	locks them.  If we used the opposite order, we'd risk deadlock.
	//
	CGLLockContext([itsContext CGLContextObj]);
	[itsContext update];
	CGLUnlockContext([itsContext CGLContextObj]);
}


#pragma mark -
#pragma mark Animation


- (void)setUpDisplayLink
{
	GLint	theSwapInterval;

	if (itsDisplayLink == NULL)	//	unnecessary but safe
	{
		//	Synchronize buffer swaps with the vertical refresh rate.
		theSwapInterval = 1;
		[itsContext setValues:&theSwapInterval forParameter:NSOpenGLCPSwapInterval]; 

		//	Create a display link capable of being used with all active displays.
		CVDisplayLinkCreateWithActiveCGDisplays(&itsDisplayLink);

		//	Set the renderer output callback function.
		CVDisplayLinkSetOutputCallback(itsDisplayLink, &WrapperForDrawOneFrame, (__bridge void *)self);

		//	Set the display link for the current renderer.
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(
			itsDisplayLink,
			[itsContext CGLContextObj],
			[itsPixelFormat CGLPixelFormatObj]);

		//	Let the subclass start the display link running
		//	once the model data is in place.
	}
}

- (void)shutDownDisplayLink
{
	if (itsDisplayLink != NULL)	//	unnecessary but safe
	{
		//	The call to -pauseAnimation should block until any currently 
		//	executing callback function (WrapperForDrawOneFrame) has returned.
		[self pauseAnimation];
		CVDisplayLinkRelease(itsDisplayLink);
		itsDisplayLink = NULL;
	}
}

- (void)pauseAnimation
{
	//	In an online forum posting, a seemingly knowledgable guy
	//	says that CVDisplayLinkStop() will block until any currently 
	//	executing callback function (WrapperForDrawOneFrame) has returned.
	if (CVDisplayLinkIsRunning(itsDisplayLink))
		CVDisplayLinkStop(itsDisplayLink);
}

- (void)resumeAnimation
{
	if ( ! CVDisplayLinkIsRunning(itsDisplayLink) )
		CVDisplayLinkStart(itsDisplayLink);
}

static CVReturn WrapperForDrawOneFrame(
	CVDisplayLinkRef	displayLink,
	const CVTimeStamp	*now,					//	current time
	const CVTimeStamp	*outputTime,			//	time that frame will get displayed
	CVOptionFlags		flagsIn,
	CVOptionFlags		*flagsOut,
	void				*displayLinkContext)
{
	GeometryGamesViewMac	*theView;
	CVReturn				theReturnValue;
	
	UNUSED_PARAMETER(displayLink);
	UNUSED_PARAMETER(now);
	UNUSED_PARAMETER(outputTime);
	UNUSED_PARAMETER(flagsIn);
	UNUSED_PARAMETER(flagsOut);
	UNUSED_PARAMETER(displayLinkContext);

	//	The CVDisplayLink runs in a separate thread,
	//	with no default autorelease pool, so we must
	//	provide our own autorelease pool to avoid leaking memory.
	@autoreleasepool
	{
		theView			= (__bridge GeometryGamesViewMac *) displayLinkContext;
		theReturnValue	= [theView updateAnimation];
	}

	return theReturnValue;
}

- (CVReturn)updateAnimation
{
	ModelData			*md				= NULL;
	GraphicsDataGL		*gd				= NULL;
	double				theCurrentTime,	//	in seconds
						theElapsedTime;	//	in seconds
	TitledErrorMessage	theDrawError	= {NULL, NULL};

	//	Get a lock on the ModelData, waiting, if necessary,
	//	for the main thread to finish with it first.
	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];

	//	Note theElapsedTime.
	theCurrentTime		= CFAbsoluteTimeGetCurrent();
	theElapsedTime		= theCurrentTime - itsLastUpdateTime;
	itsLastUpdateTime	= theCurrentTime;
		
	//	Need to redraw the image?
	if ( ! [[self window] isMiniaturized]
	  && ! [self isHidden]
	  && SimulationWantsUpdates(md))
	{
		//	Update the simulation.
		SimulationUpdate(md, theElapsedTime);

		//	Redraw the scene.
		theDrawError = [self drawWithModelData:md graphicsData:gd];
	}
	
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];

	//	This is the perfect moment to display theDrawError.
	//	We've unlocked the ModelData and itsContext,
	//	so we needn't worry about blocking the main thread.
	if (theDrawError.itsMessage != NULL && theDrawError.itsTitle != NULL)
		FatalError(theDrawError.itsMessage, theDrawError.itsTitle);

	return kCVReturnSuccess;
}

- (void)drawRect:(NSRect)dirtyRect
{
	//	The main thread may call this -drawRect method.

	ModelData			*md				= NULL;
	GraphicsDataGL		*gd				= NULL;
	TitledErrorMessage	theDrawError	= {NULL, NULL};

	UNUSED_PARAMETER(dirtyRect);
	
	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	theDrawError = [self drawWithModelData:md graphicsData:gd];
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];

	//	We've unlocked the ModelData and itsContext,
	//	so we may safely display theDrawError.
	if (theDrawError.itsMessage != NULL && theDrawError.itsTitle != NULL)
		FatalError(theDrawError.itsMessage, theDrawError.itsTitle);
}

- (TitledErrorMessage)drawWithModelData:(ModelData *)md graphicsData:(GraphicsDataGL *)gd
{
	ErrorText		theSetUpError	= NULL,
					theRenderError	= NULL;
	NSSize			theViewSize;
#ifdef LOG_FRAME_RATE
	unsigned int	i;
	double			theAverageElapsedTime;	//	in seconds
	NSString		*theTitleString	= NULL;
#endif

#ifdef LOG_FRAME_RATE
	for (i = LOG_NUM_FRAMES - 1; i > 0; i--)
		itsElapsedTimes[i] = itsElapsedTimes[i-1];
#endif

	//	If a previous call to -drawWithModelData: has failed,
	//	it will have displayed a fatal error message.
	//	When that error message gets displayed,
	//	the AppKit will try to re-draw the window underneath it,
	//	which would generate a second copy of the error message.
	//	To avoid that, suppress all drawing after a fatal error errors.
	if (itsDrawErrorHasOccurred)
		return (TitledErrorMessage){NULL, NULL};

	//	Get the view's size in pixels (not points).
	theViewSize = [self convertSizeToBacking:[self bounds].size];

	//	Use a mutex to prevent the main thread and the CVDisplayLink thread 
	//	from accessing the context simultaneously.
	[self lockAndSetContext];

	theSetUpError = SetUpGraphicsAsNeeded(md, gd);

	if (theSetUpError == NULL)
		theRenderError = Render(md, gd,
								(unsigned int)theViewSize.width, (unsigned int)theViewSize.height,
#ifdef LOG_FRAME_RATE
								&itsElapsedTimes[0]);
#else
								NULL);
#endif

	[itsContext flushBuffer];

	[self unsetAndUnlockContext];

#ifdef LOG_FRAME_RATE
	theAverageElapsedTime = 0.0;
	for (i = 0; i < LOG_NUM_FRAMES; i++)
		theAverageElapsedTime += itsElapsedTimes[i];
	theAverageElapsedTime *= 1.0e-9 / LOG_NUM_FRAMES;	//	10⁻⁹ converts nanoseconds to seconds

	//	NSWindow is thread-unsafe.  Call -setTitle: from the main thread only.
	//	For more details on AppKit thread safety, see
	//
	//		http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/Multithreading/ThreadSafetySummary/ThreadSafetySummary.html
	//
	theTitleString = ((theAverageElapsedTime > 0.0) ?
						[NSString stringWithFormat:@"%4d fps",
							(unsigned int)(1.0 / theAverageElapsedTime)] :
						@"???? fps" );
	[[self window]
		performSelectorOnMainThread:	@selector(setTitle:)
		withObject:						theTitleString
		waitUntilDone:					NO];
#endif
	
	if (theSetUpError != NULL || theRenderError != NULL)
		itsDrawErrorHasOccurred = true;

	if (theSetUpError != NULL)
		return (TitledErrorMessage){theSetUpError,  u"OpenGL Setup Error"	};
	if (theRenderError != NULL)
		return (TitledErrorMessage){theRenderError, u"OpenGL Rendering Error"};
		
	return (TitledErrorMessage){NULL, NULL};
}


#pragma mark -
#pragma mark Mouse events


- (DisplayPoint)mouseLocation:(NSEvent *)anEvent
{
	NSPoint			theMouseLocation;
	NSSize			theViewSize;
	DisplayPoint	theMousePoint;

	//	mouse location in window coordinates (hmmm... this might be pixels)
	theMouseLocation = [anEvent locationInWindow];

	//	mouse location in view coordinates
	//	(0,0) to (theViewSize.width, theViewSize.height)
	theMouseLocation = [self convertPoint:theMouseLocation fromView:nil];

	//	view size in points (not pixels, but that's OK just so we're consistent)
	theViewSize = [self bounds].size;
	
	//	Package up the result.
	theMousePoint.itsX			= theMouseLocation.x;
	theMousePoint.itsY			= theMouseLocation.y;
	theMousePoint.itsViewWidth	= theViewSize.width;
	theMousePoint.itsViewHeight	= theViewSize.height;
	
	return theMousePoint;
}

- (DisplayPointMotion)mouseDisplacement:(NSEvent *)anEvent
{
	NSSize				theViewSize;
	DisplayPointMotion	theMouseMotion;

	//	view size in points (not pixels, but that's OK just so we're consistent)
	theViewSize = [self bounds].size;
	
	//	Package up the motion.
	//	Note that deltaY is reported in top-down coordinates, because 
	//	
	//		“NSEvent computes this delta value in device space, 
	//		which is flipped, but both the screen and the window’s 
	//		base coordinate system are not flipped.”
	//
	theMouseMotion.itsDeltaX		=   [anEvent deltaX];	//	(hmmm... is this in points or pixels?)
	theMouseMotion.itsDeltaY		= - [anEvent deltaY];	//	(hmmm... is this in points or pixels?)
	theMouseMotion.itsViewWidth		= theViewSize.width;
	theMouseMotion.itsViewHeight	= theViewSize.height;
	
	return theMouseMotion;
}


#pragma mark -
#pragma mark Image as TIFF


- (NSData *)imageAsTIFF
{
	//	The caller has not specified an image size,
	//	so, as a default, pass the view size in pixels (not points).
	return [self imageAsTIFFofSize:[self convertSizeToBacking:[self bounds].size]];
}

- (NSData *)imageAsTIFFofSize:(NSSize)anImageSizePx	//	size in pixels, not points
{
	NSBitmapImageRep	*theBitmap	= nil;
	NSData				*theTIFF	= nil;
	
	theBitmap = [self imageAsBitmapImageRepOfSize:anImageSizePx];

	if (theBitmap != nil)
	{
		theTIFF = [theBitmap TIFFRepresentationUsingCompression:NSTIFFCompressionLZW factor:0.0f];
		if (theTIFF == nil)
			ErrorMessage(u"Couldn't create TIFF representation.", u"Image Copying Error");
	}

	return theTIFF;
}

- (NSBitmapImageRep *)imageAsBitmapImageRepOfSize:(NSSize)anImageSizePx
{
	ErrorText			theError	= NULL;
	unsigned int		theWidth,
						theHeight;
	NSBitmapImageRep	*theBitmap	= nil;
	PixelRGBA			*thePixels	= NULL;
	
	theWidth	= anImageSizePx.width;
	theHeight	= anImageSizePx.height;

	if (theWidth == 0 || theHeight == 0)
	{
		theError = u"Image width or height is zero.";
		goto CleanUpImageAsBitmapImageRep;
	}
	
//	if (theWidth > 1024 || theHeight > 1024)
//	{
		//	My 2006 iMac's Radeon X1600 reports GL_MAX_RENDERBUFFER_SIZE = 4096.
		//	In actual practice, 1600×1600 works fine, but 2048×2048 crashes the whole computer.
		//	Let's set conservative limits, and extend them if higher-resolution images are ever needed.
		//
		//	More empirical data:
		//
		//	The 2006 iMac crashes at high-def video dimensions 1920×1080.
		//
		//	The 2006 iMac crashes when copying 1440×900 in Curved Spaces,
		//		but not in 4D Draw.
		//		The presence of a depth buffer might explain the difference.
		//
		//	My 2008 MacBook's GeForce 9400M does fine at 1920×1080.
		//
		//	To allow for older hardware, set a conservative 1024×1024 limit.
		//	After a few years, that limit may be increased, as older hardware falls out of use.
		//	In the meantime, I may need to temporarily increase the limit
		//	if I want to generate HD video.
		//
		
		//	2012-02-03  Rather than refusing to create the image,
		//	let's just rescale it to an acceptable size.
		
		//	2012-06-11  People occasionally write to complain
		//	about the artificial limit on the resolution.
		//	As an imperfect -- but hopefully good -- compromise,
		//	let's limit the image size when running under OpenGL 2,
		//	but leave it unmodified under OpenGL 3.
		
		//	2013-05-04  The Geometry Games now require OpenGL 3.
		//	Attempt to create the image at the requested size
		//	and hope for the best.

	//	if (theWidth  > 1024)
	//	{
	//		theHeight	= theHeight * 1024 / theWidth;
	//		theWidth	= 1024;
	//	}
	//	if (theHeight > 1024)
	//	{
	//		theWidth	= theWidth  * 1024 / theHeight;
	//		theHeight	= 1024;
	//	}
//	}

	//	Create an empty bitmap of the correct dimensions.
	//	By default the NSBitmapImageRep assumes premultiplied alpha.
	theBitmap = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:	NULL
		pixelsWide:					theWidth
		pixelsHigh:					theHeight
		bitsPerSample:				8
		samplesPerPixel:			4
		hasAlpha:					YES
		isPlanar:					NO
		colorSpaceName:				NSDeviceRGBColorSpace
		bytesPerRow:				theWidth * 4
		bitsPerPixel:				32
		];
	if (theBitmap == nil)
	{
		theError = u"Couldn't allocate theBitmap.";
		goto CleanUpImageAsBitmapImageRep;
	}
	
	//	Get a pointer to theBitmap's pixels.
	thePixels = (PixelRGBA *) [theBitmap bitmapData];
	
	//	Let the subclass provide the pixels.
	theError = [self writeImageToBuffer:thePixels width:theWidth height:theHeight];
	if (theError != NULL)
		goto CleanUpImageAsBitmapImageRep;
	
	//	OpenGL reads the image bottom-to-top, 
	//	but Cocoa seems to be expecting it top-to-bottom.
	//	So we must reverse the rows.
	InvertRawImage(theWidth, theHeight, thePixels);
	
CleanUpImageAsBitmapImageRep:

	if (theError != NULL)
	{
		ErrorMessage(theError, u"Error in -imageAsBitmapImageRep");
		theBitmap = nil;
	}
	
	return theBitmap;
}

- (ErrorText)writeImageToBuffer:(PixelRGBA *)aPixelBuffer width:(unsigned int)aWidth height:(unsigned int)aHeight
{
	ErrorText		theError	= NULL;
	ModelData		*md			= NULL;
	GraphicsDataGL	*gd			= NULL;

	//	Write the image rows into aPixelBuffer 
	//	in OpenGL's bottom-to-top row order.
	//	The caller will convert to Cocoa's top-to-bottom format.
	
	//	Write each PixelRGBA with premultiplied alpha. 

	//	Lock the model data before locking the context, because 
	//	that's the order in which other parts of the program do it.
	//	If we used the opposite order, we'd risk deadlock.
	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	[self lockAndSetContext];

	theError = RenderToBuffer(	md,
								gd,
								true,		//	May use multisampling even if live animation does not.
								itsDepthBufferFlag,
								&Render,	//	in <ProgramName>Graphics-OpenGL.c
								aWidth, aHeight, aPixelBuffer);

	[self unsetAndUnlockContext];
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];
	
	return theError;
}


@end
