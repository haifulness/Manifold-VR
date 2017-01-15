package org.geometrygames.geometrygamesshared;


import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

import android.app.Fragment;
import android.app.FragmentManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;


public class GeometryGamesMainActivity extends GeometryGamesActivity
{
	protected GeometryGamesModel		itsModel;
	protected GeometryGamesSurfaceView	itsSurfaceView;
	
	//	When the user rotates the device, Android completely destroys and re-creates
	//	the Activity (a bad design decision in my opinion, but we must live with it).
	//	The subclass may want to know whether the Activity 
	//	is being created with a new empty GeometryGamesModel,
	//	or being re-created with a pre-existing GeometryGamesModel.
	protected boolean	itsModelIsNew;
	
	public static final boolean	GEOMETRY_GAMES_DEPTH_BUFFER_DISABLED	= false,
								GEOMETRY_GAMES_DEPTH_BUFFER_ENABLED		= true,
								GEOMETRY_GAMES_MULTISAMPLING_DISABLED	= false,
								GEOMETRY_GAMES_MULTISAMPLING_ENABLED	= true;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		FragmentManager				theFragmentManager;
		ModelDataReferenceFragment	theModelDataReferenceFragment;
		final String				MODEL_DATA_REFERENCE_FRAGMENT_TAG = "model data reference fragment tag";

		super.onCreate(savedInstanceState);

		//	If this GeometryGamesMainActivity is getting re-created
		//	after a device rotation or other configuration change,
		//	re-install the same ModelDataReferenceFragment we had the last time.
		//	Otherwise create a new ModelDataReferenceFragment
		//	and put a new ModelData instance in it.
		//
		//	As ugly as this all looks, this is the preferred way to maintain an object
		//	across a configuration changes.  For further discussion, see
		//
		//		developer.android.com/guide/topics/resources/runtime-changes.html
		//
		theFragmentManager				= getFragmentManager();
		theModelDataReferenceFragment	= (ModelDataReferenceFragment)
											theFragmentManager.findFragmentByTag(MODEL_DATA_REFERENCE_FRAGMENT_TAG);
		if (theModelDataReferenceFragment != null)
		{
			//	Recover a clean reference to the ModelData.
			itsModel = theModelDataReferenceFragment.getDataReference();
			
			itsModelIsNew = false;
		}
		else
		{
			itsModel = new GeometryGamesModel();	//	creates app-specific model
			
			//	Stash the ModelData away in a ModelDataReferenceFragment
			//	so it will be available after a configuration change.
			theModelDataReferenceFragment = new ModelDataReferenceFragment();
			theModelDataReferenceFragment.setDataReference(itsModel);
			theFragmentManager.beginTransaction()
								.add(theModelDataReferenceFragment, MODEL_DATA_REFERENCE_FRAGMENT_TAG)
								.commit();
			
			itsModelIsNew = true;
		}
				
		//	A subclass of GeometryGamesMainActivity typically
		//	keeps all its instance state in itsModel,
		//	so we may ignore savedInstanceState.
		//	If the subclass did have additional instance state to store,
		//	it would read it in onRestoreInstanceState(), not in onCreate();.
	}
	
	public GeometryGamesModel getModelData()	//	The various options pickers will need itsModel
	{
		return itsModel;
	}
	
	public void requestRender()
	{
		if (itsSurfaceView != null)
			itsSurfaceView.requestRender();
	}
	
	//	This nested ModelDataReferenceFragment class lets the GeometryGamesModel
	//	survive device rotation and other configuration changes,
	//	during which Android destroys and re-creates the GeometryGamesMainActivity instance itself.
	public static class ModelDataReferenceFragment extends Fragment
	{
		private	GeometryGamesModel	itsModelDataReference;
		
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			
			//	Retain this fragment across configuration changes, even though
			//	the GeometryGamesMainActivity is getting destroyed and re-created.
			setRetainInstance(true);
		}
		
		public void setDataReference(GeometryGamesModel aModelDataReference)
		{
			itsModelDataReference = aModelDataReference;
		}
		
		public GeometryGamesModel getDataReference()
		{
			return itsModelDataReference;
		}
	}
	
	protected void prepareAnimationView(
		GeometryGamesSurfaceView	aSurfaceView,	//	typically (but not necessarily) itsSurfaceView
		GeometryGamesRenderer		aRenderer,		//	aRenderer typically contains a GeometryGamesCallbackHandler
													//		which contain a weak reference to aSurfaceView
		boolean						aDepthBufferFlag,
		boolean						aMultisamplingFlag)
	{
		//	setEGLContextClientVersion(3)
		//	asks the default EGLConfigChooser to choose an OpenGL ES 3 compatible configuration, and
		//	asks the default EGLContextFactory to create an OpenGL ES 3 compatible context.
		//	It has no effect on any custom EGLConfigChooser or EGLContextFactory that we might specify.
		aSurfaceView.setEGLContextClientVersion(3);	//	call this *before* setRenderer()
		
		//	A GLSurfaceView's default EGLConfigChooser provides RGB color at 8 bits/channel
		//	along with a 16-bit depth buffer, but with no alpha channel and no multisampling.
		//	Set a custom EGLConfigChooser to add the alpha channel and the multisampling.
		aSurfaceView.setEGLConfigChooser(new GeometryGamesMainActivity.ConfigChooser(aDepthBufferFlag, aMultisamplingFlag));

		//	We could call aSurfaceView.setEGLContextFactory() if we wanted to,
		//	but we don't need to because we're happy with the default context,
		//	which seems to support OpenGL ES 2 just fine.
		//	
		//		Note:  If we did call aSurfaceView.setEGLContextFactory()
		//		we'd want to specify a value for EGL_CONTEXT_CLIENT_VERSION.
		//		EGL_CONTEXT_CLIENT_VERSION isn't a documented part of EGL until EGL 1.3,
		//		which first appears in Android API level 17.  Fortunately it seems
		//		that informal support has been available much longer.  For example,
		//		the sample code in the section "Checking the OpenGL ES Version"
		//		of the official Android page
		//
		//			developer.android.com/guide/topics/graphics/opengl.html
		//
		//		defines EGL_CONTEXT_CLIENT_VERSION manually as 0x3098
		//		and passes it to the (apparent) EGL10 version of eglCreateContext.
		//		The sample code that comes with the NDK contains a "hello-gl2" app
		//		whose file "GL2JNIView.java" does something similar.
		//		This seems to be the original way to get OpenGL ES 2 support on Android,
		//		which works with the EGL10 methods, hopefully on all devices 
		//		running API level 14 or higher.

		//	Set the renderer.
		aSurfaceView.setRenderer(aRenderer);

		//	Start in RENDERMODE_CONTINUOUSLY to ensure at least one call to the renderer.
		//	Thereafter the renderer's onDrawFrame() requests 
		//	RENDERMODE_CONTINUOUSLY or RENDERMODE_WHEN_DIRTY as needed.
		aSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);	//	call this *after* setRenderer()
	}
	
	public static class ConfigChooser implements GLSurfaceView.EGLConfigChooser
	{
		protected boolean	itsDepthBufferFlag,
							itsMultisamplingFlag;

		protected int		itsDesiredDepthSize;
							
		protected final int	DESIRED_RED_SIZE		=  8,
							DESIRED_GREEN_SIZE		=  8,
							DESIRED_BLUE_SIZE		=  8,
							DESIRED_ALPHA_SIZE		=  8,
							POSSIBLE_DEPTH_SIZE		= 16,
							DESIRED_STENCIL_SIZE	=  0;
		
		public ConfigChooser(
			boolean	aDepthBufferFlag,
			boolean	aMultisamplingFlag)
		{
			super();
			
			itsDepthBufferFlag		= aDepthBufferFlag;
			itsMultisamplingFlag	= aMultisamplingFlag;
			
			itsDesiredDepthSize = (itsDepthBufferFlag ? POSSIBLE_DEPTH_SIZE : 0);
		}
		
		public EGLConfig chooseConfig(
			EGL10		anEGL,
			EGLDisplay	aDisplay)
		{
			int[]		theNumConfigs		= new int[1];	//	receives the number of acceptable configurations
			EGLConfig[]	theConfigs			= null;
			EGLConfig	thePreferredConfig	= null;

			//	EGL_OPENGL_ES2_BIT isn't a documented part of EGL until EGL 1.4,
			//	and doesn't officially appear in Android until API level 17,
			//	as part of the class EGL14.  Fortunately it seems that informal support
			//	has been available much longer.  The sample code that comes with the NDK 
			//	contains a "hello-gl2" app whose file "GL2JNIView.java" manually
			//	defines EGL_OPENGL_ES2_BIT = 4.  This seems to be the original way 
			//	to get OpenGL ES 2 support on Android, so let's manually define
			//	EGL_OPENGL_ES2_BIT here, pass it to the EGL10 version of eglChooseConfig(),
			//	and hope it works on all devices running API level 14 or higher.
			//
			final int	EGL_OPENGL_ES2_BIT	= 4;
		
			final int[] theConfigAttributes =
						{
							EGL10.EGL_SURFACE_TYPE,		EGL10.EGL_WINDOW_BIT,
							EGL10.EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
							EGL10.EGL_RED_SIZE,			DESIRED_RED_SIZE,
							EGL10.EGL_GREEN_SIZE,		DESIRED_GREEN_SIZE,
							EGL10.EGL_BLUE_SIZE,		DESIRED_BLUE_SIZE,
							EGL10.EGL_ALPHA_SIZE,		DESIRED_ALPHA_SIZE,
							EGL10.EGL_DEPTH_SIZE,		itsDesiredDepthSize,
							EGL10.EGL_STENCIL_SIZE,		DESIRED_STENCIL_SIZE,
					//	We don't know ahead of time how many samples
					//	the host OpenGL ES implementation supports,
					//	so leave these values unspecified and choose
					//	the best configuration from the list that 
					//	the host will provide.
					//		EGL10.EGL_SAMPLE_BUFFERS,	 1,
					//		EGL10.EGL_SAMPLES,			 4,
							EGL10.EGL_NONE
						};

			//	Get a list of all configurations.
			anEGL.eglChooseConfig(aDisplay, theConfigAttributes, null, 0, theNumConfigs);	//	Pass null to learn theNumConfigs.
			if (theNumConfigs[0] != 0)
			{
				theConfigs = new EGLConfig[theNumConfigs[0]];	//	Allocate an array largest enough to hold all acceptable configurations.
				anEGL.eglChooseConfig(aDisplay, theConfigAttributes, theConfigs, theNumConfigs[0], theNumConfigs);	//	Get the configurations.

				//	theConfigs[0] is the minimal configuration that meets our request.
				thePreferredConfig = theConfigs[0];
				
				if (itsMultisamplingFlag)
				{
					//	Select a configuration similar to that minimal one,
					//	but with the best available multisampling
					//	and the least other stuff, e.g. no stencil buffer if possible.
					//
					for (EGLConfig theCandidateConfig : theConfigs)
					{
						if (SecondConfigIsBetter(anEGL, aDisplay, thePreferredConfig, theCandidateConfig))
							thePreferredConfig = theCandidateConfig;
					}
				}
			}
			else
			{
				//	Section 4.4.4 of the OpenGL ES 3.0 spec says that
				//
				//		An internal format is color-renderable if it is one of the formats
				//		from table 3.12 noted as color-renderable or...
				//
				//	and Table 3.12 includes RGBA8 marked as color-renderable,
				//	which seems to imply that an OpenGL ES 3.0 implementation
				//	must provide at least one valid RGBA8 configuration.
				//	So this "else" clause should never be needed.
				//	Even still, let's provide a fallback configuration, just in case.
				
				//	The following lines of code may never get run, but if they do,
				//	it means we're on a very minimal device, so forget about multisampling
				//	and use the most basic configuration available, most likely RGB565.
				//
				//	Now that the Geometry Games require OpenGL ES 3.0,
				//	this whole "else" clause is probably unnecessary,
				//	but I'm leaving it here any, just in case this code someday
				//	gets run on some bizarrely configured device.

				int[] theFallbackConfigAttributes =
						{
							EGL10.EGL_SURFACE_TYPE,		EGL10.EGL_WINDOW_BIT,
							EGL10.EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
							EGL10.EGL_DEPTH_SIZE,		itsDesiredDepthSize,
							EGL10.EGL_STENCIL_SIZE,		DESIRED_STENCIL_SIZE,
							EGL10.EGL_NONE
						};

				theConfigs = new EGLConfig[1];
				anEGL.eglChooseConfig(aDisplay, theFallbackConfigAttributes, theConfigs, 1, theNumConfigs);
				if (theNumConfigs[0] != 0)	//	should never fail
					thePreferredConfig = theConfigs[0];
				else
					return null;	//	No acceptable EGL configurations found
			}
			
			return thePreferredConfig;
		}
		
		protected boolean SecondConfigIsBetter(
			EGL10		anEGL,
			EGLDisplay	aDisplay,
			EGLConfig	aFirstConfig,	//	The best config we've found so far
			EGLConfig	aSecondConfig)	//	A potentially better config
		{
			int[]	m0 = {0},
					r1 = {0}, g1 = {0}, b1 = {0}, a1 = {0}, d1 = {0}, m1 = {0};

			//	Our goals are for the second config to
			//
			//		- match the desired R, G, B and A sizes exactly
			//		- match or exceed the desired depth buffer size
			//		- have the best possible multisampling
			//		- avoid unnecessary features (no stencil buffer,
			//			no unnecessarily deep depth buffer, no extra features
			//			that this code might not even be aware of)
			//
			//	eglChooseConfig() gives us a list of available configurations
			//	that's pre-sorted lexicographically according to the various parameters.
			//	So to realize those goals, it should suffice to return "true" whenever
			//	the two configurations' RGBA vales agree exactly and the multisampling quality
			//	improves, but only the first time it improves.  For example, say eglChooseConfig()
			//	gives us a list of configurations with {R, G, B, A, depth D, stencil S, samples M} values
			//	
			//			R, G, B, A,  D, S, M
			//			-------------------------
			//			8, 8, 8, 8, 16, 0, 0
			//			8, 8, 8, 8, 24, 8, 0
			//			8, 8, 8, 8, 16, 0, 2
			//			8, 8, 8, 8, 24, 8, 2
			//			8, 8, 8, 8, 16, 0, 4
			//			8, 8, 8, 8, 24, 8, 4
			//	
			//	If we start with row 0, then we'll reject row 1,
			//	because only the irrelevant parameters D and S improve.
			//	But we'll then accept row 2, because the number of samples M
			//	improves from 0 to 2 while everything else remains the same
			//	as in row 0.  So row 2 becomes our new favorite.
			//	We reject row 3 because (relative to row 2) because only
			//	irrelevant parameters improve.  But then we accept row 4
			//	(still relative to row 2) because the number of sample M 
			//	improves from 2 to 4, and row 4 becomes our new favorite.
			//	Finally we reject row 5 because only irrelevant parameters improve.

			anEGL.eglGetConfigAttrib(aDisplay, aFirstConfig,  EGL10.EGL_SAMPLES,	m0);

			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_RED_SIZE,	r1);
			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_GREEN_SIZE,	g1);
			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_BLUE_SIZE,	b1);
			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_ALPHA_SIZE,	a1);
			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_DEPTH_SIZE,	d1);
			anEGL.eglGetConfigAttrib(aDisplay, aSecondConfig, EGL10.EGL_SAMPLES,	m1);
			
			return r1[0] == DESIRED_RED_SIZE	//	exactly 8 red   bits
				&& g1[0] == DESIRED_GREEN_SIZE	//	exactly 8 green bits
				&& b1[0] == DESIRED_BLUE_SIZE	//	exactly 8 blue  bits
				&& a1[0] == DESIRED_ALPHA_SIZE	//	exactly 8 alpha bits
				&& d1[0] >= itsDesiredDepthSize	//	at least the desired depth bits
				&& m1[0] >  m0[0];				//	more samples per pixel
		}
	}
	
	@Override
	protected void onRestart()
	{
		super.onRestart();
	}
	
	@Override
	protected void onStart()
	{
		super.onStart();
	}
	
	@Override
	public void onRestoreInstanceState(Bundle savedInstanceState)
	{
		super.onRestoreInstanceState(savedInstanceState);
		
		//	A typical Geometry Games app keeps all its instance state in itsModel,
		//	but when an app has additional instance state to store
		//	(for example itsInitialGamePickerHasBeenShown in Torus Games),
		//	the subclass can override this method to read the data here as, for example,
		//
		//		itsValue = savedInstanceState.getInt(<key>);
		//
	}

	@Override
	protected void onResume()
	{
		super.onResume();

		//	itsSurfaceView.onResume() creates a new EGL context.
		//	The ModelData might enter onResume() with invalid references
		//	to OpenGL ES shaders, textures, etc., but that's harmless,
		//	as explained in onPause() below.
		itsSurfaceView.onResume();
	}

	@Override
	protected void onPause()
	{
		//	itsSurfaceView.onPause() releases the EGL context.
		//	The ModelData's references to OpenGL ES shaders, textures, etc. 
		//	will be left invalid, but that's harmless:  When the EGL context gets re-created later, 
		//	the renderer's onSurfaceCreated() method will call on_surface_created(),
		//	which in turn calls ClearPreparationFlagsAndGraphicsNames()
		//	to clear all references to OpenGL ES objects and reset the preparation flags.
		//	Soon thereafter, when the first frame gets drawn, the renderer's
		//	onDrawFrame() method will call on_draw_frame(), which in turn
		//	calls SetUpGraphicsAsNeeded() to restore all required
		//	OpenGL ES shaders, textures, etc.
		//
		itsSurfaceView.onPause();

		super.onPause();
	}
	
	@Override
	public void onSaveInstanceState(Bundle savedInstanceState)
	{
		//	A typical Geometry Games app keeps all its instance state in itsModel,
		//	but when an app has additional instance state to store
		//	(for example itsInitialGamePickerHasBeenShown in Torus Games),
		//	the subclass can override this method to write the data here as, for example,
		//
		//		savedInstanceState.putInt(<key>, itsValue);
		//
		
		super.onSaveInstanceState(savedInstanceState);
	}
	
	@Override
	protected void onStop()
	{
		super.onStop();
	}
	
	@Override
	protected void onDestroy()
	{
		super.onDestroy();
	}
}
