package org.geometrygames.geometrygamesshared;

import java.lang.ref.WeakReference;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.os.Message;

public class GeometryGamesCallbackHandler extends Handler
{
	public static final int	TURN_ANIMATION_ON									= 0,
							TURN_ANIMATION_OFF									= 1,
							SHOW_ERROR_MESSAGE									= 2,
							NUM_GEOMETRY_GAMES_CALLBACK_HANDLER_MESSAGE_TYPES	= 3;

	//	I haven't confirmed this hypothesis, but it seems likely
	//	that we would have a retain cycle:
	//
	//		The GeometryGamesCallbackHandler (explicitly) retains the GeometryGamesSurfaceView.
	//		The GeometryGamesSurfaceView     (implicitly) retains the GeometryGamesRenderer.
	//		The GeometryGamesRenderer        (explicitly) retains the GeometryGamesCallbackHandler.
	//
	//	To avoid such a cycle, keep only a weak reference to the GLSufaceView.
	//
	//	The same logic applies to our reference to the GeometryGamesMainActivity,
	//	which holds a reference to the GeometryGamesSurfaceView,
	//	so keep only a weak reference to the GeometryGamesMainActivity as well.
	//
	protected WeakReference<GLSurfaceView>	itsSurfaceViewWeakReference;
	protected WeakReference<Activity>		itsActivityWeakReference;	//	needed to show AlertDialog,
																		//		and for subclass's use

	//	Show at most one AlertDialog at a time.
	private boolean	itsAlertIsUp;

	
	public GeometryGamesCallbackHandler(
		GLSurfaceView	aSurfaceView,
		Activity		anActivity)	//	needed only to show AlertDialog
	{
		super();
		
		itsSurfaceViewWeakReference = new WeakReference<GLSurfaceView>(aSurfaceView);
		itsActivityWeakReference	= new WeakReference<Activity>(anActivity);
		
		itsAlertIsUp = false;
	}
	
	@Override
	public void handleMessage(Message msg)
	{
		GLSurfaceView		theSurfaceView;
		Activity			theActivity;
		AlertDialog.Builder	theAlertBuilder;
		AlertDialog			theAlert;
		
		switch (msg.what)
		{
			case TURN_ANIMATION_ON:
				theSurfaceView = itsSurfaceViewWeakReference.get();
				if (theSurfaceView != null)
					theSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
				break;

			case TURN_ANIMATION_OFF:
				theSurfaceView = itsSurfaceViewWeakReference.get();
				if (theSurfaceView != null)
					theSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
				break;
			
			case SHOW_ERROR_MESSAGE:
				theActivity = itsActivityWeakReference.get();
				if (theActivity != null
				 && ! itsAlertIsUp)	//	show at most one alert at a time
				{
					itsAlertIsUp = true;
					theAlertBuilder = new AlertDialog.Builder(theActivity);
					theAlertBuilder.setMessage((String)msg.obj);
					theAlertBuilder.setNeutralButton("OK",
						new DialogInterface.OnClickListener()
						{
							@Override
							public void onClick(DialogInterface dialog, int which)
							{
								itsAlertIsUp = false;
							}
						}
					);
					
					//	The default value for cancelledOnTouchOutside varies
					//	on different Android versions, so set it explicitly.
					//	Insist that the user hit the OK button,
					//	so itsAlertIsUp will get reset to false;
					theAlert = theAlertBuilder.create();
					theAlert.setCancelable(false);
					theAlert.setCanceledOnTouchOutside(false);
					theAlert.show();
				}
				break;
		}
	}
}
