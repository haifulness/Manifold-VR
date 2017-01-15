package org.geometrygames.geometrygamesshared;

import android.annotation.SuppressLint;
import android.content.Context;
import android.widget.ScrollView;

@SuppressLint("ViewConstructor")
public class GeometryGamesScrollView extends ScrollView
{
	public interface ScrollListener
	{
		public void onScrollChanged();
	}
	
	protected ScrollListener	itsListener;
	
	public GeometryGamesScrollView(
		Context			aContext,
		ScrollListener	aListener)
	{
		super(aContext);
		
		itsListener = aListener;
	}

	@Override
	protected void onScrollChanged(
		int	aNewLeft,
		int	aNewTop,
		int	anOldLeft,
		int	anOldTop)
	{
		itsListener.onScrollChanged();
	}
}
