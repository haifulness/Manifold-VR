package org.geometrygames.geometrygamesshared;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.webkit.WebView;

public class GeometryGamesHelpPage extends DialogFragment
{
	public static GeometryGamesHelpPage newInstance(String aHelpPageURL)
	{
		//	Android grumbles if a DialogFragment has a constructor with arguments.
		//	The page
		//
		//		developer.android.com/reference/android/app/DialogFragment.html
		//
		//	instead recommends a static newInstance method,
		//	which achieves the same functionality of a constructor with arguments,
		//	but without running afoul of Android's rules.
		
		Bundle					theArguments;
		GeometryGamesHelpPage	theHelpPage;
		
		theArguments = new Bundle();
		theArguments.putString("help page URL", aHelpPageURL);
		
		theHelpPage = new GeometryGamesHelpPage();
		theHelpPage.setArguments(theArguments);
		
		return theHelpPage;
	}
	
	public GeometryGamesHelpPage()
	{
		//	From the Fragment documentation:
		//
		//		Every fragment must have an empty constructor, so it can 
		//		be instantiated when restoring its activity's state.
		//		It is strongly recommended that subclasses do not have 
		//		other constructors with parameters, since these constructors 
		//		will not be called when the fragment is re-instantiated; 
		//		instead, arguments can be 
		//		supplied by the caller with setArguments(Bundle) and later 
		//		retrieved by the Fragment with getArguments().
		//
		//	So I should avoid the temptation to pass in any parameters
		//	as arguments to the constructor.  Or anywhere else for that matter,
		//	except via the mechanism described in the paragraph quoted above.
		super();
	}

	
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState)
	{
		String				theHelpPageURL;
		Activity			theActivity;
		DisplayMetrics		theMetrics;
		WebView				theWebView;
		AlertDialog.Builder	theDialogBuilder;
		AlertDialog			theDialog;
		
		theHelpPageURL = getArguments().getString("help page URL");

		theActivity	= getActivity();
		theMetrics	= theActivity.getResources().getDisplayMetrics();
			
		theWebView = new WebView(theActivity);
		//	In Android 4.4, and also in 4.0.3 according to
		//		http://stackoverflow.com/questions/13959460/webview-fading-edge-is-a-solid-block
		//	WebView's implement of "fading edge" is buggy.
		//	The workaround (taking from that same stackoverflow post cited above)
		//	is to disable hardware acceleration for the WebView.
		theWebView.setLayerType(View.LAYER_TYPE_SOFTWARE, null);	//	workaround for WebView bug
		theWebView.setVerticalFadingEdgeEnabled(true);
		theWebView.setFadingEdgeLength(
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 64.0f, theMetrics));
		theWebView.loadUrl(theHelpPageURL);
		
		theDialogBuilder = new AlertDialog.Builder(theActivity);
	//	theDialogBuilder.setTitle("foo");
	//	theDialogBuilder.setMessage("bar");
		theDialogBuilder.setView(theWebView);

		//	The default value for cancelledOnTouchOutside varies
		//	on different Android versions, so set it explicitly.
		theDialog = theDialogBuilder.create();
		theDialog.setCanceledOnTouchOutside(true);
		return theDialog;
	}
}
