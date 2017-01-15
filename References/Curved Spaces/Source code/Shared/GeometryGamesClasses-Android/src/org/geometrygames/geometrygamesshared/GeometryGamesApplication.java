package org.geometrygames.geometrygamesshared;


import java.util.Locale;

import android.app.Application;
import android.content.res.AssetManager;
import android.content.res.Configuration;

public class GeometryGamesApplication extends Application
{
	//	onConfigurationChanged() may be called for many different reasons.
	//	It should call set_current_language() iff the system language has changed.
	protected String			itsSystemLanguage;	//	for example en_US or zh_CN (not zh_Hans!)

	//	A Geometry Games app might eventually want its C code to call Java methods.
	//	Some Java methods (for example GeometryGamesUtilities.playSound) will need 
	//	an asset manager, but because they get called through the platform-independent C code,
	//	such methods have no knowledge of any Java Context.
	//	So let's stash a static reference to an AssetManager here.
	private static AssetManager	itsAssetManager;

	@Override
	public void onCreate()
	{
		super.onCreate();
		
		itsAssetManager = getAssets();								//	for use by Java code (called through C code)
		GeometryGamesJNIWrapper.init_asset_manager(getAssets());	//	for use by C code
				
		itsSystemLanguage = Locale.getDefault().toString();
		GeometryGamesJNIWrapper.set_current_language(itsSystemLanguage);
	}

	@Override
	public void onTerminate()
	{
		super.onTerminate();
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		String	theNewLanguage;
		
		super.onConfigurationChanged(newConfig);

		//	Compare theNewLanguage to the previously saved value of itsSystemLanguage,
		//	not to the platform-independent code's current language.
		//	Say, for example, the app launches in French and then later the user 
		//	switches to Arabic within the app itself (leaving the operating system in French).
		//	If still later the system calls onConfigurationChanged() 
		//	for a screen orientation change, we should leave the app in Arabic.
		//	But if the system calls onConfigurationChanged()
		//	for a language change, we should set the app to that new language.
		//
		theNewLanguage = newConfig.locale.toString();	//	= Locale.getDefault().toString();
		if ( ! itsSystemLanguage.equals(theNewLanguage) )
		{
			itsSystemLanguage = theNewLanguage;
			GeometryGamesJNIWrapper.set_current_language(itsSystemLanguage);
			
			//	In practice, it seems that Android first calls 
			//	this onConfigurationChanged() method, and then
			//	completely re-creates the activity.
			//	So the options menu will get re-created in the new language
			//	with no need for us to call invalidateOptionsMenu()
			//	(which I wouldn't know how to do in any case --
			//	it's not obvious how the Application gets a reference
			//	to the current Activity).
		}		
	}
	
	public static AssetManager getAssetManager()
	{
		return itsAssetManager;
	}

	public String getFullPath(
		String	aFileName,		//	for example "MyDrawing"
		String	aFileExtension)	//	for example ".txt"
	{
		String	theFilesDirectoryPath,
				theFilePath;
		
		theFilesDirectoryPath	= getFilesDir().getAbsolutePath();
		theFilePath				= theFilesDirectoryPath + "/" + aFileName + aFileExtension;
		
		return theFilePath;
	}
}
