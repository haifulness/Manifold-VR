package org.geometrygames.geometrygamesshared;

import java.io.File;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

@SuppressLint("ViewConstructor")
public class GeometryGamesLabeledThumbnail extends ViewGroup
{
	//	A GeometryGamesLabeledThumbnail comprises an ImageView and a TextView.
	ImageView			itsImageView;
	TextView			itsTextView;

	//	Keep an explicit reference to its Bitmap, so we can easily test whether it's present or not.
	Bitmap				itsBitmap;

	//	Keep a reference to its LoadThumbnailTask while such a task is active,
	//	so that we may cancel it if necessary and also so we may avoid
	//	having more than one LoadThumbnailTask queued up at the same time.
	LoadThumbnailTask	itsLoadThumbnailTask;
	
	protected static final int	BACKGROUND_COLOR_UNSELECTED	= 0x00000000,
								BACKGROUND_COLOR_SELECTED	= 0xFFFFFF80,
								IMAGE_BACKGROUND_COLOR		= 0xFF808080;	// visible only when bitmap is missing

	//	Dimensions in "density-independent pixels" (dp).
	//
	//		+--------------------------------+
	//		|             padding            |
	//		|    +----------------------+    |
	//		|    |                      |    |
	//		|    |                      |    |
	//		|    |                      |    |
	//		|    |       thumbnail      |    |
	//		|    |                      |    |
	//		|    |                      |    |
	//		|    |                      |    |
	//		|    |                      |    |
	//		|    +----------------------+    |     ---    ---
	//		|    |         label        |    |      ↑      ↑    "nominal" label height
	//		|    |                      |    |      |      ↓      (excludes padding)
	//		|    +......................+    |      |     ---
	//		|    |   padding + label    |    |      ↓  "extended" label height (includes padding)
	//		+----+----------------------+----+     ---
	//
	//	The label extends into what is conceptually the padding,
	//	to accommodate extra long drawing names.  In the following code,
	//	LABEL_HEIGHT_NOM gives the label's "nominal" size, 
	//	which avoids the padding region, while 
	//	LABEL_HEIGHT_EXT gives the label's "extended" size,
	//	which extends across the padding region to the outer border.
	//
	//	The GeometryGamesPortfolioView will organize the layout so that
	//	the spacing looks good relative to the unpadded size (thumbnail + label only)
	//	but then at the last moment take the padding into account,
	//	to allow for a possible selection highlight.
	//
	//	Allow for at least three columns of thumbnails
	//	even on a "small" or "normal" 320dp wide screen.
	//
	private static final int	MIN_LARGE_DISPLAY_SIZE_DP	= 600,	//	use "large" layout when portrait width is at least this big

								//	On a 7" tablet or larger ( ≥ 600dp wide )
								//
								//			3*128 + 4*48 = 576 < 600
								//
								LG_INTERNAL_PADDING_DP		=  16,	//	thumbnail's inset within ViewGroup, to allow selection highlight
								LG_THUMBNAIL_WIDTH_DP		= 128,
								LG_THUMBNAIL_HEIGHT_DP		= LG_THUMBNAIL_WIDTH_DP,
								LG_LABEL_WIDTH_DP			= LG_THUMBNAIL_WIDTH_DP,
								LG_LABEL_HEIGHT_NOM_DP		=  40,
								LG_LABEL_HEIGHT_EXT_DP		= LG_LABEL_HEIGHT_NOM_DP + LG_INTERNAL_PADDING_DP,
								LG_TOTAL_UNPADDED_WIDTH_DP	= LG_THUMBNAIL_WIDTH_DP,
								LG_TOTAL_UNPADDED_HEIGHT_DP	= LG_THUMBNAIL_HEIGHT_DP + LG_LABEL_HEIGHT_NOM_DP,
								LG_TOTAL_PADDED_WIDTH_DP	= LG_TOTAL_UNPADDED_WIDTH_DP  + 2*LG_INTERNAL_PADDING_DP,
								LG_TOTAL_PADDED_HEIGHT_DP	= LG_TOTAL_UNPADDED_HEIGHT_DP +   LG_INTERNAL_PADDING_DP,
								LG_MIN_MARGIN_DP			=  48,	//	for parent view's use only

								//	On a phone ( < 600dp wide)
								//
								//			3*64 + 4*24 = 288 < 320
								//
								SM_INTERNAL_PADDING_DP		=   8,	//	thumbnail's inset within ViewGroup, to allow selection highlight
								SM_THUMBNAIL_WIDTH_DP		=  64,
								SM_THUMBNAIL_HEIGHT_DP		= SM_THUMBNAIL_WIDTH_DP,
								SM_LABEL_WIDTH_DP			= SM_THUMBNAIL_WIDTH_DP,
								SM_LABEL_HEIGHT_NOM_DP		=  40,
								SM_LABEL_HEIGHT_EXT_DP		= SM_LABEL_HEIGHT_NOM_DP + SM_INTERNAL_PADDING_DP,
								SM_TOTAL_UNPADDED_WIDTH_DP	= SM_THUMBNAIL_WIDTH_DP,
								SM_TOTAL_UNPADDED_HEIGHT_DP	= SM_THUMBNAIL_HEIGHT_DP + SM_LABEL_HEIGHT_NOM_DP,
								SM_TOTAL_PADDED_WIDTH_DP	= SM_TOTAL_UNPADDED_WIDTH_DP  + 2*SM_INTERNAL_PADDING_DP,
								SM_TOTAL_PADDED_HEIGHT_DP	= SM_TOTAL_UNPADDED_HEIGHT_DP +   SM_INTERNAL_PADDING_DP,
								SM_MIN_MARGIN_DP			=  24;	//	for parent view's use only
	
	//	Dimensions in true hardware pixels (px).
	public int	itsThumbnailWidthPx,
				itsThumbnailHeightPx,
				itsLabelWidthPx,
				itsLabelHeightNomPx,
				itsLabelHeightExtPx,
				itsInternalPaddingPx,
				itsTotalPaddedWidthPx,
				itsTotalPaddedHeightPx;
	
	public static int totalUnpaddedWidthInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_TOTAL_UNPADDED_WIDTH_DP : SM_TOTAL_UNPADDED_WIDTH_DP,
						aContext.getResources().getDisplayMetrics());
	}
	
	public static int totalUnpaddedHeightInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_TOTAL_UNPADDED_HEIGHT_DP : SM_TOTAL_UNPADDED_HEIGHT_DP,
						aContext.getResources().getDisplayMetrics());
	}

	public static int totalPaddedWidthInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_TOTAL_PADDED_WIDTH_DP : SM_TOTAL_PADDED_WIDTH_DP,
						aContext.getResources().getDisplayMetrics());
	}
	
	public static int totalPaddedHeightInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_TOTAL_PADDED_HEIGHT_DP : SM_TOTAL_PADDED_HEIGHT_DP,
						aContext.getResources().getDisplayMetrics());
	}
	
	public static int internalPaddingInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_INTERNAL_PADDING_DP : SM_INTERNAL_PADDING_DP,
						aContext.getResources().getDisplayMetrics());
	}

	public static int minimumMarginInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_MIN_MARGIN_DP : SM_MIN_MARGIN_DP,
						aContext.getResources().getDisplayMetrics());
	}

	//	The drawing activity's onPause() method will (re)create a .png file for the thumbnail
	//	using the dimensions we specify here.  Note that we make the (presumably very safe)
	//	assumption that the drawing activity runs on the same screen as the portfolio activity
	//	and therefore uses the same pixel density (that is, the same number of physical pixels
	//	per density-independent pixel).
	 
	public static int thumbnailWidthInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_THUMBNAIL_WIDTH_DP : SM_THUMBNAIL_WIDTH_DP,
						aContext.getResources().getDisplayMetrics());
	}
	
	public static int thumbnailHeightInPixels(Context aContext)
	{
		return (int) TypedValue.applyDimension(
						TypedValue.COMPLEX_UNIT_DIP,
						aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP ?
							LG_THUMBNAIL_HEIGHT_DP : SM_THUMBNAIL_HEIGHT_DP,
						aContext.getResources().getDisplayMetrics());
	}

	
	public GeometryGamesLabeledThumbnail(
		Context	aContext,
		String	aLabel)
	{
		super(aContext);

		DisplayMetrics	theMetrics;

		theMetrics = aContext.getResources().getDisplayMetrics();
		if (aContext.getResources().getConfiguration().smallestScreenWidthDp >= MIN_LARGE_DISPLAY_SIZE_DP)
		{
			itsThumbnailWidthPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_THUMBNAIL_WIDTH_DP,		theMetrics);
			itsThumbnailHeightPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_THUMBNAIL_HEIGHT_DP,		theMetrics);
			itsLabelWidthPx			= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_LABEL_WIDTH_DP,			theMetrics);
			itsLabelHeightNomPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_LABEL_HEIGHT_NOM_DP,		theMetrics);
			itsLabelHeightExtPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_LABEL_HEIGHT_EXT_DP,		theMetrics);
			itsInternalPaddingPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_INTERNAL_PADDING_DP,		theMetrics);
			itsTotalPaddedWidthPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_TOTAL_PADDED_WIDTH_DP,	theMetrics);
			itsTotalPaddedHeightPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, LG_TOTAL_PADDED_HEIGHT_DP,	theMetrics);
		}
		else
		{
			itsThumbnailWidthPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_THUMBNAIL_WIDTH_DP,		theMetrics);
			itsThumbnailHeightPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_THUMBNAIL_HEIGHT_DP,		theMetrics);
			itsLabelWidthPx			= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_LABEL_WIDTH_DP,			theMetrics);
			itsLabelHeightNomPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_LABEL_HEIGHT_NOM_DP,		theMetrics);
			itsLabelHeightExtPx		= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_LABEL_HEIGHT_EXT_DP,		theMetrics);
			itsInternalPaddingPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_INTERNAL_PADDING_DP,		theMetrics);
			itsTotalPaddedWidthPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_TOTAL_PADDED_WIDTH_DP,	theMetrics);
			itsTotalPaddedHeightPx	= (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SM_TOTAL_PADDED_HEIGHT_DP,	theMetrics);
		}
				
		itsImageView = new ImageView(aContext);
		itsImageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
		itsImageView.setBackgroundColor(IMAGE_BACKGROUND_COLOR);
		addView(itsImageView);
				
		itsTextView = new TextView(aContext);
		itsTextView.setText(aLabel);
		itsTextView.setGravity(Gravity.CENTER_HORIZONTAL);
		addView(itsTextView);
		
		itsBitmap				= null;
		itsLoadThumbnailTask	= null;
		

		//	Android's documentation
		//
		//		http://developer.android.com/reference/android/view/View.html#getLayoutParams()
		//
		//	says that we must set the LayoutParams.  I'm not sure why, though,
		//	given that our GeometryGamesPortfolioView never calls getLayoutParams()
		//	and in fact the app runs fine with the following call to setLayoutParams() commented out.
		//	Nevertheless, for future robustness let's go ahead and call setLayoutParams().
		//	At the very least it will protect against nasty surprises
		//	if we ever install a GeometryGamesLabeledThumbnail into some other kind of ViewGroup.
		//
		setLayoutParams(new ViewGroup.LayoutParams(itsTotalPaddedWidthPx, itsTotalPaddedHeightPx));
	}
	
	public void loadBitmapInBackground(
		String aDrawingPath)
	{
		//	If no LoadThumbnailTask is pending, submit a new one.
		if (itsLoadThumbnailTask == null)
		{
			//	Start a separate thread running to load the thumbnail image.
			itsLoadThumbnailTask = new LoadThumbnailTask();
			itsLoadThumbnailTask.execute(aDrawingPath);
		}
	}
	
	public void unloadBitmap()
	{
//Log.e("Geometry Games", "unloadBitmap");
		if (itsLoadThumbnailTask != null)
		{
			itsLoadThumbnailTask.cancel(false);
			itsLoadThumbnailTask = null;
		}
		
		setThumbnailBitmap(null);
	}

	protected void setThumbnailBitmap(Bitmap aBitmap)
	{
		itsBitmap = aBitmap;
		itsImageView.setImageBitmap(aBitmap);
	}
	
	public boolean needsBitmap()
	{
		return (itsBitmap == null
			&& itsLoadThumbnailTask == null);
	}
	
	public void setLabel(String aLabel)
	{
		itsTextView.setText(aLabel);
	}
	
	public void setSelected(boolean aSelectionFlag)
	{
		if (aSelectionFlag)
			setBackgroundColor(BACKGROUND_COLOR_SELECTED);
		else
			setBackgroundColor(BACKGROUND_COLOR_UNSELECTED);
		
		super.setSelected(aSelectionFlag);
	}
	
	@Override
	protected void onMeasure(
		int	widthMeasureSpec,
		int heightMeasureSpec)
	{
		itsImageView.measure(
			View.MeasureSpec.makeMeasureSpec(itsThumbnailWidthPx,	View.MeasureSpec.EXACTLY),
			View.MeasureSpec.makeMeasureSpec(itsThumbnailHeightPx,	View.MeasureSpec.EXACTLY));
		itsTextView.measure(
			View.MeasureSpec.makeMeasureSpec(itsLabelWidthPx,		View.MeasureSpec.EXACTLY),
			View.MeasureSpec.makeMeasureSpec(itsLabelHeightExtPx,	View.MeasureSpec.EXACTLY));

		//	When GeometryGamesPortfolioView calls measure() for this GeometryGamesLabeledThumbnail,
		//	it passes View.MeasureSpec.EXACTLY.  So the following calls to resolveSize()
		//	ignore itsTotalPaddedWidthPx and itsTotalPaddedHeightPx completely,
		//	and simply return the dimensions provided in widthMeasureSpec and heightMeasureSpec.
		//	Nevertheless I'm leaving those values in place for future robustness.
		setMeasuredDimension(
			resolveSize(itsTotalPaddedWidthPx,  widthMeasureSpec ),
			resolveSize(itsTotalPaddedHeightPx, heightMeasureSpec));
	}

	@Override
	protected void onLayout(
		boolean	changed,
		int		left,
		int		top,
		int		right,
		int		bottom)
	{
		itsImageView.layout(
						itsInternalPaddingPx,
						itsInternalPaddingPx,
						itsInternalPaddingPx + itsThumbnailWidthPx,
						itsInternalPaddingPx + itsThumbnailHeightPx);

		itsTextView.layout(
						itsInternalPaddingPx,
						itsInternalPaddingPx + itsThumbnailHeightPx,
						itsInternalPaddingPx + itsLabelWidthPx,
						itsInternalPaddingPx + itsThumbnailHeightPx + itsLabelHeightExtPx);
	}
	
	
	protected class LoadThumbnailTask extends AsyncTask<String, Void, Bitmap>
	{
		public LoadThumbnailTask()
		{
			super();
		}
		
		protected Bitmap doInBackground(String... aDrawingPathList)
		{
			String	theDrawingPath;
			File	theFile;
			Bitmap	theBitmap;
			
			theBitmap = null;

			if (aDrawingPathList.length == 1)	//	should never fail
			{
				theDrawingPath	= aDrawingPathList[0];
				theFile			= new File(theDrawingPath);
				if (theFile.exists())			//	may fail when new drawing is first created
					theBitmap = BitmapFactory.decodeFile(theDrawingPath);
			}

			return theBitmap;
		}
		
		//	Technical note:  In practice it seems that either onPostExecute() or onCancelled()
		//	will always get called, even if the LoadThumbnailTask gets cancelled
		//	before doInBackground() begins, so we could perhaps use those methods
		//	to set itsLoadThumbnailTask to null.  However... I found no clear
		//	statement in the Android documentation that onCancelled() will
		//	get called even if doInBackground() never runs, so let's not rely
		//	on that behavior, but instead let unloadBitmap() set
		//	itsLoadThumbnailTask to null immediately after it calls cancel().
		
		protected void onPostExecute(Bitmap aThumbnailBitmap)
		{
//Log.e("Geometry Games", "onPostExecute");
			//	If aThumbnailBitmap == null, then itsLabeledThumbnail
			//	will set a medium grey background instead.
			GeometryGamesLabeledThumbnail.this.setThumbnailBitmap(aThumbnailBitmap);
			
			itsLoadThumbnailTask = null;
		}
	}
}
