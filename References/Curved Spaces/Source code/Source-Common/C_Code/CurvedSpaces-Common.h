//	CurvedSpaces-Common.h
//
//	Platform-independent definitions for Curved Spaces' internal code.
//	The internal code doesn't know or care what platform
//	(MacOS, Win32, iOS, Android) it's running on.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include "GeometryGames-Common.h"
#ifdef SUPPORT_OPENGL
#include "GeometryGames-OpenGL.h"
#endif
#include "GeometryGamesMatrix44.h"


//	Use a mouse-based interface on MacOS or Win32, but
//	use a touch-based interface on iOS or Android.
//	Exception:  Windows 10 touchscreen devices need a touch interface.

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#define CURVED_SPACES_TOUCH_INTERFACE
#endif

#ifdef __ANDROID__
#define CURVED_SPACES_TOUCH_INTERFACE
#endif

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#define CURVED_SPACES_TOUCH_INTERFACE
//#define CURVED_SPACES_MOUSE_INTERFACE
#ifdef CURVED_SPACES_TOUCH_INTERFACE
#warning CURVED_SPACES_TOUCH_INTERFACE is enabled for testing
#endif
#endif

#ifdef __WIN32__
//#define CURVED_SPACES_MOUSE_INTERFACE
#define CURVED_SPACES_TOUCH_INTERFACE
#ifdef CURVED_SPACES_TOUCH_INTERFACE
#warning CURVED_SPACES_TOUCH_INTERFACE is enabled for use on Win10 touchscreen devices
#endif
#endif

#if ! defined(CURVED_SPACES_TOUCH_INTERFACE) && ! defined(CURVED_SPACES_MOUSE_INTERFACE)
#error Must define either touch interface or  mouse interface.
#endif
#if   defined(CURVED_SPACES_TOUCH_INTERFACE) &&   defined(CURVED_SPACES_MOUSE_INTERFACE)
#error Cannot define both touch interface and mouse interface.
#endif


//	Don't let the user open more than one window at once.  
//	On my 2006 iMac's Radeon X1600, opening 2 or 3 windows 
//	with depth buffers, or 3 or 4 windows without depth buffers, 
//	suddenly brings the computer to a near standstill.
//	On my 2008 MacBook's GeForce 9400M the problem seems not to occur.
//	To enable multiple-window support for testing purposes, 
//	uncomment the following line.
//#define ALLOW_MULTIPLE_WINDOWS
#ifdef ALLOW_MULTIPLE_WINDOWS
#warning ALLOW_MULTIPLE_WINDOWS is enabled
#endif

//	When the 3-torus is first introduced in the Shape of Space lecture,
//	it's nice to start with walls closed and zero speed.
//#define START_STILL
#ifdef START_STILL
#warning START_STILL is enabled
#endif

//	For the Shape of Space presentation it’s nice
//	to be able to move the Earth.  I've suppressed this feature
//	from the public version because it introduces artifacts:
//	1.	The centerpiece may extend beyond the fundamental domain,
//		leading to a "popping" artifact when the fundamental domain
//		passes outside the view pyramid and no longer gets drawn.
//	2.	The centerpiece also suffers a noticeable "popping" artifact
//		when it's near the back of the visible region in the Euclidean
//		case.  In such cases the beams also "pop" in theory, but in
//		practice they're not noticeable.
//	3.	The wrong resolution might get used for the spinning Earth,
//		because it's no longer at the center.  DrawEarthVAO()
//		include a hack to render the Earth more smoothly,
//		so even an off-center Earth looks OK.  (This feature is
//		for my personal use only. Release-quality code would decide
//		the Earth's resolution based on its true position in world space.)
//#define CENTERPIECE_DISPLACEMENT
#ifdef CENTERPIECE_DISPLACEMENT
#warning CENTERPIECE_DISPLACEMENT is enabled
#endif

//	For the Shape of Space presentation it’s nice
//	to start outside the dodecahedron and then move in.
//	I’m not sure whether this will work well
//	in the publicly released version or not, partly because
//	of conflicts with existing features and partly because
//	it might not look so good with spaces like slab spaces.
//	So for now let’s keep it optional.
//	We could eventually make it permanent if desired.
//#define START_OUTSIDE
#ifdef START_OUTSIDE
#warning START_OUTSIDE is enabled
#endif

//	For the Curvature presentation it's nice to start 
//	with the walls mostly open, leaving beams to represent
//	the geodesics (at least in spaces where the beams
//	continue straight through the vertices and out the other side).
//	This option also leaves the spaceship hidden by default.
//#define START_WALLS_OPEN
#ifdef START_WALLS_OPEN
#warning START_WALLS_OPEN is enabled
#endif

//	To produce a good screenshot, tile deep (can also omit the centerpiece
//	in the home cell to avoid obscuring the view) (might also want
//	to improve the level-of-detail of the spinning Earths)
//	(hmmm... maybe current off-the-shelf settings are OK,
//	aside from the 1024×1024 limit on saved image size).
//#define HIGH_RESOLUTION_SCREENSHOT
#ifdef HIGH_RESOLUTION_SCREENSHOT
#warning HIGH_RESOLUTION_SCREENSHOT is enabled
#endif

//	A quick-and-dirty hack to show the corkscrew axes
//	in the Hantzsche-Wendt space, for Jon Rogness to use
//	in one of his MathFest 2010 talks.
//#define HANTZSCHE_WENDT_AXES
#ifdef HANTZSCHE_WENDT_AXES
#warning HANTZSCHE_WENDT_AXES is enabled
#endif

//	For the Spazi Finiti or Spin Graphics talks
//	we want to start with beams but no centerpiece.
//	We also want to be able to show Clifford parallels
//	while in a prism space.  And for the Spin Graphics talk
//	we need to show 1, 2 and 3 sets of mutually orthogonal
//	Clifford parallels.
//#define CLIFFORD_FLOWS_FOR_TALKS
#ifdef CLIFFORD_FLOWS_FOR_TALKS
#warning CLIFFORD_FLOWS_FOR_TALKS is enabled
#endif


//	Clifford parallels options
typedef enum
{
	CliffordNone,
	CliffordBicolor,
	CliffordCenterlines,
	CliffordOneSet,
	CliffordTwoSets,
	CliffordThreeSets
} CliffordMode;


//	π
#define PI	3.14159265358979323846

//	The  up -arrow key decreases the user's speed by USER_SPEED_INCREMENT.
//	The down-arrow key increases the user's speed by USER_SPEED_INCREMENT.
//	The space bar sets the user's speed to zero.
#define USER_SPEED_INCREMENT	0.02


//	Opaque typedef
typedef struct HEPolyhedron	DirichletDomain;


//	Transparent typedefs

typedef enum
{
	SpaceNone,
    SpaceSpherical,
    SpaceFlat,
    SpaceHyperbolic
} SpaceType;

typedef enum
{
	ImagePositive,	//	not mirror-reversed
	ImageNegative	//	mirror-reversed
} ImageParity;

typedef struct
{
	double	v[4];
} Vector;

typedef struct
{
	double		m[4][4];
	ImageParity	itsParity;	//	Is determinant positive or negative?
} Matrix;

typedef struct
{
	unsigned int	itsNumMatrices;
	Matrix			*itsMatrices;
} MatrixList;

//	Technical note:  Why does a Honeycell use a Dirichlet domain's
//	full set of vertices instead of a bounding box?
//	1.	For the most common manifolds, the number of vertices is fairly small.
//	2.	Computing a bounding box is a small nuisance when the fundamental domain
//		may extend as far as -- or even into -- the southern hemisphere of S³,
//		as happens with lens spaces and slab spaces.
typedef struct
{
	Matrix			itsMatrix;
	Vector			itsCenter,
					*itsVertices;
	unsigned int	itsNumVertices;
	double			itsDistance;	//	distance from origin to cell center after applying view matrix
} Honeycell;
typedef struct
{
	//	A fixed list of the cells, sorted relative
	//	to their distance from the basepoint (0,0,0,1).
	unsigned int	itsNumCells;
	Honeycell		*itsCells;

	//	At render time, make a temporary list of the visible cells
	//	and sort them according to their distance from the observer.
	unsigned int	itsNumVisibleCells;
	Honeycell		**itsVisibleCells;
} Honeycomb;

typedef enum
{
	CenterpieceNone,
	CenterpieceEarth,
	CenterpieceGalaxy,
	CenterpieceGyroscope
} CenterpieceType;

#ifdef START_OUTSIDE
typedef enum
{
	ViewpointIntrinsic,		//	normal operation
	ViewpointExtrinsic,		//	external view of fundamental domain
	ViewpointEntering		//	transition from extrinsic to intrinsic
} Viewpoint;
#endif


//	All platform-independent data about the space and
//	how it's displayed live in the ModelData structure.
struct ModelData
{	
	//	Stereo 3D mode
	StereoMode		itsStereoMode;

	//	Environmental measurements permit accurate simulation.
	//
	//	The function CharacteristicViewSize() defines a view's "characteristic size"
	//	as some function of the view's width and height.  The exact function is set 
	//	at compile time, and determines how the field-of-view responds to changes 
	//	in the view's aspect ratio:
	//
	//		If CharacteristicViewSize(width, height) = width,
	//			then the view maintains a constant 90° horizontal field-of-view,
	//			while letting the vertical field-of-view vary
	//			according to the view's aspect ratio.
	//
	//		If CharacteristicViewSize(width, height) = height,
	//			then the view maintains a constant 90° vertical field-of-view,
	//			while letting the horizontal field-of-view vary
	//			according to the view's aspect ratio.
	//
	//		If CharacteristicViewSize(width, height) = sqrt(width*height),
	//			then the horizontal and vertical fields-of-view both vary,
	//			while maintaining an average of 90°.
	//
	//	The characteristic size will always correspond 
	//	to a given number of intrinsic units (itsCharacteristicSizeIU),
	//	even as the user resizes the view and thus changes
	//	the number of pixels lying within it (theCharacteristicSizePx).
	//	At render time itsCharacteristicSizeIU will be used to deduce
	//	the view's width and height in intrinsic units.
	//
	//	"Characteristic-size coordinates" have origin at the center of the view,
	//	horizontal axis directed to the right, vertical axis directed upward,
	//	and are measured in units of half the characteristic size.
	//	Thus, in a square window, the characteristic-size coordinates
	//	would run from -1.0 to +1.0 in each direction.
	//
	//	Note #1.  Curved Spaces works differently from the other 
	//	Geometry Games programs.  When the user resizes the window,
	//	the other programs change the field of view according to the user's
	//	physical distance (in centimeters) from the display.
	//	Curved Spaces ignores the user's physical distance from the display
	//	and instead maintains a 90° field of view in one of the senses listed above.
	//	If Curved Spaces were to account for the user's distance in centimeters,
	//	as in the other Geometry Games programs, then the field of view
	//	would be much too narrow, and the user wouldn't see enough of the space.
	//	In effect, the user would have "tunnel vision".
	//
	//	Note #2.  At the moment, Curved Spaces provides no interface 
	//	for modifying these parameters.  If I ever want to add such an interface,
	//	the user could specify window and viewing distances in centimeters,
	//	and the program could convert to intrinsic units.
	//
	double			itsCharacteristicSizeIU,	//	in intrinsic units;  see definition above
					itsViewingDistanceIU,		//	in intrinsic units;  presumed distance from observer to window
					itsEyeOffsetIU;				//	in intrinsic units;  presumed distance from observer's eye to bridge of nose
	
	//	When some part of the program (for example the mouse-handling code)
	//	wants to request a redraw, it sets itsRedrawRequestFlag.
	//	The idle-time routine will handle the request and clear the flag.
	//
	//	Note:  It's essential that all redraws funnel through the idle-time routine.
	//	For example, if mouse movements generated redraw events directly, this could
	//	saturate the event loop, thus blocking normal idle-time motion.
	bool			itsRedrawRequestFlag;

	//	Most of the code doesn't need to know the curvature
	//	of space.  However, some parts do, for example
	//	the part that draws the back hemisphere of S³.
	SpaceType		itsSpaceType;
	
	//	For flat and hyperbolic spaces, this flag is ignored.
	bool			itsDrawBackHemisphere;
	
	//	An arbitrary finite set of Clifford parallels
	//	lives most naturally in the 3-sphere,
	//	so enable the Clifford Parallels option only there.
	bool			itsThreeSphereFlag;
	
	//	How far out should we tile?
	//	The tiling radius is the maximum distance that
	//	a group element may translate the basepoint (0,0,0,1).
	//	The tiling radius should be greater than π
	//	in the spherical case if you want to tile all of S³.
	//	In the hyperbolic case, try a radius in the range 3.0 - 6.0,
	//	depending on your GPU.
	//	The drawing radius is similar, but is slightly smaller.
	//	By omitting a few of the outermost tiles, the number
	//	of visible tiles -- and therefore the frame rate -- remains
	//	more constant as we pass through a face of the Dirichlet domain.
	//	This avoids a sudden "lurching" effect that would otherwise
	//	occur when the frame rate suddenly drops.
	double			itsTilingRadius,
					itsDrawingRadius;
	
	//	Keep track of the user's placement in the world.  The transformation moves
	//	the eye from its default position (0,0,0,1) with right vector (1,0,0,0),
	//	up vector (0,1,0,0) and forward vector (0,0,1,0) to the user's current placement.
	Matrix			itsUserPlacement;
	
	//	How fast is the user moving?
	//	The only sustained momentum is straight forward.
	double			itsUserSpeed;

#ifdef CENTERPIECE_DISPLACEMENT
	//	Keep track of the centerpiece's placement in the world.  The transformation moves
	//	the centerpiece from its default position (0,0,0,1) with right vector (1,0,0,0),
	//	up vector (0,1,0,0) and forward vector (0,0,1,0) to its current placement.
	Matrix			itsCenterpiecePlacement;
#endif
	
	//	Keep a Dirichlet domain for the discrete group.
	//	Assume no group element fixes the origin.
	DirichletDomain	*itsDirichletDomain;
	
	//	Keep a list of all translates of the Dirichlet domain
	//	that sit sufficiently close to the origin.
	//	For a spherical manifold the list will typically include
	//	the whole finite group.  For all manifolds the list
	//	is sorted near-to-far. 
	Honeycomb		*itsHoneycomb;
	
	//	The aperture in each face of the Dirichlet domain may be
	//	fully closed (0.0), fully open (1.0), or anywhere in between.
	//	Use the left and right arrow keys to control it.
	//	The arrow keys change itsDesiredAperture immediately,
	//	but itsCurrentAperture catches up only gradually,
	//	providing a smooth animation.
	double			itsDesiredAperture,
					itsCurrentAperture;
	
	//	What centerpiece should we display within each translate
	//	of the fundamental cell?
	CenterpieceType	itsCenterpiece;
	
	//	Let the centerpiece (Earth, galaxy or gyroscope) rotate.
	double			itsRotationAngle;	//	in radians
	
	//	Draw the observer (as a small colored dart,
	//	representing the user's spaceship) ?
	bool			itsShowObserver;
	
	//	Color code the faces?
	bool			itsShowColorCoding;
	
	//	Draw Clifford parallels in spherical spaces?
	CliffordMode	itsCliffordMode;

#ifdef CLIFFORD_FLOWS_FOR_TALKS
	//	Rotate in XY and/or ZW planes?
	bool			itsCliffordFlowXYEnabled,
					itsCliffordFlowZWEnabled;
#endif
	
	//	Draw vertex figures?
	bool			itsShowVertexFigures;
	
	//	Enable fog?
	bool			itsFogFlag;			//	Does the user want fog?
	double			itsFogSaturation;	//	Current saturation level
										//		0.0 = fully transparent
										//		1.0 = fully opaque

#ifdef START_OUTSIDE
	//	View the fundamental domain from within, from without,
	//	or somewhere in between?
	Viewpoint		itsViewpoint;

	//	If we’re viewing the fundamental domain from somewhere
	//	in between, how far along are we?
	//		0.0 = ViewpointIntrinsic
	//		1.0 = ViewpointExtrinsic
	double			itsViewpointTransition;
	
	//	Keep the fundamental domain spinning.
	double			itsExtrinsicRotation;
#endif

#ifdef HANTZSCHE_WENDT_AXES
	bool			itsHantzscheWendtSpaceIsLoaded,
					itsShowHantzscheWendtAxes;
#endif

};


//	Ordinary rendering uses a single viewpoint,
//	while stereoscopic 3D uses separate left- and right-eye views.
typedef enum
{
	EyeOnly,
	EyeLeft,
	EyeRight
} EyeType;

//	Color definitions

typedef struct
{
	double	h,
			s,
			l,
			a;
} HSLAColor;

typedef struct
{
	double	r,	//	red,   premultiplied by alpha
			g,	//	green, premultiplied by alpha
			b,	//	blue,  premultiplied by alpha
			a;	//	alpha = opacity
} RGBAColor;


//	Platform-independent global functions

//	in CurvedSpacesView.c
extern double		CharacteristicViewSize(double aViewWidth, double aViewHeight);

//	in CurvedSpacesOptions.c
extern void			SetCenterpiece(ModelData *md, CenterpieceType aCenterpieceChoice);
extern void			SetShowObserver(ModelData *md, bool aShowObserverChoice);
extern void			SetShowColorCoding(ModelData *md, bool aShowColorCodingChoice);
extern void			SetShowCliffordParallels(ModelData *md, CliffordMode aCliffordMode);
extern void			SetShowVertexFigures(ModelData *md, bool aShowVertexFiguresChoice);
extern void			SetFogFlag(ModelData *md, bool aFogFlag);
extern void			SetStereo3DMode(ModelData *md, StereoMode aStereo3DMode);
#ifdef HANTZSCHE_WENDT_AXES
extern void			SetShowHantzscheWendtAxes(ModelData *md, bool aShowHantzscheWendtAxesChoice);
#endif

//	in CurvedSpacesSimulation.c
extern void			ChangeAperture(ModelData *md, bool aDilationFlag);
extern void			FastGramSchmidt(Matrix *aMatrix, SpaceType aSpaceType);

//	in CurvedSpacesMouse.c
extern void			MouseMoved(ModelData *md, DisplayPoint aMouseLocation, DisplayPointMotion aMouseMotion, bool aShiftKeyIsDown, bool aCtrlKeyIsDown, bool anAltKeyIsDown);

//	in CurvedSpacesFileIO.c
extern ErrorText	LoadGeneratorFile(ModelData *md, Byte *anInputText);

//	in CurvedSpacesTiling.c
extern ErrorText	ConstructHolonomyGroup(MatrixList *aGeneratorList, double aTilingRadius, MatrixList **aHolonomyGroup);
extern ErrorText	NeedsBackHemisphere(MatrixList *aHolonomyGroup, SpaceType aSpaceType, bool *aDrawBackHemisphereFlag);

//	in CurvedSpacesGraphics-OpenGL.c
extern void			SendModelViewMatrixToShader(double aModelViewMatrix[4][4]);

//	in CurvedSpacesDirichlet.c
extern ErrorText	ConstructDirichletDomain(MatrixList *aHolonomyGroup, DirichletDomain **aDirichletDomain);
extern void			FreeDirichletDomain(DirichletDomain **aDirichletDomain);
extern void			StayInDirichletDomain(DirichletDomain *aDirichletDomain, Matrix *aPlacement);
extern ErrorText	ConstructHoneycomb(MatrixList *aHolonomyGroup, DirichletDomain *aDirichletDomain, Honeycomb **aHoneycomb);
extern void			FreeHoneycomb(Honeycomb **aHoneycomb);
extern ErrorText	MakeDirichletVBO(GLuint aVertexBufferName, GLuint anIndexBufferName, DirichletDomain *aDirichletDomain, double anAperture, bool aColorCodingFlag, bool aGreyscaleFlag);
extern void			MakeDirichletVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindDirichletVAO(GLuint aVertexArrayName);
extern void			DrawDirichletVAO(GLuint aDirichletTexture, DirichletDomain *aDirichletDomain, Honeycomb *aHoneycomb, Matrix *aWorldPlacement, double aCurrentAperture);
extern void			MakeVertexFiguresVBO(GLuint aVertexBufferName, GLuint anIndexBufferName, DirichletDomain *aDirichletDomain);
extern void			MakeVertexFiguresVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindVertexFiguresVAO(GLuint aVertexArrayName);
extern void			DrawVertexFiguresVAO(GLuint aVertexFigureTexture, DirichletDomain *aDirichletDomain, Honeycomb *aHoneycomb, Matrix *aWorldPlacement);
extern void			SortVisibleCells(Honeycomb *aHoneycomb, Matrix *aViewProjectionMatrix, Matrix *aViewMatrix, double aDrawingRadius);

//	in CurvedSpacesEarth.c
extern void			MakeEarthVBO(GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			MakeEarthVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindEarthVAO(GLuint aVertexArrayName);
extern void			DrawEarthVAO(GLuint anEarthTexture, Honeycomb *aHoneycomb, Matrix *aWorldPlacement, Matrix *anEarthPlacement);

//	in CurvedSpacesGalaxy.c
extern void			MakeGalaxyVBO(GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			MakeGalaxyVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindGalaxyVAO(GLuint aVertexArrayName);
extern void			DrawGalaxyVAO(GLuint aGalaxyTexture, Honeycomb *aHoneycomb, Matrix *aWorldPlacement, Matrix *aGalaxyPlacement);

//	in CurvedSpacesGyroscope.c
extern void			MakeGyroscopeVBO(GLuint aVertexBufferName, GLuint anIndexBufferName, bool aGreyscaleFlag);
extern void			MakeGyroscopeVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindGyroscopeVAO(GLuint aVertexArrayName);
extern void			DrawGyroscopeVAO(GLuint aGyroscopeTexture, Honeycomb *aHoneycomb, Matrix *aWorldPlacement, Matrix *aGyroscopePlacement);

//	in CurvedSpacesObserver.c
extern void			MakeObserverVBO(GLuint aVertexBufferName, GLuint anIndexBufferName, bool aGreyscaleFlag);
extern void			MakeObserverVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindObserverVAO(GLuint aVertexArrayName);
extern void			DrawObserverVAO(GLuint anObserverTexture, Honeycomb *aHoneycomb, Matrix *aWorldPlacement, Matrix *anObserverPlacement);

//	in CurvedSpacesClifford.c
extern void			MakeCliffordVBO(GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			MakeCliffordVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindCliffordVAO(GLuint aVertexArrayName);
extern void			DrawCliffordVAO(GLuint aCliffordTexture, CliffordMode aCliffordMode, StereoMode aStereoMode, Matrix *aWorldPlacement);

#ifdef HANTZSCHE_WENDT_AXES
//	in CurvedSpacesHantzscheWendt.c
extern void			MakeHantzscheWendtVBO(GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			MakeHantzscheWendtVAO(GLuint aVertexArrayName, GLuint aVertexBufferName, GLuint anIndexBufferName);
extern void			BindHantzscheWendtVAO(GLuint aVertexArrayName);
extern void			DrawHantzscheWendtVAO(GLuint aHantzscheWendtTexture, Honeycomb *aHoneycomb, Matrix *aWorldPlacement);
#endif

//	in CurvedSpacesMatrices.c
extern void			MatrixIdentity(Matrix *aMatrix);
extern bool			MatrixIsIdentity(Matrix *aMatrix);
extern void			MatrixAntipodalMap(Matrix *aMatrix);
extern void			MatrixTranslation(Matrix *aMatrix, SpaceType aSpaceType, double dx, double dy, double dz);
extern void			MatrixRotation(Matrix *aMatrix, double da, double db, double dc);
extern void			MatrixGeometricInverse(Matrix *aMatrix, Matrix *anInverse);
extern double		MatrixDeterminant(Matrix *aMatrix);
extern void			VectorTernaryCrossProduct(Vector *aFactorA, Vector *aFactorB, Vector *aFactorC, Vector *aProduct);
extern bool			MatrixEquality(Matrix *aMatrixA, Matrix *aMatrixB, double anEpsilon);
extern void			MatrixProduct(const Matrix *aMatrixA, const Matrix *aMatrixB, Matrix *aProduct);
extern void			VectorNegate(Vector *aVector, Vector *aNegation);
extern void			VectorSum(Vector *aVectorA, Vector *aVectorB, Vector *aSum);
extern void			VectorDifference(Vector *aVectorA, Vector *aVectorB, Vector *aDifference);
extern void			VectorInterpolate(Vector *aVectorA, Vector *aVectorB, double t, Vector *aResult);
extern double		VectorDotProduct(Vector *aVectorA, Vector *aVectorB);
extern ErrorText	VectorNormalize(Vector *aRawVector, SpaceType aSpaceType, Vector *aNormalizedVector);
extern double		VectorGeometricDistance(Vector *aVector);
extern double		VectorGeometricDistance2(Vector *aVectorA, Vector *aVectorB);
extern void			VectorTimesMatrix(Vector *aVector, Matrix *aMatrix, Vector *aProduct);
extern void			ScalarTimesVector(double aScalar, Vector *aVector, Vector *aProduct);
extern MatrixList	*AllocateMatrixList(unsigned int aNumMatrices);
extern void			FreeMatrixList(MatrixList **aMatrixList);

//	in CurvedSpacesSafeMath.c
extern double		SafeAcos(double x);
extern double		SafeAcosh(double x);

//	in CurvedSpacesColors.c
extern void			HSLAtoRGBA(HSLAColor *anHSLAColor, RGBAColor *anRGBAColor);
