package org.geometrygames.geometrygamesshared;

import android.app.Activity;


public class GeometryGamesActivity extends Activity
{
	public static final int		PICKER_HEADER_TEXT_COLOR			= 0xFF0080FF,	//	0xAARRGGBB
								PICKER_ENABLED_ITEM_TEXT_COLOR		= 0xFF000000,	//	0xAARRGGBB
								PICKER_DISABLED_ITEM_TEXT_COLOR		= 0xFFC0C0C0;	//	0xAARRGGBB
	private static final float	PICKER_HEADER_MAX_TEXT_SIZE	= 20.0f,	//	in density-independent pixels (not scaled pixels)
								PICKER_ITEM_MAX_TEXT_SIZE	= 24.0f;	//	in density-independent pixels (not scaled pixels)


	public float getPickerHeaderTextSize()	//	returns text size in dp (not scaled pixels)
	{
		float	thePickerHeaderTextSize;
		
		thePickerHeaderTextSize = PICKER_HEADER_MAX_TEXT_SIZE;
		
		//	On devices as big as my Nexus 7 mini-tablet or larger,
		//		use the full PICKER_HEADER_MAX_TEXT_SIZE.
		//	On smaller devices,
		//		scale the text size proportionally.
		if (getResources().getConfiguration().smallestScreenWidthDp < 600)
		{
			thePickerHeaderTextSize *= (float)getResources().getConfiguration().smallestScreenWidthDp / 600.0;
			thePickerHeaderTextSize = (float) Math.floor(thePickerHeaderTextSize);
		}
		
		return thePickerHeaderTextSize;
	}
	
	public float getPickerItemTextSize()	//	returns text size in dp (not scaled pixels)
	{
		float	thePickerItemTextSize;
		
		thePickerItemTextSize = PICKER_ITEM_MAX_TEXT_SIZE;
		
		//	On devices as big as my Nexus 7 mini-tablet or larger,
		//		use the full PICKER_ITEM_MAX_TEXT_SIZE.
		//	On smaller devices,
		//		scale the text size proportionally.
		if (getResources().getConfiguration().smallestScreenWidthDp < 600)
		{
			thePickerItemTextSize *= (float)getResources().getConfiguration().smallestScreenWidthDp / 600.0;
			thePickerItemTextSize = (float) Math.floor(thePickerItemTextSize);
		}
		
		return thePickerItemTextSize;
	}
}
