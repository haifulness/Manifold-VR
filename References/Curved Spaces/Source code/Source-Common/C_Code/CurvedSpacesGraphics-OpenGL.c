//	CurvedSpacesGraphics-OpenGL.c
//
//	Drawing code for Curved Spaces.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#include "CurvedSpacesGraphics-OpenGL.h"
#include "CurvedSpaces-Common.h"
#include <math.h>


//	The near clipping distance is 1/INVERSE_NEAR_CLIP.
//
//	See comments in CurvedSpacesObserver.c for an explanation
//	of why INVERSE_NEAR_CLIP should be at least 1/(0.004/2) ≈ 500.
//	However, it shouldn't be unnecessarily large, to avoid
//	needless loss of precision in the depth buffer.
#define INVERSE_NEAR_CLIP	512.0

//	How fast should the galaxy, Earth and gyroscope spin?
//	Express their speeds as integer multiples of the default rotation speed.
//	The reason they're integer multiples is that itsRotationAngle
//	occasionally jumps by 2π.
#define GALAXY_SPEED			1
#define EARTH_SPEED				2
#define GYROSCOPE_SPEED			6

#ifdef START_OUTSIDE
//	When viewing the fundamental polyhedron from outside,
//	how far away should it sit?
#define EXTRINSIC_VIEWING_DISTANCE	0.75
#endif


typedef enum
{
	BoxFull,	//	render into the full clipping box -w ≤ z ≤ w
	BoxFront,	//	render into the front half        -w ≤ z ≤ 0
	BoxBack		//	render into the back  half         0 ≤ z ≤ w
} ClippingBoxPortion;


//	Dimensions of the view and its surroundings in intrinsic units.
//	Intrinsic units are the units of the model itself.
typedef struct
{
	double	itsViewWidthIU,
			itsViewHeightIU,
			itsViewingDistanceIU,	//	bridge of user's nose to center of display
			itsEyeOffsetIU;			//	bridge of user's nose to eye
} IntrinsicDimensions;


static void		ProjectAndDraw(ModelData *md, GraphicsDataGL *gd, IntrinsicDimensions *someIntrinsicDimensions, EyeType anEyeType);
static void		GetIntrinsicDimensions(ModelData *md, unsigned int aViewWidthPx, unsigned int aViewHeightPx, IntrinsicDimensions *someIntrinsicDimensions);
static void		SetProjectionMatrix(IntrinsicDimensions *someIntrinsicDimensions, EyeType anEyeType, SpaceType aSpaceType, ClippingBoxPortion aClippingBoxPortion, double aProjectionMatrix[4][4]);
static void		DrawTheScene(ModelData *md, GraphicsDataGL *gd, double aProjectionMatrix[4][4], bool aSceneryInversionFlag);
static void		DrawTheSceneIntrinsically(ModelData *md, GraphicsDataGL *gd, double aProjectionMatrix[4][4], bool aSceneryInversionFlag);
#ifdef START_OUTSIDE
static void		DrawTheSceneExtrinsically(ModelData *md, GraphicsDataGL *gd, bool aSceneryInversionFlag);
#endif


unsigned int SizeOfGraphicsDataGL(void)
{
	return sizeof(GraphicsDataGL);
}


ErrorText Render(
	ModelData		*md,
	GraphicsDataGL	*gd,
	unsigned int	aViewWidthPx,	//	input,  in pixels (not points)
	unsigned int	aViewHeightPx,	//	input,  in pixels (not points)
	unsigned int	*anElapsedTime)	//	output, in nanoseconds, may be NULL
{
	unsigned int		theShaderProgram;
	IntrinsicDimensions	theIntrinsicDimensions;

	//	If the framebuffer isn't ready, don't try to draw into it.
	//
	//	(For example, on Mac OS the framebuffer might not be ready
	//	if the CVDisplayLink thread tries to draw before
	//	the main thread has attached the context to its view.
	//	The framebuffer might also be temporarily unavailable
	//	when switching to and from fullscreen mode.)
	//
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return NULL;
	
#ifdef SUPPORT_DESKTOP_OPENGL
	//	Note the starting time on the GPU clock.
	if (anElapsedTime != NULL)
		glBeginQuery(GL_TIME_ELAPSED, gd->itsQueryNames[QueryTotalRenderTime]);
#endif

	//	Clear the color buffer and the depth buffer.
	//	For Curved Spaces an opaque black background works well.
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//	Depth testing serves us well.
	glEnable(GL_DEPTH_TEST);
	
	//	Set up a single default shader for the whole scene.

	//	Select a shader according to the SpaceType.
	switch (md->itsSpaceType)
	{
		case SpaceSpherical:	theShaderProgram = gd->itsShaderPrograms[ShaderSph];	break;
		case SpaceFlat:			theShaderProgram = gd->itsShaderPrograms[ShaderEuc];	break;
		case SpaceHyperbolic:	theShaderProgram = gd->itsShaderPrograms[ShaderHyp];	break;
		
		//	At launch no space is present.  This is fine.
		//	The user will select a space momentarily.
		case SpaceNone:
		default:
			goto CleanUpRender;
	}

	//	Enable the selected shader.
	glUseProgram(theShaderProgram);

	//	Set the amount of fog, ranging from 0.0 (fully transparent) to 1.0 (fully opaque).
	glUniform1f(glGetUniformLocation(theShaderProgram, "uniFogFactor"), md->itsFogSaturation);

	//	Blending determines how the final fragment
	//	blends in with the previous color buffer contents.
	//	For opaque surfaces we can disable blending.
	//	For partially transparent surfaces, such as the galaxy,
	//	we may enable blending but must take care to draw the scene
	//	in back-to-front order.
	//	The comment accompanying the definition of PREMULTIPLY_RGBA()
	//	explains the (1, 1 - α) blending coefficients.
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	//	Convert dimensions to intrinsic units.
	GetIntrinsicDimensions(md, aViewWidthPx, aViewHeightPx, &theIntrinsicDimensions);


	//	Draw the scene.
	switch (md->itsStereoMode)
	{
		case StereoNone:

			//	Set the viewport.
			glViewport(0, 0, aViewWidthPx, aViewHeightPx);

			//	Draw a full color image for a single eye.
			ProjectAndDraw(md, gd, &theIntrinsicDimensions, EyeOnly);

			break;

		case StereoGreyscale:
		case StereoColor:

			//	Set the viewport.
			glViewport(0, 0, aViewWidthPx, aViewHeightPx);

			//	Restrict to the red channel.
			glColorMask(true, false, false, true);

			//	Draw the left eye image.
			ProjectAndDraw(md, gd, &theIntrinsicDimensions, EyeLeft);

			//	Clear the z-buffer.
			glClear(GL_DEPTH_BUFFER_BIT);

			//	Restrict to the green and blue channels.
			glColorMask(false, true, true, true);

			//	Draw the right eye image.
			ProjectAndDraw(md, gd, &theIntrinsicDimensions, EyeRight);

			//	Re-enable all color channels.
			glColorMask(true, true, true, true);

			break;
	}

CleanUpRender:

	//	Note the stopping time on the GPU clock.
	if (anElapsedTime != NULL)
	{
#ifdef SUPPORT_DESKTOP_OPENGL
		glEndQuery(GL_TIME_ELAPSED);
		glGetQueryObjectuiv(gd->itsQueryNames[QueryTotalRenderTime], GL_QUERY_RESULT, anElapsedTime);
#else
		*anElapsedTime = 0;
#endif
	}

	//	Return a string describing the first of any OpenGL errors
	//	that may have occurred.  If no errors occurred, return NULL.
	return GetErrorString();
}


static void GetIntrinsicDimensions(
	ModelData			*md,						//	input
	unsigned int		aViewWidthPx,				//	input, in pixels (not points)
	unsigned int		aViewHeightPx,				//	input, in pixels (not points)
	IntrinsicDimensions	*someIntrinsicDimensions)	//	output
{
	unsigned int	theCharacteristicSizePx;	//	in pixels (not points)
	double			theIntrinsicUnitsPerPixel;

	theCharacteristicSizePx = CharacteristicViewSize(aViewWidthPx, aViewHeightPx);
	if (theCharacteristicSizePx == 0)
	{
		someIntrinsicDimensions->itsViewWidthIU			= 1.0;
		someIntrinsicDimensions->itsViewHeightIU		= 1.0;
		someIntrinsicDimensions->itsViewingDistanceIU	= 1.0;
		someIntrinsicDimensions->itsEyeOffsetIU			= 1.0;
		return;
	}

	theIntrinsicUnitsPerPixel = md->itsCharacteristicSizeIU / theCharacteristicSizePx;

	someIntrinsicDimensions->itsViewWidthIU			= aViewWidthPx  * theIntrinsicUnitsPerPixel;
	someIntrinsicDimensions->itsViewHeightIU		= aViewHeightPx * theIntrinsicUnitsPerPixel;
	someIntrinsicDimensions->itsViewingDistanceIU	= md->itsViewingDistanceIU;
	someIntrinsicDimensions->itsEyeOffsetIU			= md->itsEyeOffsetIU;
}


static void ProjectAndDraw(
	ModelData			*md,
	GraphicsDataGL		*gd,
	IntrinsicDimensions	*someIntrinsicDimensions,
	EyeType				anEyeType)
{
	double	theProjectionMatrixDouble[4][4];
	float	theProjectionMatrixFloat[4][4];

	switch (md->itsSpaceType)
	{
		case SpaceSpherical:

			if (md->itsDrawBackHemisphere)
			{
				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterNear"), 0.000);	//	distance 0
				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterFar" ), 0.750);	//	distance π
				SetProjectionMatrix(someIntrinsicDimensions, anEyeType, SpaceSpherical, BoxFront, theProjectionMatrixDouble);
				Matrix44DoubleToFloat(theProjectionMatrixFloat, theProjectionMatrixDouble);
				glUniformMatrix4fv(	glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniProjectionMatrix"),
									1, GL_FALSE, (float *)theProjectionMatrixFloat);
				DrawTheScene(md, gd, theProjectionMatrixDouble, false);

				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterNear"), 0.750);	//	distance  π
				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterFar" ), 0.875);	//	distance 2π
				SetProjectionMatrix(someIntrinsicDimensions, anEyeType, SpaceSpherical, BoxBack,  theProjectionMatrixDouble);
				Matrix44DoubleToFloat(theProjectionMatrixFloat, theProjectionMatrixDouble);
				glUniformMatrix4fv(	glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniProjectionMatrix"),
									1, GL_FALSE, (float *)theProjectionMatrixFloat);
				DrawTheScene(md, gd, theProjectionMatrixDouble, true);
			}
			else
			{
				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterNear"), 0.000);	//	distance 0
				glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniFogParameterFar" ), 1.0);//0.750);	//	distance π
				SetProjectionMatrix(someIntrinsicDimensions, anEyeType, SpaceSpherical, BoxFull,  theProjectionMatrixDouble);
				Matrix44DoubleToFloat(theProjectionMatrixFloat, theProjectionMatrixDouble);
				glUniformMatrix4fv(	glGetUniformLocation(gd->itsShaderPrograms[ShaderSph], "uniProjectionMatrix"),
									1, GL_FALSE, (float *)theProjectionMatrixFloat);
				DrawTheScene(md, gd, theProjectionMatrixDouble, false);
			}

			break;
		
		case SpaceFlat:

			//	Fog is proportional to d².
			//	Rather than passing that saturation distance directly,
			//	and thus forcing the vertex shader to compute an inverse square,
			//	pass the saturation distance in a pre-digested form.
			//	Better to compute the inverse square once and for all here,
			//	rather than over and over in the shader, once for each vertex.
			glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderEuc], "uniInverseSquareFogSaturationDistance"),
						1.0 / (md->itsDrawingRadius*md->itsDrawingRadius));
			SetProjectionMatrix(someIntrinsicDimensions, anEyeType, SpaceFlat, BoxFull, theProjectionMatrixDouble);
			Matrix44DoubleToFloat(theProjectionMatrixFloat, theProjectionMatrixDouble);
			glUniformMatrix4fv(	glGetUniformLocation(gd->itsShaderPrograms[ShaderEuc], "uniProjectionMatrix"),
								1, GL_FALSE, (float *)theProjectionMatrixFloat);
			DrawTheScene(md, gd, theProjectionMatrixDouble, false);

			break;
		
		case SpaceHyperbolic:

			//	Fog is proportional to log(w) = log(cosh(d)).
			//	Rather than passing that saturation distance directly,
			//	and thus forcing the vertex shader to compute an inverse log cosh,
			//	pass the saturation distance in a pre-digested form.
			//	Better to compute the inverse log cosh once and for all here,
			//	rather than over and over in the shader, once for each vertex.
			//
			//	Letting the fog saturate at itsTilingRadius instead of 
			//	at itsDrawingRadius shows a little more of the tiling
			//	at the expense of a tiny bit of "popping".
			glUniform1f(glGetUniformLocation(gd->itsShaderPrograms[ShaderHyp], "uniInverseLogCoshFogSaturationDistance"),
#if defined(START_WALLS_OPEN) || defined(HIGH_RESOLUTION_SCREENSHOT)
						//	For the Curvature talk we're tiling deep enough 
						//	that we don't need fog to suppress flickering
						//	(the beams are so thin and the Earth so small that
						//	they "self-suppress" at large distances).
						//	We do still need some fog for a depth cue, though,
						//	so try half-strength fog.
						0.5 / log(cosh(md->itsTilingRadius)));
#else
						1.0 / log(cosh(md->itsTilingRadius)));
#endif
			SetProjectionMatrix(someIntrinsicDimensions, anEyeType, SpaceHyperbolic, BoxFull, theProjectionMatrixDouble);
			Matrix44DoubleToFloat(theProjectionMatrixFloat, theProjectionMatrixDouble);
			glUniformMatrix4fv(	glGetUniformLocation(gd->itsShaderPrograms[ShaderHyp], "uniProjectionMatrix"),
								1, GL_FALSE, (float *)theProjectionMatrixFloat);
			DrawTheScene(md, gd, theProjectionMatrixDouble, false);

			break;
		
		default:
			return;
	}
}


static void SetProjectionMatrix(
	IntrinsicDimensions	*someIntrinsicDimensions,
	EyeType				anEyeType,
	SpaceType			aSpaceType,
	ClippingBoxPortion	aClippingBoxPortion,
	double				aProjectionMatrix[4][4])
{
	//	How to Build a Projection Matrix
	//
	//	The key is to think... projectively!  After applying our projection matrix,
	//	the GPU will divide through by the last coordinate,
	//	so points (x,y,z,w) and c(x,y,z,w) are equivalent for all positive c.
	//	(They'd be equivalent for negative c as well, except for clipping considerations.)
	//	Thus each projective point corresponds to a ray from the origin.
	//	Rays from the origin correspond, in turn, to points on S³,
	//	so we may visualize world space as S³ if we wish.
	//
	//	The curvature of the space being modelled (spherical, flat or hyperbolic)
	//	is almost irrelevant.  The only difference is that spherical space occupies
	//	all of S³, while flat space occupies only the northern hemisphere
	//	(excluding the equator, which corresponds to the Euclidean sphere at infinity)
	//	and hyperbolic space occupies only the disk above 45° north latitude.
	//
	//	OpenGL clips to a "clipping wedge" bounded by the six hyperplanes
	//
	//			-w ≤ x ≤ w
	//			-w ≤ y ≤ w
	//			-w ≤ z ≤ w
	//
	//	This wedge lies entirely in the upper half space w ≥ 0, which is why
	//	points (x,y,z,w) and c(x,y,z,w) are *not* equivalent when c is negative.
	//	The clipping wedge intersects the hyperplane w == 1 in a cube
	//	-1 ≤ x ≤ +1, -1 ≤ y ≤ +1, -1 ≤ z ≤ +1, which is a convenient way
	//	to visualize it.  The whole purpose of a projection matrix is to move
	//	a desired "view volume" (the portion of world space that we want to see)
	//	into the "clipping wedge".
	//
	//	Let us construct a projection matrix in steps.
	//	Each step corresponds to a block of code below,
	//	but the order in which we explain the steps differs
	//	from the order in which we apply them in the code.
	//
	//	Step 0.  The simplest possible starting point is the identity matrix
	//
	//			1  0  0  0
	//			0  1  0  0
	//			0  0  1  0
	//			0  0  0  1
	//
	//	The clipping wedge itself defines the view volume, which
	//	the user sees from the perspective of an observer at (0,0,-1,0).
	//	In the flat case, this is effectively an observer
	//	at negative infinity on the z-axis.
	//	In all cases, the observer's field of view extends 45°
	//	to the left, to the right, above and below the centerline.
	//
	//	Step 1.  Initialize the projection matrix to the quarter turn
	//
	//			1  0  0  0
	//			0  1  0  0
	//			0  0  0  1
	//			0  0 -1  0
	//
	//		which rotates a view volume in the front hemisphere (z > 0)
	//		onto the clipping wedge.  The observer sits at (0,0,0,1)
	//		in world coordinates (before applying the quarter turn),
	//		in agreement with usual practices.
	//		In the flat case, the near clipping plane is at z = 1
	//		while the far clipping plane lies "beyond infinity".
	//		In the spherical case, the view volume extends from π/4 to 3π/4 radians.
	//
	//	Step 2.  Adjust the near and far clipping planes.
	//
	//		In all three geometries we want the near clipping plane to pass through
	//		the point (0, 0, NEAR_CLIP, 1), which after the rotation of Step 1
	//		becomes (0, 0, -1, NEAR_CLIP) or equivalently (0, 0, -1/NEAR_CLIP, 1).
	//
	//		In the spherical case the far clipping plane should lie at an equal
	//		distance from the south pole, at the point (0, 0, NEAR_CLIP, -1), which
	//		rotates to (0, 0, +1, NEAR_CLIP) or equivalently (0, 0, +1/NEAR_CLIP, 1).
	//
	//		In the flat case the far clipping plane passes through (0, 0, 1, 0),
	//		which rotates to (0, 0, 0, 1).
	//
	//		In the hyperbolic case the far clipping plane passes through (0, 0, 1, 1),
	//		which rotates to (0, 0, -1, 1).  However, the distance from
	//		(0, 0, -1/NEAR_CLIP, 1) to (0, 0, -1, 1) is almost as great as
	//		the distance from (0, 0, -1/NEAR_CLIP, 1) to (0, 0, 0, 1), so there's
	//		no harm in folding the hyperbolic case in with the flat case.
	//		(Folding it in could keep all quantities exact powers of 2,
	//		even though it's hard to image that the speed or accuracy
	//		of the subsequent matrix algebra would ever be an issue.)
	//
	//		In all three geometries, a shear transformation translates
	//		the hyperplane w == 1 through a distance (NearClip + FarClip)/2
	//		to center the desired view volume, and a compression by a factor
	//		of (FarClip - NearClip)/2 aligns it with the front and back
	//		of the standard clipping wedge.
	//
	//	Step 3.  Adjust the left, right, bottom and top clipping planes
	//			to accommodate a field of view other than ±45°.
	//
	//		Similar triangles dictate that the desired view volume's
	//		left and right faces pass through the points
	//		( ±WindowHalfWidth, 0, 0, WindowDistance) or equivalently
	//		( ±WindowHalfWidth/WindowDistance, 0, 0, 1), so rescale by
	//		a factor of WindowDistance/WindowHalfWidth to align
	//		the view volume's left and right faces with those of the standard
	//		clipping box, and similarly for the top and bottom faces.
	//
	//	Step 4.  Adjust the left, right, bottom and top clipping planes
	//			to allow off-axis viewing for stereoscopic 3D.
	//
	//		The user's left eye sees the display screen slightly to its right,
	//		so we need to use a view volume that's also slightly to the right.
	//		In other words, we want to translate the desired view volume
	//		leftwards by NoseToEyeDistance/WindowHalfWidth.
	//
	//	Step 5.  Translate the scenery to accommodate stereoscopic 3D.
	//
	//		The left eye sees the scenery slightly to the right of where
	//		a cyclops would see it.  So strictly speaking the last factor
	//		in the modelview matrix should be a rightwards translation
	//		through a distance NoseToEyeDistance/WindowHalfWidth.
	//		For convenience, though, let's make it the first factor
	//		in the projection matrix.
	//
	//	Step 6.  Handle the back hemisphere of S³, if required.
	//
	//		Most spherical manifolds yield tilings that are symmetrical
	//		under the antipodal map, in which case there's no need to draw
	//		the contents of the back hemisphere.  However, for tilings lacking
	//		antipodal symmetry (such as those arising from odd-order lens spaces)
	//		we do need to draw the back hemisphere as well as the front one.
	//		To do this, two modifications are required.
	//
	//		a.	The program draws the front hemisphere into the front half
	//			of the clipping box ( -w ≤ z ≤ 0 ) and draws the back hemisphere
	//			into the back half of the clipping box.
	//
	//		b.	For the back hemisphere, the program applies the antipodal map
	//			to all back-hemisphere scenery to bring it to an equivalent position
	//			in the front hemisphere.

	double	w,
			h,
			d,
			e,
			n,	//	near clipping plane passes through (0, 0, n, 1)
			f,	//	far  clipping plane passes through (0, 0, f, 1)
			theFactor[4][4],
			theFudgeFactor;

	w = 0.5 * someIntrinsicDimensions->itsViewWidthIU;	//	half width
	h = 0.5 * someIntrinsicDimensions->itsViewHeightIU;	//	half height
	d = someIntrinsicDimensions->itsViewingDistanceIU;
	switch (anEyeType)
	{
		case EyeOnly:	e = 0.0;										break;
		case EyeLeft:	e = +someIntrinsicDimensions->itsEyeOffsetIU;	break;
		case EyeRight:	e = -someIntrinsicDimensions->itsEyeOffsetIU;	break;
		default:		e = 0.0;										break;	//	unused
	}

	//	Initialize aProjectionMatrix to the identity.
	Matrix44Identity(aProjectionMatrix);

	//	Check our inputs just to be safe.
	if (w <= 0.0 || h <= 0.0 || d <= 0.0)
	{
		return;
	}


	//	Step 6a.  When drawing both hemispheres of S³,
	//	compress the front hemisphere into the front half of the clipping box,
	//	and the back hemisphere into the back half of the clipping box.
	if (aClippingBoxPortion != BoxFull)
	{
		Matrix44Identity(theFactor);
		theFactor[2][2] = 0.5;
		theFactor[3][2] = (aClippingBoxPortion == BoxFront ? -0.5 : +0.5);
		Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);
	}

	//	Step 4.  Adjust the left and right clipping planes for off-axis viewing.
	if (anEyeType != EyeOnly)
	{
		Matrix44Identity(theFactor);
		theFactor[3][0] = -e/w;
		Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);
	}

	//	Step 3.  Adjust the side clipping planes to match the window geometry.
	Matrix44Identity(theFactor);
	theFactor[0][0] = d/w;
	theFactor[1][1] = d/h;
	Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);

	//	Step 2.  Adjust the near and far clipping planes.
	//
	//	Clipping considerations for the observer's antipodal image:
	//
	//		In the spherical case, +INVERSE_NEAR_CLIP works 
	//		for a ±45° field of view, ensuring that the antipodal image 
	//		of the observer's spaceship remains fully visible.
	//		If the user widen's his/her field of view, we must multiply
	//		by theFudgeFactor to ensure that no visible portions 
	//		of that spaceship get clipped.
	//
	//		Note #1.  We multiply by w/d instead of d/w because we're working
	//		with an inverse clipping distance, not a plain clipping distance.
	//
	//		Note #2.  We're willing to move the far clipping plane closer
	//		to the antipode (so the spaceship's sides don't get clipped) 
	//		but we're not willing to move it further from the antipode
	//		(if it moved past the spaceship's transom, we'd wouldn't see 
	//		the spaceship at all!  plus we'd risk clipping away other stuff as well).
	//
	theFudgeFactor = w/d;		//	see Note #1
	if (theFudgeFactor < 1.0)	//	see Note #2
		theFudgeFactor = 1.0;
	n = -INVERSE_NEAR_CLIP;
	f = (aSpaceType == SpaceSpherical) ?
		+INVERSE_NEAR_CLIP * theFudgeFactor :	//	spherical
		0.0;									//	flat or hyperbolic
	Matrix44Identity(theFactor);
	theFactor[2][2] = 2/(f - n);
	theFactor[3][2] = (n + f)/(n - f);
	Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);

	//	Step 1.  Apply the quarter turn.
	Matrix44Identity(theFactor);
	theFactor[2][2] =  0.0;
	theFactor[2][3] = +1.0;
	theFactor[3][2] = -1.0;
	theFactor[3][3] =  0.0;
	Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);

	//	Step 5.  Translate the scenery to accommodate stereoscopic 3D.
	if (anEyeType != EyeOnly)
	{
		Matrix44Identity(theFactor);

		switch (aSpaceType)
		{
			case SpaceSpherical:
				theFactor[0][0] =  cos(e);	theFactor[0][3] = -sin(e);
				theFactor[3][0] =  sin(e);	theFactor[3][3] =  cos(e);
				break;

			case SpaceFlat:
				theFactor[0][0] =   1.0;  	theFactor[0][3] =   0.0;
				theFactor[3][0] =    e;   	theFactor[3][3] =   1.0;
				break;

			case SpaceHyperbolic:
				theFactor[0][0] = cosh(e);	theFactor[0][3] = sinh(e);
				theFactor[3][0] = sinh(e);	theFactor[3][3] = cosh(e);
				break;

			case SpaceNone:
				break;
		}

		Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);
	}

	//	Step 6b.  To draw the back hemisphere, invert all scenery.
	//
	//	I moved the scenery inversion from the projection matrix
	//	to the view matrix, so that the shader will fog the scenery correctly.
	//
//	if (aSceneryInversionFlag)
//	{
//		Matrix44Identity(theFactor);
//		theFactor[0][0] = -1.0;
//		theFactor[1][1] = -1.0;
//		theFactor[2][2] = -1.0;
//		theFactor[3][3] = -1.0;
//		Matrix44Product(theFactor, aProjectionMatrix, aProjectionMatrix);
//	}
}


static void DrawTheScene(
	ModelData		*md,
	GraphicsDataGL	*gd,
	double			aProjectionMatrix[4][4],
	bool			aSceneryInversionFlag)	//	Invert the scenery to draw the back half of S³?
{
#ifdef START_OUTSIDE
	if (md->itsViewpoint == ViewpointIntrinsic)
		DrawTheSceneIntrinsically(md, gd, aProjectionMatrix, aSceneryInversionFlag);
	else
		DrawTheSceneExtrinsically(md, gd, aSceneryInversionFlag);
#else
	DrawTheSceneIntrinsically(md, gd, aProjectionMatrix, aSceneryInversionFlag);
#endif
}


static void DrawTheSceneIntrinsically(
	ModelData		*md,
	GraphicsDataGL	*gd,
	double			aProjectionMatrix[4][4],
	bool			aSceneryInversionFlag)
{
	Matrix	theViewMatrix,
			theAntipodalMap,
			theProjectionMatrix,
			theViewProjectionMatrix,
			theSpin,
			theTilt,
			theOrientation,	//	“orientation” in the sense of an element of O(3),
							//		not a connected component of O(3)
			thePlacement;

	//	Set the current placement.
	//	The view matrix is the inverse of the eye matrix.
	MatrixGeometricInverse(&md->itsUserPlacement, &theViewMatrix);

	//	To draw the back hemisphere, invert all scenery.
	if (aSceneryInversionFlag)
	{
		MatrixAntipodalMap(&theAntipodalMap);
		MatrixProduct(&theViewMatrix, &theAntipodalMap, &theViewMatrix);
	}

	//	Compute the viewprojection transformation
	//	(into clipping coordinates) for use in culling.
	Matrix44Copy(theProjectionMatrix.m, aProjectionMatrix);
	theProjectionMatrix.itsParity = ImagePositive;	//	parity will be ignored
	MatrixProduct(&theViewMatrix, &theProjectionMatrix, &theViewProjectionMatrix);

	//	Determine which cells are visible relative
	//	to the current viewprojection matrix, and sort them
	//	in order of increasing distance from the observer
	//	(so transparency effects come out right, and also
	//	so level-of-detail gets applied correctly).
	SortVisibleCells(	md->itsHoneycomb,
						&theViewProjectionMatrix,
						&theViewMatrix,
						md->itsDrawingRadius);

	//	Draw all visible translates of the Dirichlet domain.
	if (md->itsCurrentAperture < 1.0)
	{
		BindDirichletVAO(gd->itsVertexArrayNames[VertexArrayObjectDirichlet]);
		DrawDirichletVAO(	gd->itsTextureNames[
								md->itsShowColorCoding ? TextureWallPaper : TextureWallWood],
							md->itsDirichletDomain,
							md->itsHoneycomb,
							&theViewMatrix,
							md->itsCurrentAperture);
	}

	//	Draw all visible translates of the observer if desired.
	//	Exception:  Suppress the observer in stereo 3D,
	//	because the user sees the two sides of the spaceship,
	//	the same way you see the two sides of your own nose
	//	in your peripheral version in everyday life.
	//	(Note:  Suppressing only the nearest copy of the spaceship
	//	doesn't quite solve the problem, because one encounters flickering
	//	as one flies exactly along a face of the fundamental domain.)
	if (md->itsShowObserver && md->itsStereoMode == StereoNone)
	{
		//	The central image of the Dirichlet domain never moves relative to space,
		//	so the observer's position in the Dirichlet domain is the same
		//	as his/her position in space.
		BindObserverVAO(gd->itsVertexArrayNames[VertexArrayObjectObserver]);
		DrawObserverVAO(gd->itsTextureNames[TextureObserver], md->itsHoneycomb, &theViewMatrix, &md->itsUserPlacement);
	}

	//	Draw all visible translates of the vertex figures if desired.
	if (md->itsShowVertexFigures)
	{
		BindVertexFiguresVAO(gd->itsVertexArrayNames[VertexArrayObjectVertexFigures]);
		DrawVertexFiguresVAO(gd->itsTextureNames[TextureVertexFigures], md->itsDirichletDomain, md->itsHoneycomb, &theViewMatrix);
	}

	//	Draw Clifford parallels if desired.
	if(md->itsCliffordMode != CliffordNone
#ifndef CLIFFORD_FLOWS_FOR_TALKS
	 && md->itsThreeSphereFlag
#endif
	)
	{
		BindCliffordVAO(gd->itsVertexArrayNames[VertexArrayObjectClifford]);
		DrawCliffordVAO(gd->itsTextureNames[TextureClifford],
						md->itsCliffordMode,
						md->itsStereoMode,
						&theViewMatrix);
	}

#ifdef HANTZSCHE_WENDT_AXES
	//	Draw Hantzsche-Wendt axes if desired.
	if (md->itsHantzscheWendtSpaceIsLoaded
	 && md->itsShowHantzscheWendtAxes)
	{
		BindHantzscheWendtVAO(gd->itsVertexArrayNames[VertexArrayObjectHantzscheWendt]);
			//	HACK ALERT:  The Hantzsche-Wendt axis doesn't have its own texture.
			//	Instead it uses the Clifford parallels' texture.
		DrawHantzscheWendtVAO(gd->itsTextureNames[TextureClifford], md->itsHoneycomb, &theViewMatrix);
	}
#endif

	//	Draw all visible translates of the centerpiece if desired.
	//	The centerpiece gets drawn last, because it may be partially transparent
	//	(at present the galaxy is partially transparent, but the Earth and gyroscope are not).
	//	Transparent objects must be drawn last, and in strict back-to-front order.
	if (md->itsCenterpiece != CenterpieceNone)
	{
		//	How should we position the centerpiece?
		switch (md->itsCenterpiece)
		{
			case CenterpieceEarth:
				MatrixRotation(&theSpin, 0.0, 0.0,   EARTH_SPEED   * md->itsRotationAngle);
				MatrixRotation(&theTilt, -PI/2, 0.0, 0.0);	//	takes Earth's spin axis from z-axis to y-axis
				break;

			case CenterpieceGalaxy:
				MatrixRotation(&theSpin, 0.0, 0.0,   GALAXY_SPEED  * md->itsRotationAngle);
				MatrixRotation(&theTilt, 0.2, 0.3, 0.0);	//	arbitrary spin axis that looks nice
				break;

			case CenterpieceGyroscope:
				MatrixRotation(&theSpin, 0.0, 0.0, GYROSCOPE_SPEED * md->itsRotationAngle);
				MatrixRotation(&theTilt, -PI/2, 0.0, 0.0);	//	takes gyroscope's spin axis from z-axis to y-axis
				break;

			default:	//	should never occur
				MatrixRotation(&theSpin, 0.0, 0.0, 0.0);
				MatrixRotation(&theTilt, 0.0, 0.0, 0.0);
				break;
		}
		
		//	Keeping in mind our left-to-right matrix conventions 
		//	(see “Curved Spaces 3/Read Me.txt”), first apply theSpin,
		//	then theTilt, and then the overall placement if it's enabled.
		MatrixProduct(&theSpin, &theTilt, &theOrientation);
#ifdef CENTERPIECE_DISPLACEMENT
		MatrixProduct(&theOrientation, &md->itsCenterpiecePlacement, &thePlacement);
#else
		thePlacement = theOrientation;
#endif

		//	Draw all visible translates of the centerpiece.
		switch (md->itsCenterpiece)
		{
			case CenterpieceEarth:
				BindEarthVAO(gd->itsVertexArrayNames[VertexArrayObjectEarth]);
				DrawEarthVAO(gd->itsTextureNames[TextureEarth], md->itsHoneycomb, &theViewMatrix, &thePlacement);
				break;

			case CenterpieceGalaxy:
				BindGalaxyVAO(gd->itsVertexArrayNames[VertexArrayObjectGalaxy]);
				DrawGalaxyVAO(gd->itsTextureNames[TextureGalaxy], md->itsHoneycomb, &theViewMatrix, &thePlacement);
				break;

			case CenterpieceGyroscope:
				BindGyroscopeVAO(gd->itsVertexArrayNames[VertexArrayObjectGyroscope]);
				DrawGyroscopeVAO(gd->itsTextureNames[TextureGyroscope], md->itsHoneycomb, &theViewMatrix, &thePlacement);
				break;

			default:	//	should never occur
				break;
		}
	}
}


#ifdef START_OUTSIDE
static void DrawTheSceneExtrinsically(
	ModelData		*md,
	GraphicsDataGL	*gd,
	bool			aSceneryInversionFlag)
{
	Matrix	theTranslation,
			theRotation,
			thePlacement,
			theSpin,
			theTilt,
			theOrientation;	//	“orientation” in the sense of an element of O(3),
							//		not a connected component of O(3)
	
	//	Set up a Honeycomb containing the identity matrix alone.
	//	Omit fields related to depth sorting -- DrawDirichletVAO() will ignore them.
	static Honeycell	theIdentityCell =
						{
							{
								{
									{1.0, 0.0, 0.0, 0.0},
									{0.0, 1.0, 0.0, 0.0},
									{0.0, 0.0, 1.0, 0.0},
									{0.0, 0.0, 0.0, 1.0}
								},
								ImagePositive
							},
							{{0.0, 0.0, 0.0, 1.0}},	//	ignored (but nevertheless correct!)
							NULL,					//	ignored
							0,						//	ignored
							0.0						//	ignored (but nevertheless correct!)
						},
						*theSingletonArray[1] =
						{
							&theIdentityCell
						};
	static Honeycomb	theSingletonHoneycomb =
						{
							0,		//	ignored
							NULL,	//	ignored
							1,
							theSingletonArray
						};
	
	//	This is just a quick hack for personal use.
	//	We won't need to draw the back hemisphere.
	UNUSED_PARAMETER(aSceneryInversionFlag);

	//	Do we have a space loaded?
	if (md->itsSpaceType == SpaceNone)
		return;

	//	Disable depth testing.
	//	Normally it would be harmless, except that the transparent corners
	//	of the galaxy square extend slightly beyond the faces of the
	//	Poincaré dodecahedral space's fundamental polyhedron.
	glDisable(GL_DEPTH_TEST);

	//	The current placement consists of a rotation
	//	followed by a translation.
	MatrixGeometricInverse(&md->itsUserPlacement, &theRotation);
	MatrixTranslation(	&theTranslation,
						md->itsSpaceType,
						0.0,
						0.0,
						md->itsViewpointTransition * EXTRINSIC_VIEWING_DISTANCE);
	MatrixProduct(&theRotation, &theTranslation, &thePlacement);

	//	Draw the Dirichlet domain's inside faces.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	BindDirichletVAO(gd->itsVertexArrayNames[VertexArrayObjectDirichlet]);
	DrawDirichletVAO(	gd->itsTextureNames[
							md->itsShowColorCoding ? TextureWallPaper : TextureWallWood],
						md->itsDirichletDomain,
						&theSingletonHoneycomb,
						&thePlacement,
						md->itsCurrentAperture);

	//	Draw the centerpiece.
	//	Please remember that DrawTheSceneExtrinsically()
	//	is just a quick hack for personal use!
	MatrixRotation(&theSpin, 0.0, 0.0, GALAXY_SPEED  * md->itsRotationAngle);
	MatrixRotation(&theTilt, 0.2, 0.3, 0.0);	//	arbitrary spin axis that looks nice
	MatrixProduct(&theSpin, &theTilt, &theOrientation);
	BindGalaxyVAO(gd->itsVertexArrayNames[VertexArrayObjectGalaxy]);
	DrawGalaxyVAO(	gd->itsTextureNames[TextureGalaxy],
					&theSingletonHoneycomb,
					&thePlacement,
					&theOrientation);

	//	Draw the Dirichlet domain's outside faces.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	BindDirichletVAO(gd->itsVertexArrayNames[VertexArrayObjectDirichlet]);
	DrawDirichletVAO(	gd->itsTextureNames[
							md->itsShowColorCoding ? TextureWallPaper : TextureWallWood],
						md->itsDirichletDomain,
						&theSingletonHoneycomb,
						&thePlacement,
						md->itsCurrentAperture);

	//	Just for good form...
	glCullFace(GL_BACK);

	//	Re-enable depth testing.
	glEnable(GL_DEPTH_TEST);
}
#endif


void SendModelViewMatrixToShader(
	double	aModelViewMatrix[4][4])
{
	float	theModelViewMatrixFloat[4][4];

	//	The projection matrix gets passed in as a uniform, once per frame.
	//	Here we need only pass in aModelViewMatrix, which varies from
	//	tile to tile.

	Matrix44DoubleToFloat(theModelViewMatrixFloat, aModelViewMatrix);

	glVertexAttrib4fv(ATTRIBUTE_MV_MATRIX_ROW_0, theModelViewMatrixFloat[0]);
	glVertexAttrib4fv(ATTRIBUTE_MV_MATRIX_ROW_1, theModelViewMatrixFloat[1]);
	glVertexAttrib4fv(ATTRIBUTE_MV_MATRIX_ROW_2, theModelViewMatrixFloat[2]);
	glVertexAttrib4fv(ATTRIBUTE_MV_MATRIX_ROW_3, theModelViewMatrixFloat[3]);
}


#endif	//	SUPPORT_OPENGL

