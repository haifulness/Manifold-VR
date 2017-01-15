//	GeometryGamesViewMac.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Cocoa/Cocoa.h>
#import <CoreVideo/CVDisplayLink.h>
#import "GeometryGamesUtilities-Common.h"


//	Log the frame rate to the window title?
//	For testing purposes only.
//#define LOG_FRAME_RATE

//	When logging the frame rate to the window title,
//	average over the last several frames to avoid jumpiness.
#ifdef LOG_FRAME_RATE
#define LOG_NUM_FRAMES	8
#endif

//	Virtual key codes
#define       ENTER_KEY		 36
#define         TAB_KEY		 48
#define    SPACEBAR_KEY		 49
#define      DELETE_KEY		 51
#define	     ESCAPE_KEY		 53
#define  LEFT_ARROW_KEY		123
#define	RIGHT_ARROW_KEY		124
#define  DOWN_ARROW_KEY		125
#define    UP_ARROW_KEY		126


@class	GeometryGamesModel,
		GeometryGamesGraphicsDataGL;

//	Let GeometryGamesViewMac inherit from NSView instead of NSOpenGLView
//	and create the context manually.
@interface GeometryGamesViewMac : NSView
{
	GeometryGamesModel			*itsModel;
	GeometryGamesGraphicsDataGL	*itsGraphicsDataGL;
	NSOpenGLPixelFormat			*itsPixelFormat;
	NSOpenGLContext				*itsContext;

	bool						itsOpacityFlag,
								itsDepthBufferFlag,
								itsMultisamplingFlag,
								itsStereoBuffersFlag,
								itsStencilBufferFlag;
	
	bool						itsDrawErrorHasOccurred;
	
	CVDisplayLinkRef			itsDisplayLink;
	double						itsLastUpdateTime;		//	in seconds, from arbitrary start time

#ifdef LOG_FRAME_RATE
	unsigned int				itsElapsedTimes[LOG_NUM_FRAMES];	//	in nanoseconds,
																	//	automatically initialized to 0
#endif
}

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(NSRect)aFrame opaque:(bool)anOpacityFlag
	depthBuffer:(bool)aDepthBufferFlag multisampling:(bool)aMultisamplingFlag
	stereo:(bool)aStereoBuffersFlag stencilBuffer:(bool)aStencilBufferFlag;
- (void)dealloc;

- (void)viewDidMoveToWindow;

- (void)handleApplicationWillResignActiveNotification:(NSNotification *)aNotification;
- (void)handleApplicationDidBecomeActiveNotification:(NSNotification *)aNotification;
- (void)handleWindowDidMiniaturizeNotification:(NSNotification *)aNotification;
- (void)handleWindowDidDeminiaturizeNotification:(NSNotification *)aNotification;

- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)anEvent;

- (void)lockAndSetContext;
- (void)unsetAndUnlockContext;

- (void)lockFocus;
- (BOOL)isOpaque;
- (void)handleViewGlobalFrameDidChangeNotification:(NSNotification *)aNotification;

- (void)pauseAnimation;
- (void)resumeAnimation;
- (CVReturn)updateAnimation;
- (void)drawRect:(NSRect)dirtyRect;

- (DisplayPoint)mouseLocation:(NSEvent *)anEvent;
- (DisplayPointMotion)mouseDisplacement:(NSEvent *)anEvent;

- (NSData *)imageAsTIFF;
- (NSData *)imageAsTIFFofSize:(NSSize)anImageSizePx;
- (NSBitmapImageRep *)imageAsBitmapImageRepOfSize:(NSSize)anImageSizePx;
- (ErrorText)writeImageToBuffer:(PixelRGBA *)aPixelBuffer width:(unsigned int)aWidth height:(unsigned int)aHeight;

@end
