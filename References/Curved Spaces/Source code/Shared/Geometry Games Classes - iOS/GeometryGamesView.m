//	GeometryGamesView.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesView.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesUtilities-Common.h"


//	Privately-declared methods
@interface GeometryGamesView()
@end


@implementation GeometryGamesView
{
}

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag
{
	self = [super initWithFrame:aFrame];
	if (self != nil)
	{
		itsModel = aModel;
		if (itsModel == nil)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}

		itsMultisamplingFlag	= aMultisamplingFlag;
		itsDepthBufferFlag		= aDepthBufferFlag;
		itsStencilBufferFlag	= aStencilBufferFlag;

		//	Full-resolution drawing on retina displays is the default
		//	for most views, but not for views backed by a CAEAGLLayer,
		//	because of the performance penalty of quadrupling (or more)
		//	the number of pixels.  Let's opt in to full-resolution
		//	even for OpenGL ES views.  The Geometry Games use pretty simple
		//	fragment shaders, so the cost of evaluating all those pixels
		//	isn't so bad.
		//
		//		Note:  An iPhone 6 Plus normally draws its 412×736 pt layout
		//		at exactly 3× into a virtual 1242×2208 px screen, which then
		//		gets downsampled to the physical 1080×1920 display.  See
		//
		//			www.paintcodeapp.com/news/iphone-6-screens-demystified
		//
		//		for a well-illustrated explanation.  Thus on the iPhone 6 Plus,
		//
		//				scale		= 3.0
		//				nativeScale	= 2.60869565217391
		//
		//		The page
		//
		//			http://oleb.net/blog/2014/11/iphone-6-plus-screen/
		//
		//		goes on to explain that by setting a view's contentScaleFactor
		//		to nativeScale rather than plain scale, it can avoid
		//		the downsampling phase altogether (and thus also render fewer pixels).
		//
		[self setContentScaleFactor:[[UIScreen mainScreen] nativeScale]];
	}
	return self;
}


- (void)setUpGraphics
{
	//	GeometryGamesMetalView and GeometryGamesGLESView override this method.
}

- (void)shutDownGraphics
{
	//	GeometryGamesMetalView and GeometryGamesGLESView override this method.
}


- (void)updateForElapsedTime:(CFTimeInterval)anElapsedTime
{
//NSLog(@"updateForElapsedTime");
	ModelData	*md				= NULL;
	bool		theSimulationWantsUpdates;

	if ( ! [self isHidden])
	{
		[itsModel lockModelData:&md];
		theSimulationWantsUpdates = SimulationWantsUpdates(md);
		[itsModel unlockModelData:&md];

		if (theSimulationWantsUpdates)
		{
			//	Update the simulation.
			[itsModel lockModelData:&md];
			SimulationUpdate(md, anElapsedTime);
			[itsModel unlockModelData:&md];

			//	Redraw the view.
			[self drawView];
		}
	}
}


- (void)drawView
{
	//	GeometryGamesMetalView and GeometryGamesGLESView override this method.
}


#pragma mark -
#pragma mark Export actions

- (void)saveImage
{
	NSData	*thePNGData;
	UIImage	*thePNGImage;
	
	//	To write a JPEG...
//	UIImageWriteToSavedPhotosAlbum([self image], nil, nil, nil);

	//	To write a PNG...
	thePNGData	= UIImagePNGRepresentation([self image]);
	thePNGImage	= [UIImage imageWithData:thePNGData];
	UIImageWriteToSavedPhotosAlbum(thePNGImage, nil, nil, nil);
}

- (void)copyImage
{
	[[UIPasteboard generalPasteboard]
		setData:			UIImagePNGRepresentation([self image])
		forPasteboardType:	@"public.png"];
}

- (UIImage *)image
{
	CGSize			theImageSizePt,
					theExportImageSize;
	unsigned int	theMagnificationFactor;

	theImageSizePt			= [self bounds].size;
	theMagnificationFactor	= GetUserPrefInt(u"exported image magnification factor");
	theExportImageSize		= (CGSize) {
								theMagnificationFactor * theImageSizePt.width,
								theMagnificationFactor * theImageSizePt.height };

	return [self imageWithSize:theExportImageSize];
}

- (UIImage *)imageWithSize:(CGSize)anImageSize	//	in pixels, not points
{
	//	GeometryGamesMetalView and GeometryGamesGLESView override this method.
	
	UNUSED_PARAMETER(anImageSize);
	
	return nil;
}

- (unsigned int)maxExportedImageMagnificationFactor
{
	CGSize			theViewImageSizePt;
	CGFloat			theLargerSidePt;
	GLuint			theMaxFramebufferSizePx;
	unsigned int	theMaxExportedImageMagnificationFactor;

	theViewImageSizePt	= [self bounds].size;
	theLargerSidePt		= MAX(theViewImageSizePt.width, theViewImageSizePt.height);
	if (theLargerSidePt <= 0.0)
		return 1;	//	should never occur

	theMaxFramebufferSizePx = [self maxFramebufferSize];
	
	theMaxExportedImageMagnificationFactor
		= (unsigned int) floor(theMaxFramebufferSizePx / theLargerSidePt);
	
	if (theMaxExportedImageMagnificationFactor < 1)	//	should never occur
		theMaxExportedImageMagnificationFactor = 1;
	
	return theMaxExportedImageMagnificationFactor;
}

- (unsigned int)maxFramebufferSize
{
	//	GeometryGamesMetalView and GeometryGamesGLESView override this method.
	
	return 1;
}

@end
