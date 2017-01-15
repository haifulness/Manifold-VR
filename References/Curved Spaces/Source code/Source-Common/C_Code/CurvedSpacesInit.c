//	CurvedSpacesInit.c
//
//	Initializes the ModelData.
//	Prepares OpenGL shaders, textures and Vertex Array Objects (VAOs).
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#ifdef SUPPORT_OPENGL
#include "CurvedSpacesGraphics-OpenGL.h"
#endif
#include "GeometryGamesLocalization.h"	//	needed only for "missing shaders" error message


//	To add a new language, please see the instructions
//	in the file "add a new language.txt".
const Char16			gLanguages[][3]			= {u"en", u"es", u"fr", u"ja", u"zs", u"zt"};
const unsigned int		gNumLanguages			= BUFFER_LENGTH(gLanguages);
const Char16 * const	gLanguageFileBaseName	= u"CurvedSpaces";


static ErrorText	SetUpShaders(GraphicsDataGL *gd);
static void			ShutDownShaders(GraphicsDataGL *gd);
static ErrorText	SetUpTextures(GraphicsDataGL *gd, StereoMode aStereoMode);
static void			ShutDownTextures(GraphicsDataGL *gd);
static ErrorText	SetUpVBOs(GraphicsDataGL *gd, DirichletDomain *aDirichletDomain,
						double aCurrentAperture, bool aShowColorCoding, StereoMode aStereoMode,
						CenterpieceType aCenterpiece, bool aShowObserver, bool aShowVertexFigures,
						CliffordMode aCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
						, bool aShowHantzscheWendtAxes
#endif
						);
static void			ShutDownVBOs(GraphicsDataGL *gd);
static ErrorText	SetUpVAOs(GraphicsDataGL *gd, CenterpieceType aCenterpiece,
						bool aShowObserver, bool aShowVertexFigures, CliffordMode aCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
						, bool aShowHantzscheWendtAxes
#endif
						);
static void			ShutDownVAOs(GraphicsDataGL *gd);
static ErrorText	SetUpQueries(GraphicsDataGL *gd);
static void			ShutDownQueries(GraphicsDataGL *gd);


unsigned int SizeOfModelData(void)
{
	return sizeof(ModelData);
}

void SetUpModelData(ModelData *md)
{
#if defined(CLIFFORD_FLOWS_FOR_TALKS)
	md->itsStereoMode	= StereoColor;
#else
	//	Leave stereo off by default.
	md->itsStereoMode	= StereoNone;
#endif

	//	Set up the view dimensions.
	//
	//	Initialize the parameters for a 90° field of view.
	//	In monoscopic 3D this gives a nice wide-angle view.
	//	In stereoscopic 3D a 90° field of view will be practical
	//	only with a very large display -- for example in a lecture hall --
	//	relative to which the separation between the user's eyes is small.
	//	Accurate stereoscopic 3D is more difficult on an ordinary monitor,
	//	but is workable if you underreport the separation between your eyes.
	//
	//	Curved Spaces, unlike the other Geometry Games,
	//	maintains all measurements in intrinsic units.
	//	When the user resizes the display the image gets larger or smaller,
	//	but the field of view remains the same (90° by default, in a square window).
	//
	md->itsCharacteristicSizeIU	= GetUserPrefFloat(u"characteristic size iu");
	md->itsViewingDistanceIU	= GetUserPrefFloat(u"viewing distance iu");
	md->itsEyeOffsetIU			= GetUserPrefFloat(u"eye offset iu");

	//	No redraw request is pending.
	md->itsRedrawRequestFlag	= false;


	//	Set up application-specific data.

	md->itsSpaceType			= SpaceNone;
	md->itsDrawBackHemisphere	= false;
	md->itsThreeSphereFlag		= false;
	md->itsTilingRadius			= 0.0;	//	LoadGenerators() will set the tiling radius.
	md->itsDrawingRadius		= 0.0;	//	LoadGenerators() will set the drawing radius.
	MatrixIdentity(&md->itsUserPlacement);
	md->itsUserSpeed			= 0.0;	//	LoadGenerators() will set the speed.

#ifdef CENTERPIECE_DISPLACEMENT
	MatrixIdentity(&md->itsCenterpiecePlacement);
#endif

	md->itsDirichletDomain		= NULL;
	md->itsHoneycomb			= NULL;

#if defined(START_STILL)
	md->itsDesiredAperture		= 0.00;
	md->itsCurrentAperture		= 0.00;
	md->itsCenterpiece			= CenterpieceEarth;
#elif defined(CENTERPIECE_DISPLACEMENT)
	md->itsDesiredAperture		= 0.00;
	md->itsCurrentAperture		= 0.00;
	md->itsCenterpiece			= CenterpieceEarth;
#elif defined(START_OUTSIDE)
	md->itsDesiredAperture		= 0.00;
	md->itsCurrentAperture		= 0.00;
	md->itsCenterpiece			= CenterpieceGalaxy;
#elif defined(START_WALLS_OPEN)
	md->itsDesiredAperture		= 0.875;
	md->itsCurrentAperture		= 0.875;
	md->itsCenterpiece			= CenterpieceNone;
#elif defined(CLIFFORD_FLOWS_FOR_TALKS)
	md->itsDesiredAperture		= 0.9375;
	md->itsCurrentAperture		= 0.9375;
	md->itsCenterpiece			= CenterpieceNone;
#elif defined(HIGH_RESOLUTION_SCREENSHOT)
	md->itsDesiredAperture		= 0.875;
	md->itsCurrentAperture		= 0.875;
	md->itsCenterpiece			= CenterpieceNone;
#else
	md->itsDesiredAperture		= 0.25;
	md->itsCurrentAperture		= 0.25;
	md->itsCenterpiece			= CenterpieceEarth;
#endif

	md->itsRotationAngle		= 0.0;

#if    defined(START_STILL)					\
	|| defined(CENTERPIECE_DISPLACEMENT)	\
	|| defined(START_OUTSIDE)				\
	|| defined(START_WALLS_OPEN)			\
	|| defined(HANTZSCHE_WENDT_AXES)		\
	|| defined(CLIFFORD_FLOWS_FOR_TALKS)	\
	|| defined(HIGH_RESOLUTION_SCREENSHOT)
	md->itsShowObserver			= false;
#else
	md->itsShowObserver			= true;
#endif

#ifdef CENTERPIECE_DISPLACEMENT
	md->itsShowColorCoding		= true;
#else
	md->itsShowColorCoding		= false;
#endif

	md->itsCliffordMode			= CliffordNone;
#ifdef CLIFFORD_FLOWS_FOR_TALKS
	md->itsCliffordFlowXYEnabled = false;
	md->itsCliffordFlowZWEnabled = false;
#endif
	md->itsShowVertexFigures	= false;
#ifdef HIGH_RESOLUTION_SCREENSHOT
	md->itsFogFlag				= false;
#else
	md->itsFogFlag				= true;
#endif
	md->itsFogSaturation		= 1.0;

#ifdef START_OUTSIDE
	md->itsViewpoint			= ViewpointExtrinsic;
	md->itsViewpointTransition	= 1.0;
	md->itsExtrinsicRotation	= 0.0;
#endif

#ifdef HANTZSCHE_WENDT_AXES
	md->itsHantzscheWendtSpaceIsLoaded	= false;
	md->itsShowHantzscheWendtAxes		= false;
#endif
}

void ShutDownModelData(ModelData *md)
{
	//	Free any allocated memory.
	//	Leave other information untouched.
	FreeDirichletDomain(&md->itsDirichletDomain);
	FreeHoneycomb(&md->itsHoneycomb);
}


#ifdef SUPPORT_OPENGL

void ZeroGraphicsDataGL(GraphicsDataGL *gd)
{
	unsigned int	i;
	
	//	Request that all OpenGL objects be (re)created.
	gd->itsPreparedGLVersion	= false;
	gd->itsPreparedShaders		= false;
	gd->itsPreparedTextures		= false;
	gd->itsPreparedVBOs			= false;
	gd->itsPreparedVAOs			= false;
	gd->itsPreparedQueries		= false;
	
	//	Initialize itsDirichletVBOAperture to an invalid value.
	//	This will trigger a single unnecessary reconstruction of the Dirichlet VBO,
	//	but is otherwise safe and robust.
	gd->itsDirichletVBOAperture = -1.0;
	
	//	No shaders, textures, etc. are present.

	for (i = 0; i < NumShaders; i++)
		gd->itsShaderPrograms[i] = 0;

	for (i = 0; i < NumTextures; i++)
		gd->itsTextureNames[i] = 0;

	for (i = 0; i < NumVertexBuffers; i++)
	{
		gd->itsVertexBufferNames[i] = 0;
		gd->itsIndexBufferNames [i] = 0;
	}

	for (i = 0; i < NumVertexArrayObjects; i++)
		gd->itsVertexArrayNames[i] = 0;

	for (i = 0; i < NumQueries; i++)
		gd->itsQueryNames[i] = 0;
}

ErrorText SetUpGraphicsAsNeeded(
	ModelData		*md,
	GraphicsDataGL	*gd)
{
	//	These are one-time initializations,
	//	so usually no work is required here.
	//	Only in exceptional circumstances, for example
	//	when the user has selected a new manifold,
	//	will something need an update.

	//	Assume the OpenGL context has already been set.
	
	ErrorText	theError;

	if ( ! gd->itsPreparedGLVersion )
	{
		if ((theError = ConfirmOpenGLVersion()) != NULL)
			return theError;
		gd->itsPreparedGLVersion = true;

		//	Availability of extensions may influence
		//	the construction of textures, etc.,
		//	so rebuild everything from scratch.
		//	(The current implementation no longer uses
		//	any extensions, but leave this code in place anyhow,
		//	for future robustness.)
		gd->itsPreparedShaders	= false;
		gd->itsPreparedTextures	= false;
		gd->itsPreparedVBOs		= false;
		gd->itsPreparedVAOs		= false;
		gd->itsPreparedQueries	= false;
	}

	if ( ! gd->itsPreparedShaders )
	{
		if ((theError = SetUpShaders(gd)) != NULL)
			return theError;
		gd->itsPreparedShaders	= true;
	}

	if ( ! gd->itsPreparedTextures )
	{
		if ((theError = SetUpTextures(gd, md->itsStereoMode)) != NULL)
			return theError;
		gd->itsPreparedTextures	= true;
	}

	if ( ! gd->itsPreparedVBOs )
	{
		if ((theError = SetUpVBOs(	gd,
									md->itsDirichletDomain,
									md->itsCurrentAperture,
									md->itsShowColorCoding,
									md->itsStereoMode,
									md->itsCenterpiece,
									md->itsShowObserver,
									md->itsShowVertexFigures,
									md->itsCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
									, md->itsShowHantzscheWendtAxes
#endif
									)) != NULL)
			return theError;
		gd->itsPreparedVBOs = true;
		
		//	Rebuild VAOs using new VBOs.
		gd->itsPreparedVAOs = false;
	}

	if ( ! gd->itsPreparedVAOs )
	{
		if ((theError = SetUpVAOs(	gd,
									md->itsCenterpiece,
									md->itsShowObserver,
									md->itsShowVertexFigures,
									md->itsCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
									, md->itsShowHantzscheWendtAxes
#endif
									)) != NULL)
			return theError;
		gd->itsPreparedVAOs = true;
	}

	if ( ! gd->itsPreparedQueries )
	{
		if ((theError = SetUpQueries(gd)) != NULL)
			return theError;
		gd->itsPreparedQueries = true;
	}
	
	//	When the user resizes the aperture,
	//	rebuild the Dirichlet domain's VBO with the new aperture size.
	if (gd->itsDirichletVBOAperture != md->itsCurrentAperture)
	{
		//	It's fine to let MakeDirichletVBO() call glBufferData(),
		//	which may be more efficient than calling glBufferSubData()
		//	because a full glBufferData() call lets the driver know
		//	that it needn't preserve the buffer's previous contents.
		//
		//	Rewriting the index buffer's contents is redundant,
		//	but it's not a big deal and keeps the code clean.
		//
		if ((theError = MakeDirichletVBO(	gd->itsVertexBufferNames[VertexBufferDirichlet],
											gd->itsIndexBufferNames [VertexBufferDirichlet],
											md->itsDirichletDomain,
											md->itsCurrentAperture,
											md->itsShowColorCoding,
											md->itsStereoMode == StereoGreyscale)) != NULL)
			return theError;

		gd->itsDirichletVBOAperture = md->itsCurrentAperture;
	}

	return NULL;
}

void ShutDownGraphicsAsNeeded(
	ModelData		*md,
	GraphicsDataGL	*gd)
{
	UNUSED_PARAMETER(md);

	//	Assume the OpenGL context has already been set.

	//	Shut down the various elements in the opposite order
	//	from that in which they were created.  In particular,
	//	shut down the extensions last, so the other shutdown routines
	//	will know what extensions have been available.
	ShutDownQueries(gd);
	ShutDownVAOs(gd);
	ShutDownVBOs(gd);
	ShutDownTextures(gd);
	ShutDownShaders(gd);

	gd->itsPreparedGLVersion	= false;
	gd->itsPreparedShaders		= false;
	gd->itsPreparedTextures		= false;
	gd->itsPreparedVBOs			= false;
	gd->itsPreparedVAOs			= false;
	gd->itsPreparedQueries		= false;
}


static ErrorText SetUpShaders(GraphicsDataGL *gd)
{
	//	Technical Note:  At run time, swapping simple shaders in and out as needed
	//	turns out to be faster than running a single all-purpose shader with options,
	//	because (much to my surprise) an if/then block containing texture sampling 
	//	slows the fragment shader to a crawl (at least on my Radeon X1600).

	ErrorText	theError;
	
	static const VertexAttributeBinding	theVertexAttributeBindings[] =
	{
		{ATTRIBUTE_POSITION,		"atrPosition"			},
		{ATTRIBUTE_TEX_COORD,		"atrTextureCoordinates"	},
		{ATTRIBUTE_COLOR,			"atrColor"				},
		{ATTRIBUTE_MV_MATRIX_ROW_0,	"atrModelViewMatrix"	}
	};

	glUseProgram(0);

	theError = SetUpOneShaderProgram(	&gd->itsShaderPrograms[ShaderSph],
										u"CurvedSpaces.vs",
										u"CurvedSpaces.fs",
										BUFFER_LENGTH(theVertexAttributeBindings),
										theVertexAttributeBindings,
										"#define SPHERICAL_FOG\n");
	if (theError != NULL)
		return theError;

	theError = SetUpOneShaderProgram(	&gd->itsShaderPrograms[ShaderEuc],
										u"CurvedSpaces.vs",
										u"CurvedSpaces.fs",
										BUFFER_LENGTH(theVertexAttributeBindings),
										theVertexAttributeBindings,
										"#define EUCLIDEAN_FOG\n");
	if (theError != NULL)
		return theError;

	theError = SetUpOneShaderProgram(	&gd->itsShaderPrograms[ShaderHyp],
										u"CurvedSpaces.vs",
										u"CurvedSpaces.fs",
										BUFFER_LENGTH(theVertexAttributeBindings),
										theVertexAttributeBindings,
										"#define HYPERBOLIC_FOG\n");
	if (theError != NULL)
		return theError;
	
	//	Did any OpenGL errors occur?
	return GetErrorString();
}

static void ShutDownShaders(GraphicsDataGL *gd)
{
	unsigned int	i;

	glUseProgram(0);

	for (i = 0; i < NumShaders; i++)
	{
		glDeleteProgram(gd->itsShaderPrograms[i]);
		gd->itsShaderPrograms[i] = 0;
	}
}


static ErrorText SetUpTextures(
	GraphicsDataGL	*gd,
	StereoMode		aStereoMode)
{
	ErrorText		theFirstError	= NULL;
	GLint			theMinificationMode;
	AnisotropicMode	theAnisotropicMode;
	GreyscaleMode	theGreyscaleMode;

#ifdef SUPPORT_DESKTOP_OPENGL
	theMinificationMode	= GL_LINEAR_MIPMAP_LINEAR;
	theAnisotropicMode	= AnisotropicOn;
#elif defined(SUPPORT_OPENGL_ES)
	theMinificationMode	= GL_LINEAR_MIPMAP_NEAREST;
	theAnisotropicMode	= AnisotropicOff;
#else
#error Compile-time error:  no OpenGL support.  
#endif
	theGreyscaleMode	= (aStereoMode == StereoGreyscale ? GreyscaleOn : GreyscaleOff);
	

	SetUpOneTexture(&gd->itsTextureNames[TextureWallPaper],			u"Paper.rgba",
		GL_REPEAT,			theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureWallWood],			u"Wood.rgba",
		GL_REPEAT,			theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureEarth],				u"Earth.rgba",
		GL_CLAMP_TO_EDGE,	theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureGalaxy],			u"Galaxy.rgba",
		GL_CLAMP_TO_EDGE,	theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureGyroscope],			u"White.rgba",
		GL_CLAMP_TO_EDGE,	theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureObserver],			u"White.rgba",
		GL_CLAMP_TO_EDGE,	theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureVertexFigures],		u"Stone.rgba",
		GL_REPEAT,			theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	SetUpOneTexture(&gd->itsTextureNames[TextureClifford],			u"Clifford.rgba",
		GL_REPEAT,			theMinificationMode, theAnisotropicMode, theGreyscaleMode,
		TextureRGBA, &theFirstError);

	if (theFirstError != NULL)
		return theFirstError;
	else
		return GetErrorString();
}

static void ShutDownTextures(GraphicsDataGL *gd)
{
	unsigned int	i;

	//	Delete all textures.
	//	glDeleteTextures() will silently ignore zeros and invalid names.
	glDeleteTextures(NumTextures, gd->itsTextureNames);

	//	Clear all texture names.
	for (i = 0; i < NumTextures; i++)
		gd->itsTextureNames[i] = 0;
}


static ErrorText SetUpVBOs(
	GraphicsDataGL	*gd,
	DirichletDomain	*aDirichletDomain,
	double			aCurrentAperture,
	bool			aShowColorCoding,
	StereoMode		aStereoMode,
	CenterpieceType	aCenterpiece,
	bool			aShowObserver,
	bool			aShowVertexFigures,
	CliffordMode	aCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
	, bool			aShowHantzscheWendtAxes
#endif
	)
{
	ErrorText	theError	= NULL;

	//	Release any pre-existing VBOs.
	ShutDownVBOs(gd);
	
	//	Generate new VBO names.
	glGenBuffers(NumVertexBuffers, gd->itsVertexBufferNames);
	glGenBuffers(NumVertexBuffers, gd->itsIndexBufferNames );

	//	Set up the individual Vertex Buffer Objects.

	theError = MakeDirichletVBO(	gd->itsVertexBufferNames[VertexBufferDirichlet],
									gd->itsIndexBufferNames [VertexBufferDirichlet],
									aDirichletDomain,
									aCurrentAperture,
									aShowColorCoding,
									aStereoMode == StereoGreyscale);
	if (theError != NULL)
		return theError;


	switch (aCenterpiece)
	{
		case CenterpieceNone:
			break;

		case CenterpieceEarth:
			//	The Earth is the only centerpiece that can return an error message,
			//	because it's the only one that relies on dynamically allocated memory.
			MakeEarthVBO(	gd->itsVertexBufferNames[VertexBufferEarth],
							gd->itsIndexBufferNames [VertexBufferEarth]);
			break;

		case CenterpieceGalaxy:
			MakeGalaxyVBO(	gd->itsVertexBufferNames[VertexBufferGalaxy],
							gd->itsIndexBufferNames [VertexBufferGalaxy]);
			break;

		case CenterpieceGyroscope:
			MakeGyroscopeVBO(	gd->itsVertexBufferNames[VertexBufferGyroscope],
								gd->itsIndexBufferNames [VertexBufferGyroscope],
								aStereoMode == StereoGreyscale);
			break;
	}
	
	if (aShowObserver)
		MakeObserverVBO(	gd->itsVertexBufferNames[VertexBufferObserver],
							gd->itsIndexBufferNames [VertexBufferObserver],
							aStereoMode == StereoGreyscale);

	if (aShowVertexFigures)
		MakeVertexFiguresVBO(	gd->itsVertexBufferNames[VertexBufferVertexFigures],
								gd->itsIndexBufferNames [VertexBufferVertexFigures],
								aDirichletDomain);

	if (aCliffordMode != CliffordNone)
		MakeCliffordVBO(	gd->itsVertexBufferNames[VertexBufferClifford],
							gd->itsIndexBufferNames [VertexBufferClifford]);

#ifdef HANTZSCHE_WENDT_AXES
	if (aShowHantzscheWendtAxes)
		MakeHantzscheWendtVBO(	gd->itsVertexBufferNames[VertexBufferHantzscheWendt],
								gd->itsIndexBufferNames [VertexBufferHantzscheWendt]);
#endif

	//	Did any OpenGL errors occur?
	return GetErrorString();	
}

static void ShutDownVBOs(GraphicsDataGL *gd)
{
	unsigned int	i;

	//	Delete all Vertex Buffer Objects.
	//	glDeleteBuffers() will silently ignore zeros and invalid names.
	glDeleteBuffers(NumVertexBuffers, gd->itsVertexBufferNames);
	glDeleteBuffers(NumVertexBuffers, gd->itsIndexBufferNames );

	//	Clear all VBO names.
	for (i = 0; i < NumVertexBuffers; i++)
	{
		gd->itsVertexBufferNames[i] = 0;
		gd->itsIndexBufferNames [i] = 0;
	}
}


static ErrorText SetUpVAOs(
	GraphicsDataGL	*gd,
	CenterpieceType	aCenterpiece,
	bool			aShowObserver,
	bool			aShowVertexFigures,
	CliffordMode	aCliffordMode
#ifdef HANTZSCHE_WENDT_AXES
	, bool			aShowHantzscheWendtAxes
#endif
	)
{
	//	Release any pre-existing VAOs.
	ShutDownVAOs(gd);
	
	//	Generate new VAO names.
	glGenVertexArrays(NumVertexArrayObjects, gd->itsVertexArrayNames);

	//	Set up the individual Vertex Array Objects.

	MakeDirichletVAO(	gd->itsVertexArrayNames[VertexArrayObjectDirichlet],
						gd->itsVertexBufferNames[VertexBufferDirichlet],
						gd->itsIndexBufferNames [VertexBufferDirichlet]);

	switch (aCenterpiece)
	{
		case CenterpieceNone:
			break;

		case CenterpieceEarth:
			MakeEarthVAO(	gd->itsVertexArrayNames[VertexArrayObjectEarth],
							gd->itsVertexBufferNames[VertexBufferEarth],
							gd->itsIndexBufferNames [VertexBufferEarth]);
			break;

		case CenterpieceGalaxy:
			MakeGalaxyVAO(	gd->itsVertexArrayNames[VertexArrayObjectGalaxy],
							gd->itsVertexBufferNames[VertexBufferGalaxy],
							gd->itsIndexBufferNames [VertexBufferGalaxy]);
			break;

		case CenterpieceGyroscope:
			MakeGyroscopeVAO(	gd->itsVertexArrayNames[VertexArrayObjectGyroscope],
								gd->itsVertexBufferNames[VertexBufferGyroscope],
								gd->itsIndexBufferNames [VertexBufferGyroscope]);
			break;
	}
	
	if (aShowObserver)
		MakeObserverVAO(	gd->itsVertexArrayNames[VertexArrayObjectObserver],
							gd->itsVertexBufferNames[VertexBufferObserver],
							gd->itsIndexBufferNames [VertexBufferObserver]);

	if (aShowVertexFigures)
		MakeVertexFiguresVAO(	gd->itsVertexArrayNames[VertexArrayObjectVertexFigures],
								gd->itsVertexBufferNames[VertexBufferVertexFigures],
								gd->itsIndexBufferNames [VertexBufferVertexFigures]);

	if (aCliffordMode != CliffordNone)
		MakeCliffordVAO(	gd->itsVertexArrayNames[VertexArrayObjectClifford],
							gd->itsVertexBufferNames[VertexBufferClifford],
							gd->itsIndexBufferNames [VertexBufferClifford]);

#ifdef HANTZSCHE_WENDT_AXES
	if (aShowHantzscheWendtAxes)
		MakeHantzscheWendtVAO(	gd->itsVertexArrayNames[VertexArrayObjectHantzscheWendt],
								gd->itsVertexBufferNames[VertexBufferHantzscheWendt],
								gd->itsIndexBufferNames [VertexBufferHantzscheWendt]);
#endif

	//	Did any OpenGL errors occur?
	return GetErrorString();
}

static void ShutDownVAOs(GraphicsDataGL *gd)
{
	unsigned int	i;

	//	Delete all Vertex Array Objects.
	//	glDeleteVertexArrays() will silently ignore zeros and invalid names.
	glDeleteVertexArrays(NumVertexArrayObjects, gd->itsVertexArrayNames);

	//	Clear all VAO names.
	for (i = 0; i < NumVertexArrayObjects; i++)
		gd->itsVertexArrayNames[i] = 0;
}


static ErrorText SetUpQueries(GraphicsDataGL *gd)
{
#ifdef SUPPORT_DESKTOP_OPENGL

	//	Release any pre-existing objects.
	ShutDownQueries(gd);
	
	//	Generate new query names.
	glGenQueries(NumQueries, gd->itsQueryNames);
	
	//	Did any OpenGL errors occur?
	return GetErrorString();

#else

	UNUSED_PARAMETER(md);

	return NULL;

#endif
}

static void ShutDownQueries(GraphicsDataGL *gd)
{
#ifdef SUPPORT_DESKTOP_OPENGL

	unsigned int	i;

	//	Delete all queries.
	//	glDeleteQueries() will silently ignore zeros and unused names.
	glDeleteQueries(NumQueries, gd->itsQueryNames);

	//	Clear all query names.
	for (i = 0; i < NumQueries; i++)
		gd->itsQueryNames[i] = 0;

#else

	UNUSED_PARAMETER(md);

#endif
}

#endif	//	SUPPORT_OPENGL
