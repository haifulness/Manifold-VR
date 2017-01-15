package org.geometrygames.geometrygamesshared;

import java.util.ArrayList;
import java.util.ListIterator;

import android.animation.LayoutTransition;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;


@SuppressLint("ViewConstructor")	//	Eclipse/ADT would otherwise insist on a constructor with Context argument.
public class GeometryGamesPortfolioView extends ViewGroup
	implements View.OnTouchListener, GeometryGamesScrollView.ScrollListener
{
	GeometryGamesPortfolioActivity				itsParentActivity;
	ArrayList<GeometryGamesLabeledThumbnail>	itsThumbnails;
	GeometryGamesLabeledThumbnail				itsGestureSubview;
	CountDownTimer								itsGestureTimer;	//	Create itsGestureTimer only when needed, to avoid a retain cycle.
	float										itsGestureOrigX,	//	in pixels, not dp
												itsGestureOrigY,	//	in pixels, not dp
												itsGesturePrevX,	//	in pixels, not dp
												itsGesturePrevY,	//	in pixels, not dp
												itsGestureLongPressRadius;	//	in pixels, not dp;  touch must stay within this radius to count as a long press
	GeometryGamesContextualCallback				itsContextualActionCallback;
	ActionMode									itsContextualActionMode;
	int											itsNumSelectedDrawings;

	private int					itsGestureState = GESTURE_STATE_NONE;
	private static final int	GESTURE_STATE_NONE			= 0,	//	No gesture is in progress.
								GESTURE_STATE_AMBIGUOUS		= 1,	//	Exactly one finger has touched down, but the gesture type isn't yet determined.
								GESTURE_STATE_SINGLE_TAP	= 2,
								GESTURE_STATE_SCROLL		= 3,
								GESTURE_STATE_MULTITOUCH	= 4;	//	GeometryGamesPortfolioView ignores multi-touch gestures.

	private static final int	LONG_PRESS_DURATION	= 500;		//	minimum long-press duration, in milliseconds
	private static final float	LONG_PRESS_RADIUS	= 0.125f;	//	as fraction of labeled-thumbnail width
	
	private static final int	BACKGROUND_COLOR_NORMAL				= 0xFFFFFFFF,
								BACKGROUND_COLOR_MANAGE_DRAWINGS	= 0xFFC0FFE0;

	//	Cache the layout parameters computed in onMeasure() 
	//	for use in onLayout() very soon thereafter,
	//	and also for possible use in onDragEndedInLabeledThumbnail()
	//	if the user drags a thumbnail to a new position.
	//
	//		Confession:  I'm a little uneasy about caching values
	//		computed in one method for later use in another method,
	//		but in this case it seems fairly safe to assume that
	//		onMeasure() will get called, and the last call to onMeasure()
	//		will always be the relevant one.
	//
	private boolean	itsMeasurementsAreAvailable;	//	unnecessary but safe
	private int		itsUnpaddedThumbnailWidth,
					itsUnpaddedThumbnailHeight,
					itsPaddedThumbnailWidth,
					itsPaddedThumbnailHeight,
					itsThumbnailInternalPadding,
					itsNumColumns,
					itsNumRows,
					itsMargin,
					itsHStride,
					itsVStride;

	public GeometryGamesPortfolioView(GeometryGamesPortfolioActivity aParentActivity)
	{
		super(aParentActivity);
		
		int					theNumDrawings,
							i;
		LayoutTransition	theLayoutTransition;
		
		theNumDrawings = aParentActivity.getNumDrawings();
		
		itsParentActivity			= aParentActivity;
		itsThumbnails				= new ArrayList<GeometryGamesLabeledThumbnail>(theNumDrawings);
		itsGestureLongPressRadius	= LONG_PRESS_RADIUS * GeometryGamesLabeledThumbnail.totalUnpaddedWidthInPixels(aParentActivity);
		itsContextualActionCallback	= new GeometryGamesContextualCallback();
		itsContextualActionMode		= null;
		itsMeasurementsAreAvailable	= false;
		
		//	Android offers no obvious way to disable multitouch in a view,
		//	so we'll take a 2-step approach of sending any second, third, etc.
		//	pointer to the same subview in which the first pointer falls
		//	(that's what setMotionEventSplittingEnabled(false) does)
		//	and then, in onTouch(), we'll ignore all but the first "pointer".
		setMotionEventSplittingEnabled(false);

		for (i = 0; i < theNumDrawings; i++)
			insertThumbnail(aParentActivity.getDrawingName(i), i);
		
		//	By setting a LayoutTransition, we in effect are asking that changes 
		//	to the layout caused by adding and removing views be animated.
		//	In addition, by calling enableTransitionType(LayoutTransition.CHANGING),
		//	we are asking that other changes to the layout be animated as well.
		theLayoutTransition = new LayoutTransition();
		theLayoutTransition.enableTransitionType(LayoutTransition.CHANGING);
		setLayoutTransition(theLayoutTransition);
		
		//	Note:  It seems unnecessary to call setLayoutParams(),
		//	especially given that we fully implement onMeasure() and onLayout().
		//	Nevertheless if this code ever encounters layout problems
		//	in some future modification of the app, the first thing
		//	to check would be whether a call to setLayoutParams() is needed.
	}
	
	
	public void insertThumbnail(
		String	aDrawingName,
		int		aPosition)
	{
		GeometryGamesLabeledThumbnail	theLabeledThumbnail;

		if (aPosition >= 0 && aPosition <= itsThumbnails.size())
		{
			//	Create a labeled thumbnail with the correct label but no image yet.
			theLabeledThumbnail = new GeometryGamesLabeledThumbnail(itsParentActivity, aDrawingName);
			
			//	Listen for touch events in theLabeledThumbnail.
			//	We'll want to detect drags as well as clicks and long clicks.
			theLabeledThumbnail.setOnTouchListener(this);
			
			//	Add theLabeledThumbnail to our private list of subviews.
			itsThumbnails.add(aPosition, theLabeledThumbnail);
			
			//	Add theLabeledThumbnail as a subview of this GeometryGamesPortfolioView.
			addView(theLabeledThumbnail);
			
			//	onLayout() will call loadVisibleThumbnailImages(),
			//	which will load theLabeledThumbnail's bitmap in a separate thread.
		}
	}
	
	//	If the caller is already iterating over itsThumbnails,
	//	use the caller's Iterator to insert the new GeometryGamesLabeledThumbnail
	//	into the list.
	public void insertThumbnail(
		String											aDrawingName,
		ListIterator<GeometryGamesLabeledThumbnail>		anIterator)	//	must be an Iterator over this.itsThumbnails
	{
		GeometryGamesLabeledThumbnail	theLabeledThumbnail;

		if (true)	//	If wish I knew a way to verify that anIterator is iterating over this.itsThumbnails, but I don't.
		{
			//	Create a labeled thumbnail with the correct label but no image yet.
			theLabeledThumbnail = new GeometryGamesLabeledThumbnail(itsParentActivity, aDrawingName);
			
			//	Listen for touch events in theLabeledThumbnail.
			//	We'll want to detect drags as well as clicks and long clicks.
			theLabeledThumbnail.setOnTouchListener(this);
			
			//	Add theLabeledThumbnail to itsThumbnails.
			//	theLabeledThumbnail gets inserted following the labeled thumbnail
			//	the iterator most recently returned, and becomes the new "previous" object.
			anIterator.add(theLabeledThumbnail); 
			
			//	Add theLabeledThumbnail as a subview of this GeometryGamesPortfolioView.
			addView(theLabeledThumbnail);

			//	onLayout() will call loadVisibleThumbnailImages(),
			//	which will load theLabeledThumbnail's bitmap in a separate thread.
		}
	}
	
	public void renameThumnbail(
		int		aDrawingIndex,
		String	aDrawingName)
	{
		if (aDrawingIndex >= 0
		 && aDrawingIndex < itsThumbnails.size())
		{
			itsThumbnails.get(aDrawingIndex).setLabel(aDrawingName);
		}
	}
	
	
	public void onScrollChanged()
	{
		loadVisibleThumbnailImages();
	}
	
	protected void loadVisibleThumbnailImages()
	{
		GeometryGamesApplication		theApplication;
		int[]							theVisibleRange;
		int								theIndex;
		GeometryGamesLabeledThumbnail	theLabeledThumbnail;
		String							theDrawingName,
										theDrawingPath;
		
		if (itsParentActivity.getApplication() instanceof GeometryGamesApplication)
			theApplication = (GeometryGamesApplication) itsParentActivity.getApplication();
		else
			return;	//	should never occur
		
		theVisibleRange = getVisibleRange();
		
		for (theIndex = theVisibleRange[0]; theIndex <= theVisibleRange[1]; theIndex++)
		{
			theLabeledThumbnail	= itsThumbnails.get(theIndex);
			if (theLabeledThumbnail.needsBitmap())
			{
				theDrawingName = itsParentActivity.getDrawingName(theIndex);
				theDrawingPath = theApplication.getFullPath(theDrawingName, ".png");
				
				theLabeledThumbnail.loadBitmapInBackground(theDrawingPath);
			}
		}
	}
	
	public void unloadAllThumbnailImages()
	{
		//	This method should get called when the whole app
		//	(not just the GeometryGamesPortfolioActivity)
		//	is moving into background.
		
		//	Technical note:  Even when we unload a bitmap,
		//	we leave the containing GeometryGamesThumbnailView
		//	as a subview of this GeometryGamesPortfolioView.
		//	This avoids the delicate problems that would arise
		//	if we tried keep inserting and removing views;
		//	in particular, adding and removing view triggers
		//	an animation (if all goes well it would be
		//	the "null animation" but even still as the user
		//	scrolled along we'd be launching animation upon animation,
		//	for no useful purpose).  So let's leave all subviews
		//	in place at all times, and simply remove and restore
		//	the contained Bitmaps as appropriate.  That alone
		//	should keep the app's memory demands acceptable low
		//	even with 1000+ drawings.

		int	theNumDrawings,
			theIndex;

		theNumDrawings = itsThumbnails.size();
		
		for (theIndex = 0; theIndex < theNumDrawings; theIndex++)
			itsThumbnails.get(theIndex).unloadBitmap();
	}
	
	public void unloadNonvisibleThumbnailImages()
	{
		//	This method should get called when the GeometryGamesPortfolioActivity
		//	gets either
		//
		//		an onStop() message
		//	or
		//		an explicit onTrimMemory() request.
		//
		//	Otherwise we may let the non-visible thumbnails keep their bitmaps loaded,
		//	for faster scrolling later.

		int		theNumDrawings;
		int[]	theVisibleRange;
		int		theIndex;
		
		theNumDrawings	= itsThumbnails.size();
		theVisibleRange	= getVisibleRange();
		
		for (theIndex = 0; theIndex < theVisibleRange[0]; theIndex++)
			itsThumbnails.get(theIndex).unloadBitmap();
		
		for (theIndex = theVisibleRange[1] + 1; theIndex < theNumDrawings; theIndex++)
			itsThumbnails.get(theIndex).unloadBitmap();
	}

	//	In the method name "loadVisibleThumbnailImages",
	//	"visible" means "currently scrolled into view",
	//	and has nothing to do with Android's View.setVisibility() property.
	public int[] getVisibleRange()
	{
		int[]	theVisibleRange;
		Rect	theVisibleRect;
		int		theFirstVisibleRow,
				theLastVisibleRow,
				theFirstVisibleIndex,
				theLastVisibleIndex;
		
		//	Set fallback values in case of early return.
		theVisibleRange = new int[2];
		theVisibleRange[0] = 0;
		theVisibleRange[1] = 0;

		theVisibleRect = new Rect();
		if ( ! getLocalVisibleRect(theVisibleRect) )
			return theVisibleRange;	//	should never occur

		//	Locate the top-most and bottom-most rows that are currently scrolled into view.
		//	Because we get called only from onLayout() and onScrollChanged(),
		//	onMeasure() will have already been called and itsUnpaddedThumbnailWidth etc.
		//	will already be available.
		if ( ! itsMeasurementsAreAvailable )
			return theVisibleRange;	//	should never occur

		theFirstVisibleRow	= (int) Math.floor( (double)(theVisibleRect.top    - itsMargin) / (double)itsVStride );
		theLastVisibleRow	= (int) Math.floor( (double)(theVisibleRect.bottom - itsMargin) / (double)itsVStride );
	
		//	Extend the visible range by one extra row above the top edge
		//	and one extra row below the bottom edge, to reduce the likelihood
		//	of "popping" artifacts, at least if the user isn't scrolling too fast.
		theFirstVisibleRow--;
		theLastVisibleRow++;
		
		theFirstVisibleIndex	= theFirstVisibleRow * itsNumColumns;
		theLastVisibleIndex		= (theLastVisibleRow + 1) * itsNumColumns  -  1;
		
		if (theFirstVisibleIndex < 0)
			theFirstVisibleIndex = 0;
		if (theLastVisibleIndex  > itsThumbnails.size() - 1)
			theLastVisibleIndex  = itsThumbnails.size() - 1;
		
		theVisibleRange[0] = theFirstVisibleIndex;
		theVisibleRange[1] = theLastVisibleIndex;
		
		return theVisibleRange;
	}
	
	public void refreshOneThumbnailImageAsynchronously(
		int	aPosition)
	{
		GeometryGamesLabeledThumbnail	theLabeledThumbnail;
		String							theDrawingName,
										theDrawingPath;

		if (aPosition >= 0 && aPosition < itsThumbnails.size())
		{
			theLabeledThumbnail	= itsThumbnails.get(aPosition);
			theDrawingName		= itsParentActivity.getDrawingName(aPosition);
			theDrawingPath		= ((GeometryGamesApplication)itsParentActivity.getApplication()).getFullPath(theDrawingName, ".png");

			theLabeledThumbnail.unloadBitmap();	//	will also stop any pending LoadThumbnailTask
			theLabeledThumbnail.loadBitmapInBackground(theDrawingPath);
		}
	}
	

	@Override
	protected void onMeasure(
		int	widthMeasureSpec,
		int heightMeasureSpec)
	{
		int	theWidthSize,
			theWidthMode,
			theHeightSize,
			theHeightMode,
			theMinMargin,
			theNumThumbnails,
			theWidth,
			theHeight,
			i;
		
		//	For an explanation of onMeasure()/onLayout() see
		//
		//		http://developer.android.com/guide/topics/ui/how-android-draws.html
		//
	
		theWidthSize	= View.MeasureSpec.getSize(widthMeasureSpec );
		theWidthMode	= View.MeasureSpec.getMode(widthMeasureSpec );
		theHeightSize	= View.MeasureSpec.getSize(heightMeasureSpec);
		theHeightMode	= View.MeasureSpec.getMode(heightMeasureSpec);
		
		itsUnpaddedThumbnailWidth	= GeometryGamesLabeledThumbnail.totalUnpaddedWidthInPixels(this.getContext());
		itsUnpaddedThumbnailHeight	= GeometryGamesLabeledThumbnail.totalUnpaddedHeightInPixels(this.getContext());
		itsPaddedThumbnailWidth		= GeometryGamesLabeledThumbnail.totalPaddedWidthInPixels(this.getContext());
		itsPaddedThumbnailHeight	= GeometryGamesLabeledThumbnail.totalPaddedHeightInPixels(this.getContext());
		itsThumbnailInternalPadding	= GeometryGamesLabeledThumbnail.internalPaddingInPixels(this.getContext());
		theMinMargin				= GeometryGamesLabeledThumbnail.minimumMarginInPixels(this.getContext());
		
		theNumThumbnails = itsThumbnails.size();
		
		//	Determine theWidth.
		switch (theWidthMode)
		{
			case View.MeasureSpec.UNSPECIFIED:
				//	An unspecified width would be awkward:  how would be know how many columns to use?
				//	I'm guessing that this situation will never arise, but in case it does,
				//	request a single column with left and right margins and see what happens.
				theWidth = itsUnpaddedThumbnailWidth + 2*theMinMargin;
				break;

			case View.MeasureSpec.AT_MOST:
			case View.MeasureSpec.EXACTLY:
			default:	//	suppress compiler warning
				theWidth = theWidthSize;
				break;
		}
		
		//	Let theNumColumns be as large as possible, subject to the constraint
		//
		//		     theNumColumns    * theThumbnailWidth
		//		+ (theNumColumns + 1) * theMinMargin
		//		≤              theWidth
		//
		itsNumColumns = (int) Math.floor((double)(theWidth - theMinMargin) / (double)(itsUnpaddedThumbnailWidth + theMinMargin));
		if (itsNumColumns < 1)	//	theNumColumns could be 0 or even -1
			itsNumColumns = 1;
			
		//	Compute the actual margin, which is typically greater than the minimum acceptable margin.
		itsMargin	= (theWidth - itsNumColumns*itsUnpaddedThumbnailWidth) / (itsNumColumns + 1);	//	integer division discards remainder
	
		//	Compute the strides.
		itsHStride = itsMargin + itsUnpaddedThumbnailWidth;
		itsVStride = itsMargin + itsUnpaddedThumbnailHeight;

		//	Compute the total height of the whole layout.
		itsNumRows	= (theNumThumbnails + (itsNumColumns - 1)) / itsNumColumns;	//	integer division discards remainder
		theHeight	= itsNumRows*itsUnpaddedThumbnailHeight + (itsNumRows + 1)*itsMargin;
		
		//	Note for future reference that the measurements have been computed.
		itsMeasurementsAreAvailable = true;

		//	Reconcile theHeight with the constraints we have been given.
		//	In practice this GeometryGamesPortfolioView sits in a ScrollView,
		//	so we expect the height constraint to be View.MeasureSpec.UNSPECIFIED.
		switch (theHeightMode)
		{
			case View.MeasureSpec.UNSPECIFIED:
				//	theHeight is OK as computed.
				break;
			
			case View.MeasureSpec.AT_MOST:
				//	Respect the maximum size.
				if (theHeight > theHeightSize)
					theHeight = theHeightSize;
				break;

			case View.MeasureSpec.EXACTLY:
				//	Forget theHeight and use given size.
				theHeight = theHeightSize;
				break;
				
			default:			//	suppress compiler warning
				theHeight = 64;	//	should never occur
				break;
		}

		for (i = 0; i < theNumThumbnails; i++)
		{
			itsThumbnails.get(i).measure(
				View.MeasureSpec.makeMeasureSpec(itsPaddedThumbnailWidth,  View.MeasureSpec.EXACTLY),
				View.MeasureSpec.makeMeasureSpec(itsPaddedThumbnailHeight, View.MeasureSpec.EXACTLY));
		}

		setMeasuredDimension(theWidth, theHeight);
	}

	@Override
	protected void onLayout(
		boolean	changed,
		int		left,
		int		top,
		int		right,
		int		bottom)
	{
		boolean							theLanguageReadsRightToLeft;
		int								i,
										theRow,
										theCol;
		GeometryGamesLabeledThumbnail	theThumbnail;

		//	Android guarantees that onMeasure() gets called before onLayout(),
		//	so we may (with full safety, I hope!) rely on the values it computes for itsMargin, etc.
		//
		//	For a full explanation of onMeasure()/onLayout() see
		//
		//		http://developer.android.com/guide/topics/ui/how-android-draws.html
		//
	
		theLanguageReadsRightToLeft = GeometryGamesJNIWrapper.current_language_reads_right_to_left();
		
		for (i = 0; i < itsThumbnails.size(); i++)
		{
			theRow = i / itsNumColumns;
			theCol = i % itsNumColumns;
			if (theLanguageReadsRightToLeft)
				theCol = (itsNumColumns - 1) - theCol;
			
			theThumbnail = itsThumbnails.get(i);
			
			theThumbnail.layout(
				itsMargin + theCol*itsHStride - itsThumbnailInternalPadding,
				itsMargin + theRow*itsVStride - itsThumbnailInternalPadding,
				itsMargin + theCol*itsHStride + itsUnpaddedThumbnailWidth  + itsThumbnailInternalPadding,
				itsMargin + theRow*itsVStride + itsUnpaddedThumbnailHeight + itsThumbnailInternalPadding);
		}
		
		loadVisibleThumbnailImages();
	}
	
	
	//	This onInterceptTouchEvent() says whether we want the ViewGroup
	//	to steal touch events away from its children.
	@Override
	public boolean onInterceptTouchEvent(MotionEvent aMotionEvent)
	{
		return false;
	}
	
	//	This onTouchEvent() is for this ViewGroup itself (the GeometryGamesPortfolioView).
	@Override
	public boolean onTouchEvent(MotionEvent aMotionEvent)
	{
		return false;
	}

	
	//	This onTouch() is the on-touch listener for our child Views (the GeometryGamesLabeledThumbnails).
	public boolean onTouch(
		View		aView,
		MotionEvent	aMotionEvent)
	{
		float	theNewX,
				theNewY,
				theTotalDeltaX,
				theTotalDeltaY,
				theDeltaX,
				theDeltaY;

		//	Our constructor called setMotionEventSplittingEnabled(false) to ensure
		//	that all touches get directed to the same GeometryGamesLabeledThumbnail,
		//	but Android doesn't seem to provide any way to explicitly block multi-touch.
		//	So let's provide our own code to restrict to a single touch.
		//
		switch (aMotionEvent.getActionMasked())
		{
			case MotionEvent.ACTION_DOWN:			//	first finger has touched the display

				if ( ! (aView instanceof GeometryGamesLabeledThumbnail) )	//	should never occur
					return false;											//	reject touches in unexpected views

				itsGestureState		= GESTURE_STATE_AMBIGUOUS;
				itsGestureSubview	= (GeometryGamesLabeledThumbnail) aView;
				itsGestureOrigX		= aMotionEvent.getRawX();
				itsGestureOrigY		= aMotionEvent.getRawY();
				itsGesturePrevX		= itsGestureOrigX;
				itsGesturePrevY		= itsGestureOrigY;

				if (itsContextualActionMode != null)
				{
					//	If the user drags aView while in "contextual action mode",
					//	we want to treat the drag as a request to move that 
					//	GeometryGamesLabeledThumbnail within this GeometryGamesPortfolioView,
					//	not as a request to scroll the whole GeometryGamesPortfolioView
					//	within the enclosing ScrollView.
					//	To realize this behavior, we must ask the ScrollView
					//	not to intercept this gesture.
					getParent().requestDisallowInterceptTouchEvent(true);
				}
				else	//	itsContextualActionMode == null
				{
					//	Set a timer to enter "contextual action mode"
					//	if the user completes a long press while we're
					//	still in GESTURE_STATE_AMBIGUOUS.
					itsGestureTimer		= new CountDownTimer(LONG_PRESS_DURATION, LONG_PRESS_DURATION)
										{
											public void onTick(long millisUntilFinished)
											{
											}
											
											public void onFinish()
											{
												//	By entering "contextual action mode" only if
												//	itsGestureState is still GESTURE_STATE_AMBIGUOUS,
												//	we guarantee that the "contextual action mode"
												//	can't change during a drag begin/moved/ended sequence.
												//
												if (itsGestureState == GESTURE_STATE_AMBIGUOUS)
												{
													GeometryGamesPortfolioView.this.startGeometryGamesContextualActionMode();
													getParent().requestDisallowInterceptTouchEvent(true);
												}
											}
										};
					itsGestureTimer.start();
				}

				break;

			case MotionEvent.ACTION_POINTER_DOWN:	//	subsequent finger has touched the display
				if (itsGestureState == GESTURE_STATE_SCROLL)
					onDragEndedInLabeledThumbnail(aMotionEvent);
				itsGestureState = GESTURE_STATE_MULTITOUCH;
				break;

			case MotionEvent.ACTION_MOVE:			//	some finger has moved
			
				theNewX = aMotionEvent.getRawX();	//	use getRawX(), not getX(), because the view is moving
				theNewY = aMotionEvent.getRawY();	//	use getRawY(), not getY(), because the view is moving
				
				switch (itsGestureState)
				{
					case GESTURE_STATE_AMBIGUOUS:

						theTotalDeltaX = theNewX - itsGestureOrigX;
						theTotalDeltaY = theNewY - itsGestureOrigY;

						if (theTotalDeltaX * theTotalDeltaX
						  + theTotalDeltaY * theTotalDeltaY
						  > itsGestureLongPressRadius * itsGestureLongPressRadius)
						{
							itsGestureState = GESTURE_STATE_SCROLL;
							onDragBeganInLabeledThumbnail();
						}
						
						break;
						
					case GESTURE_STATE_SCROLL:

						theDeltaX = theNewX - itsGesturePrevX;
						theDeltaY = theNewY - itsGesturePrevY;

						onDragMovedInLabeledThumbnail(theDeltaX, theDeltaY);
						
						break;
				}

				itsGesturePrevX = theNewX;
				itsGesturePrevY = theNewY;
				
				break;

			case MotionEvent.ACTION_POINTER_UP:		//	non-final finger has lifted
				//	itsGestureState remains GESTURE_STATE_MULTITOUCH
				break;

			case MotionEvent.ACTION_UP:				//	final finger has lifted -- gesture is done
			case MotionEvent.ACTION_CANCEL:			//	user has flung the enclosing ScrollView, or rotated the device, or... ?

				//	Fortunately it seems we're guaranteed to reach this point sooner or later.
				//	In particular, if the user rotates the device we'll get an ACTION_CANCEL event.
				//	So we may be fairly confident that itsGestureTimer will get released 
				//	and a retain cycle avoided.
				//
				if (itsGestureTimer != null)
				{
					itsGestureTimer.cancel();
					itsGestureTimer = null;
				}

				switch (itsGestureState)
				{
					case GESTURE_STATE_NONE:
						//	should never occur
						break;

					case GESTURE_STATE_AMBIGUOUS:
						//	Only a legitimate ACTION_UP (not an ACTION_CANCEL) counts as a single tap.
						if (aMotionEvent.getActionMasked() == MotionEvent.ACTION_UP)
						{
							itsGestureState = GESTURE_STATE_SINGLE_TAP;	//	to become GESTURE_STATE_NONE below
							onSingleTapUpInLabeledThumbnail(aMotionEvent);
						}
						break;

					case GESTURE_STATE_SINGLE_TAP:
						//	should never occur
						break;

					case GESTURE_STATE_SCROLL:
						onDragEndedInLabeledThumbnail(aMotionEvent);
						break;

					case GESTURE_STATE_MULTITOUCH:
						//	ignore multitouch gestures
						break;
				}

				itsGestureSubview	= null;
				itsGestureState		= GESTURE_STATE_NONE;

				break;
		}
		
		return true;
	}

	protected void onSingleTapUpInLabeledThumbnail(MotionEvent aMotionEvent)
	{
		if (itsGestureSubview != null)	//	should never fail
		{
			if (itsContextualActionMode != null)
			{
				if (itsGestureSubview.isSelected())
				{
					itsGestureSubview.setSelected(false);
					itsNumSelectedDrawings--;
					itsContextualActionMode.invalidate();
				}
				else
				{
					itsGestureSubview.setSelected(true);
					itsNumSelectedDrawings++;
					itsContextualActionMode.invalidate();
				}
			}
			else
			{
				itsParentActivity.startDrawingActivity( itsThumbnails.indexOf(itsGestureSubview) );
			}
		}
	}

	protected void onDragBeganInLabeledThumbnail()
	{
		//	Let the selected thumbnail appear on top of all other thumbnails.
		this.bringChildToFront(itsGestureSubview);
	}

	protected void onDragMovedInLabeledThumbnail(
		float	aDeltaX,	//	in pixels, since last ACTION_MOVE
		float	aDeltaY)	//	in pixels, since last ACTION_MOVE
	{
		itsGestureSubview.offsetLeftAndRight((int)aDeltaX);
		itsGestureSubview.offsetTopAndBottom((int)aDeltaY);
	}

	protected void onDragEndedInLabeledThumbnail(MotionEvent aMotionEvent)
	{
		int		theNumThumbnails,
				theOldIndex,
				theNewRow,
				theNewCol,
				theNewIndex;
		double	h,	//	coordinates of thumbnail's center relative to center of top left thumbnail
				v,
				theDeltaX;
		String	theDrawingName;
		int		i;
		
		theOldIndex = itsThumbnails.indexOf(itsGestureSubview);

		//	Figure out at where theThumbnailView currently sits in the layout,
		//	to decide at what index to insert it into itsThumbnails.

		h = itsGestureSubview.getLeft() - itsMargin;
		v = itsGestureSubview.getTop()  - itsMargin;
		
		h /= itsHStride;
		v /= itsVStride;
		
		theNewRow = (int) Math.floor(v + 0.5);
		theNewCol = (int) Math.floor(h + 0.5);
		if (theNewRow < 0)
			theNewRow = 0;
		if (theNewCol < 0)
			theNewCol = 0;
		if (theNewCol > itsNumColumns - 1)
			theNewCol = itsNumColumns - 1;

		theDeltaX = h - theNewCol;

		if (GeometryGamesJNIWrapper.current_language_reads_right_to_left())
		{
			theNewCol = (itsNumColumns - 1) - theNewCol;
			theDeltaX = - theDeltaX;
		}

		theNumThumbnails = itsThumbnails.size();

		theNewIndex  =  theNewRow * itsNumColumns  +  theNewCol;
		if (theNewIndex > theNumThumbnails - 1)
			theNewIndex = theNumThumbnails - 1;

		//	If the user places theThumbnailView over the center of another thumbnail...
		if (Math.abs(theDeltaX) < 0.125)
		{
			//	...assume s/he want to put it into that cell.
			//	(So our choice of theNewCol is already correct.)
		}
		else	//	But if s/he places it between two thumbnails...
		{
			//	...assume s/he wants to place it between the those two.
			//	Which way we move it depends on which direction
			//	the thumbnail came from, because that's where there's
			//	a free space.
			if (theDeltaX > 0.0 && theOldIndex > theNewIndex)
				theNewIndex++;
			if (theDeltaX < 0.0 && theOldIndex < theNewIndex)
				theNewIndex--;
		}
		
		if (theNewIndex != theOldIndex)
		{
			//	Move the thumbnail to its new position in itsThumbnails.
			itsThumbnails.remove(theOldIndex);
			itsThumbnails.add(theNewIndex, itsGestureSubview);

			//	Update the order of the drawing names in the persistent user prefs.
			//	Move the dragged drawing's name to its new index,
			//	shifting all intervening names up or down one position.
			if (theNewIndex > theOldIndex)
			{
				theDrawingName = itsParentActivity.getDrawingName(theOldIndex);
				for (i = theOldIndex; i < theNewIndex; i++)
					itsParentActivity.setDrawingName(i, itsParentActivity.getDrawingName(i+1));
				itsParentActivity.setDrawingName(theNewIndex, theDrawingName);
			}
			else	//	theNewIndex < theOldIndex
			{
				theDrawingName = itsParentActivity.getDrawingName(theOldIndex);
				for (i = theOldIndex; i > theNewIndex; i--)
					itsParentActivity.setDrawingName(i, itsParentActivity.getDrawingName(i-1));
				itsParentActivity.setDrawingName(theNewIndex, theDrawingName);
			}

			//	Animate all thumbnails into their new positions.
			this.requestLayout();
		}
		else
		{
			//	The user left theThumbnailView more-or-less at its original position.
			//	Request a layout to align it exactly.  The transition will be animated automatically.
			this.requestLayout();
		}
	}
	
	public void startGeometryGamesContextualActionMode()
	{
		if (itsContextualActionMode == null)
		{
			//	onCreateActionMode() will set itsNumSelectedDrawings to 0.
			itsContextualActionMode = itsParentActivity.startActionMode(itsContextualActionCallback);
		}
	}

	public class GeometryGamesContextualCallback implements ActionMode.Callback
	{
		private static final int	ACTION_ID_DUPLICATE	= 1,
									ACTION_ID_DELETE	= 2,
									ACTION_ID_RENAME	= 3;

		public boolean onCreateActionMode(
			ActionMode	anActionMode,
			Menu		aMenu)
		{
			int			theButtonLabelFlag;
			String		theTitle;
			MenuItem	theItem;
			
			//	Keep track of how many drawings are selected,
			//	so we'll know which menu items to enable.
			itsNumSelectedDrawings = 0;
			
			//	Give the user a visual cue that we're in a different mode now.
			GeometryGamesPortfolioView.this.setBackgroundColor(BACKGROUND_COLOR_MANAGE_DRAWINGS);
			
			//	Create the menu items.

			theButtonLabelFlag = (getResources().getConfiguration().screenWidthDp >= 600 ?
									MenuItem.SHOW_AS_ACTION_WITH_TEXT : 0);
	
			theTitle = GeometryGamesJNIWrapper.get_localized_text_as_java_string("Duplicate");
			theItem = aMenu.add(Menu.NONE,
								ACTION_ID_DUPLICATE,
								Menu.NONE,
								theTitle);
			theItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | theButtonLabelFlag);
			theItem.setEnabled(false);
//			theItem.setIcon(R.drawable.ic_action_duplicate);

			theTitle = GeometryGamesJNIWrapper.get_localized_text_as_java_string("Delete");
			theItem = aMenu.add(Menu.NONE,
								ACTION_ID_DELETE,
								Menu.NONE,
								theTitle);
			theItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | theButtonLabelFlag);
			theItem.setEnabled(false);
//			theItem.setIcon(R.drawable.ic_action_delete);

			theTitle = GeometryGamesJNIWrapper.get_localized_text_as_java_string("Rename");
			theItem = aMenu.add(Menu.NONE,
								ACTION_ID_RENAME,
								Menu.NONE,
								theTitle);
			theItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS | theButtonLabelFlag);
			theItem.setEnabled(false);
//			theItem.setIcon(R.drawable.ic_action_rename);

			return true;
		}
		
		public boolean onPrepareActionMode(
			ActionMode	anActionMode,
			Menu		aMenu)
		{
			aMenu.findItem(ACTION_ID_DUPLICATE	).setEnabled(itsNumSelectedDrawings > 0);
			aMenu.findItem(ACTION_ID_DELETE		).setEnabled(itsNumSelectedDrawings > 0);
			aMenu.findItem(ACTION_ID_RENAME		).setEnabled(itsNumSelectedDrawings > 0);
			
			return (itsNumSelectedDrawings <= 1);	//	Return true only if we (might have) updated aMenu.
		}
		
		public boolean onActionItemClicked(
			ActionMode	anActionMode,
			MenuItem	aMenuItem)
		{
			String				theQuestionText,
								theDrawingName,
								theKey;
			int					theNumThumbnails,
								i;
			Bundle				theArguments;
			ConfirmDeleteAlert	theAlert;

			switch (aMenuItem.getItemId())
			{
				case ACTION_ID_DUPLICATE:
					GeometryGamesPortfolioView.this.itsParentActivity.duplicateSelectedDrawings(
																		GeometryGamesPortfolioView.this,
																		itsThumbnails);
					break;
				
				case ACTION_ID_DELETE:
				
					switch (itsNumSelectedDrawings)
					{
						case 0:
							theQuestionText = "Internal error in onActionItemClicked() for ACTION_ID_DELETE case 0";
							break;
						
						case 1:
							theDrawingName		= "Internal error in onActionItemClicked() for ACTION_ID_DELETE case 1";
							theNumThumbnails	= itsThumbnails.size();
							for (i = 0; i < theNumThumbnails; i++)
								if (itsThumbnails.get(i).isSelected())
									theDrawingName = itsParentActivity.getDrawingName(i);
							theQuestionText = GeometryGamesJNIWrapper.get_localized_text_as_java_string("DeleteDrawing?")
												.replace("%@", theDrawingName);
							break;
						
						default:
							theKey = GeometryGamesJNIWrapper.adjust_key_for_number("DeleteNDrawings?**", itsNumSelectedDrawings);
							theQuestionText = String.format(
									GeometryGamesJNIWrapper.get_localized_text_as_java_string(theKey),
									itsNumSelectedDrawings);
							break;
					}

					theArguments = new Bundle();
					theArguments.putString("confirm delete question text", theQuestionText);
					theAlert = new ConfirmDeleteAlert();
					theAlert.setArguments(theArguments);
					theAlert.show(itsParentActivity.getFragmentManager(), "confirm delete alert");

					break;
				
				case ACTION_ID_RENAME:
					GeometryGamesPortfolioView.this.itsParentActivity.renameSelectedDrawings(
																		GeometryGamesPortfolioView.this,
																		itsThumbnails);
					break;
				
				default:	//	should never occur
					break;
			}
			
//			anActionMode.finish();
			
			return true;
		}
		
		public void onDestroyActionMode(
			ActionMode	anActionMode)
		{
			itsContextualActionMode = null;
			
			for (GeometryGamesLabeledThumbnail theThumbnail : itsThumbnails)
				theThumbnail.setSelected(false);

			GeometryGamesPortfolioView.this.setBackgroundColor(BACKGROUND_COLOR_NORMAL);
		}
	};

	public static class ConfirmDeleteAlert extends DialogFragment
	{
		@Override
		public Dialog onCreateDialog(Bundle saveInstanceState)
		{
			String				theQuestionText;
			AlertDialog.Builder	theDialogBuilder;
			AlertDialog			theDialog;
			
			theQuestionText = getArguments().getString("confirm delete question text");
			if (theQuestionText == null)	//	should never occur
				theQuestionText = "???";
	
			theDialogBuilder = new AlertDialog.Builder(getActivity());
			theDialogBuilder.setTitle(theQuestionText);
			theDialogBuilder.setPositiveButton(
				GeometryGamesJNIWrapper.get_localized_text_as_java_string("Delete"),
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface aDialog, int anId)
					{
						if (getActivity() instanceof GeometryGamesPortfolioActivity)	//	should never fail
						{
							((GeometryGamesPortfolioActivity)getActivity()).deleteSelectedDrawings();
						}
						
					}
				});
			theDialogBuilder.setNegativeButton(
				GeometryGamesJNIWrapper.get_localized_text_as_java_string("Cancel"),
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface aDialog, int anId)
					{
					}
				});
			theDialog = theDialogBuilder.create();
			theDialog.setCanceledOnTouchOutside(true);
			return theDialog;
		}
	}
	public ArrayList<GeometryGamesLabeledThumbnail> getViewList()
	{
		//	I apologize for the messy organization of the Android version
		//	of this app.  It seems that to create an AlertDialog one must
		//	wrap it in a DialogFragment so that it will survive device rotation.
		//	One can pass parameters like ints and Strings to a DialogFragment
		//	via a Bundle, but one can't pass arbitrary objects that way.
		//	So... to make a long story short, the GeometryGamesPortfolioActivity
		//	needs to call us back to get the objects it needs to properly
		//	delete the selected files.
		return itsThumbnails;
	}
	public void selectedDrawingsHaveBeenDeleted()
	{
		//	See apology in getViewList() above.
		
		itsNumSelectedDrawings = 0;
		
		if (itsContextualActionMode != null)	//	should never fail
			itsContextualActionMode.invalidate();
	}

	public String checkForDuplicateNameAmongUnselectedDrawings(
		String[]	someNewNames,
		boolean		aZerothDrawingIsImplicitlySelected)	//	= true if new drawing is being renamed when not in "manage drawings" mode
	{
		int								theNumThumbnails,
										theStartingIndex,
										i;
		GeometryGamesLabeledThumbnail	theThumbnail;
		String							thePreexistingName;

		//	Check whether any name in someNewNames already occurs
		//	among the non-selected drawings, and if so return it.
		//	Otherwise return null.
		
		//	Technical note:  The natural tendency would be to write
		//
		//		for each new name in someNewNames
		//			for each pre-existing name
		//				do they match?
		//
		//	but it may be slightly more efficient to swap the inner and outer loops
		//
		//		for each pre-existing name
		//			for each new name in someNewNames
		//				do they match?
		//
		//	so we'll need to fetch each pre-existing name from the user prefs only once.
		
		theNumThumbnails	= itsThumbnails.size();
		theStartingIndex	= (aZerothDrawingIsImplicitlySelected ? 1 : 0);
		for (i = theStartingIndex; i < theNumThumbnails; i++)
		{
			theThumbnail = itsThumbnails.get(i);
			if ( ! theThumbnail.isSelected() )
			{
				thePreexistingName = itsParentActivity.getDrawingName(i);
				for (String theNewName : someNewNames)
				{
					if (theNewName.equals(thePreexistingName))
						return theNewName;
				}
			}
		}
		
		return null;
	}
}
