package org.geometrygames.geometrygamesshared;


import android.content.Context;
import android.opengl.GLSurfaceView;


public class GeometryGamesSurfaceView extends GLSurfaceView
{
	protected GeometryGamesModel	itsModel;

	public GeometryGamesSurfaceView(
		Context				context,
		GeometryGamesModel	aModel)
	{
		super(context);

		itsModel = aModel;
	}

	//	I don't expect this constructor to get called,
	//	but the Java compiler gives me a warning if I don't have it.
	public GeometryGamesSurfaceView(
		Context	context)
	{
		this(context, null);
	}
}
