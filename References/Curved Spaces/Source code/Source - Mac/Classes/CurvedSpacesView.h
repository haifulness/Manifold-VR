//	CurvedSpacesView.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesViewMac.h"


//	A quick-and-dirty hack for saving the frames to a QuickTime movie.
//#define SAVE_ANIMATION
//#define HIGH_DEF_ANIMATION			//	See comment in -imageAsBitmapImageRepOfSize
//#define SAVE_ANIMATION_CODEC	@"tiff"	//	TIFF looks great, but files are huge.
//#define SAVE_ANIMATION_CODEC	@"mp4v"	//	MP4V looks good, files are small, but half goes missing.
//#define SAVE_ANIMATION_CODEC	@"png "	//	PNG  looks good, medium size files


@interface CurvedSpacesView : GeometryGamesViewMac

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(NSRect)aFrame;

- (void)requestShaderUpdate;
- (void)requestTextureUpdate;
- (void)requestVBOUpdate;

#ifdef CURVED_SPACES_MOUSE_INTERFACE
- (void)handleApplicationWillResignActiveNotification:(NSNotification *)aNotification;
- (void)handleApplicationDidBecomeActiveNotification:(NSNotification *)aNotification;
- (void)handleWindowDidMiniaturizeNotification:(NSNotification *)aNotification;
- (void)handleWindowDidDeminiaturizeNotification:(NSNotification *)aNotification;
#endif

#ifdef SAVE_ANIMATION
- (CVReturn)updateAnimation;
#endif

- (void)keyDown:(NSEvent *)anEvent;

#ifdef CURVED_SPACES_MOUSE_INTERFACE
- (void)enterNavigationalMode;
- (void)exitNavigationalMode;
- (void)mouseMoved:(NSEvent *)anEvent;
- (void)mouseDown:(NSEvent *)anEvent;
- (void)rightMouseDown:(NSEvent *)anEvent;
- (void)errorWithTitle:(ErrorText)aTitle message:(ErrorText)aMessage;
#endif
- (void)scrollWheel:(NSEvent *)anEvent;
#ifdef CURVED_SPACES_TOUCH_INTERFACE
- (void)mouseDragged:(NSEvent *)anEvent;
#endif

#ifdef SAVE_ANIMATION
- (void)qtPlayMovieToFile:(bool)aSaveToFileFlag;
#endif

@end
