//	GeometryGamesMetalView.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#if ( ! defined(SUPPORT_METAL) ) || ( ! defined(SUPPORT_OPENGL) )
#warning Publicly released versions of the Geometry Games \
should support both OpenGL ES and Metal \
at least for the immediate future.
#endif

#ifdef SUPPORT_METAL

#import "GeometryGamesView.h"
#import <Metal/Metal.h>

#define METAL_MULTISAMPLING_NUM_SAMPLES		4	//	4 samples per pixel

//	In practice, CAMetalLayer keeps a set of three framebuffer textures,
//	which makes sense because while the first is being displayed to the user,
//	the GPU can be rendering into the second, and the CPU can be preparing
//	to submit the third.
//
//	Let's use the same number of instances of each per-frame ("in-flight") buffer,
//	on the grounds that there's no point in letting the CPU start on a fourth frame
//	before the GPU has finished rendering the second frame.
//
//	The alternative would be to use 4 instances of each buffer instead of 3,
//	to guarantee that buffer availablity would never be a bottleneck,
//	but at the expense of slightly greater latency.
//
#define NUM_INFLIGHT_BUFFERS	3


@interface GeometryGamesMetalView : GeometryGamesView
{
	id<MTLDevice>	itsDevice;	//	visible to subclasses only
}

+ (Class)layerClass;

- (id)initWithModel:(GeometryGamesModel *)aModel frame:(CGRect)aFrame
	multisampling:(bool)aMultisamplingFlag depthBuffer:(bool)aDepthBufferFlag
	stencilBuffer:(bool)aStencilBufferFlag;

- (void)setUpGraphics;
- (void)shutDownGraphics;

- (void)layoutSubviews;

- (void)drawView;
- (NSDictionary<NSString *, id<MTLBuffer>> *)prepareInflightDataBuffersAtIndex:(unsigned int)anInflightBufferIndex;
- (NSDictionary<NSString *, id<MTLBuffer>> *)prepareInflightDataBuffersForOffscreenRenderingAtSize:(CGSize)anImageSize;
- (MTLClearColor)clearColor;
- (void)encodeCommandsToCommandBuffer:(id<MTLCommandBuffer>)aCommandBuffer
		withRenderPassDescriptor:(MTLRenderPassDescriptor *)aRenderPassDescriptor
		inflightDataBuffers:(NSDictionary<NSString *, id<MTLBuffer>> *)someInflightDataBuffers;

- (UIImage *)imageWithSize:(CGSize)anImageSize;
- (unsigned int)maxFramebufferSize;

@end

#endif	//	SUPPORT_METAL
