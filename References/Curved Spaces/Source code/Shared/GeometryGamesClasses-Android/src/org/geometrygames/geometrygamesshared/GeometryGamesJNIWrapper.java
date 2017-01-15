package org.geometrygames.geometrygamesshared;

import android.content.res.AssetManager;


public class GeometryGamesJNIWrapper
{
	private static native void cache_Java_objects();
	static
	{
		//	Originally I tried to build a separate library for the GeometryGamesJNIWrapper methods,
		//	but I couldn't get Android to load it at run time.  Rather than deal with that mess,
		//	it seems simpler and easier to let each app include GeometryGamesJNI.c
		//	along with its own <AppName>JNI.c, so all C-based method implementations
		//	get compiled into a single library.
//		System.loadLibrary("GeometryGamesJNI");

		cache_Java_objects();
	}

	public static native void		init_asset_manager(AssetManager anAssetManager);
		//
		//	For some reason (perhaps as a result of having installed Android Studio and then removed it???)
		//	javah is no longer able to process this file with the AssetManager reference present.
		//	Instead, I get the error message
		//
		//		Error: Class android.content.res.AssetManager could not be found.
		//
		//	Given that I'll be moving to a new computer in a few month anyhow,
		//	it hardly seems worth losing time trying to resolve the problem.
		//	Instead, as a workaround:
		//
		//	1.	Comment out the two lines
		//
		//				import android.content.res.AssetManager;
		//		and
		//				public static native void		init_asset_manager(AssetManager anAssetManager);
		//
		//	2.	Run javah on this file.
		//
		//	3.	Uncomment those two lines from Step 1.
		//
		//	4.	Manually insert the missing declaration
		//
		//			/*
		//			 * Class:     org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper
		//			 * Method:    init_asset_manager
		//			 * Signature: (Landroid/content/res/AssetManager;)V
		//			 */
		//			JNIEXPORT void JNICALL Java_org_geometrygames_geometrygamesshared_GeometryGamesJNIWrapper_init_1asset_1manager
		//			  (JNIEnv *, jclass, jobject);
		//
		//		into the freshly generated GeometryGamesJNI.h .
		//
		//	It's an ugly solution, but it works.
	
	public static native void		set_current_language(String aLanguage);
	public static native int		get_num_languages();
	public static native String		get_language_code(int anIndex);
	public static native String		get_language_endonym(int anIndex);
	public static native boolean	is_current_language_index(int anIndex);
	public static native boolean	is_current_language_code(String aTwoLetterLanguageCode);
	public static native String		get_current_two_letter_language_code();
	public static native boolean	current_language_reads_left_to_right();
	public static native boolean	current_language_reads_right_to_left();
	public static native String		get_localized_text_as_java_string(String aKey);
	public static native String		adjust_key_for_number(String aKey, int aNumber);

	public static native long		alloc_model_data();		//	returns a C pointer
	public static native long		free_model_data(long mdAsLong);	//	returns 0, as a convenience
	public static native void		set_up_model_data(long mdAsLong);
	public static native void		shut_down_model_data(long mdAsLong);

	public static native long		alloc_graphics_data_gl();	//	returns a C pointer
	public static native long		free_graphics_data_gl(long gdAsLong);	//	returns 0, as a convenience
	public static native void		zero_graphics_data_gl(long gdAsLong);
		 
	public static native String		on_draw_frame(long mdAsLong, long gdAsLong, int width, int height, double time);
	public static native boolean	simulation_wants_update(long mdAsLong);

	public static native String		get_and_clear_generic_error_message();
}
