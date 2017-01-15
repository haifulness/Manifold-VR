package org.geometrygames.geometrygamesshared;

import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.preference.PreferenceManager;

public class GeometryGamesUtilities
{
	//	I have no idea how threadsafe the MediaPlayer class is,
	//	but for maximal safety let's take the overly cautious approach
	//	and make playSound() be "synchronized".
	//	Even though the present implementation creates 
	//	a new MediaPlayer object each time and is unlikely
	//	to run into trouble, if we someday use a single MediaPlayer
	//	for the lifetime of the process, then the "synchronized" declaration
	//	could keep us out of trouble.
	public static synchronized void playSound(
		String	aRelativePath)	//	"foo.mid" or "foo.wav"
	{
		//	When the C function PlayTheSound() in GeometryGamesSound-Android.c
		//	needs to play a sound, it calls this Java method,
		//	which has better access to the Android MediaPlayer.
		
		String				theFilePath;
		AssetFileDescriptor	theAssetFileDescriptor;
		FileDescriptor		theFileDescriptor;
		long				theDataStart,
							theDataLength;
		final MediaPlayer	theMediaPlayer;
		
		theFilePath = "Sounds/" + aRelativePath;

		try
		{
			theAssetFileDescriptor = GeometryGamesApplication.getAssetManager().openFd(theFilePath);
		} catch (IOException e)
		{
			return;
		}
		theFileDescriptor	= theAssetFileDescriptor.getFileDescriptor();
		theDataStart		= theAssetFileDescriptor.getStartOffset();
		theDataLength		= theAssetFileDescriptor.getLength();

		theMediaPlayer = new MediaPlayer();

		try
		{
			theMediaPlayer.setDataSource(theFileDescriptor, theDataStart, theDataLength);
		}
		catch (IllegalArgumentException e)
		{
			return;
		}
		catch (IllegalStateException e)
		{
			return;
		}
		catch (IOException e)
		{
			return;
		}
		
		//	The setDataSource() documentation says
		//
		//		It is the caller's responsibility to close the file descriptor. 
		//		It is safe to do so as soon as this call [setDataSource] returns.
		//
		//	The argument that we pass it is a FileDescriptor, which has no close() method.
		//	So let's assume it wants us to close the AssetFileDescriptor instead,
		//	and hope that's correct.  In any case, I'd imagine that when
		//	theAssetFileDescriptor gets deallocated, all its resources would be released.
		try
		{
			theAssetFileDescriptor.close();
		}
		catch (IOException e1)
		{
			return;
		}
		
		try
		{
			theMediaPlayer.prepare();
		}
		catch (IllegalStateException e)
		{
			return;
		}
		catch (IOException e)
		{
			return;
		}
		
		theMediaPlayer.setOnCompletionListener(
			new OnCompletionListener()
			{
				public void onCompletion(MediaPlayer mp)
				{
					theMediaPlayer.release();
				}
			});

		theMediaPlayer.start();		
	}
	
	public static byte[] alphaTextureFromString(
		String	aString,
		int		aWidthPx,
		int		aHeightPx,
		String	aFontName,		//	pass "" to request the default font (passing null from C code could be dicey)
		int		aFontSize,		//	height in pixels, excluding descent
		int		aFontDescent,	//	vertical space below baseline, in pixels
		boolean	aCenteringFlag,	//	Center the text horizontally in the texture image?
		int		aMargin)		//	If aCenteringFlag == false, what horizontal inset to use?
	{
		Bitmap		theBitmap;
		Canvas		theCanvas;
		Paint		thePaint;
		byte[]		thePixels;
		ByteBuffer	theBuffer;
		float		theOffsetH,
					theOffsetV;
		
		theBitmap	= Bitmap.createBitmap(aWidthPx, aHeightPx, Bitmap.Config.ALPHA_8);
		theCanvas	= new Canvas(theBitmap);
		theCanvas.scale(1.0f, -1.0f, aWidthPx/2, aHeightPx/2);	//	flip from top-down coordinates to bottom-up coordinates
		thePaint	= new Paint(Paint.ANTI_ALIAS_FLAG);
		thePixels	= new byte[aWidthPx * aHeightPx];
		theBuffer	= ByteBuffer.wrap(thePixels);
		
		thePaint.setTextSize(aFontSize);
		thePaint.setTextAlign(Paint.Align.CENTER);

		//	We're drawing the text relative to Paint.Align.CENTER,
		//	so the horizontal offset may be the exact midpoint.
		theOffsetH = 0.5f * aWidthPx;

		//	The font's total height is
		//
		//		ascent + descent
		//
		//	The extra vertical space (if any) is
		//
		//		cell_height - (ascent + descent)
		//
		//	In local top-down coordinates, the text baseline sits at
		//
		//			(extra vertical space)/2 + ascent
		//		  = (cell_height - (ascent + descent))/2 + ascent
		//		  = (cell_height + ascent - descent)/2
		//
		//	However, Android reports the ascent as a negative number,
		//	so the following line of code subtracts it instead of adding it.
		theOffsetV = 0.5f * (aHeightPx - thePaint.descent() - thePaint.ascent());

		theCanvas.drawText(aString, theOffsetH, theOffsetV, thePaint);

		theBitmap.copyPixelsToBuffer(theBuffer);
		
		return thePixels;
	}


	//	Provide wrapper methods for setting and getting user preferences.

	public static void setUserPrefBool(
		Context	aContext,
		String	aKey,
		boolean	aValue)
	{
		SharedPreferences			theSharedPreferences;
		SharedPreferences.Editor	thePreferencesEditor;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(aContext);
		thePreferencesEditor = theSharedPreferences.edit();
		thePreferencesEditor.putBoolean(aKey, aValue);
		thePreferencesEditor.apply();
	}
	
	public static boolean getUserPrefBool(
		Context	aContext,
		String	aKey,
		boolean	aDefaultValue)
	{
		return PreferenceManager.getDefaultSharedPreferences(aContext).getBoolean(aKey, aDefaultValue);
	}

	public static void setUserPrefInt(
		Context	aContext,
		String	aKey,
		int		aValue)
	{
		SharedPreferences			theSharedPreferences;
		SharedPreferences.Editor	thePreferencesEditor;

		theSharedPreferences = PreferenceManager.getDefaultSharedPreferences(aContext);
		thePreferencesEditor = theSharedPreferences.edit();
		thePreferencesEditor.putInt(aKey, aValue);
		thePreferencesEditor.apply();
	}
	
	public static int getUserPrefInt(
		Context	aContext,
		String	aKey,
		int		aDefaultValue)
	{
		return PreferenceManager.getDefaultSharedPreferences(aContext).getInt(aKey, aDefaultValue);
	}
}
