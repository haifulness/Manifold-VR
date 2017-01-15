package org.geometrygames.geometrygamesshared;

import java.io.FileOutputStream;
import java.util.concurrent.CountDownLatch;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.graphics.Bitmap;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Message;
import android.os.SystemClock;

public class GeometryGamesRenderer implements Renderer
{
	protected GeometryGamesModel			itsModel;
	protected GeometryGamesCallbackHandler	itsCallbackHandler;

	protected GeometryGamesGraphicsDataGL	itsGraphicsDataGL;
	
	//	The platform-independent Render() function expects to be told 
	//	the image's dimensions explicitly, thus giving it the flexibility 
	//	to export high-resolution images, for example.
	//	The Render() function may also choose to render different parts
	//	of the image separately (for example, KaleidoTile renders its controls
	//	separately from the polyhedron), so it may set the glViewport() 
	//	to various subsets of the full image.  It's not required 
	//	to restore the glViewport() to the full image size.
	//
	//	On Android, the Renderer gets passed the surface's dimensions
	//	in onSurfaceChanged(), which gets called only once each time 
	//	the surface's dimensions change, and not in onDrawFrame(), 
	//	which gets called once per frame.  So onSurfaceChanged() must
	//	cache the surface's dimensions for later use by onDrawFrame().
	//	
	//	Caution:  Storing the surface's dimensions here means
	//	that we can't use the same Renderer to draw two different views
	//	of the same ModelData.  Each view must have its own instance
	//	of a Draw4DRenderer.  But that's OK, because a Draw4DRenderer
	//	is a very lightweight object.
	//
	protected int	itsSurfaceWidth,	//	in pixels
					itsSurfaceHeight;	//	in pixels

	public GeometryGamesRenderer(
		GeometryGamesModel				aModel,
		GeometryGamesCallbackHandler	aCallbackHandler)
	{
		super();
		
		itsModel			= aModel;
		itsCallbackHandler	= aCallbackHandler;
		
		itsGraphicsDataGL	= null;	//	will get created in onSurfaceCreated
		
		itsSurfaceWidth		= 0;
		itsSurfaceHeight	= 0;
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config)
	{
		//	The GLSurfaceView.Renderer documentation says that on_surface_created() gets
		//
		//		Called when the surface is created or recreated.
		//
		//		Called when the rendering thread starts and
		//		whenever the EGL context is lost.
		//		The EGL context will typically be lost
		//		when the Android device awakes after going to sleep.
		//
		//			[Added by JRW:
		//			An Android GLSurfaceView's onPause() method destroys the OpenGL ES context.
		//			Later, its onResume() method creates a new OpenGL ES context and sends
		//			this onSurfaceCreated() message asking us to re-create shaders, textures, etc.]
		//
		//		Since this method is called at the beginning of rendering,
		//		as well as every time the EGL context is lost,
		//		this method is a convenient place to put code
		//		to create resources that need to be created when the rendering starts,
		//		and that need to be recreated when the EGL context is lost.
		//		Textures are an example of a resource that you might want to create here.
		//
		//		Note that when the EGL context is lost, all OpenGL resources
		//		associated with that context will be automatically deleted.
		//		You do not need to call the corresponding "glDelete" methods
		//		such as glDeleteTextures to manually delete these lost resources.
		//
		//	So this is a good moment to create (or replace) itsGraphicsDataGL,
		//	which will
		//
		//		-	have all "prepared" flags set to false, so all shaders, textures, etc.
		//			will get re-created when on_draw_frame() calls SetUpGraphicsAsNeeded(),
		//
		//		-	have all references to shaders, textures, etc. set to zero.
		//
		itsGraphicsDataGL = new GeometryGamesGraphicsDataGL();

		//	If you're curious to see the framebuffer configuration...
//		int[] temp = new int[1];
//		final int	GL10_GL_SAMPLE_BUFFERS	= 0x80a8,	//	These constants aren't part of GL10,
//					GL10_GL_SAMPLES			= 0x80a9;	//		but glGetIntegerv() knows about them anyhow.
//
//		gl.glGetIntegerv(GL10.GL_RED_BITS,		temp, 0);	Log.i("Geometry Games", "red "				+ temp[0]);	
//		gl.glGetIntegerv(GL10.GL_GREEN_BITS,	temp, 0);	Log.i("Geometry Games", "green "			+ temp[0]);	
//		gl.glGetIntegerv(GL10.GL_BLUE_BITS,		temp, 0);	Log.i("Geometry Games", "blue "				+ temp[0]);	
//		gl.glGetIntegerv(GL10.GL_ALPHA_BITS,	temp, 0);	Log.i("Geometry Games", "alpha "			+ temp[0]);	
//		gl.glGetIntegerv(GL10.GL_DEPTH_BITS,	temp, 0);	Log.i("Geometry Games", "depth "			+ temp[0]);	
//		gl.glGetIntegerv(GL10.GL_STENCIL_BITS,	temp, 0);	Log.i("Geometry Games", "stencil "			+ temp[0]);	
//		gl.glGetIntegerv(GL10_GL_SAMPLE_BUFFERS,temp, 0);	Log.i("Geometry Games", "sample buffers "	+ temp[0]);	
//		gl.glGetIntegerv(GL10_GL_SAMPLES,		temp, 0);	Log.i("Geometry Games", "samples "			+ temp[0]);	
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height)
	{
		itsSurfaceWidth		= width;
		itsSurfaceHeight	= height;
	}

	@Override
	public void onDrawFrame(GL10 gl)
	{
		long	md,	//	C pointer to ModelData
				gd;	//	C pointer to GraphicsDataGL
		String	theOnDrawFrameError;
		boolean	theWantsAnimationFlag;
		String	theOtherError;
		
		md = itsModel.lockModelData();
		gd = itsGraphicsDataGL.lockGraphicsData();
		
		//	Note:  I couldn't find a safe, documented way for on_draw_frame()
		//	to return the null object, so instead it returns an empty string
		//	in the typical case that no error has occurred.
		theOnDrawFrameError = GeometryGamesJNIWrapper.on_draw_frame(
								md,
								gd,
								itsSurfaceWidth,
								itsSurfaceHeight,
								SystemClock.elapsedRealtimeNanos() * 1.0e-9 );
		
		//	on_draw_frame() just now cleared itsRedrawRequestFlag,
		//	so simulation_wants_update() will return true iff
		//	the ModelData wants a continuous animation (for example, 
		//	if the scene's rotational velocity is non-zero).
		//
		theWantsAnimationFlag = GeometryGamesJNIWrapper.simulation_wants_update(md);

		gd = itsGraphicsDataGL.unlockGraphicsData();
		md = itsModel.unlockModelData();

		//	Because onDrawFrame() gets called in the GeometryGamesSurfaceView's 
		//	animation thread, we can't call setRenderMode() directly.  
		//	Instead we must let itsCallbackHandler call setRenderMode()
		//	on the main UI thread.
		itsCallbackHandler.sendEmptyMessage(theWantsAnimationFlag ?
							GeometryGamesCallbackHandler.TURN_ANIMATION_ON : 
							GeometryGamesCallbackHandler.TURN_ANIMATION_OFF);
		
		//	If an error occurred, tell the user.
		if (theOnDrawFrameError.length() > 0)
		{
			Message.obtain(	itsCallbackHandler,
							GeometryGamesCallbackHandler.SHOW_ERROR_MESSAGE,
							theOnDrawFrameError)
					.sendToTarget();
		}
		
		//	If some other error occurred (perhaps an internal error),
		//	tell the user that too.
		//
		//	Note:  This method for displaying generic errors is far from ideal,
		//	because they get fetched only at the end of onDrawFrame.
		//	It would be better to display the error immediately when
		//	ErrorMessage() gets called, but things get very messy 
		//	when trying to call from the C code out to the Java code,
		//	and then trying to locate the current renderer and itsCallbackHandler.
		//	So for now let's just check for errors here, at the end of onDrawFrame.
		//
		theOtherError = GeometryGamesJNIWrapper.get_and_clear_generic_error_message();
		if (theOtherError.length() > 0)
		{
			Message.obtain(	itsCallbackHandler,
							GeometryGamesCallbackHandler.SHOW_ERROR_MESSAGE,
							theOtherError)
					.sendToTarget();
		}
	}

	//	ThumbnailMaker gets used in portfolio-based apps only
	//		(for example, in KaleidoPaint and 4D Draw)
	public class ThumbnailMaker implements Runnable	//	non-static "inner class"
	{
		protected CountDownLatch	itsThumbnailLatch;
		protected String			itsThumbnailFilePath;
		protected int				itsThumbnailWidth,
									itsThumbnailHeight;

		public ThumbnailMaker(
			CountDownLatch	aLatch,
			String			aThumbnailFilePath,
			int				aThumbnailWidth,
			int				aThumbnailHeight)
		{
			itsThumbnailLatch		= aLatch;
			itsThumbnailFilePath	= aThumbnailFilePath;
			itsThumbnailWidth		= aThumbnailWidth;
			itsThumbnailHeight		= aThumbnailHeight;
		}
		
		public void	run()	//	has access to itsModel and other GeometryGamesRenderer and Draw4DRenderer instance variables
		{
			int[]				thePixelArray	= null;
			Bitmap				theBitmap		= null;
			FileOutputStream	theFile			= null;
	
			//	Allocate an array for the bitmap data
			thePixelArray = new int[itsThumbnailWidth * itsThumbnailHeight];

			//	Draw the figure into the buffer
			renderToBuffer(thePixelArray);
	
			//	Convert thePixelArray to a Bitmap
			theBitmap = Bitmap.createBitmap(thePixelArray, itsThumbnailWidth, itsThumbnailHeight, Bitmap.Config.ARGB_8888);
			
			//	Write the Bitmap to a file
			try
			{
				theFile = new FileOutputStream(itsThumbnailFilePath);	//	no need for BufferedOutputStream here
				theBitmap.compress(Bitmap.CompressFormat.PNG, 100, theFile);
			}
			catch (Exception e)
			{
			}
	
			//	Let the main thread know we're done
			itsThumbnailLatch.countDown();
		}
		
		protected void renderToBuffer(
			int[]	aPixelArray)
		{
			//	The app-specific subclass of ThumbnailMaker must override this method
			//	to call its own app-specific native render_to_buffer().
		}
	}
}
