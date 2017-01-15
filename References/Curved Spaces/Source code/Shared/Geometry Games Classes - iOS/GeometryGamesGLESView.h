//	GeometryGamesGLESView.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#import "GeometryGamesView.h"


@class	GeometryGamesGraphicsDataGL,
		EAGLContext;


@interface GeometryGamesGLESView : GeometryGamesView
{
	GeometryGamesGraphicsDataGL	*itsGraphicsDataGL;
	EAGLContext					*itsContext;
}

+ (Class)layerClass;

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag;

- (void)setOpenGLContext;
- (void)unsetOpenGLContext;

- (void)setUpGraphics;
- (void)shutDownGraphics;

- (void)layoutSubviews;

- (void)drawView;

- (UIImage *)imageWithSize:(CGSize)anImageSize;
- (unsigned int)maxFramebufferSize;

@end

#endif	//	SUPPORT_OPENGL
