package org.geometrygames.geometrygamesshared;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;

public class GeometryGamesDrawingNameEditor extends DialogFragment
{
	LinearLayout	itsListView;

	public GeometryGamesDrawingNameEditor()
	{
		//	From the Fragment documentation:
		//
		//		Every fragment must have an empty constructor, so it can 
		//		be instantiated when restoring its activity's state.
		//		It is strongly recommended that subclasses do not have 
		//		other constructors with parameters, since these constructors 
		//		will not be called when the fragment is re-instantiated;  instead,
		//		arguments can be supplied by the caller with setArguments(Bundle)
		//		and later retrieved by the Fragment with getArguments().
		//
		//	So I should avoid the temptation to pass in any parameters
		//	as arguments to the constructor.  Or anywhere else for that matter,
		//	except via the mechanism described in the paragraph quoted above.
		super();
	}
	
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState)
	{
		GeometryGamesPortfolioActivity	thePortfolioActivity;
		DisplayMetrics					theMetrics;
		ScrollView						theScrollView;
		int[]							theSelectedDrawingIndices;
		int								theNumSelectedDrawings,
										i;
		EditText						theEditText;
		String							theNameKey;
		AlertDialog.Builder				theDialogBuilder;
		AlertDialog						theDialog;
		
		if ( ! (getActivity() instanceof GeometryGamesPortfolioActivity) )
			return null;	//	should never occur
		
		thePortfolioActivity	= (GeometryGamesPortfolioActivity) getActivity();
		theMetrics				= thePortfolioActivity.getResources().getDisplayMetrics();
		
		theScrollView = new ScrollView(thePortfolioActivity);
		theScrollView.setVerticalFadingEdgeEnabled(true);
		theScrollView.setFadingEdgeLength(
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 128.0f, theMetrics));
	
		itsListView = new LinearLayout(thePortfolioActivity);
		itsListView.setOrientation(LinearLayout.VERTICAL);
		itsListView.setPadding(
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 16.0f, theMetrics),	//	left
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 16.0f, theMetrics),	//	top
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 16.0f, theMetrics),	//	right
			(int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 16.0f, theMetrics));	//	bottom;
		theScrollView.addView(	itsListView,
								new ViewGroup.LayoutParams(
									ViewGroup.LayoutParams.MATCH_PARENT,
									ViewGroup.LayoutParams.WRAP_CONTENT));

		theSelectedDrawingIndices	= getArguments().getIntArray("selected drawing indices");
		theNumSelectedDrawings		= theSelectedDrawingIndices.length;
		for (i = 0; i < theNumSelectedDrawings; i++)
		{
			theEditText = new EditText(thePortfolioActivity);
			//	Calling setInputType(InputType.TYPE_CLASS_TEXT) has the happy effect
			//	of replacing the default keyboard's "return" key (which inserts a newline)
			//	with a button that initially says "Next" and when tapped moves
			//	the focus to the next text field in the list, and then when you
			//	reach the end of the list that button changes from "Next" to "Done"
			//	and tapping it dismisses the keyboard.  Perfect!
			//
			//		Note #1:  I would have liked to have either blocked the '/' character,
			//		which is illegal in file names, or replaced it with something
			//		harmless like '|'.  Alas Android's text filtering is a mess.
			//		After wasting an hour of my own time over it, I found a summary at
			//
			//			http://androidrant.com/2012/10/05/edittext-filtering-overcomplicated-and-poorly-documented/
			//
			//		which if nothing else reassured me that it's Android's framework
			//		that's messed up, and it's not just that I'm missing something obvious.
			//		Instead I'll try replacing '/' with '|' after the user dismisses the dialog.
			//
			//		Note #2:  Calling
			//			setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
			//		would avoid  the (harmless) IME error message "E/SpannableStringBuilder(14035): 
			//		SPAN_EXCLUSIVE_EXCLUSIVE spans cannot have a zero length" that shows up in LogCat
			//		when the EditText is empty.  But users may prefer to have the keyboard
			//		offer suggestions (for example if a user wants to name a file "pomegranate"
			//		but doesn't want to type the whole word on a tiny smartphone display).
			//
			theEditText.setInputType(InputType.TYPE_CLASS_TEXT);
			theEditText.setText(thePortfolioActivity.getDrawingName(theSelectedDrawingIndices[i]));
			theEditText.setSelectAllOnFocus(true);
			itsListView.addView(theEditText,
								new LinearLayout.LayoutParams(
									LinearLayout.LayoutParams.WRAP_CONTENT,
									LinearLayout.LayoutParams.WRAP_CONTENT,
									0.0f));
		}
		
		theNameKey = (getArguments().getBoolean("is new drawing") ?
				"Name new drawing" :
				(theNumSelectedDrawings > 1 ? "Rename drawings" : "Rename drawing"));
		
		theDialogBuilder = new AlertDialog.Builder(thePortfolioActivity);
		theDialogBuilder.setTitle(GeometryGamesJNIWrapper.get_localized_text_as_java_string(theNameKey));
		theDialogBuilder.setView(theScrollView);
		theDialogBuilder.setPositiveButton(
			GeometryGamesJNIWrapper.get_localized_text_as_java_string("OK"),
			new DialogInterface.OnClickListener()
			{
				@Override
				public void onClick(DialogInterface aDialog, int anId)
				{
					//	On the one hand, code like this might well be creating a retain cycle
					//	(if the GeometryGamesDrawingNameEditor retains the Dialog which retains
					//	the OnClickListener which retains the GeometryGamesDrawingNameEditor).
					//	On the other hand, the garbage collector should free the objects anyhow
					//	when it does a "concurrent mark sweep" (and indeed experiments
					//	with forcing a garbage collection from the debugger show that
					//	finalize() really does get called on the affected objects).
					//	So maybe this is just the Android way of doing things.
					GeometryGamesDrawingNameEditor.this.validateAndSaveNewNames();
				}
			});
		theDialogBuilder.setNegativeButton(
			GeometryGamesJNIWrapper.get_localized_text_as_java_string("Cancel"),
			new DialogInterface.OnClickListener()
			{
				@Override
				public void onClick(DialogInterface aDialog, int anId)
				{
					//	Discard changes
				}
			});

		//	The default value for cancelledOnTouchOutside varies
		//	on different Android versions, so set it explicitly.
		theDialog = theDialogBuilder.create();
		theDialog.setCanceledOnTouchOutside(true);
		return theDialog;
	}
	
	public void validateAndSaveNewNames()
	{
		String	theInvalidName;
		
		replaceSlashWithBar();
		
		theInvalidName = checkForDuplicateDrawingNames();

		if (theInvalidName == null)
			saveNewNames();		
		else
			reportInvalidName(theInvalidName);
	}
	
	protected void replaceSlashWithBar()
	{
		int				theNumTextFields,
						theStringLength,
						i,
						j;
		View			theChildView;
		EditText		theEditText;
		StringBuilder	theRevisedString;
		
		theNumTextFields = itsListView.getChildCount();
		for (i = 0; i < theNumTextFields; i++)
		{
			theChildView = itsListView.getChildAt(i);
			if (theChildView instanceof EditText)	//	should never fail
			{
				theEditText			= (EditText) theChildView;
				theRevisedString	= new StringBuilder(theEditText.getText().toString());
				theStringLength		= theRevisedString.length();
				for (j = 0; j < theStringLength; j++)
					if (theRevisedString.charAt(j) == '/')
						theRevisedString.setCharAt(j, '|');
				theEditText.setText(theRevisedString.toString());
			}
		}
	}
	
	protected String checkForDuplicateDrawingNames()
	{
		int[]							theSelectedDrawingIndices;
		int								theNumSelectedDrawings;
		String[]						theNewNames;
		int								i,
										j;
		GeometryGamesPortfolioActivity	thePortfolioActivity;
		String							theDuplicateName;

		//	Alas when the user taps the Done button, Android automatically
		//	dismisses the Dialog.  So if the revised drawings names aren't 
		//	unique among themselves or relative to the other drawing names, 
		//	then we'll be left in the awkward (but ultimately tolerable) 
		//	position of having to show an error message after the dialog
		//	has been dismissed.
		
		//	Make sure the selected drawing names are unique among themselves
		//	and also relative to all the remaining (un-selected) drawing names.

		theSelectedDrawingIndices	= getArguments().getIntArray("selected drawing indices");
		theNumSelectedDrawings		= theSelectedDrawingIndices.length;

		//	For efficiency, gather the new names into an array.
		theNewNames = new String[theNumSelectedDrawings];
		for (i = 0; i < theNumSelectedDrawings; i++)
		{
			if (itsListView.getChildAt(i) instanceof EditText)	//	should never fail
				theNewNames[i] = ((EditText) itsListView.getChildAt(i)).getText().toString();
		}

		//	Are the new drawing names unique among themselves?
		for (i = 0; i < theNumSelectedDrawings; i++)
			for (j = i + 1; j < theNumSelectedDrawings; j++)
				if (theNewNames[i].equals(theNewNames[j]))
					return theNewNames[i];
		
		//	Are the new drawing names distinct from all the non-selected drawing names?
		if ( ! (getActivity() instanceof GeometryGamesPortfolioActivity) )	//	should never fail
			return "Internal error in checkForDuplicateDrawingNames()";
		thePortfolioActivity = (GeometryGamesPortfolioActivity) getActivity();
		theDuplicateName = thePortfolioActivity.checkForDuplicateNameAmongUnselectedDrawings(
														theNewNames,
														getArguments().getBoolean("is new drawing"));
		if (theDuplicateName != null)
			return theDuplicateName;
		
		return null;
	}
	
	protected void saveNewNames()
	{
		GeometryGamesPortfolioActivity	thePortfolioActivity;
		int[]							theSelectedDrawingIndices;
		int								theNumSelectedDrawings,
										i,
										theDrawingIndex;
		String							theNewDrawingName;

		if ( ! (getActivity() instanceof GeometryGamesPortfolioActivity) )	//	should never fail
			return;
		thePortfolioActivity = (GeometryGamesPortfolioActivity) getActivity();

		theSelectedDrawingIndices	= getArguments().getIntArray("selected drawing indices");
		theNumSelectedDrawings		= theSelectedDrawingIndices.length;

		for (i = 0; i < theNumSelectedDrawings; i++)
		{
			theDrawingIndex = theSelectedDrawingIndices[i];

			if (itsListView.getChildAt(i) instanceof EditText)	//	should never fail
				theNewDrawingName = ((EditText) itsListView.getChildAt(i)).getText().toString();
			else
				theNewDrawingName = "Internal error in saveNewNames()";

			thePortfolioActivity.renameDrawing(theDrawingIndex, theNewDrawingName);
		}
	}
	
	protected void reportInvalidName(String anInvalidDrawingName)
	{
		Bundle					theArguments;
		InvalidDrawingNameAlert	theAlert;
		
		//	Put anInvalidDrawingName into a Bundle,
		//	as Android requires for passing arguments to a DialogFragment.
		theArguments = new Bundle();
		theArguments.putString("invalid drawing name", anInvalidDrawingName);

		//	Create and show the InvalidDrawingNameAlert.
		theAlert = new InvalidDrawingNameAlert();
		theAlert.setArguments(theArguments);
		theAlert.show(getFragmentManager(), "invalid drawing name alert");
	}
	
	public static class InvalidDrawingNameAlert extends DialogFragment
	{
		@Override
		public Dialog onCreateDialog(Bundle saveInstanceState)
		{
			String				theInvalidDrawingName;
			AlertDialog.Builder	theDialogBuilder;
			AlertDialog			theDialog;
			
			theInvalidDrawingName = getArguments().getString("invalid drawing name");
			if (theInvalidDrawingName == null)	//	should never occur
				theInvalidDrawingName = "???";
	
			theDialogBuilder = new AlertDialog.Builder(getActivity());
			theDialogBuilder.setTitle(
				GeometryGamesJNIWrapper.get_localized_text_as_java_string("NameInUse-Title"));
			theDialogBuilder.setMessage(
				GeometryGamesJNIWrapper.get_localized_text_as_java_string("NameInUse-Message")
												.replace("%@", theInvalidDrawingName));
			theDialogBuilder.setPositiveButton(
				GeometryGamesJNIWrapper.get_localized_text_as_java_string("OK"),
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface aDialog, int anId)
					{
						//	Nothing to do here.
					}
				});
			theDialog = theDialogBuilder.create();
			theDialog.setCanceledOnTouchOutside(true);
			return theDialog;
		}
	}
}
