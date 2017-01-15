package org.geometrygames.geometrygamesshared;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.ListIterator;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;


abstract public class GeometryGamesPortfolioActivity extends GeometryGamesActivity
{
	protected GeometryGamesPortfolioView	itsPortfolioView;
		
	//	Remember which drawing the user opened in the <AppName>DrawingActivity,
	//	so we can update its thumbnail when this GeometryGamesPortfolioActivity restarts.
	private int					itsDrawingUnderRevision;	//	∈ {0, 1, … , NumDrawings - 1}	
	private static final int	NO_DRAWING_UNDER_REVISION	= -1;
	private static final String	DRAWING_UNDER_REVISION_KEY	= "drawing under revision";
	
	//	Remember whether a drawing was newly created or not, so we'll know 
	//	whether to prompt the user to name it when s/he closes it.
	private boolean				itsDrawingStillHasDefaultName	= false;
	private static final String	DRAWING_HAS_DEFAULT_NAME_KEY	= "drawing still has default name";
	
	private static final String	NUM_DRAWINGS_KEY		= "number of drawings",
								DRAWING_NAME_KEY_BASE	= "drawing name ";
	public static final String	DRAWING_NAME_KEY		= "org.geometrygames.geometrygamesshared.DrawingName";
	

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		GeometryGamesScrollView	theScrollView;
		
		//	At first creation, no drawing is under revision.
		//	When this GeometryGamesPortfolioActivity gets re-created
		//	after a device rotation or other configuration change,
		//	onSaveInstanceState() / onRestoreInstanceState()
		//	will preserve this value.
		itsDrawingUnderRevision = NO_DRAWING_UNDER_REVISION;
		
		//	Let the content view be a GeometryGamesPortfolioView contained in a ScrollView.
		itsPortfolioView	= new GeometryGamesPortfolioView(this);
		theScrollView		= new GeometryGamesScrollView(this, itsPortfolioView);
		theScrollView.setFillViewport(true);
		theScrollView.addView(itsPortfolioView);
		setContentView(theScrollView);
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
		
		itsDrawingUnderRevision			= savedInstanceState.getInt(DRAWING_UNDER_REVISION_KEY, NO_DRAWING_UNDER_REVISION);
		itsDrawingStillHasDefaultName	= savedInstanceState.getBoolean(DRAWING_HAS_DEFAULT_NAME_KEY, false);
	}

	@Override
	protected void onResume()
	{
		Bundle							theArguments;
		GeometryGamesDrawingNameEditor	theNameEditor;

		super.onResume();

		//	If the user just finished editing a drawing, update the corresponding thumbnail.
		//	The <AppName>DrawingActivity's onPause() method will have already saved the revised drawing file.
		if (itsDrawingUnderRevision != NO_DRAWING_UNDER_REVISION)
		{
			itsPortfolioView.refreshOneThumbnailImageAsynchronously(itsDrawingUnderRevision);
			
			itsDrawingUnderRevision = NO_DRAWING_UNDER_REVISION;
		}
		
		//	If the user just finished editing a newly created drawing,
		//	prompt him/her to replace the default untitled name
		//	with something more meaningful.
		if (itsDrawingStillHasDefaultName)
		{
			//	A newly created drawing always gets inserted at index 0.
			//	Put that value into a Bundle, as Android requires 
			//	for passing arguments to a DialogFragment.
			theArguments = new Bundle();
			theArguments.putIntArray("selected drawing indices", new int[]{0});
			theArguments.putBoolean("is new drawing", true);
			
			//	Create and show the GeometryGamesDrawingNameEditor.
			theNameEditor = new GeometryGamesDrawingNameEditor();
			theNameEditor.setArguments(theArguments);
			theNameEditor.show(getFragmentManager(), "drawing name editor dialog");
			
			//	Clear the flag.
			itsDrawingStillHasDefaultName = false;
		}

		//	It's logically OK if no drawings are present, but the user
		//	might be happier with a default empty drawing available.
		//	On the other hand...  when I implemented that approach,
		//	Otso Helenius tested it and wrote that
		//
		//		The only thing I first did not get was how to get 
		//		into the list of drawings, I was expecting a "Save" option 
		//		to pop up by either pressing the icon in the left upper corner 
		//		or the three dots in the right.  So, many Android users will 
		//		probably at first be surprised because their first expectation 
		//		is that when they open the program and press back, the program 
		//		is left in the background and the desktop will be presented.
		//
		//	So let's *not* auto-create a new empty drawing at first launch,
		//	but instead let the user tap New Drawing, so that s/he might later
		//	think to tap the Back button to return to the Portfolio view.
		//
//		if (getNumDrawings() == 0)
//			createNewEmptyFile();
	}

	@Override
	protected void onPause()
	{
		super.onPause();
	}
	
	@Override
	public void onSaveInstanceState(Bundle savedInstanceState)
	{
		savedInstanceState.putInt(DRAWING_UNDER_REVISION_KEY, itsDrawingUnderRevision);
		savedInstanceState.putBoolean(DRAWING_HAS_DEFAULT_NAME_KEY, itsDrawingStillHasDefaultName);
		
		super.onSaveInstanceState(savedInstanceState);
	}
	
	@Override
	protected void onStop()
	{
		super.onStop();
		
		itsPortfolioView.unloadNonvisibleThumbnailImages();
	}
	
	@Override
	protected void onDestroy()
	{
		super.onDestroy();
	}
	
	
	@Override
	public void onTrimMemory(int aLevel)
	{
//Log.e("Geometry Games", "onTrimMemory");
		if (aLevel >= TRIM_MEMORY_UI_HIDDEN)
			itsPortfolioView.unloadAllThumbnailImages();
		else
			itsPortfolioView.unloadNonvisibleThumbnailImages();
	}

	
	public void startDrawingActivity(
		int	aDrawingIndex)	//	∈ {0, 1, … , NumDrawings - 1}
	{
		Intent	anIntent;
		
		itsDrawingUnderRevision = aDrawingIndex;
				
		anIntent = new Intent(this, getDrawingActivityClass());
		anIntent.putExtra(DRAWING_NAME_KEY, getDrawingName(aDrawingIndex));
		startActivity(anIntent);
	}

	abstract protected Class<?> getDrawingActivityClass();
	
	protected void setNumDrawings(int aNumDrawings)
	{
		SharedPreferences			theSharedPreferences;
		SharedPreferences.Editor	thePreferencesEditor;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		thePreferencesEditor = theSharedPreferences.edit();
		thePreferencesEditor.putInt(NUM_DRAWINGS_KEY, aNumDrawings);
		thePreferencesEditor.apply();
	}

	protected int getNumDrawings()
	{
		SharedPreferences	theSharedPreferences;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		
		return theSharedPreferences.getInt(NUM_DRAWINGS_KEY, 0);
	}
	
	protected void renameDrawing(
		int		aDrawingIndex,
		String	aNewDrawingName)
	{
		GeometryGamesApplication	theApplication;
		String						theOldDrawingName;
		File						theOldTxtFilePath,
									theNewTxtFilePath,
									theOldPngFilePath,
									theNewPngFilePath;
		
		if (aDrawingIndex < 0 || aDrawingIndex >= PreferenceManager.getDefaultSharedPreferences(this).getInt(NUM_DRAWINGS_KEY, 0))
			return;	//	should never occur
		
		theApplication = (GeometryGamesApplication) getApplication();

		theOldDrawingName = getDrawingName(aDrawingIndex);
		
		theOldTxtFilePath = new File(theApplication.getFullPath(theOldDrawingName, ".txt"));
		theNewTxtFilePath = new File(theApplication.getFullPath(  aNewDrawingName, ".txt"));
		theOldPngFilePath = new File(theApplication.getFullPath(theOldDrawingName, ".png"));
		theNewPngFilePath = new File(theApplication.getFullPath(  aNewDrawingName, ".png"));
		
		//	Our top priority is ensuring that the user never lose any drawings, 
		//	so before we change anything else, let try to rename the main .txt drawing file.
		//	If that operation fails, we'll return immediately without changing anything else.
		//	(Here we assume that if the rename operation fails, the file will be left
		//	with its original name.)
		if ( ! theOldTxtFilePath.renameTo(theNewTxtFilePath) )
			return;
		
		//	While we hope to rename the .png thumbnail file,
		//	if this operation fails it won't be a disaster.
		//	The user would just need to open and close the drawing
		//	to generate a new .png thumbnail file.
		//	In any case, we still need to revise the drawing name
		//	to match the successfully renamed .txt file.
		theOldPngFilePath.renameTo(theNewPngFilePath);
		
		//	Write aNewDrawingName to the persistent user prefs.
		setDrawingName(aDrawingIndex, aNewDrawingName);

		//	Write aNewDrawingName to the labeled thumbnail's label.
		itsPortfolioView.renameThumnbail(aDrawingIndex, aNewDrawingName);
	}
	
	protected void setDrawingName(
		int		aDrawingIndex,
		String	aDrawingName)
	{
		SharedPreferences			theSharedPreferences;
		SharedPreferences.Editor	thePreferencesEditor;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		
		if (aDrawingIndex >= 0 && aDrawingIndex < theSharedPreferences.getInt(NUM_DRAWINGS_KEY, 0))	//	should never fail
		{
			thePreferencesEditor = theSharedPreferences.edit();
			thePreferencesEditor.putString(DRAWING_NAME_KEY_BASE + aDrawingIndex, aDrawingName);
			thePreferencesEditor.apply();
		}
	}
	
	protected String getDrawingName(int aDrawingIndex)
	{
		SharedPreferences	theSharedPreferences;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		
		if (aDrawingIndex < 0 || aDrawingIndex >= theSharedPreferences.getInt(NUM_DRAWINGS_KEY, 0))
			return "internal error:  getDrawingName() received an invalid drawing index";
		
		return theSharedPreferences.getString(DRAWING_NAME_KEY_BASE + aDrawingIndex, "internal error:  missing drawing name");
	}
	
	protected void createNewEmptyFile()
	{
		String	theNewUntitledName;
		int		theNumDrawings,
				i;

		//	Choose a name.
		theNewUntitledName = makeUniqueUntitledName();
	
		//	Create empty drawing and thumbnail files.
		createEmptyDrawingFile(theNewUntitledName);
		createEmptyThumbnailFile(theNewUntitledName);
		
		//	Update the drawing name manifest.
		theNumDrawings = getNumDrawings();
		theNumDrawings++;
		setNumDrawings(theNumDrawings);
		for (i = theNumDrawings - 1; i > 0; i--)
			setDrawingName(i, getDrawingName(i-1));
		setDrawingName(0, theNewUntitledName);
		
		//	Create a labeled thumbnail to represent the new drawing.
		itsPortfolioView.insertThumbnail(theNewUntitledName, 0);
		
		//	Note that when the user closes this drawing, we'll want
		//	to prompt him/her to replace the default untitled name
		//	with something more meaningful.
		itsDrawingStillHasDefaultName = true;
		
		//	Open the new drawing.
		startDrawingActivity(0);
		
		//	Scroll itsPortfolioView to the top.  The user won't see this immediately
		//	because the drawing activity will be opening on top of it, but the user
		//	will at least find the new drawing when s/he returns to the portfolio view.
		//
		//		The methods scrollTo(), setScrollY() and fullScroll()
		//		all animate the ScrollView through various intermediate positions,
		//		which causes lots of thumbnail images to get needlessly loaded along the way.
		//		I don't see any obvious way to avoid this, short of some ugly kludge
		//		like temporarily setting a flag to block thumbnail image loading,
		//		and then clearing the flag when the ScrollView reaches the top.
		//		But such a cure would be worse than the disease.
		//		For now I'll stick with fullScroll() which seems to pass through
		//		fewer intermediate positions, which should minimize the number 
		//		of unnecessarily loaded drawings (but that's just a quick
		//		subjective impression -- don't rely on it).
		//
		if (itsPortfolioView.getParent() instanceof GeometryGamesScrollView)	//	should never fail
			((GeometryGamesScrollView)itsPortfolioView.getParent()).fullScroll(View.FOCUS_UP);
	}
	
	protected String makeUniqueUntitledName()
	{
		String	theBaseName,
				theNewFileName;
		int		theCount;
		
		//	To pick a name, first try a localized version of "untitled", with no number.
		//	If that name is already in use, try "untitled 2", "untitled 3", etc.
		//	until an unused name is found.
	
		theBaseName		= GeometryGamesJNIWrapper.get_localized_text_as_java_string("Untitled");
		theNewFileName	= theBaseName;
		theCount		= 1;
		
		while (nameIsAlreadyInUse(theNewFileName))
		{
			theCount++;
			theNewFileName = theBaseName + " " + theCount;
		}
	
		return theNewFileName;
	}
	
	protected boolean nameIsAlreadyInUse(String aFileNameCandidate)
	{
		int	theNumDrawings,
			i;
		
		theNumDrawings = getNumDrawings();
		for (i = 0; i < theNumDrawings; i++)
		{
			if (aFileNameCandidate.equals(getDrawingName(i)))
				return true;
		}
		
		return false;
	}
	
	protected String makeUniqueCopyName(
		String				anOldName,
		ArrayList<String>	aPreexistingNameList)
	{
		String[]	theNameComponents;
		String		theLastComponent,
					theBaseName,
					theCopyName;
		int			theOldNumber,
					theNewNumber;

		//	If anOldName ends with a space followed by a natural number,
		//	then theBaseName will be anOldName with the space and the number removed;
		//	otherwise theBaseName will be anOldName itself.
		theNameComponents	= anOldName.split(" ");
		theLastComponent	= theNameComponents[theNameComponents.length - 1];
		if (theNameComponents.length >= 2
		 && theLastComponent.matches("[0-9]+"))
		{
			theBaseName = anOldName.substring(
								0,
								anOldName.length()
									- 1								//	omit separating space
									- theLastComponent.length());	//	omit final number
			try
			{
				theOldNumber = Integer.parseInt(theLastComponent);
			}
			catch (NumberFormatException e)
			{
				theOldNumber = 1;
			}
		}
		else
		{
			theBaseName		= anOldName;
			theOldNumber	= 1;	//	The original file is implicitly copy #1.
		}
				
		//	Let theCopyName be theBaseName followed by a space and
		//	the smallest number greater than theOldNumber that's
		//	not already in use with theBaseName.
		theNewNumber = theOldNumber + 1;
		do
		{
			theCopyName = theBaseName + " " + theNewNumber;
			theNewNumber++;
			
		} while (nameIsAlreadyOnList(theCopyName, aPreexistingNameList));
		
		return theCopyName;
	}
	
	protected boolean nameIsAlreadyOnList(
		String				aFileNameCandidate,
		ArrayList<String>	aPreexistingNameList)
	{
		int	theNumDrawings,
			i;
		
		//	Technical note:  It's tempting to use a ListIterator over aPreexistingNameList here,
		//	but that could be risky, given that our "grand-caller" (our caller's caller) 
		//	is already using a ListIterator over that same ArrayList.  
		//	In theory using a second ListIterator here should be harmless 
		//	(just so we don't modify the ArrayList, obviously)
		//	but in practice it would expose us to whatever implementation quirks
		//	ListIterator and ArrayList might contain.  The official documentation
		//	doesn't say anything one way or the other about nested iterations
		//	over the same ArrayList, so let's play it safe and use a plain for-loop on i.

		theNumDrawings = aPreexistingNameList.size();
		for (i = 0; i < theNumDrawings; i++)
		{
			if (aFileNameCandidate.equals(aPreexistingNameList.get(i)))
				return true;
		}
		
		return false;
	}
	
	protected void deleteSelectedDrawings()
	{
		ArrayList<GeometryGamesLabeledThumbnail>	theViewList;
		SharedPreferences							theSharedPreferences;
		int											theOldNumDrawings,
													theNewNumDrawings;
		SharedPreferences.Editor					thePreferencesEditor;
		int											theReadIndex,
													theWriteIndex;
		String										theDrawingName;
		ListIterator<GeometryGamesLabeledThumbnail>	theIterator;
		GeometryGamesLabeledThumbnail				theThumbnail;
		
		theViewList = itsPortfolioView.getViewList();

		//	First pass:  Delete the drawing and thumbnail files 
		//	for each drawing marked for deletion, while updating
		//	drawing names to correspond to the new indices.

		theSharedPreferences	= PreferenceManager.getDefaultSharedPreferences(this);
		theOldNumDrawings		= theSharedPreferences.getInt(NUM_DRAWINGS_KEY, 0);
		if (theViewList.size() != theOldNumDrawings)	//	should never occur
			return;									//	something went horribly wrong
		
		thePreferencesEditor = theSharedPreferences.edit();
		{
			theReadIndex	= 0;
			theWriteIndex	= 0;

			//	Copy the names of the drawings that the user wants to keep,
			//	while deleting the drawing and thumbnail files for the drawings
			//	that the user wants to delete.
			while (theReadIndex < theOldNumDrawings)
			{
				theDrawingName = theSharedPreferences.getString(
									DRAWING_NAME_KEY_BASE + theReadIndex,
									"internal error in deleteSelectedDrawingNames()");
				
				if (theViewList.get(theReadIndex).isSelected())	//	delete this drawing?
				{
					new File(((GeometryGamesApplication)getApplication()).getFullPath(theDrawingName, ".txt")).delete();
					new File(((GeometryGamesApplication)getApplication()).getFullPath(theDrawingName, ".png")).delete();
				}
				else
				{
					if (theWriteIndex != theReadIndex)			//	for efficiency, avoid redundant writes
						thePreferencesEditor.putString(DRAWING_NAME_KEY_BASE + theWriteIndex, theDrawingName);
					theWriteIndex++;
				}

				theReadIndex++;
			}
			
			//	Update the number of drawings.
			theNewNumDrawings = theWriteIndex;
			thePreferencesEditor.putInt(NUM_DRAWINGS_KEY, theNewNumDrawings);

			//	The remaining indices are no longer in use, so delete those keys entirely.
			while (theWriteIndex < theOldNumDrawings)
			{
				thePreferencesEditor.remove(DRAWING_NAME_KEY_BASE + theWriteIndex);
				theWriteIndex++;
			}
		}
		thePreferencesEditor.apply();
		
		
		//	Second pass:  Delete the GeometryGamesLabeledThumbnail used
		//	to represent each deleted drawing, removing it both from aViewList
		//	and from the GeometryGamesPortfolioView itself.
		//
		//	Technical note:  This is most likely an O(n·m) algorithm, 
		//	where n is the total number of elements on the list
		//	and m is the number of elements to be deleted.
		//	But this shouldn't be a problem, because even though
		//	n may be very large (~1000), m will typically be small.
		//
		theIterator = theViewList.listIterator();
		while (theIterator.hasNext())
		{
			theThumbnail = theIterator.next();
			if (theThumbnail.isSelected())
			{
				theThumbnail.unloadBitmap();	//	cancel any LoadThumbnailTask (clean but not really necessary)
				itsPortfolioView.removeView(theThumbnail);
				theIterator.remove();
			}
		}
		
		//	Clean up.
		itsPortfolioView.selectedDrawingsHaveBeenDeleted();
	}
	
	protected void duplicateSelectedDrawings(
		GeometryGamesPortfolioView					aPortfolioView,
		ArrayList<GeometryGamesLabeledThumbnail>	aViewList)
	{
		int											theOldNumDrawings;
		ArrayList<String>							theNames;
		int											theOldIndex;
		ListIterator<GeometryGamesLabeledThumbnail>	theViewIterator;
		ListIterator<String>						theNameIterator;
		GeometryGamesLabeledThumbnail				theOldThumbnail;
		String										theOldName,
													theNewName;
		int											theNewNumDrawings;
		SharedPreferences							theSharedPreferences;
		SharedPreferences.Editor					thePreferencesEditor;
		int											theNewIndex;

		//	We expect to be called only by our own GeometryGamesPortfolioView.
		if (aPortfolioView != itsPortfolioView)		//	should never occur
			return;
		
		//	The number of names should equal the number of thumbnails.
		theOldNumDrawings = getNumDrawings();
		if (theOldNumDrawings != aViewList.size())	//	should never occur
			return;
		
		//	Copy all existing drawing names into theNames.
		theNames = new ArrayList<String>(2 * theOldNumDrawings);	//	allow for 2n names, to avoid array resizing later
		for (theOldIndex = 0; theOldIndex < theOldNumDrawings; theOldIndex++)
			theNames.add(getDrawingName(theOldIndex));

		//	First pass:  For each drawing that needs to be duplicated,
		//	create a new name and insert it into theNames,
		//	duplicate the drawing and thumbnail files,
		//	and insert a new GeometryGamesLabeledThumbnail
		//	into both a ViewList and aPortfolioView itself.
		//
		//	  Note:  Be careful to keep theNames and aViewList
		//			 rigorously synchronized at all times.
		//
		theViewIterator = aViewList.listIterator();
		theNameIterator = theNames.listIterator();
		while (theViewIterator.hasNext()
			&& theNameIterator.hasNext())
		{
			theOldThumbnail	= theViewIterator.next();
			theOldName		= theNameIterator.next();
			
			if (theOldThumbnail.isSelected())
			{
				theNewName = makeUniqueCopyName(theOldName, theNames);
				
				copyFile(
					((GeometryGamesApplication)getApplication()).getFullPath(theOldName, ".txt"),
					((GeometryGamesApplication)getApplication()).getFullPath(theNewName, ".txt"));
				copyFile(
					((GeometryGamesApplication)getApplication()).getFullPath(theOldName, ".png"),
					((GeometryGamesApplication)getApplication()).getFullPath(theNewName, ".png"));
				
				theNameIterator.add(theNewName);	//	theNewName gets inserted following theOldName, and becomes the new "previous" object
				itsPortfolioView.insertThumbnail(theNewName, theViewIterator);	//	will get inserted using theViewIterator
			}
		}
		
		//	Second pass:  Copy theNames to the persistent user prefs,
		//	saving all drawings names at their new indices, and updating the total too.
		//
		theNewNumDrawings = theNames.size();
		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		thePreferencesEditor = theSharedPreferences.edit();
		{
			thePreferencesEditor.putInt(NUM_DRAWINGS_KEY, theNewNumDrawings);
			for (theNewIndex = 0; theNewIndex < theNewNumDrawings; theNewIndex++)
				thePreferencesEditor.putString(DRAWING_NAME_KEY_BASE + theNewIndex, theNames.get(theNewIndex));	
		}
		thePreferencesEditor.apply();
	}
	
	protected static void copyFile(
		String	aSrcPath,
		String	aDstPath)
	{
		File		theSrcFile,
					theDstFile;
		FileChannel	theSrcChannel	= null,
					theDstChannel	= null;
		
		theSrcFile = new File(aSrcPath);
		theDstFile = new File(aDstPath);
		
		try
		{
			theSrcChannel = new FileInputStream (theSrcFile).getChannel();
			theDstChannel = new FileOutputStream(theDstFile).getChannel();
			
			theDstChannel.transferFrom(theSrcChannel, 0, theSrcChannel.size());
		}
		catch (FileNotFoundException e)
		{
		}
		catch (IOException e)
		{
		}
		
		try
		{
			if (theSrcChannel != null)
				theSrcChannel.close();
			if (theDstChannel != null)
				theDstChannel.close();
		}
		catch (IOException e)
		{
		}
	}
	
	protected void renameSelectedDrawings(
		GeometryGamesPortfolioView					aPortfolioView,
		ArrayList<GeometryGamesLabeledThumbnail>	aThumbnailList)
	{
		int								theNumSelectedDrawings;
		int[]							theSelectedDrawingIndices;
		int								theIndex,
										theCount;
		Bundle							theArguments;
		GeometryGamesDrawingNameEditor	theNameEditor;
		
		//	Create an array containing the selected drawings' indices.
		
		theNumSelectedDrawings = 0;
		for (GeometryGamesLabeledThumbnail theThumbnail : aThumbnailList)
		{
			if (theThumbnail.isSelected())
				theNumSelectedDrawings++;
		}

		theSelectedDrawingIndices = new int[theNumSelectedDrawings];

		theIndex = 0;
		theCount = 0;
		for (GeometryGamesLabeledThumbnail theThumbnail : aThumbnailList)
		{
			if (theThumbnail.isSelected())
				theSelectedDrawingIndices[theCount++] = theIndex;
			theIndex++;
		}
		
		//	Put theSelectedDrawingIndices into a Bundle,
		//	as Android requires for passing arguments to a DialogFragment.
		theArguments = new Bundle();
		theArguments.putIntArray("selected drawing indices", theSelectedDrawingIndices);
		theArguments.putBoolean("is new drawing", false);
		
		//	Create and show the GeometryGamesDrawingNameEditor.
		theNameEditor = new GeometryGamesDrawingNameEditor();
		theNameEditor.setArguments(theArguments);
		theNameEditor.show(getFragmentManager(), "drawing name editor dialog");
	}
	
	public String checkForDuplicateNameAmongUnselectedDrawings(
		String[]	someNewNames,
		boolean		aZerothDrawingIsImplicitlySelected)
	{
		return itsPortfolioView.checkForDuplicateNameAmongUnselectedDrawings(someNewNames, aZerothDrawingIsImplicitlySelected);
	}

	
	abstract protected void createEmptyDrawingFile(String anEmptyDrawingName);
	abstract protected void createEmptyThumbnailFile(String aDrawingName);
}
