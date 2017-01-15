//	GeometryGamesMetalView.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_METAL

#import "GeometryGamesMetalView.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesUtilities-Common.h"
#import "GeometryGamesUtilities-iOS.h"


//	Privately-declared methods
@interface GeometryGamesMetalView()
- (MTLRenderPassDescriptor *)renderPassDescriptorUsingDrawable:(id<CAMetalDrawable>)aDrawable;
@end


@implementation GeometryGamesMetalView
{
	CAMetalLayer			* __weak itsMetalLayer;

	id<MTLCommandQueue>		itsCommandQueue;
	
	id<MTLTexture>			itsMultisampleBuffer,
							itsDepthBuffer,
							itsStencilBuffer;

	//	Keep several instances of each "inflight" data buffer,
	//	so that while the GPU renders a frame using one buffer,
	//	the CPU can already be filling another buffer in preparation
	//	for the next frame.  To synchronize buffer access,
	//	the CPU will wait on itsInflightBufferSemaphore
	//	before writing to the next buffer, and then
	//	the GPU will signal itsInflightBufferSemaphore
	//	when it's done with that buffer.
	unsigned int			itsInflightBufferIndex;	//	= 0, 1, … , (NUM_INFLIGHT_BUFFERS - 1)
	dispatch_semaphore_t	itsInflightBufferSemaphore;
}


+ (Class)layerClass
{
	//	Specify the class we want to use to create 
	//	the Core Animation layer for this view.  
	//	Instead of the default CALayer, request a CAMetalLayer.
	return [CAMetalLayer class];
}


- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag
{
	self = [super initWithModel:aModel frame:aFrame multisampling:aMultisamplingFlag
		depthBuffer:aDepthBufferFlag stencilBuffer:aStencilBufferFlag];
	if (self != nil)
	{
		itsInflightBufferSemaphore	= dispatch_semaphore_create(NUM_INFLIGHT_BUFFERS);
		itsInflightBufferIndex		= 0;

		itsMetalLayer	= (CAMetalLayer *)[self layer];
		itsDevice		= MTLCreateSystemDefaultDevice();
		itsCommandQueue = [itsDevice newCommandQueue];
		
		//	If the caller requested multisampling but the hardware
		//	doesn't support the expected number of samples per pixel,
		//	then disable multisampling.
 		if (itsMultisamplingFlag && ! [itsDevice supportsTextureSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES])
			itsMultisamplingFlag = false;

		[itsMetalLayer setDevice:itsDevice];
		[itsMetalLayer setPixelFormat:MTLPixelFormatBGRA8Unorm];
		[itsMetalLayer setFramebufferOnly:YES];
		
		itsMultisampleBuffer	= nil;	//	to be created upon demand
		itsDepthBuffer			= nil;	//	to be created upon demand
		itsStencilBuffer		= nil;	//	to be created upon demand

		itsFramebufferWidthPx	= 0;	//	to be set in layoutSubview
		itsFramebufferHeightPx	= 0;	//	to be set in layoutSubview
	}
	return self;
}


- (void)setUpGraphics
{
	//	The app-specific subclass will override this method,
	//	and must call this superclass implementation.
}

- (void)shutDownGraphics
{
	//	The app-specific subclass will override this method,
	//	and must call this superclass implementation.
	
	//	Clear our references to the multisample, depth and stencil buffers (if present),
	//	so that they maybe deallocated.  Our renderPassDescriptorUsingDrawable method
	//	will re-create them when needed.
	itsMultisampleBuffer	= nil;
	itsDepthBuffer			= nil;
	itsStencilBuffer		= nil;
	
	//	Keep our references to the other Metal objects (itsMetalLayer,
	//	itsDevice, itsCommandQueue and also itsInflightBufferSemaphore).
	//	While it might be possible to delete and later re-create, say,
	//	itsCommandQueue, I'd rather not go looking for trouble at this point.
}


- (void)layoutSubviews	//	View size may have changed.
{
//NSLog(@"layoutSubviews for %.1f x %.1f",
//	[self bounds].size.width,
//	[self bounds].size.height);

	//	In the case of a GeometryGamesMetalView, a call to -layoutSubviews
	//	has absolutely nothing to do with subviews, because the view has no subviews.
	//	Rather the call is telling us that the view's dimensions may have changed.
	//	In response we should replace the framebuffer with a new framebuffer of the correct size.

	itsFramebufferWidthPx	= [self contentScaleFactor] * [self bounds].size.width;
	itsFramebufferHeightPx	= [self contentScaleFactor] * [self bounds].size.height;
	
	[itsMetalLayer setDrawableSize:(CGSize){itsFramebufferWidthPx, itsFramebufferHeightPx}];
}


- (void)drawView
{
	NSDictionary<NSString *, id<MTLBuffer>>	*theInflightDataBuffers;	//	app-specific collection of buffers
	id<CAMetalDrawable>						theCurrentDrawable;
	MTLRenderPassDescriptor					*theRenderPassDescriptor;
	id<MTLCommandBuffer>					theCommandBuffer;
//NSLog(@"drawView");

	//	Wait for the next available uniform buffer.
	dispatch_semaphore_wait(itsInflightBufferSemaphore, DISPATCH_TIME_FOREVER);	//	may block while waiting for next available uniform buffer
	itsInflightBufferIndex = (itsInflightBufferIndex + 1) % NUM_INFLIGHT_BUFFERS;

	//	Let the subclass prepare all per-frame data buffers
	//	*before* calling [itsMetalLayer nextDrawable].
	//	This way, the CPU and the GPU may work in parallel,
	//	with the CPU preparing the data buffers for the next frame,
	//	while the GPU finishes rendering the preceding frame.
	theInflightDataBuffers = [self prepareInflightDataBuffersAtIndex:itsInflightBufferIndex];

	//	Ask itsMetalLayer for the next available color buffer.
	//	This call typically blocks while waiting
	//	for the GPU to finish rendering the preceding frame.
	theCurrentDrawable = [itsMetalLayer nextDrawable];	//	typically blocks

	//	The call to nextDrawable shouldn't ever fail.
	//	But if it did ever fail, simply abort this render call
	//	and wait for the animation timer to fire again.
	if (theCurrentDrawable == nil)
	{
		//	Push the current uniform buffer back, so we can reuse it next time
		//	(thus avoiding any risk of writing to a uniform buffer that the GPU is still using).
		//
		//		Note:  No matter what, we must rigorously ensure
		//		that each call to dispatch_semaphore_wait()
		//		gets balanced by a call to dispatch_semaphore_signal().
		//		Otherwise after ~3 unmatched dispatch_semaphore_wait() calls,
		//		the drawing thread (which is also the main thread) would
		//		block forever at the next one.
		//
		itsInflightBufferIndex = (itsInflightBufferIndex + (NUM_INFLIGHT_BUFFERS - 1)) % NUM_INFLIGHT_BUFFERS;
		dispatch_semaphore_signal(itsInflightBufferSemaphore);
		
		return;
	}
	
	//	Create a MTLRenderPassDescriptor using theCurrentDrawable.
	theRenderPassDescriptor = [self renderPassDescriptorUsingDrawable:theCurrentDrawable];
	
	//	Create theCommandBuffer.
	theCommandBuffer = [itsCommandQueue commandBuffer];

	//	Ask theCommandBuffer to signal theUniformBufferSemaphore when it's done rendering this frame,
	//	so we'll know when the next uniform buffer is ready for reuse.
	dispatch_semaphore_t	theUniformBufferSemaphore = itsInflightBufferSemaphore;	//	avoids implicit reference to self
	[theCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> aCommandBuffer)
	{
		UNUSED_PARAMETER(aCommandBuffer);
		dispatch_semaphore_signal(theUniformBufferSemaphore);
	}];
	
	//	Let the subclass encode commands to draw the scene,
	//	using the already-prepared inflight data buffers.
	//
	//		Note:  theRenderPassDescriptor uses MTLLoadActionClear, so the subclass's
	//		encodeCommandsToCommandBuffer:withRenderPassDescriptor:inflightDataBuffers:
	//		should use theRenderPassDescriptor for a single MTLRenderCommandEncoder only.
	//		An app like KaleidoPaint that requires multiple MTLRenderCommandEncoders
	//		will need to modify theRenderPassDescriptor to use MTLLoadActionLoad instead.
	//
	[self encodeCommandsToCommandBuffer: theCommandBuffer
			   withRenderPassDescriptor: theRenderPassDescriptor
					inflightDataBuffers: theInflightDataBuffers];
	
	[theCommandBuffer presentDrawable:theCurrentDrawable];
	[theCommandBuffer commit];
	
	//	The GPU now has sole ownership of theCurrentDrawable.
}

- (NSDictionary<NSString *, id<MTLBuffer>> *)prepareInflightDataBuffersAtIndex:(unsigned int)anInflightBufferIndex	//	called by drawView
{
	UNUSED_PARAMETER(anInflightBufferIndex);
	
	//	Each app-specific subclass will override this method.
	return nil;
}

- (NSDictionary<NSString *, id<MTLBuffer>> *)prepareInflightDataBuffersForOffscreenRenderingAtSize:(CGSize)anImageSize
{
	//	Each app-specific subclass will override this method.

	UNUSED_PARAMETER(anImageSize);

	return nil;
}


- (MTLRenderPassDescriptor *)renderPassDescriptorUsingDrawable:(id<CAMetalDrawable>)aDrawable
{
	id<MTLTexture>								theColorBuffer;
	MTLRenderPassDescriptor						*theRenderPassDescriptor;
	MTLRenderPassColorAttachmentDescriptor		*theColorAttachmentDescriptor;
	MTLRenderPassDepthAttachmentDescriptor		*theDepthAttachmentDescriptor;
	MTLRenderPassStencilAttachmentDescriptor	*theStencilAttachmentDescriptor;
	MTLTextureDescriptor						*theMultisampleTextureDescriptor,
												*theDepthTextureDescriptor,
												*theStencilTextureDescriptor;


	theColorBuffer = [aDrawable texture];

	theRenderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	
	theColorAttachmentDescriptor = [theRenderPassDescriptor colorAttachments][0];
	[theColorAttachmentDescriptor setClearColor:[self clearColor]];		//	opaque for best performance
	[theColorAttachmentDescriptor setLoadAction:MTLLoadActionClear];	//	always MTLLoadActionClear for best performance

	if (itsMultisamplingFlag)
	{
		if (itsMultisampleBuffer == nil
		 || [itsMultisampleBuffer  width] != [theColorBuffer width ]
		 || [itsMultisampleBuffer height] != [theColorBuffer height])
		//	could also check [itsMultisampleBuffer sampleCount] if relevant
		{
			theMultisampleTextureDescriptor = [MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:	MTLPixelFormatBGRA8Unorm
											 width:	[theColorBuffer width]
											height:	[theColorBuffer height]
										 mipmapped:	NO];
			[theMultisampleTextureDescriptor setTextureType:MTLTextureType2DMultisample];
			[theMultisampleTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];	//	must match value in pipeline state
			if ([theMultisampleTextureDescriptor respondsToSelector:@selector(setUsage:)])
				[theMultisampleTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
			if ([theMultisampleTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
				[theMultisampleTextureDescriptor setStorageMode:MTLStorageModePrivate];
				

			itsMultisampleBuffer = [itsDevice newTextureWithDescriptor:theMultisampleTextureDescriptor];
		}

		[theColorAttachmentDescriptor setTexture:itsMultisampleBuffer];
		[theColorAttachmentDescriptor setResolveTexture:theColorBuffer];
		[theColorAttachmentDescriptor setStoreAction:MTLStoreActionMultisampleResolve];
	}
	else	//	! itsMultisamplingFlag
	{
		[theColorAttachmentDescriptor setTexture:theColorBuffer];
		[theColorAttachmentDescriptor setStoreAction:MTLStoreActionStore];
	}
	
	if (itsDepthBufferFlag)
	{
		if (itsDepthBuffer == nil
		 || [itsDepthBuffer  width] != [theColorBuffer width ]
		 || [itsDepthBuffer height] != [theColorBuffer height])
		//	could also check [itsDepthBuffer sampleCount] if relevant
		{
			theDepthTextureDescriptor = [MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:	MTLPixelFormatDepth32Float
											 width:	[theColorBuffer width]
											height:	[theColorBuffer height]
										 mipmapped:	NO];
			if (itsMultisamplingFlag)
			{
				[theDepthTextureDescriptor setTextureType:MTLTextureType2DMultisample];
				[theDepthTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];
			}
			else
			{
				[theDepthTextureDescriptor setTextureType:MTLTextureType2D];
				[theDepthTextureDescriptor setSampleCount:1];
			}
			if ([theDepthTextureDescriptor respondsToSelector:@selector(setUsage:)])
				[theDepthTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
			if ([theDepthTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
				[theDepthTextureDescriptor setStorageMode:MTLStorageModePrivate];

			itsDepthBuffer = [itsDevice newTextureWithDescriptor:theDepthTextureDescriptor];
		}
		
		theDepthAttachmentDescriptor = [theRenderPassDescriptor depthAttachment];
		[theDepthAttachmentDescriptor setTexture:itsDepthBuffer];
		[theDepthAttachmentDescriptor setClearDepth:1.0];
		[theDepthAttachmentDescriptor setLoadAction:MTLLoadActionClear];
		[theDepthAttachmentDescriptor setStoreAction:MTLStoreActionDontCare];
	}
	
	if (itsStencilBufferFlag)
	{
		if (itsStencilBuffer == nil
		 || [itsStencilBuffer  width] != [theColorBuffer width ]
		 || [itsStencilBuffer height] != [theColorBuffer height])
		//	could also check [itsStencilBuffer sampleCount] if relevant
		{
			theStencilTextureDescriptor = [MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:	MTLPixelFormatStencil8
											 width:	[theColorBuffer width]
											height:	[theColorBuffer height]
										 mipmapped:	NO];
			if (itsMultisamplingFlag)
			{
				[theStencilTextureDescriptor setTextureType:MTLTextureType2DMultisample];
				[theStencilTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];
			}
			else
			{
				[theStencilTextureDescriptor setTextureType:MTLTextureType2D];
				[theStencilTextureDescriptor setSampleCount:1];
			}
			if ([theStencilTextureDescriptor respondsToSelector:@selector(setUsage:)])
				[theStencilTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
			if ([theStencilTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
				[theStencilTextureDescriptor setStorageMode:MTLStorageModePrivate];

			itsStencilBuffer = [itsDevice newTextureWithDescriptor:theStencilTextureDescriptor];
		}
		
		theStencilAttachmentDescriptor = [theRenderPassDescriptor stencilAttachment];
		[theStencilAttachmentDescriptor setTexture:itsStencilBuffer];
		[theStencilAttachmentDescriptor setClearStencil:0];
		[theStencilAttachmentDescriptor setLoadAction:MTLLoadActionClear];
		[theStencilAttachmentDescriptor setStoreAction:MTLStoreActionDontCare];
	}
	
	return theRenderPassDescriptor;
}

- (MTLClearColor)clearColor
{
	//	Each app-specific subclass will override this method.
	return MTLClearColorMake(0.0, 0.0, 0.0, 0.0);	//	fully transparent background
}

- (void)encodeCommandsToCommandBuffer:(id<MTLCommandBuffer>)aCommandBuffer		//	called by drawView
	withRenderPassDescriptor:(MTLRenderPassDescriptor *)aRenderPassDescriptor
	inflightDataBuffers:(NSDictionary<NSString *, id<MTLBuffer>> *)someInflightDataBuffers
{
	UNUSED_PARAMETER(aCommandBuffer);
	UNUSED_PARAMETER(aRenderPassDescriptor);
	UNUSED_PARAMETER(someInflightDataBuffers);

	//	Each app-specific subclass will override this method.
}


#pragma mark -
#pragma mark Export actions

- (UIImage *)imageWithSize:(CGSize)anImageSize	//	in pixels, not points
{
	CGFloat										theMaxImageSizePx;
	NSDictionary<NSString *, id<MTLBuffer>>		*theInflightDataBuffers;	//	app-specific collection of buffers
	MTLTextureDescriptor						*theColorBufferDescriptor;
	id<MTLTexture>								theColorBuffer,
												theMultisampleBuffer,
												theDepthBuffer,
												theStencilBuffer;
	MTLRenderPassDescriptor						*theRenderPassDescriptor;
	MTLRenderPassColorAttachmentDescriptor		*theColorAttachmentDescriptor;
	MTLRenderPassDepthAttachmentDescriptor		*theDepthAttachmentDescriptor;
	MTLRenderPassStencilAttachmentDescriptor	*theStencilAttachmentDescriptor;
	MTLTextureDescriptor						*theMultisampleTextureDescriptor,
												*theDepthTextureDescriptor,
												*theStencilTextureDescriptor;
	id<MTLCommandBuffer>						theCommandBuffer;
	PixelRGBA									*thePixels	= NULL;
	UIImage										*theImage	= nil;
	unsigned int								theCount;
	PixelRGBA									*thePixel;
	Byte										theSwapValue;

	//	Clamp anImageSize to the largest allowable framebuffer size.
	theMaxImageSizePx = (CGFloat) [self maxFramebufferSize];
	if (anImageSize.width  > theMaxImageSizePx)
		anImageSize.width  = theMaxImageSizePx;
	if (anImageSize.height > theMaxImageSizePx)
		anImageSize.height = theMaxImageSizePx;
	
	//	Let the subclass prepare all per-frame data buffers.
	theInflightDataBuffers = [self prepareInflightDataBuffersForOffscreenRenderingAtSize:(CGSize)anImageSize];

	//	Create theColorBuffer.
	theColorBufferDescriptor = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:	MTLPixelFormatBGRA8Unorm
									 width:	anImageSize.width
									height:	anImageSize.height
								 mipmapped:	NO];
	[theColorBufferDescriptor setTextureType:MTLTextureType2D];
	if ([theColorBufferDescriptor respondsToSelector:@selector(setUsage:)])
		[theColorBufferDescriptor setUsage:MTLTextureUsageRenderTarget];
	if ([theColorBufferDescriptor respondsToSelector:@selector(setStorageMode:)])
		[theColorBufferDescriptor setStorageMode:MTLStorageModeShared];
	theColorBuffer = [itsDevice newTextureWithDescriptor:theColorBufferDescriptor];
	
	//	Create theRenderPassDescriptor.

	theRenderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	theColorAttachmentDescriptor = [theRenderPassDescriptor colorAttachments][0];
	[theColorAttachmentDescriptor setClearColor:[self clearColor]];
	[theColorAttachmentDescriptor setLoadAction:MTLLoadActionClear];

	if (itsMultisamplingFlag)
	{
		theMultisampleTextureDescriptor = [MTLTextureDescriptor
			texture2DDescriptorWithPixelFormat:	MTLPixelFormatBGRA8Unorm
										 width:	[theColorBuffer width]
										height:	[theColorBuffer height]
									 mipmapped:	NO];
		[theMultisampleTextureDescriptor setTextureType:MTLTextureType2DMultisample];
		[theMultisampleTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];	//	must match value in pipeline state
		if ([theMultisampleTextureDescriptor respondsToSelector:@selector(setUsage:)])
			[theMultisampleTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
		if ([theMultisampleTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
			[theMultisampleTextureDescriptor setStorageMode:MTLStorageModePrivate];
		
		theMultisampleBuffer = [itsDevice newTextureWithDescriptor:theMultisampleTextureDescriptor];

		[theColorAttachmentDescriptor setTexture:theMultisampleBuffer];
		[theColorAttachmentDescriptor setResolveTexture:theColorBuffer];
		[theColorAttachmentDescriptor setStoreAction:MTLStoreActionMultisampleResolve];
	}
	else	//	! itsMultisamplingFlag
	{
		[theColorAttachmentDescriptor setTexture:theColorBuffer];
		[theColorAttachmentDescriptor setStoreAction:MTLStoreActionStore];
	}
	
	if (itsDepthBufferFlag)
	{
		theDepthTextureDescriptor = [MTLTextureDescriptor
			texture2DDescriptorWithPixelFormat:	MTLPixelFormatDepth32Float
										 width:	[theColorBuffer width]
										height:	[theColorBuffer height]
									 mipmapped:	NO];
		if (itsMultisamplingFlag)
		{
			[theDepthTextureDescriptor setTextureType:MTLTextureType2DMultisample];
			[theDepthTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];
		}
		else
		{
			[theDepthTextureDescriptor setTextureType:MTLTextureType2D];
			[theDepthTextureDescriptor setSampleCount:1];
		}
		if ([theDepthTextureDescriptor respondsToSelector:@selector(setUsage:)])
			[theDepthTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
		if ([theDepthTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
			[theDepthTextureDescriptor setStorageMode:MTLStorageModePrivate];

		theDepthBuffer = [itsDevice newTextureWithDescriptor:theDepthTextureDescriptor];
		
		theDepthAttachmentDescriptor = [theRenderPassDescriptor depthAttachment];
		[theDepthAttachmentDescriptor setTexture:theDepthBuffer];
		[theDepthAttachmentDescriptor setClearDepth:1.0];
		[theDepthAttachmentDescriptor setLoadAction:MTLLoadActionClear];
		[theDepthAttachmentDescriptor setStoreAction:MTLStoreActionDontCare];
	}
	
	if (itsStencilBufferFlag)
	{
		theStencilTextureDescriptor = [MTLTextureDescriptor
			texture2DDescriptorWithPixelFormat:	MTLPixelFormatStencil8
										 width:	[theColorBuffer width]
										height:	[theColorBuffer height]
									 mipmapped:	NO];
		if (itsMultisamplingFlag)
		{
			[theStencilTextureDescriptor setTextureType:MTLTextureType2DMultisample];
			[theStencilTextureDescriptor setSampleCount:METAL_MULTISAMPLING_NUM_SAMPLES];
		}
		else
		{
			[theStencilTextureDescriptor setTextureType:MTLTextureType2D];
			[theStencilTextureDescriptor setSampleCount:1];
		}
		if ([theStencilTextureDescriptor respondsToSelector:@selector(setUsage:)])
			[theStencilTextureDescriptor setUsage:MTLTextureUsageRenderTarget];
		if ([theStencilTextureDescriptor respondsToSelector:@selector(setStorageMode:)])
			[theStencilTextureDescriptor setStorageMode:MTLStorageModePrivate];

		theStencilBuffer = [itsDevice newTextureWithDescriptor:theStencilTextureDescriptor];
		
		theStencilAttachmentDescriptor = [theRenderPassDescriptor stencilAttachment];
		[theStencilAttachmentDescriptor setTexture:theStencilBuffer];
		[theStencilAttachmentDescriptor setClearStencil:0];
		[theStencilAttachmentDescriptor setLoadAction:MTLLoadActionClear];
		[theStencilAttachmentDescriptor setStoreAction:MTLStoreActionDontCare];
	}
	
	//	Create theCommandBuffer.
	theCommandBuffer = [itsCommandQueue commandBuffer];
	
	//	Encode the commands needed to draw the scene.
	[self encodeCommandsToCommandBuffer: theCommandBuffer
			   withRenderPassDescriptor: theRenderPassDescriptor
					inflightDataBuffers: theInflightDataBuffers];
	
	//	Draw the scene.
	[theCommandBuffer commit];
	[theCommandBuffer waitUntilCompleted];	//	This blocks!
	
	//	Copy the image to "ordinary" memory.
	thePixels = GET_MEMORY(anImageSize.width * anImageSize.height * sizeof(PixelRGBA));
	[theColorBuffer getBytes: thePixels
				 bytesPerRow: (anImageSize.width * sizeof(PixelRGBA))
				  fromRegion: (MTLRegion)	{
												{
													0,
													0,
													0},
												{
													(NSUInteger)anImageSize.width,
													(NSUInteger)anImageSize.height,
													(NSUInteger)1
												}
											}
				 mipmapLevel: 0];
	
	//	Swap color components from BGRA to RGBA.
	thePixel	= thePixels;	//	initialize thePixel to point to the first pixel in the array
	theCount	= anImageSize.width * anImageSize.height;
	while (theCount-- > 0)
	{
		theSwapValue	= thePixel->r;
		thePixel->r		= thePixel->b;
		thePixel->b		= theSwapValue;
		
		thePixel++;
	}
	
	//	theImage takes ownership of thePixels, then sets thePixels = NULL.
	theImage = ImageWithPixels(anImageSize, &thePixels);
	
	return theImage;
}

- (unsigned int)maxFramebufferSize
{
	unsigned int	theMaxTextureSize;

#ifdef TARGET_IOS

	theMaxTextureSize = 4096;
	
	if ([itsDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v2]
	 || [itsDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2])
	{
		theMaxTextureSize = 8192;
	}
	
	if ([itsDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1])
	{
		theMaxTextureSize = 16384;
	}
	
#else	//	for use in a possible future Mac OS X version
	theMaxTextureSize = 16384;
#endif
	
	return theMaxTextureSize;
}

@end

#endif	//	SUPPORT_METAL
