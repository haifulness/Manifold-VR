//	GeometryGamesView.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>


@class	GeometryGamesModel;

@interface GeometryGamesView : UIView
{
	GeometryGamesModel	*itsModel;

	//	Options flags
	bool				itsMultisamplingFlag,
						itsDepthBufferFlag,
						itsStencilBufferFlag;

	//	With Metal, if drawView (and maybe other code too) were to re-compute
	//	the framebuffer size as [self bounds] * contentScaleFactor,
	//	we'd need to worry about roundoff leading to an off-by-one error
	//	in the computed framebuffer size.  Yes, I know, this is unlikely
	//	to be a problem, but even still it seems safer
	//	to cache the framebuffer size once and for all.
	//
	//	With OpenGL(ES), we record the framebuffer dimensions
	//	when we create the framebuffer, then use them in every Render() call.
	//
	GLint	itsFramebufferWidthPx,
			itsFramebufferHeightPx;
}

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag;

- (void)setUpGraphics;
- (void)shutDownGraphics;

- (void)updateForElapsedTime:(CFTimeInterval)anElapsedTime;

- (void)drawView;

- (void)saveImage;
- (void)copyImage;
- (UIImage *)image;
- (UIImage *)imageWithSize:(CGSize)anImageSize;
- (unsigned int)maxExportedImageMagnificationFactor;
- (unsigned int)maxFramebufferSize;

@end
