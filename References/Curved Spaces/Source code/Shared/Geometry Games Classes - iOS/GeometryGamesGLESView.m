//	GeometryGamesGLESView.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#import "GeometryGamesGLESView.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesGraphicsDataGL.h"
#import "GeometryGames-OpenGL.h"
#import "GeometryGamesUtilities-Common.h"
#import "GeometryGamesUtilities-iOS.h"
#import <QuartzCore/QuartzCore.h>	//	for CAEAGLLayer


//	OpenGL ES supports GL_DEPTH_COMPONENT16 and GL_DEPTH_COMPONENT24_OES.
#define DEPTH_BUFFER_DEPTH	GL_DEPTH_COMPONENT16


//	Privately-declared methods
@interface GeometryGamesGLESView()
- (bool)setUpFramebuffer;
- (void)shutDownFramebuffer;
@end


@implementation GeometryGamesGLESView
{
	//	OpenGL framebuffer names for multisampling
	unsigned int	itsMultisampleFramebuffer,
					itsMultisampleColorRenderbuffer,
					itsMultisampleDepthStencilRenderbuffer,
					itsResolveFramebuffer,
					itsResolveColorRenderbuffer;

	//	OpenGL framebuffer names for single-sampling
	unsigned int	itsFramebuffer,
					itsColorRenderbuffer,
					itsDepthStencilRenderbuffer;

	//	Ready to render?
	bool			itsFramebufferIsReady;
}


+ (Class)layerClass
{
	//	Specify the class we want to use to create 
	//	the Core Animation layer for this view.  
	//	Instead of the default CALayer, request a CAEAGLLayer.
	return [CAEAGLLayer class];
}


- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag
{
	CAEAGLLayer	*theEAGLLayer	= nil;

	self = [super initWithModel:aModel frame:aFrame multisampling:aMultisamplingFlag
		depthBuffer:aDepthBufferFlag stencilBuffer:aStencilBufferFlag];
	if (self != nil)
	{
		itsGraphicsDataGL = [[GeometryGamesGraphicsDataGL alloc] init];
		if (itsGraphicsDataGL == nil)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		
		itsContext = nil;	//	gets set below

		itsMultisampleFramebuffer				= 0;
		itsMultisampleColorRenderbuffer			= 0;
		itsMultisampleDepthStencilRenderbuffer	= 0;
		itsResolveFramebuffer					= 0;
		itsResolveColorRenderbuffer				= 0;
		itsFramebuffer							= 0;
		itsColorRenderbuffer					= 0;
		itsDepthStencilRenderbuffer				= 0;

		itsFramebufferIsReady	= false;

		//	Set the pixel format.
		theEAGLLayer = (CAEAGLLayer *)[self layer];
		[theEAGLLayer setOpaque:YES];
		[theEAGLLayer setDrawableProperties:
			@{
				kEAGLDrawablePropertyRetainedBacking : @NO,
				    kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
			}
		];

		//	Create an OpenGL ES 2 context.
		itsContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		if (itsContext == nil
		 || ! [EAGLContext setCurrentContext:itsContext])
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
	}
	return self;
}


- (void)setOpenGLContext
{
	//	Note #1.  Assume the ModelData has already been locked,
	//	for consistency with the Mac OS version of the program
	//	which, by convention, always locks the ModelData
	//	before locking the context, and then later
	//	unlocks them in the reverse order.
	//	By following a consistent convention
	//	it avoids all risk of deadlock.

	//	Note #2.  In the purest case, the view controller
	//	would handle menu commands and update the ModelData,
	//	with no need to interact with the view.
	//	The flaw in that plan is that the ModelData may want
	//	to cache OpenGL objects.  For example, KaleidoPaint
	//	caches an OpenGL texture for each drawing's unit cell, 
	//	so thumbnails may be drawn quickly.  Such optimizations
	//	break the otherwise strict separation between the abstract
	//	representation of the drawing and its realization in this view.  
	//	Fortunately such optimizations are easy to accommodate:
	//	the view controller need only set and unset the graphics
	//	context whenever it creates or deletes OpenGL objects.

	[EAGLContext setCurrentContext:itsContext];
	//	itsContext is ready to use...
}

- (void)unsetOpenGLContext
{
	//	...done using itsContext.
	[EAGLContext setCurrentContext:nil];
}


- (void)setUpGraphics
{
	//	In the OpenGL(ES) implementation,
	//	SetUpGraphicsAsNeeded() gets called once per frame,
	//	so no need to set anything up explicitly here.
}

- (void)shutDownGraphics
{
	ModelData		*md	= NULL;
	GraphicsDataGL	*gd	= NULL;

	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	[self setOpenGLContext];
	ShutDownGraphicsAsNeeded(md, gd);
	[self unsetOpenGLContext];
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];
}


- (void)layoutSubviews	//	View size may have changed.
{
	//	In the case of a GeometryGamesGLESView, a call to -layoutSubviews
	//	has absolutely nothing to do with subviews, because the view has no subviews.
	//	Rather the call is telling us that the view's dimensions may have changed.
	//	In response we should replace the framebuffer with a new framebuffer of the correct size.
	[self shutDownFramebuffer];
	itsFramebufferIsReady = [self setUpFramebuffer];

	if (itsFramebufferIsReady)
	{
		[self drawView];
	}
	else
	{
		//	With split-screen multitasking on iOS 9, when the user slides
		//	the app entirely off the screen, our call to setUpFramebuffer
		//	fails because the system gives it a framebuffer of width zero
		//	and height zero.  We can hope that some future version of iOS
		//	will fix that bug.  In the meantime, let's request another call
		//	to layoutSubviews, once things have had a chance to settle down
		//	a bit.  (Yes, I know, this is an ugly hack.  But I don't see
		//	any clean solution.)
		//
		//	Note:  Metal on iOS 9 seems not to have this problem,
		//	perhaps for the simple reason that we request the framebuffer resize
		//	ourselves, based on the reported view size which seems to never be zero.
		//	So by migrating from OpenGL ES to Metal, this bug goes away.
		//
		[self performSelector:@selector(setNeedsLayout) withObject:nil afterDelay:0.1];
	}
}

- (bool)setUpFramebuffer
{
	bool	theReturnValue	= true;
	GLuint	theNumSamples;

	//	Note:  We don't need to use the ModelData, and the app is running 
	//	in a single thread, but if iOS apps someday become multithreaded by default,
	//	then we'll want to respect the convention of locking the model data 
	//	before "locking" (currently meaning "using") the context,
	//	and thus avoiding even the remotest chance of deadlock.
//	[itsModel lockModelData:&md];
	[EAGLContext setCurrentContext:itsContext];

	if (itsMultisamplingFlag)
	{
		glGetIntegerv(GL_MAX_SAMPLES_APPLE, (int *)&theNumSamples);

		glGenFramebuffers(1, &itsResolveFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, itsResolveFramebuffer);

		glGenRenderbuffers(1, &itsResolveColorRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, itsResolveColorRenderbuffer);
		[itsContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)[self layer]];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, itsResolveColorRenderbuffer);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			NSLog(@"The “resolve framebuffer” is incomplete.  Error code: %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
			theReturnValue = false;
			goto CleanUpSetUpFramebuffer;
		}

		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &itsFramebufferWidthPx );
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &itsFramebufferHeightPx);

		glGenFramebuffers(1, &itsMultisampleFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, itsMultisampleFramebuffer);

		glGenRenderbuffers(1, &itsMultisampleColorRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, itsMultisampleColorRenderbuffer);
		glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, theNumSamples, GL_RGBA8_OES, itsFramebufferWidthPx, itsFramebufferHeightPx);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, itsMultisampleColorRenderbuffer);

		//	iOS implements a stencil buffer as part of a combined depth-stencil buffer,
		//	via the GL_OES_packed_depth_stencil extension, available on iOS 4.2 and up.
		if (itsDepthBufferFlag || itsStencilBufferFlag)
		{
			glGenRenderbuffers(1, &itsMultisampleDepthStencilRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, itsMultisampleDepthStencilRenderbuffer);
			glRenderbufferStorageMultisampleAPPLE(	GL_RENDERBUFFER,
													theNumSamples,
													itsStencilBufferFlag ? GL_DEPTH24_STENCIL8_OES : DEPTH_BUFFER_DEPTH,
													itsFramebufferWidthPx,
													itsFramebufferHeightPx);
			if (itsDepthBufferFlag)
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, itsMultisampleDepthStencilRenderbuffer);
			if (itsStencilBufferFlag)
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, itsMultisampleDepthStencilRenderbuffer);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			NSLog(@"The “multisample framebuffer” is incomplete.  Error code: %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
			theReturnValue = false;
			goto CleanUpSetUpFramebuffer;
		}
	}
	else	//	single-sampling
	{
		glGenFramebuffers(1, &itsFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, itsFramebuffer);

		glGenRenderbuffers(1, &itsColorRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, itsColorRenderbuffer);
		[itsContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)[self layer]];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, itsColorRenderbuffer);

		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &itsFramebufferWidthPx );
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &itsFramebufferHeightPx);
		if (itsFramebufferWidthPx  == 0	//	Can occur on iOS 9 when app gets completely hidden
		 || itsFramebufferHeightPx == 0)	//		during split-screen multitasking.
		{
			NSLog(@"Invalid framebuffer dimensions (%d %d)", itsFramebufferWidthPx, itsFramebufferHeightPx);
			theReturnValue = false;
			goto CleanUpSetUpFramebuffer;
		}

		//	See comments above about combined depth-stencil buffer.
		if (itsDepthBufferFlag || itsStencilBufferFlag)
		{
			glGenRenderbuffers(1, &itsDepthStencilRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, itsDepthStencilRenderbuffer);
			glRenderbufferStorage(	GL_RENDERBUFFER,
									itsStencilBufferFlag ? GL_DEPTH24_STENCIL8_OES : DEPTH_BUFFER_DEPTH,
									itsFramebufferWidthPx,
									itsFramebufferHeightPx);
			if (itsDepthBufferFlag)
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, itsDepthStencilRenderbuffer);
			if (itsStencilBufferFlag)
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, itsDepthStencilRenderbuffer);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			NSLog(@"The single-sample framebuffer is incomplete.  Error code: %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
			theReturnValue = false;
			goto CleanUpSetUpFramebuffer;
		}
	}

CleanUpSetUpFramebuffer:

	[EAGLContext setCurrentContext:nil];
//	[itsModel unlockModelData:&md];

	return theReturnValue;
}

- (void)shutDownFramebuffer
{
//	[itsModel lockModelData:&md];
	[EAGLContext setCurrentContext:itsContext];

	if (itsMultisamplingFlag)
	{
		glDeleteFramebuffers(1, &itsMultisampleFramebuffer);
		itsMultisampleFramebuffer = 0;

		glDeleteRenderbuffers(1, &itsMultisampleColorRenderbuffer);
		itsMultisampleColorRenderbuffer = 0;

		if (itsDepthBufferFlag || itsStencilBufferFlag)
		{
			glDeleteRenderbuffers(1, &itsMultisampleDepthStencilRenderbuffer);
			itsMultisampleDepthStencilRenderbuffer = 0;
		}

		glDeleteFramebuffers(1, &itsResolveFramebuffer);
		itsResolveFramebuffer = 0;

		glDeleteRenderbuffers(1, &itsResolveColorRenderbuffer);
		itsResolveColorRenderbuffer = 0;
	}
	else	//	single-sampling
	{
		glDeleteFramebuffers(1, &itsFramebuffer);
		itsFramebuffer = 0;

		glDeleteRenderbuffers(1, &itsColorRenderbuffer);
		itsColorRenderbuffer = 0;

		if (itsDepthBufferFlag || itsStencilBufferFlag)
		{
			glDeleteRenderbuffers(1, &itsDepthStencilRenderbuffer);
			itsDepthStencilRenderbuffer = 0;
		}
	}

	itsFramebufferWidthPx	= 0;
	itsFramebufferHeightPx	= 0;

	[EAGLContext setCurrentContext:nil];
//	[itsModel unlockModelData:&md];
}


- (void)drawView
{
	//	Apple's OpenGL ES Programming Guide for iOS says
	//	
	//		When you present a frame, Core Animation caches the frame 
	//		and uses it until a new frame is presented. 
	//		By only rendering new frames when you need to, 
	//		you conserve battery power on the device, 
	//		and leave more time for the device to perform other actions.
	//		
	//		Note: An OpenGL ES-aware view should not implement 
	//		a drawRect: method to render the view’s contents; 
	//		instead, implement your own method to draw and present 
	//		a new OpenGL ES frame and call it when your data changes. 
	//		Implementing a drawRect: method causes other changes 
	//		in how UIKit handles your view.
	//
	//	Thus GeometryGamesGLESView does not implement -drawRect,
	//	but implements this custom -drawView method instead.

	ModelData		*md				= NULL;
	GraphicsDataGL	*gd				= NULL;
	ErrorText		theSetUpError	= NULL,
					theRenderError	= NULL;
	unsigned int	theNumDiscardBuffers;
	GLenum			theDiscardBuffers[3];

	if ( ! itsFramebufferIsReady )
		return;

	//	Lock itsModel and itsGraphicsDataGL (in that order)
	//	before setting the context, if only for consistency
	//	with Geometry Games' Mac conventions
	//	and for possible future robustness if the iOS version
	//	ever becomes multithreaded and needs to lock itsContext.

	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	[EAGLContext setCurrentContext:itsContext];

	theSetUpError = SetUpGraphicsAsNeeded(md, gd);

	if (itsMultisamplingFlag)
		glBindFramebuffer(GL_FRAMEBUFFER, itsMultisampleFramebuffer);
	else
		glBindFramebuffer(GL_FRAMEBUFFER, itsFramebuffer);

	//	The framebuffer will almost surely be complete,
	//	but in rare circumstance it might not be.
	//	For example, in Move & Turn the animation timer
	//	might try to draw a polyhedron in a presented view
	//	before the view has been placed into the window.
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		if (theSetUpError == NULL)
			theRenderError = Render(md, gd, itsFramebufferWidthPx, itsFramebufferHeightPx, NULL);
		else
		{
			glClearColor(1.0, 0.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		if (itsMultisamplingFlag)
		{
			//	Resolve itsMultisampleFramebuffer into itsResolveFramebuffer.
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, itsResolveFramebuffer);
			glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, itsMultisampleFramebuffer);
			glResolveMultisampleFramebufferAPPLE();
		}

		//	Let the graphics driver know that it does not need
		//	to preserve the contents of itsMultisampleColorRenderbuffer
		//	and itsMultisampleDepthStencilRenderbuffer (if multisampling)
		//	or itsDepthStencilRenderbuffer (if single-sampling).
		//	For details, see the Overview section of
		//
		//		www.khronos.org/registry/gles/extensions/EXT/EXT_discard_framebuffer.txt
		//
		//	glDiscardFramebufferEXT is available on iOS 4.0 and later,
		//	on all hardware, so we don't explicitly test for its presence.

		if (itsMultisamplingFlag)
			glBindFramebuffer(GL_FRAMEBUFFER, itsMultisampleFramebuffer);

		theNumDiscardBuffers = 0;

		if (itsMultisamplingFlag
		 && theNumDiscardBuffers < BUFFER_LENGTH(theDiscardBuffers))
		{
			theDiscardBuffers[theNumDiscardBuffers++] = GL_COLOR_ATTACHMENT0;
		}

		if (itsDepthBufferFlag
		 && theNumDiscardBuffers < BUFFER_LENGTH(theDiscardBuffers))
		{
			theDiscardBuffers[theNumDiscardBuffers++] = GL_DEPTH_ATTACHMENT;
		}

		if (itsStencilBufferFlag
		 && theNumDiscardBuffers < BUFFER_LENGTH(theDiscardBuffers))
		{
			theDiscardBuffers[theNumDiscardBuffers++] = GL_STENCIL_ATTACHMENT;
		}

		glDiscardFramebufferEXT(GL_FRAMEBUFFER, theNumDiscardBuffers, theDiscardBuffers);

		//	Present its(Resolve)ColorRenderbuffer to the user.
		//	This will implicitly discard the buffer's contents, 
		//	in the sense of glDiscardFramebufferEXT(), because we created
		//	a context with kEAGLDrawablePropertyRetainedBacking = NO.
		if (itsMultisamplingFlag)
			glBindRenderbuffer(GL_RENDERBUFFER, itsResolveColorRenderbuffer);
		else	//	single-sampling
			glBindRenderbuffer(GL_RENDERBUFFER, itsColorRenderbuffer);
		[itsContext presentRenderbuffer:GL_RENDERBUFFER];
	}

	[EAGLContext setCurrentContext:nil];
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];

	//	Check for errors.
	//
	//	Note:  On iOS, FatalError() terminates the app asynchronously.
	if (theSetUpError != NULL)
		FatalError(theSetUpError, u"OpenGL Setup Error");
	if (theRenderError != NULL)
		FatalError(theRenderError, u"OpenGL Rendering Error");
}


#pragma mark -
#pragma mark Export actions

- (UIImage *)imageWithSize:(CGSize)anImageSize	//	in pixels, not points
{
	ErrorText		theError		= NULL;
	PixelRGBA		*thePixels		= NULL;
	ModelData		*md				= NULL;
	GraphicsDataGL	*gd				= NULL;
	UIImage			*theImage		= nil;

	//	Allocate thePixels.
	thePixels = GET_MEMORY(anImageSize.width * anImageSize.height * sizeof(PixelRGBA));
	if (thePixels == NULL)
	{
		theError = u"Couldn't get memory to Copy/Save image.";
		goto CleanUpImage;
	}
	
	//	Draw the image into thePixels.
	[itsModel lockModelData:&md];
	[itsGraphicsDataGL lockGraphicsDataGL:&gd];
	[EAGLContext setCurrentContext:itsContext];
	SetUpGraphicsAsNeeded(md, gd);
	theError = RenderToBuffer(	md,
								gd,
								itsMultisamplingFlag,	//	May use multisampling even if live animation does not.
								itsDepthBufferFlag,
								&Render,				//	in <ProgramName>Graphics-OpenGL.c
								anImageSize.width, anImageSize.height, thePixels);
	[EAGLContext setCurrentContext:nil];
	[itsGraphicsDataGL unlockGraphicsDataGL:&gd];
	[itsModel unlockModelData:&md];
	if (theError != NULL)
		goto CleanUpImage;
	
	//	theImage takes ownership of thePixels, then sets thePixels = NULL.
	theImage = ImageWithPixels(anImageSize, &thePixels);

CleanUpImage:

	FREE_MEMORY_SAFELY(thePixels);
	
	//	On iOS, FatalError() terminates the app asynchronously.
	if (theError != NULL)
		FatalError(theError, u"Error in -image");

	return theImage;
}

- (unsigned int)maxFramebufferSize
{
	GLuint	theMaxRenderbufferSizePx;

	[EAGLContext setCurrentContext:itsContext];
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,	(int *)&theMaxRenderbufferSizePx);
	[EAGLContext setCurrentContext:nil];
	
	return theMaxRenderbufferSizePx;
}

@end

#endif	//	SUPPORT_OPENGL
