//	CurvedSpacesDirichlet.c
//
//	Given a set of matrix generators, construct a Dirichlet domain.
//	The geometry may be spherical, Euclidean or hyperbolic,
//	but no group element may fix the origin.
//
//	The algorithm works in a projective context in which
//	rays from the origin represent the Dirichlet domain's vertices,
//	planes through the origin represent lines containing
//		the Dirichlet domain's edges, and
//	hyperplanes through the origin represent planes containing
//		the Dirichlet domain's faces.
//
//	For convenience we may visualize this space as the unit 3-sphere,
//	because each ray from the origin determines a unique point on S³.
//
//	The Dirichlet domain's basepoint sits at (0,0,0,1).
//
//	The geometry (spherical, Euclidean or hyperbolic) comes into play
//	only briefly, when deciding what halfspace a given matrix represents.
//	Thereafter the construction is geometry-independent,
//	because it's simply a matter of intersecting halfspaces.
//	[HMMM... THERE COULD BE SOME MORE SUBTLE ISSUES WITH VERTEX NORMALIZATION.]
//
//	Note that in the hyperbolic case, this projective model
//	includes the region outside the lightcone, which corresponds
//	to a region beyond the usual hyperbolic sphere-at-infinity
//	(the latter being the lightcone itself).  But as long as a given
//	Dirichlet domain sits within the lightcone (possibly with
//	vertices on the lightcone) everything will work great.
//	In particular, this model makes it easy to work with Dirichlet
//	domains for cusped manifolds, although of course when the user
//	flies down into the cusp, s/he will see past the finite available
//	portion of the tiling.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt


#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>
#include <stdlib.h>	//	for qsort()


//	Three vectors will be considered linearly independent iff their
//	ternary cross product has squared length at least PLANARITY_EPSILON
//	(meaning length at least √PLANARITY_EPSILON).  Use a fairly large value
//	here -- the low-index matrices should be blatently independent.
#define PLANARITY_EPSILON		1e-4

//	A fourth hyperplane normal will be considered linearly independent
//	iff it avoids the (antipodal) endpoints of the banana defined by
//	the first three hyperplanes.  Numerical accuracy should be good
//	and the linear independence should be robust, so any plausible value
//	should work OK here.  (Famous last words?)
#define HYPERPLANARITY_EPSILON	1e-4

//	How precisely do we expect to be able to infer the order
//	of a cyclic matrix?
#define ORDER_EPSILON			1e-6

//	How well must a vertex satisfy a halfspace equation
//	to be considered lying on that halfspace's boundary?
//	(I HAVEN'T THOUGHT CAREFULLY ABOUT THIS VALUE.)
#define VERTEX_HALFSPACE_EPSILON	1e-6

//	Matching faces should have equal matrices to pretty high precision.
//	Nevertheless, we can safely choose a large value here, since *all*
//	matrix entries must agree to that precision.
#define MATE_MATRIX_EPSILON		1e-6

//	Make sure we're well clear of a face before applying
//	a face-pairing matrix to stay within the fundamental domain.
//	In particular, if we happen to run *along* a face,
//	we don't want to be flipping back and forth.
#define RESTORING_EPSILON		1e-8

//	How many times should the face texture repeat across a single quad?
#define FACE_TEXTURE_MULTIPLE_PLAIN	6
#define FACE_TEXTURE_MULTIPLE_WOOD	1

//	How large should a vertex figure be?
#define VERTEX_FIGURE_SIZE		0.1	//	in radians of S³

//	How large a hole should get cut the face of a vertex figure?
#define VERTEX_FIGURE_CUTOUT	0.7	//	as fraction of face size

//	__cdecl isn't defined or needed on MacOS X,
//	so make it disappear from our callback function prototypes.
#ifndef __cdecl
#define __cdecl
#endif


typedef enum
{
	VertexInsideHalfspace,
	VertexOnBoundary,
	VertexOutsideHalfspace
} VertexVsHalfspace;


//	The Dirichlet Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2],	//	texture coordinates (u,v)
			col[4];	//	color (r,g,b,a)
} DirichletVBOData;

//	The vertex figure Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2];	//	texture coordinates (u,v)
} VertexFiguresVBOData;


//	Use a half-edge data structure to represent a Dirichlet domain.
//	The half-edge data structure is easier to work with than
//	the winged-edge data structure used in Curved Spaces 1.0.
//
//	Orientation conventions
//
//	One may orient the faces all clockwise or all counterclockwise,
//	relative to my standard left-handed coordinate system.
//	The documentation accompanying the following typedefs allows
//	for both possibilities:   the present Curved Spaces code orients
//	faces counterclockwise as seen from *inside* the polyhedron
//	(as the end-user will see them), which is the same as orienting
//	them clockwise as seen from *outside* the polyhedron (as the programmer
//	tends to visualize them while writing the code);
//	future polyhedron-viewing software may wish to reverse the convention
//	if the end-user will view the polyhedra from the outside rather than
//	the inside.  Of course, in a pinch it's easy to change
//	GL_FRONT_FACE from GL_CCW to GL_CW, but for now I'll stick with
//	the default GL_CCW on the off chance that it works more efficiently
//	in some implementations (unlikely but you never know).

typedef struct HEVertex
{
	//	The projective approach (see comments at top of file)
	//	represents a vertex as a ray from the origin.
	//	For most purposes we need only its direction, not its length.
	//	Eventually, though, we might need its length as well,
	//	for example when determining suitable fog parameters.
	Vector				itsRawPosition,			//	normalized to unit 3-sphere at end of algorithm
						itsNormalizedPosition;	//	normalized relative to SpaceType

	//	Knowing a single adjacent half edge gives easy access to them all.
	//	The given half edge starts at this vertex and points away from it.
	struct HEHalfEdge	*itsOutboundHalfEdge;

	//	IntersectWithHalfspace() evaluates each halfspace inequality on each vertex
	//	and stores the result temporarily in itsHalfspaceStatus.
	//	Otherwise this field is unused and undefined.
	VertexVsHalfspace	itsHalfspaceStatus;

	//	The center of a face of the vertex figure.
	//	Please see the explanation of vertex figures in HEHalfEdge below.
	Vector				itsCenterPoint;

	//	The Dirichlet domain keeps all vertices on a NULL-terminated linked list.
	struct HEVertex		*itsNext;

} HEVertex;

typedef struct HEHalfEdge
{
	//	The vertex this half edge points to.
	struct HEVertex		*itsTip;

	//	The other half of the given edge, pointing in the opposite direction.
	//	As viewed from outside the polyhedron, with faces oriented
	//	clockwise (resp. counterclockwise) the two half edges look like
	//	traffic in Europe or the U.S. (resp. Australia or Japan),
	//	assuming a left-handed {x,y,z} coordinate system.
	struct HEHalfEdge	*itsMate;

	//	Traverse the adjacent face clockwise (resp. counter-clockwise),
	//	as viewed from outside the polyhedron.
	struct HEHalfEdge	*itsCycle;

	//	The face that itsCycle traverses lies to the right (resp. left)
	//	of the edge, as viewed from outside the polyhedron.
	struct HEFace		*itsFace;
	
	//	When we draw a face with a window cut out from its center,
	//	we'll need to compute texture coordinates for the window's vertices.
	//	To do this, we'll need to know the dimensions of the triangle whose base 
	//	is the present HalfEdge and whose vertex is the face's center.
	double				itsBase,		//	normalized so largest base has length 1
						itsAltitude;	//	normalized so largest base has length 1

	//	IntersectWithHalfspace() uses a temporary flag to mark half edges
	//	for deletion.	Thereafter itsDeletionFlag is unused and undefined.
	bool				itsDeletionFlag;

	//	Vertex figures are normally not shown, but if the user requests them,
	//	draw them as a framework.  That is, at each vertex of the fundamental
	//	polyhedron, draw the corresponding face of the vertex figure, but with
	//	a hollow center.  In other words, draw the face as a polyhedral annulus.
	//	Each "outer point" of the annulus sits on the outbound half-edge
	//	emanating from the given vertex of the fundamental polyhedron,
	//	while each "inner point" is interpolated between the outer point
	//	and the center of the face of the vertex figure.
	Vector				itsOuterPoint,
						itsInnerPoint;

	//	The Dirichlet domain keeps all half edges on a NULL-terminated linked list.
	struct HEHalfEdge	*itsNext;

} HEHalfEdge;

typedef struct HEFace
{
	//	Knowing a single adjacent half edge gives easy access to them all.
	//	The adjacent half edges all point clockwise (resp. counter-clockwise)
	//	around the face.
	struct HEHalfEdge	*itsHalfEdge;

	//	The Dirichlet domain is the intersection of halfspaces
	//
	//		ax + by + cz + dw ≤ 0
	//
	//	Let itsHalfspace record the coefficients (a,b,c,d) for the given face.
	Vector				itsHalfspace;

	//	Record the defining matrix.
	Matrix				itsMatrix;

	//	A face and its mate will have the same color.
	unsigned int		itsColorIndex;		//	used only temporarily
	RGBAColor			itsColorRGBA;		//	color as {αr, αg, αb, α}
	double				itsColorGreyscale;	//	color as greyscale

	//	Record the face center.
	//	Normalize to the 3-sphere to facilitate interpolating to infinite vertices.
	Vector				itsRawCenter,			//	normalized to unit 3-sphere
						itsNormalizedCenter;	//	normalized relative to SpaceType

	//	IntersectWithHalfspace() uses a temporary flag to mark faces
	//	for deletion.	Thereafter itsDeletionFlag is unused and undefined.
	bool				itsDeletionFlag;

	//	The Dirichlet domain keeps all faces on a NULL-terminated linked list.
	struct HEFace		*itsNext;

} HEFace;

struct HEPolyhedron
{
	//	Keep vertices, half edges and faces on NULL-terminated linked lists.
	HEVertex		*itsVertexList;
	HEHalfEdge		*itsHalfEdgeList;
	HEFace			*itsFaceList;
	
	//	For convenience, record the space type.
	SpaceType		itsSpaceType;
	
	//	Precompute some information for constructing...
	unsigned int	itsDirichletNumMeshVertices,		//	...the Dirichlet domain mesh and
					itsDirichletNumMeshFaces,
					itsVertexFiguresNumMeshVertices,	//	...the vertex figure mesh.
					itsVertexFiguresNumMeshFaces;
};


static ErrorText			MakeBanana(Matrix *aMatrixA, Matrix *aMatrixB, Matrix *aMatrixC, DirichletDomain **aDirichletDomain);
static ErrorText			MakeLens(Matrix *aMatrixA, Matrix *aMatrixB, DirichletDomain **aDirichletDomain);
static void					MakeHalfspaceInequality(Matrix *aMatrix, Vector *anInequality);
static ErrorText			IntersectWithHalfspace(DirichletDomain *aDirichletDomain, Matrix *aMatrix);
static void					AssignFaceColors(DirichletDomain *aDirichletDomain);
static void					ComputeFaceCenters(DirichletDomain *aDirichletDomain);
static void					ComputeWallDimensions(DirichletDomain *aDirichletDomain);
static ErrorText			ComputeVertexFigures(DirichletDomain *aDirichletDomain);
static void					PrepareForDirichletMesh(DirichletDomain *aDirichletDomain);
static void					PrepareForVertexFiguresMesh(DirichletDomain *aDirichletDomain);
static Honeycomb			*AllocateHoneycomb(unsigned int aNumCells, unsigned int aNumVertices);
static double				CellCenterDistance(Honeycell *aCell, Matrix *aViewMatrix);
static bool					CellMayBeVisible(Honeycell *aCell, Matrix *aViewProjectionMatrix);
static __cdecl signed int	CompareCellCenterDistances(const void *p1, const void *p2);


ErrorText ConstructDirichletDomain(
	MatrixList		*aHolonomyGroup,
	DirichletDomain	**aDirichletDomain)
{
	ErrorText		theErrorMessage	= NULL;
	Vector			theHalfspaceA,
					theHalfspaceB,
					theHalfspaceC,
					theHalfspaceD,
					theCrossProduct;
	unsigned int	theThirdIndex,
					theFourthIndex,
					i;
	HEVertex		*theVertex;

	if (aHolonomyGroup == NULL)
		return u"ConstructDirichletDomain() received a NULL holonomy group.";

	if (*aDirichletDomain != NULL)
		return u"ConstructDirichletDomain() received a non-NULL output location.";

	//	Do we have at least the identity and two other matrices?
	if (aHolonomyGroup->itsNumMatrices < 3)
	{
		//	Special case:  Allow the identity matrix alone,
		//	which represents the 3-sphere, or
		//	{±Id}, which represents projective 3-space.
		//	We'll need the 3-sphere to display Clifford parallels.
		//	(Confession:  This is a hack.  I hope it causes no trouble.)
		if (aHolonomyGroup->itsNumMatrices > 0)	//	{Id} or {±Id}
			return NULL;	//	Leave *aDirichletDomain == NULL, but report no error.
		else
			return u"ConstructDirichletDomain() received no matrices.";
	}

	//	Make sure group element 0 is the identity matrix, as expected.
	if ( ! MatrixIsIdentity(&aHolonomyGroup->itsMatrices[0]) )
		return u"ConstructDirichletDomain() expects the first matrix to be the identity.";

	//	Thinking projectively (as explained at the top of this file)
	//	each matrix determines a halfspace of R⁴ or, equivalently,
	//	a hemisphere of S³.  Just as any two distinct hemispheres of S²
	//	intersect in a 2-sided wedge-shaped sector (a "lune"),
	//	any three independent hemispheres of S³ intersect in a 3-sided
	//	wedge-shaped solid (a "banana") and any four independent hemispheres
	//	intersect in a tetrahedron.  Here "independent" means that
	//	the hemispheres' normal vectors are linearly independent.

	//	Which four group elements should we use?

	//	Ignore group element 0, which is the identity matrix.
	//	Groups elements 1 and 2 should be fine (they can't be colinear
	//	because we assume no group element fixes the basepoint (0,0,0,1)).
	MakeHalfspaceInequality(&aHolonomyGroup->itsMatrices[1], &theHalfspaceA);
	MakeHalfspaceInequality(&aHolonomyGroup->itsMatrices[2], &theHalfspaceB);

	//	For the third group element, use the first one we find that's
	//	not coplanar with the elements 1 and 2.
	for (theThirdIndex = 3; theThirdIndex < aHolonomyGroup->itsNumMatrices; theThirdIndex++)
	{
		MakeHalfspaceInequality(&aHolonomyGroup->itsMatrices[theThirdIndex], &theHalfspaceC);
		VectorTernaryCrossProduct(&theHalfspaceA, &theHalfspaceB, &theHalfspaceC, &theCrossProduct);
		if (fabs(VectorDotProduct(&theCrossProduct, &theCrossProduct)) > PLANARITY_EPSILON)
			break;	//	success!
	}
	if (theThirdIndex < aHolonomyGroup->itsNumMatrices)
	{
		//	Before seeking a fourth independent group element,
		//	construct the banana defined by the first three.
		theErrorMessage = MakeBanana(	&aHolonomyGroup->itsMatrices[1],
										&aHolonomyGroup->itsMatrices[2],
										&aHolonomyGroup->itsMatrices[theThirdIndex],
										aDirichletDomain);
		if (theErrorMessage != NULL)
			goto CleanUpConstructDirichletDomain;

		//	Look for a fourth independent group element.
		for (	theFourthIndex = theThirdIndex + 1;
				theFourthIndex < aHolonomyGroup->itsNumMatrices;
				theFourthIndex++)
		{
			//	We could in principle test for linear independence by computing
			//	the determinant of the four hyperplane vectors.
			//	However it's simpler (and perhaps more numerically robust?)
			//	to test with the fourth hyperplane avoids the two (antipodal)
			//	banana vertices.
			MakeHalfspaceInequality(&aHolonomyGroup->itsMatrices[theFourthIndex], &theHalfspaceD);
			if (fabs(VectorDotProduct(&theHalfspaceD, &(*aDirichletDomain)->itsVertexList->itsRawPosition)) > HYPERPLANARITY_EPSILON)
				break;	//	success!
		}
		if (theFourthIndex < aHolonomyGroup->itsNumMatrices)
		{
			//	Slice the banana with the (independent!) fourth hemisphere
			//	to get a tetrahedron.
			theErrorMessage = IntersectWithHalfspace(*aDirichletDomain, &aHolonomyGroup->itsMatrices[theFourthIndex]);
			if (theErrorMessage != NULL)
				goto CleanUpConstructDirichletDomain;
		}
		else
		{
			//	No independent fourth element was found.
			//	The group defines some sort of chimney-like space,
			//	which the current code does not support.
			//	Even though we've constructed the Dirichlet domain,
			//	the graphics code isn't prepared to draw it.
			theErrorMessage = u"Chimney-like spaces not supported.";
			goto CleanUpConstructDirichletDomain;
		}
	}
	else
	{
		//	We couldn't find three independent group elements,
		//	so most likely we have a lens space or a slab space.
		//	The current code *is* prepared to handle such a space!
		theErrorMessage = MakeLens(	&aHolonomyGroup->itsMatrices[1],
									&aHolonomyGroup->itsMatrices[2],
									aDirichletDomain);
		if (theErrorMessage != NULL)
			goto CleanUpConstructDirichletDomain;
	}

	//	Intersect the initial banana with the halfspace determined
	//	by each matrix in aHolonomyGroup.  For best numerical accuracy
	//	(and least work!) start with the nearest group elements and work
	//	towards the more distance ones.
	//
	//	Technical note:  For large tilings all but the first handful
	//	of group elements will be irrelevant.  If desired one could
	//	modify this code to break the loop when the slicing halfspaces
	//	lie further away than the most distance vertices.
	for (i = 0; i < aHolonomyGroup->itsNumMatrices; i++)
	{
		theErrorMessage = IntersectWithHalfspace(*aDirichletDomain, &aHolonomyGroup->itsMatrices[i]);
		if (theErrorMessage != NULL)
			goto CleanUpConstructDirichletDomain;
	}

	//	Record the space type.
	if (aHolonomyGroup->itsMatrices[1].m[3][3] <  1.0)
		(*aDirichletDomain)->itsSpaceType = SpaceSpherical;
	else
	if (aHolonomyGroup->itsMatrices[1].m[3][3] == 1.0)
		(*aDirichletDomain)->itsSpaceType = SpaceFlat;
	else
		(*aDirichletDomain)->itsSpaceType = SpaceHyperbolic;

	//	Normalize each vertex's position relative to the geometry.
//WILL NEED TO THINK ABOUT THIS STEP WITH VERTICES AT INFINITY.
	for (	theVertex = (*aDirichletDomain)->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		theErrorMessage = VectorNormalize(	&theVertex->itsRawPosition,
											(*aDirichletDomain)->itsSpaceType,
											&theVertex->itsNormalizedPosition);
		if (theErrorMessage != NULL)
			goto CleanUpConstructDirichletDomain;
	}

	//	Normalize each vertex's raw position to sit on the unit 3-sphere.
	//	This ignores the space's intrinsic geometry (spherical, flat or
	//	hyperbolic) but provides reasonable interpolation between
	//	finite vertices and vertices at infinity.  In addition, it serves
	//	the more prosaic purpose or making it easy to sum vertex positions
	//	to get face centers.
	//
	//	Note:  Unlike (I think) the rest of the algorithm, this step
	//	requires a division.  Consider this if moving to exact arithmetic.
	//	At any rate, the normalization isn't needed for the main algorithm.
	for (	theVertex = (*aDirichletDomain)->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		theErrorMessage = VectorNormalize(	&theVertex->itsRawPosition,
											SpaceSpherical,	//	regardless of true SpaceType
											&theVertex->itsRawPosition);
		if (theErrorMessage != NULL)
			goto CleanUpConstructDirichletDomain;
	}

	//	Assign colors to the Dirichlet domain's faces
	//	so that matching faces have the same color.
	AssignFaceColors(*aDirichletDomain);

	//	Compute the center of each face,
	//	normalized to the unit 3-sphere and to the SpaceType.
	ComputeFaceCenters(*aDirichletDomain);
	
	//	Compute the dimensions of the triangular wedges comprising each face.
	ComputeWallDimensions(*aDirichletDomain);

	//	Compute the faces of the vertex figure(s).
	//	One face of the vertex figure(s) sits at each vertex
	//	of the fundamental polyhedron.
	//	This code relies on the fact that for each vertex,
	//	itsRawPosition has already been normalized to sit on the 3-sphere.
	theErrorMessage = ComputeVertexFigures(*aDirichletDomain);
	if (theErrorMessage != NULL)
		goto CleanUpConstructDirichletDomain;
	
	//	Precompute some information in preparation for constructing
	//	the Dirichlet domain mesh and the vertex figures mesh.
	PrepareForDirichletMesh(*aDirichletDomain);
	PrepareForVertexFiguresMesh(*aDirichletDomain);

CleanUpConstructDirichletDomain:

	if (theErrorMessage != NULL)
		FreeDirichletDomain(aDirichletDomain);

	return theErrorMessage;
}


void FreeDirichletDomain(DirichletDomain **aDirichletDomain)
{
	HEVertex	*theDeadVertex;
	HEHalfEdge	*theDeadHalfEdge;
	HEFace		*theDeadFace;

	if (aDirichletDomain != NULL
	 && *aDirichletDomain != NULL)
	{
		while ((*aDirichletDomain)->itsVertexList != NULL)
		{
			theDeadVertex						= (*aDirichletDomain)->itsVertexList;
			(*aDirichletDomain)->itsVertexList	= theDeadVertex->itsNext;
			FREE_MEMORY(theDeadVertex);
		}

		while ((*aDirichletDomain)->itsHalfEdgeList != NULL)
		{
			theDeadHalfEdge							= (*aDirichletDomain)->itsHalfEdgeList;
			(*aDirichletDomain)->itsHalfEdgeList	= theDeadHalfEdge->itsNext;
			FREE_MEMORY(theDeadHalfEdge);
		}

		while ((*aDirichletDomain)->itsFaceList != NULL)
		{
			theDeadFace							= (*aDirichletDomain)->itsFaceList;
			(*aDirichletDomain)->itsFaceList	= theDeadFace->itsNext;
			FREE_MEMORY(theDeadFace);
		}

		FREE_MEMORY_SAFELY(*aDirichletDomain);
	}
}


static ErrorText MakeBanana(
	Matrix			*aMatrixA,			//	input
	Matrix			*aMatrixB,			//	input
	Matrix			*aMatrixC,			//	input
	DirichletDomain	**aDirichletDomain)	//	output
{
	ErrorText		theErrorMessage			= NULL,
					theOutOfMemoryMessage	= u"Out of memory in MakeBanana().";
	Matrix			*theMatrices[3];
	Vector			theHalfspaces[3];
	HEVertex		*theVertices[2];
	HEHalfEdge		*theHalfEdges[3][2];	//	indexed by which face, coming from which vertex
	HEFace			*theFaces[3];
	unsigned int	i,
					j;

	//	Make sure the output pointer is clear.
	if (*aDirichletDomain != NULL)
		return u"MakeBanana() received a non-NULL output location.";

	//	Put the input matrices on an array.
	theMatrices[0] = aMatrixA;
	theMatrices[1] = aMatrixB;
	theMatrices[2] = aMatrixC;

	//	Each matrix determines a halfspace
	//
	//		ax + by + cz + dw ≤ 0
	//
	for (i = 0; i < 3; i++)
		MakeHalfspaceInequality(theMatrices[i], &theHalfspaces[i]);

	//	Allocate the base DirichletDomain structure.
	//	Initialize its lists to NULL immediately, so everything will be kosher
	//	if we encounter an error later in the construction.
	*aDirichletDomain = (DirichletDomain *) GET_MEMORY(sizeof(DirichletDomain));
	if (*aDirichletDomain == NULL)
	{
		theErrorMessage = theOutOfMemoryMessage;
		goto CleanUpMakeBanana;
	}
	(*aDirichletDomain)->itsVertexList		= NULL;
	(*aDirichletDomain)->itsHalfEdgeList	= NULL;
	(*aDirichletDomain)->itsFaceList		= NULL;

	//	Allocate memory for the new vertices, half edges and faces.
	//	Put them on the Dirichlet domain's linked lists immediately,
	//	so that if anything goes wrong all memory will get freed.
	for (i = 0; i < 2; i++)
	{
		theVertices[i] = (HEVertex *) GET_MEMORY(sizeof(HEVertex));
		if (theVertices[i] == NULL)
		{
			theErrorMessage = theOutOfMemoryMessage;
			goto CleanUpMakeBanana;
		}
		theVertices[i]->itsNext				= (*aDirichletDomain)->itsVertexList;
		(*aDirichletDomain)->itsVertexList	= theVertices[i];
	}
	for (i = 0; i < 3; i++)
		for (j = 0; j < 2; j++)
		{
			theHalfEdges[i][j] = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
			if (theHalfEdges[i][j] == NULL)
			{
				theErrorMessage = theOutOfMemoryMessage;
				goto CleanUpMakeBanana;
			}
			theHalfEdges[i][j]->itsNext				= (*aDirichletDomain)->itsHalfEdgeList;
			(*aDirichletDomain)->itsHalfEdgeList	= theHalfEdges[i][j];
		}
	for (i = 0; i < 3; i++)
	{
		theFaces[i] = (HEFace *) GET_MEMORY(sizeof(HEFace));
		if (theFaces[i] == NULL)
		{
			theErrorMessage = theOutOfMemoryMessage;
			goto CleanUpMakeBanana;
		}
		theFaces[i]->itsNext				= (*aDirichletDomain)->itsFaceList;
		(*aDirichletDomain)->itsFaceList	= theFaces[i];
	}

	//	Set up the vertices.

	//	The two vertices are sit antipodally opposite each other.
	//	We must choose which vertex will be +TernaryCrossProduct(...)
	//	and which will be -TernaryCrossProduct(...).  One choice
	//	will yield clockwise-oriented faces while the other choice
	//	yields counterclockwise-oriented faces.  To figure out which
	//	is which, consider the three group elements
	//
	//			(x, y, z) → (x+ε, y,  z )
	//			(x, y, z) → ( x, y+ε, z )
	//			(x, y, z) → ( x,  y, z+ε)
	//
	//	with inequalities x ≤ ε/2, y ≤ ε/2 and z ≤ ε/2, respectively.
	//	The ternary cross product of the coefficient vectors
	//	(1,0,0,-ε/2), (0,1,0,-ε/2) and (0,0,1,-ε/2) comes out to
	//
	//				(-1, -ε/2, -ε/2, -ε/2)
	//
	//	So with the half edge pointers organized as below, we want
	//	the cross product to be vertex 0 (near the south pole (0,0,0,-1))
	//	and it's negative to be vertex 1 (near the north pole (0,0,0,+1))
	//	to give clockwise oriented faces in our left-handed coordinate system.
	//	By continuity, we expect clockwise orientations for all linearly
	//	independent halfspaces (but I should probably think about that
	//	more carefully!).

	VectorTernaryCrossProduct(	&theHalfspaces[0],
								&theHalfspaces[1],
								&theHalfspaces[2],
								&theVertices[0]->itsRawPosition);
	VectorNegate(	&theVertices[0]->itsRawPosition,
					&theVertices[1]->itsRawPosition);

	for (i = 0; i < 2; i++)
	{
		//	Let each vertex see an outbound edge on face 0.
		theVertices[i]->itsOutboundHalfEdge = theHalfEdges[0][i];
	}

	//	Set up the half edges.
	for (i = 0; i < 3; i++)
		for (j = 0; j < 2; j++)
		{
			//	Let theHalfEdges[i][j] run from vertex j to vertex ~j.
			theHalfEdges[i][j]->itsTip = theVertices[!j];

			//	It mate sits on a neighboring face.
			theHalfEdges[i][j]->itsMate = theHalfEdges[(i+1+j)%3][!j];

			//	Two two half edges on each face form their own cycle.
			theHalfEdges[i][j]->itsCycle = theHalfEdges[i][!j];

			//	The edge sees the face.
			theHalfEdges[i][j]->itsFace = theFaces[i];
		}

	//	Set up the faces.
	for (i = 0; i < 3; i++)
	{
		//	The face sees one of its edges.
		theFaces[i]->itsHalfEdge = theHalfEdges[i][0];

		//	Copy the matrix.
		theFaces[i]->itsMatrix = *theMatrices[i];

		//	Set the halfspace inequality.
		theFaces[i]->itsHalfspace = theHalfspaces[i];
	}

CleanUpMakeBanana:

	if (theErrorMessage != NULL)
		FreeDirichletDomain(aDirichletDomain);

	return theErrorMessage;
}


static ErrorText MakeLens(
	Matrix			*aMatrixA,			//	input
	Matrix			*aMatrixB,			//	input
	DirichletDomain	**aDirichletDomain)	//	output
{
	//	This is not a fully general algorithm!
	//	It assumes a central axis passing through the basepoint (0,0,0,1)
	//	and running in the z-direction.  In other words, it assumes
	//	the face planes, whether for a lens or for a slab,
	//	are “parallel” to the xy-plane.

	ErrorText		theErrorMessage			= NULL,
					theOutOfMemoryMessage	= u"Out of memory in MakeLens().";
	unsigned int	n;
	double			theApproximateN;
	HEVertex		**theVertices		= NULL;	//	*theVertices[n]
	HEHalfEdge		*(*theHalfEdges)[2]	= NULL;	//	*theHalfEdges[n][2]
	HEFace			*theFaces[2];
	unsigned int	i,
					j;

	//	Make sure the output pointer is clear.
	if (*aDirichletDomain != NULL)
		return u"MakeLens() received a non-NULL output location.";

	//	The two face planes will meet along the circle
	//	{x² + y² = 1, z² + w² = 0}, which we divide into n segments
	//	(n ≥ 3) in such a way as to respect the group.
	//
	//	Warning:  The determination of n is ad hoc and will
	//	work only with the sorts of matrices we are expecting!
	if (aMatrixA->m[3][3] == 1.0)
	{
		//	Flat space.
		//	n = 4 should work great for the sorts of reflections
		//	and half-turns we are expecting.
		n = 4;
	}
	else if (aMatrixA->m[3][3] < 1.0)
	{
		//	Lens space.
		//	Infer the order from the zw-rotation.
		if (aMatrixA->m[0][2] != 0.0 || aMatrixA->m[0][3] != 0.0
		 || aMatrixA->m[1][2] != 0.0 || aMatrixA->m[1][3] != 0.0
		 || aMatrixA->m[2][0] != 0.0 || aMatrixA->m[2][1] != 0.0
		 || aMatrixA->m[3][0] != 0.0 || aMatrixA->m[3][1] != 0.0)
		{
			theErrorMessage = u"MakeLens() confused by potential lens space.";
			goto CleanUpMakeLens;
		}

		theApproximateN = (2*PI)/fabs(atan2(aMatrixA->m[3][2], aMatrixA->m[3][3]));
		n = (unsigned int)floor(theApproximateN + 0.5);
		if (fabs(theApproximateN - n) > ORDER_EPSILON)
		{
			theErrorMessage = u"MakeLens() couldn't deduce order of potential lens space.";
			goto CleanUpMakeLens;
		}
	}
	else
	{
		theErrorMessage = u"MakeLens() can't handle hyperbolic slab spaces.";
		goto CleanUpMakeLens;
	}

	//	Allocate the base DirichletDomain structure.
	//	Initialize its lists to NULL immediately, so everything will be kosher
	//	if we encounter an error later in the construction.
	*aDirichletDomain = (DirichletDomain *) GET_MEMORY(sizeof(DirichletDomain));
	if (*aDirichletDomain == NULL)
	{
		theErrorMessage = theOutOfMemoryMessage;
		goto CleanUpMakeLens;
	}
	(*aDirichletDomain)->itsVertexList		= NULL;
	(*aDirichletDomain)->itsHalfEdgeList	= NULL;
	(*aDirichletDomain)->itsFaceList		= NULL;

	//	Allocate memory for the new vertices, half edges and faces.
	//	Put them on the Dirichlet domain's linked lists immediately,
	//	so that if anything goes wrong all memory will get freed.

	theVertices = (HEVertex **) GET_MEMORY(n * sizeof(HEVertex *));
	if (theVertices == NULL)
	{
		theErrorMessage = theOutOfMemoryMessage;
		goto CleanUpMakeLens;
	}
	for (i = 0; i < n; i++)
	{
		theVertices[i] = (HEVertex *) GET_MEMORY(sizeof(HEVertex));
		if (theVertices[i] == NULL)
		{
			theErrorMessage = theOutOfMemoryMessage;
			goto CleanUpMakeLens;
		}
		theVertices[i]->itsNext				= (*aDirichletDomain)->itsVertexList;
		(*aDirichletDomain)->itsVertexList	= theVertices[i];
	}

	theHalfEdges = (HEHalfEdge *(*)[2]) GET_MEMORY(n * sizeof(HEHalfEdge *[2]));
	if (theHalfEdges == NULL)
	{
		theErrorMessage = theOutOfMemoryMessage;
		goto CleanUpMakeLens;
	}
	for (i = 0; i < n; i++)
		for (j = 0; j < 2; j++)
		{
			theHalfEdges[i][j] = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
			if (theHalfEdges[i][j] == NULL)
			{
				theErrorMessage = theOutOfMemoryMessage;
				goto CleanUpMakeLens;
			}
			theHalfEdges[i][j]->itsNext				= (*aDirichletDomain)->itsHalfEdgeList;
			(*aDirichletDomain)->itsHalfEdgeList	= theHalfEdges[i][j];
		}

	for (i = 0; i < 2; i++)
	{
		theFaces[i] = (HEFace *) GET_MEMORY(sizeof(HEFace));
		if (theFaces[i] == NULL)
		{
			theErrorMessage = theOutOfMemoryMessage;
			goto CleanUpMakeLens;
		}
		theFaces[i]->itsNext				= (*aDirichletDomain)->itsFaceList;
		(*aDirichletDomain)->itsFaceList	= theFaces[i];
	}

	//	Set up the vertices.
	for (i = 0; i < n; i++)
	{
		//	All vertices sit on the xy circle.
		theVertices[i]->itsRawPosition.v[0] = cos(i*2*PI/n);
		theVertices[i]->itsRawPosition.v[1] = sin(i*2*PI/n);
		theVertices[i]->itsRawPosition.v[2] = 0.0;
		theVertices[i]->itsRawPosition.v[3] = 0.0;

		//	Let each vertex see an outbound edge on face 0
		//	(the face sitting at positive z).
		theVertices[i]->itsOutboundHalfEdge = theHalfEdges[i][0];
	}

	//	Set up the half edges.
	for (i = 0; i < n; i++)
	{
		//	Let theHalfEdges[i][j] connect vertex i to vertex (i+1)%n.
		//	On face 0 (at positive z) the half edge runs "forward"
		//	while on face 1 (at negative z) the half edge runs "backwards".
		theHalfEdges[i][0]->itsTip = theVertices[(i+1)%n];
		theHalfEdges[i][1]->itsTip = theVertices[   i   ];

		//	theHalfEdges[i][0] and theHalfEdges[i][1] are mates.
		theHalfEdges[i][0]->itsMate = theHalfEdges[i][1];
		theHalfEdges[i][1]->itsMate = theHalfEdges[i][0];

		//	All half edges should cycle clockwise as seen from the outside.
		theHalfEdges[i][0]->itsCycle = theHalfEdges[( i+1 )%n][0];
		theHalfEdges[i][1]->itsCycle = theHalfEdges[(i+n-1)%n][1];

		//	Note the faces.
		theHalfEdges[i][0]->itsFace = theFaces[0];
		theHalfEdges[i][1]->itsFace = theFaces[1];
	}

	//	Set up the faces.

	//		Each face sees one of its edges.
	theFaces[0]->itsHalfEdge = theHalfEdges[0][0];
	theFaces[1]->itsHalfEdge = theHalfEdges[0][1];

	//		Set the halfspace inequalities.
	MakeHalfspaceInequality(aMatrixA, &theFaces[0]->itsHalfspace);
	MakeHalfspaceInequality(aMatrixB, &theFaces[1]->itsHalfspace);

	//		Copy the matrices.
	theFaces[0]->itsMatrix = *aMatrixA;
	theFaces[1]->itsMatrix = *aMatrixB;

CleanUpMakeLens:

	if (theErrorMessage != NULL)
		FreeDirichletDomain(aDirichletDomain);

	FREE_MEMORY_SAFELY(theHalfEdges);
	FREE_MEMORY_SAFELY(theVertices);

	return theErrorMessage;
}


static void MakeHalfspaceInequality(
	Matrix	*aMatrix,		//	input
	Vector	*anInequality)	//	output
{
	//	Find the halfspace
	//
	//		ax + by + cz + dw ≤ 0
	//
	//	lying midway between the origin (0,0,0,1) and the image
	//	of the origin under the action of aMatrix, and containing the origin.

	double			theLengthSquared;
	unsigned int	i;

	//	The last row of aMatrix gives the image of the basepoint (0,0,0,1).
	//	Compute the difference vector running from the basepoint to that image.
	for (i = 0; i < 4; i++)
		anInequality->v[i] = aMatrix->m[3][i];
	anInequality->v[3] -= 1.0;

	//	Adjust the raw difference vector according to the geometry.

	if (aMatrix->m[3][3] <  1.0)	//	spherical case
	{
		//	no adjustment needed
	}

	if (aMatrix->m[3][3] == 1.0)	//	flat case
	{
		theLengthSquared = VectorDotProduct(anInequality, anInequality);
		anInequality->v[3] = -0.5 * theLengthSquared;
	}

	if (aMatrix->m[3][3] >  1.0)	//	hyperbolic case
	{
		//	mimic Minkowski metric
		anInequality->v[3] = - anInequality->v[3];
	}
}


static ErrorText IntersectWithHalfspace(
	DirichletDomain	*aDirichletDomain,	//	input and output
	Matrix			*aMatrix)			//	input
{
	ErrorText	theMemoryError = u"Memory request failed in IntersectWithHalfspace().";
	Vector		theHalfspace;
	bool		theCutIsNontrivial;
	double		theDotProduct;
	HEVertex	*theVertex,
				*theVertex1,
				*theVertex2,
				*theNewVertex;
	HEHalfEdge	*theHalfEdge1,
				*theHalfEdge1a,
				*theHalfEdge1b,
				*theHalfEdge2,
				*theHalfEdge2a,
				*theHalfEdge2b;
	HEFace		*theFace,
				*theInnerFace,
				*theOuterFace;
	HEHalfEdge	*theGoingOutEdge,
				*theGoingInEdge,
				*theHalfEdge,
				*theInnerHalfEdge,
				*theOuterHalfEdge;
	HEFace		*theNewFace;
	HEVertex	**theVertexPtr,
				*theDeadVertex;
	HEHalfEdge	**theHalfEdgePtr,
				*theDeadHalfEdge;
	HEFace		**theFacePtr,
				*theDeadFace;

	//	Ignore the identity matrix.
	if (MatrixIsIdentity(aMatrix))
		return NULL;	//	Nothing to do, but not an error.

	//	What halfspace does aMatrixDefine?
	MakeHalfspaceInequality(aMatrix, &theHalfspace);

	//	Evaluate the halfspace equation on all vertices
	//	of the provisional Dirichlet domain.
	//	Work with raw (non-normalized) positions for now.

	theCutIsNontrivial = false;

	for (	theVertex = aDirichletDomain->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		theDotProduct = VectorDotProduct(&theHalfspace, &theVertex->itsRawPosition);

		if (theDotProduct < -VERTEX_HALFSPACE_EPSILON)
		{
			theVertex->itsHalfspaceStatus = VertexInsideHalfspace;
		}
		else
		if (theDotProduct > +VERTEX_HALFSPACE_EPSILON)
		{
			theVertex->itsHalfspaceStatus = VertexOutsideHalfspace;
			theCutIsNontrivial = true;
		}
		else
		{
			theVertex->itsHalfspaceStatus = VertexOnBoundary;
		}
	}

	//	If the halfspace fails to cut aDirichletDomain,
	//	nothing needs to be done.
	if ( ! theCutIsNontrivial )
		return NULL;

	//	Wherever the slicing halfspace crosses an edge,
	//	introduce a new vertex at the cut point.
	for (	theHalfEdge1 = aDirichletDomain->itsHalfEdgeList;
			theHalfEdge1 != NULL;
			theHalfEdge1 = theHalfEdge1->itsNext)
	{
		//	Find the mate.
		theHalfEdge2 = theHalfEdge1->itsMate;

		//	Find the adjacent vertices.
		theVertex1 = theHalfEdge1->itsTip;
		theVertex2 = theHalfEdge2->itsTip;

		//	Does the edge get cut?
		//
		//	Technical note:  Consider only the case that
		//	theVertex1 lies inside the halfspace while
		//	theVertex2 lies outside it, so that we'll have
		//	a reliable orientation for the ternary cross product.
		//	The for(;;) loop will eventually consider all half edges,
		//	so all edges will get properly cut.
		if (theVertex1->itsHalfspaceStatus == VertexInsideHalfspace
		 && theVertex2->itsHalfspaceStatus == VertexOutsideHalfspace)
		{
			//	Split the pair of half edges as shown.
			//	(View this diagram in a mirror if you're orienting
			//	your faces counterclockwise rather than clockwise.)
			//
			//		vertex  <---1a---   new   <---1b---  vertex
			//		  1      ---2b---> vertex  ---2a--->   2
			//

			//	Create a new vertex and put it on the list.
			theNewVertex = (HEVertex *) GET_MEMORY(sizeof(HEVertex));
			if (theNewVertex == NULL)
				return theMemoryError;
			theNewVertex->itsNext			= aDirichletDomain->itsVertexList;
			aDirichletDomain->itsVertexList	= theNewVertex;

			//	Set the new vertex's raw position, along with itsHalfspaceStatus.
			//	We'll compute the normalized position when the Dirichlet domain
			//	is complete.
			//
			//	To be honest I'm not sure a priori what ordering of the factors
			//	will give a ternary cross product result with w > 0, but
			//	the result should vary continuously, so if we get it right
			//	for one set of inputs it should remain right for all other
			//	inputs as well.  (Yes, I know, I should give this more
			//	careful thought!)
			VectorTernaryCrossProduct(	&theHalfEdge1->itsFace->itsHalfspace,
										&theHalfEdge2->itsFace->itsHalfspace,
										&theHalfspace,
										&theNewVertex->itsRawPosition);
			theNewVertex->itsHalfspaceStatus = VertexOnBoundary;

			//	We'll set theNewVertex->itsOutboundHalfEdge in a moment,
			//	after the creating the new edges.

			//	Create two new edges and put them on the list.

			theHalfEdge1a = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
			if (theHalfEdge1a == NULL)
				return theMemoryError;
			theHalfEdge1a->itsNext				= aDirichletDomain->itsHalfEdgeList;
			aDirichletDomain->itsHalfEdgeList	= theHalfEdge1a;

			theHalfEdge2a = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
			if (theHalfEdge2a == NULL)
				return theMemoryError;
			theHalfEdge2a->itsNext				= aDirichletDomain->itsHalfEdgeList;
			aDirichletDomain->itsHalfEdgeList	= theHalfEdge2a;

			//	Recycle the existing pair of edges.
			//	Let them become theHalfEdge1b and theHalfEdge2b
			//	(not theHalfEdge1a and theHalfEdge2a) so that other
			//	vertices and edges that used to point to theHalfEdge1
			//	and theHalfEdge2 will remain valid.
			theHalfEdge1b = theHalfEdge1;
			theHalfEdge2b = theHalfEdge2;

			//	Set the tips.
			theHalfEdge1a->itsTip = theHalfEdge1b->itsTip;
			theHalfEdge2a->itsTip = theHalfEdge2b->itsTip;
			theHalfEdge1b->itsTip = theNewVertex;
			theHalfEdge2b->itsTip = theNewVertex;

			//	Set the mates.
			theHalfEdge1a->itsMate	= theHalfEdge2b;
			theHalfEdge2a->itsMate	= theHalfEdge1b;
			theHalfEdge1b->itsMate	= theHalfEdge2a;
			theHalfEdge2b->itsMate	= theHalfEdge1a;

			//	Set the cycles.
			theHalfEdge1a->itsCycle = theHalfEdge1b->itsCycle;
			theHalfEdge2a->itsCycle = theHalfEdge2b->itsCycle;
			theHalfEdge1b->itsCycle = theHalfEdge1a;
			theHalfEdge2b->itsCycle = theHalfEdge2a;

			//	Set the faces.
			theHalfEdge1a->itsFace = theHalfEdge1b->itsFace;
			theHalfEdge2a->itsFace = theHalfEdge2b->itsFace;

			//	The new vertex sits at the tail of both theHalfEdge1a and theHalfEdge2a.
			theNewVertex->itsOutboundHalfEdge = theHalfEdge1a;
		}
	}

	//	Wherever the slicing halfspace crosses a face,
	//	introduce a new edge along the cut.
	//	The required vertices are already in place
	//	from the previous step.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		//	Look for half edges where the face's cycle is about to leave
		//	the halfspace and where it's about to re-enter the halfspace.

		theGoingOutEdge	= NULL;
		theGoingInEdge	= NULL;

		theHalfEdge = theFace->itsHalfEdge;
		do
		{
			if (theHalfEdge->itsTip->itsHalfspaceStatus == VertexOnBoundary)
			{
				switch (theHalfEdge->itsCycle->itsTip->itsHalfspaceStatus)
				{
					case VertexInsideHalfspace:		theGoingInEdge  = theHalfEdge;	break;
					case VertexOnBoundary:											break;
					case VertexOutsideHalfspace:	theGoingOutEdge = theHalfEdge;	break;
				}
			}
			theHalfEdge = theHalfEdge->itsCycle;
		} while (theHalfEdge != theFace->itsHalfEdge);

		//	If the halfspace doesn't cut the face, there's nothing to be done.
		if (theGoingOutEdge	== NULL
		 || theGoingInEdge  == NULL)
			continue;

		//	Create two new half edges and one new face.
		//	The face will eventually be discarded,
		//	but install it anyhow to keep the data structure clean.

		theInnerHalfEdge = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
		if (theInnerHalfEdge == NULL)
			return theMemoryError;
		theInnerHalfEdge->itsNext			= aDirichletDomain->itsHalfEdgeList;
		aDirichletDomain->itsHalfEdgeList	= theInnerHalfEdge;

		theOuterHalfEdge = (HEHalfEdge *) GET_MEMORY(sizeof(HEHalfEdge));
		if (theOuterHalfEdge == NULL)
			return theMemoryError;
		theOuterHalfEdge->itsNext			= aDirichletDomain->itsHalfEdgeList;
		aDirichletDomain->itsHalfEdgeList	= theOuterHalfEdge;

		theOuterFace = (HEFace *) GET_MEMORY(sizeof(HEFace));
		if (theOuterFace == NULL)
			return theMemoryError;
		theOuterFace->itsNext			= aDirichletDomain->itsFaceList;
		aDirichletDomain->itsFaceList	= theOuterFace;

		//	Recycle theFace as theInnerFace.
		theInnerFace = theFace;

		//	Set the tips.
		theInnerHalfEdge->itsTip	= theGoingInEdge->itsTip;
		theOuterHalfEdge->itsTip	= theGoingOutEdge->itsTip;

		//	Set the mates.
		theInnerHalfEdge->itsMate	= theOuterHalfEdge;
		theOuterHalfEdge->itsMate	= theInnerHalfEdge;

		//	Set the cycles.
		theInnerHalfEdge->itsCycle	= theGoingInEdge->itsCycle;
		theOuterHalfEdge->itsCycle	= theGoingOutEdge->itsCycle;
		theGoingOutEdge->itsCycle	= theInnerHalfEdge;
		theGoingInEdge->itsCycle	= theOuterHalfEdge;

		//	Set the inner face (which equals the original face).
		theInnerHalfEdge->itsFace	= theInnerFace;
		theInnerFace->itsHalfEdge	= theInnerHalfEdge;

		//	Set the outer face.
		theHalfEdge = theOuterHalfEdge;
		do
		{
			theHalfEdge->itsFace = theOuterFace;
			theHalfEdge = theHalfEdge->itsCycle;
		} while (theHalfEdge != theOuterHalfEdge);
		theOuterFace->itsHalfEdge	= theOuterHalfEdge;
	}

	//	Allocate a new face to lie on the boundary of the halfspace.
	theNewFace = (HEFace *) GET_MEMORY(sizeof(HEFace));
	if (theNewFace == NULL)
		return theMemoryError;
	theNewFace->itsNext				= aDirichletDomain->itsFaceList;
	aDirichletDomain->itsFaceList	= theNewFace;

	//	Mark for deletion all half edges and faces
	//	that are incident to a VertexOutsideHalfspace.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		theFace->itsDeletionFlag = false;
	}
	for (	theHalfEdge = aDirichletDomain->itsHalfEdgeList;
			theHalfEdge != NULL;
			theHalfEdge = theHalfEdge->itsNext)
	{
		if (theHalfEdge->itsTip->itsHalfspaceStatus				== VertexOutsideHalfspace
		 || theHalfEdge->itsMate->itsTip->itsHalfspaceStatus	== VertexOutsideHalfspace)
		{
			theHalfEdge->itsDeletionFlag			= true;
			theHalfEdge->itsFace->itsDeletionFlag	= true;
		}
		else
			theHalfEdge->itsDeletionFlag			= false;
	}

	//	Make sure all surviving vertices see a surviving half edge.
	for (	theVertex = aDirichletDomain->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		if (theVertex->itsHalfspaceStatus != VertexOutsideHalfspace)
		{
			while (theVertex->itsOutboundHalfEdge->itsDeletionFlag)
			{
				theVertex->itsOutboundHalfEdge
					= theVertex->itsOutboundHalfEdge->itsMate->itsCycle;
			}
		}
	}

	//	Install the new face.
	for (	theHalfEdge = aDirichletDomain->itsHalfEdgeList;
			theHalfEdge != NULL;
			theHalfEdge = theHalfEdge->itsNext)
	{
		if ( ! theHalfEdge->itsDeletionFlag
		 && theHalfEdge->itsFace->itsDeletionFlag)
		{
			theHalfEdge->itsFace	= theNewFace;
			theNewFace->itsHalfEdge	= theHalfEdge;

			while (theHalfEdge->itsCycle->itsDeletionFlag)
			{
				theHalfEdge->itsCycle = theHalfEdge->itsCycle->itsMate->itsCycle;
			}
		}
	}

	//	Set the new face's halfspace inequality and matrix.
	theNewFace->itsHalfspace	= theHalfspace;
	theNewFace->itsMatrix		= *aMatrix;

	//	Delete excluded vertices, half edges and faces.

	theVertexPtr = &aDirichletDomain->itsVertexList;
	while (*theVertexPtr != NULL)
	{
		if ((*theVertexPtr)->itsHalfspaceStatus == VertexOutsideHalfspace)
		{
			theDeadVertex	= *theVertexPtr;
			*theVertexPtr	= theDeadVertex->itsNext;
			FREE_MEMORY(theDeadVertex);
		}
		else
			theVertexPtr = &(*theVertexPtr)->itsNext;
	}

	theHalfEdgePtr = &aDirichletDomain->itsHalfEdgeList;
	while (*theHalfEdgePtr != NULL)
	{
		if ((*theHalfEdgePtr)->itsDeletionFlag)
		{
			theDeadHalfEdge	= *theHalfEdgePtr;
			*theHalfEdgePtr	= theDeadHalfEdge->itsNext;
			FREE_MEMORY(theDeadHalfEdge);
		}
		else
			theHalfEdgePtr = &(*theHalfEdgePtr)->itsNext;
	}

	theFacePtr = &aDirichletDomain->itsFaceList;
	while (*theFacePtr != NULL)
	{
		if ((*theFacePtr)->itsDeletionFlag)
		{
			theDeadFace	= *theFacePtr;
			*theFacePtr	= theDeadFace->itsNext;
			FREE_MEMORY(theDeadFace);
		}
		else
			theFacePtr = &(*theFacePtr)->itsNext;
	}

	//	Done!
	return NULL;
}


static void AssignFaceColors(DirichletDomain *aDirichletDomain)
{
	HEFace			*theFace;
	unsigned int	theCount;
	Matrix			theInverseMatrix;
	HEFace			*theMate;
	double			theColorParameter;

	//	Initialize each color index to 0xFFFFFFFF as a marker.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		theFace->itsColorIndex = 0xFFFFFFFF;
	}

	//	Count the face pairs as we go along.
	theCount = 0;

	//	Assign an index to each face that doesn't already have one.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		if (theFace->itsColorIndex == 0xFFFFFFFF)
		{
			//	Assign to theFace the next available color index.
			theFace->itsColorIndex = theCount++;

			//	If theFace has a distinct mate,
			//	assign the same index to the mate.
			MatrixGeometricInverse(&theFace->itsMatrix, &theInverseMatrix);
			for (	theMate = theFace->itsNext;
					theMate != NULL;
					theMate = theMate->itsNext)
			{
				if (MatrixEquality(&theMate->itsMatrix, &theInverseMatrix, MATE_MATRIX_EPSILON))
				{
					theMate->itsColorIndex = theFace->itsColorIndex;
					break;
				}
			}
		}
	}

	//	Now that we know how many face pairs we've got,
	//	we can convert the temporary indices to a set
	//	of evenly spaced colors.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		//	Convert the temporary index to a parameter in the range [0,1],
		//	with uniform spacing.
		theColorParameter = (double)theFace->itsColorIndex / (double)theCount;

		//	Interpret theColorParameter as a hue.
		HSLAtoRGBA(	&(HSLAColor){theColorParameter, 0.3, 0.5, 1.0},
					&theFace->itsColorRGBA);

		//	Interpret theColorParameter as a greyscale value.
//		theFace->itsColorGreyscale = 0.5 * (theColorParameter + 1.0);
		theFace->itsColorGreyscale = (theColorParameter + 4.0) / 5.0;
	}
}


static void ComputeFaceCenters(DirichletDomain *aDirichletDomain)
{
	HEFace			*theFace;
	unsigned int	i;

	//	Compute the center of each face, normalized to the unit 3-sphere
	//	for easy interpolation to vertices at infinity.

	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		//	The center sits midway between the basepoint (0,0,0,1)
		//	and its image under the face-pairing matrix.
		for (i = 0; i < 4; i++)
			theFace->itsRawCenter.v[i] = 0.5 * theFace->itsMatrix.m[3][i];
		theFace->itsRawCenter.v[3] += 0.5;

		//	Normalize to the unit 3-sphere...
		VectorNormalize(&theFace->itsRawCenter, SpaceSpherical, &theFace->itsRawCenter);
		
		//	...and also relative to the SpaceType.
		VectorNormalize(&theFace->itsRawCenter, aDirichletDomain->itsSpaceType, &theFace->itsNormalizedCenter);
	}
}


static void ComputeWallDimensions(DirichletDomain *aDirichletDomain)
{
	double		theMaxBase;
	HEFace		*theFace;
	Vector		*theFaceCenter;
	HEHalfEdge	*theHalfEdge;
	Vector		*theTail,
				*theTip;
	double		theSide0,
				theSide1,
				theSide2,
				s,
				theArea;

	
	//	Compute the dimensions of the triangular wedges comprising each face.
	theMaxBase = 0;
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		theFaceCenter = &theFace->itsNormalizedCenter;

		theHalfEdge = theFace->itsHalfEdge;
		do
		{
			//	Advance to the next HalfEdge,
			//	but only after reading the current HalfEdge's tip,
			//	which will be the next HalfEdge's tail.
			theTail		= &theHalfEdge->itsTip->itsNormalizedPosition;
			theHalfEdge	= theHalfEdge->itsCycle;
			theTip		= &theHalfEdge->itsTip->itsNormalizedPosition;

			//	Compute the current wedge's dimensions.
			//	The computation is exact in the flat case,
			//	and serves our purposes well enough in the spherical
			//	and hyperbolic cases.
			theSide0					= VectorGeometricDistance2(theTail, theTip       );
			theSide1					= VectorGeometricDistance2(theTail, theFaceCenter);
			theSide2					= VectorGeometricDistance2(theTip,  theFaceCenter);
			s							= 0.5 * (theSide0 + theSide1 + theSide2);
			theArea						= sqrt( s * (s - theSide0) * (s - theSide1) * (s - theSide2) );	//	Heron's formula
			theHalfEdge->itsBase		= theSide0;
			theHalfEdge->itsAltitude	= 2.0 * theArea / theSide0;
			
			//	Note the largest base length.
			if (theMaxBase < theHalfEdge->itsBase)
				theMaxBase = theHalfEdge->itsBase;

		} while (theHalfEdge != theFace->itsHalfEdge);
	}
	
	//	Rescale itsBase and itsAltitude so that the largest base has length 1.
	if (theMaxBase > 0.0)
	{
		for (	theHalfEdge = aDirichletDomain->itsHalfEdgeList;
				theHalfEdge != NULL;
				theHalfEdge = theHalfEdge->itsNext)
		{
			theHalfEdge->itsBase		/= theMaxBase;
			theHalfEdge->itsAltitude	/= theMaxBase;
		}
	}
}


static ErrorText ComputeVertexFigures(DirichletDomain *aDirichletDomain)
{
	ErrorText		theErrorMessage	= NULL;
	HEVertex		*theVertex;
	HEHalfEdge		*theHalfEdge;
	Vector			theTail,
					theTip,
					theComponent,
					theNormal,
					theComponentParallel,
					theComponentPendicular,
					theScaledPointA,
					theScaledPointB;
	double			theDotProduct;

	//	Compute the faces of the vertex figure(s).
	//	One face of the vertex figure(s) sits at each vertex
	//	of the fundamental polyhedron.
	//	This code relies on the fact that for each vertex,
	//	itsRawPosition has already been normalized to sit on the 3-sphere.

	//	Compute the "outer point" on each half edge.
	for (	theHalfEdge = aDirichletDomain->itsHalfEdgeList;
			theHalfEdge != NULL;
			theHalfEdge = theHalfEdge->itsNext)
	{
		theTail			= theHalfEdge->itsMate->itsTip->itsRawPosition;
		theTip			= theHalfEdge->itsTip->itsRawPosition;
		theDotProduct	= VectorDotProduct(&theTail, &theTip);
		ScalarTimesVector(theDotProduct, &theTail, &theComponent);
		VectorDifference(&theTip, &theComponent, &theNormal);
		theErrorMessage	= VectorNormalize(&theNormal, SpaceSpherical, &theNormal);
		if (theErrorMessage != NULL)
			return theErrorMessage;
		ScalarTimesVector(cos(VERTEX_FIGURE_SIZE), &theTail,   &theComponentParallel  );
		ScalarTimesVector(sin(VERTEX_FIGURE_SIZE), &theNormal, &theComponentPendicular);
		VectorSum(&theComponentParallel, &theComponentPendicular, &theHalfEdge->itsOuterPoint);
		theErrorMessage = VectorNormalize(	&theHalfEdge->itsOuterPoint,
											aDirichletDomain->itsSpaceType,
											&theHalfEdge->itsOuterPoint);
		if (theErrorMessage != NULL)
			return theErrorMessage;
	}

	//	Compute the center of each face of the vertex figure.
	for (	theVertex = aDirichletDomain->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		theVertex->itsCenterPoint = (Vector) {{0.0, 0.0, 0.0, 0.0}};

		theHalfEdge = theVertex->itsOutboundHalfEdge;
		do
		{
			VectorSum(	&theVertex->itsCenterPoint,
						&theHalfEdge->itsOuterPoint,
						&theVertex->itsCenterPoint);

			theHalfEdge = theHalfEdge->itsMate->itsCycle;

		} while (theHalfEdge != theVertex->itsOutboundHalfEdge);

		theErrorMessage = VectorNormalize(	&theVertex->itsCenterPoint,
											aDirichletDomain->itsSpaceType,
											&theVertex->itsCenterPoint);
		if (theErrorMessage != NULL)
			return theErrorMessage;
	}

	//	Interpolate the inner vertices between
	//	the outer vertices and the center.
	for (	theHalfEdge = aDirichletDomain->itsHalfEdgeList;
			theHalfEdge != NULL;
			theHalfEdge = theHalfEdge->itsNext)
	{
		ScalarTimesVector(	VERTEX_FIGURE_CUTOUT,
							&theHalfEdge->itsOuterPoint,
							&theScaledPointA);
		ScalarTimesVector(	1.0 - VERTEX_FIGURE_CUTOUT,
							&theHalfEdge->itsMate->itsTip->itsCenterPoint,
							&theScaledPointB);
		VectorSum(&theScaledPointA, &theScaledPointB, &theHalfEdge->itsInnerPoint);
		theErrorMessage = VectorNormalize(	&theHalfEdge->itsInnerPoint,
											aDirichletDomain->itsSpaceType,
											&theHalfEdge->itsInnerPoint);
		if (theErrorMessage != NULL)
			return theErrorMessage;
	}

	return NULL;
}


static void PrepareForDirichletMesh(DirichletDomain *aDirichletDomain)
{
	HEFace			*theFace;
	HEHalfEdge		*theHalfEdge;
	unsigned int	theFaceOrder;

	//	Each n-sided face will contribute an annular region,
	//	realized as n trapezoids, each with 4 vertices and 2 faces.

	aDirichletDomain->itsDirichletNumMeshVertices	= 0;
	aDirichletDomain->itsDirichletNumMeshFaces		= 0;

	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		//	Compute the face order n.
		theFaceOrder = 0;
		theHalfEdge = theFace->itsHalfEdge;
		do
		{
			theFaceOrder++;							//	count this edge
			theHalfEdge	= theHalfEdge->itsCycle;	//	move on to the next edge
		} while (theHalfEdge != theFace->itsHalfEdge);
	
		//	Increment the global counts.
		aDirichletDomain->itsDirichletNumMeshVertices	+= 4*theFaceOrder;
		aDirichletDomain->itsDirichletNumMeshFaces		+= 2*theFaceOrder;
	}
}

static void PrepareForVertexFiguresMesh(DirichletDomain *aDirichletDomain)
{
	HEVertex		*theVertex;
	HEHalfEdge		*theHalfEdge;
	unsigned int	theVertexOrder;

	//	Each order-n vertex will contribute an annular region,
	//	realized as a triangle strip with 2n+2 vertices and 2n faces
	//	(the first pair of vertices gets repeated at the end,
	//	to accommodate possibly different texture coordinates --
	//	otherwise the number of faces would be the same as
	//	the number of vertices).

	aDirichletDomain->itsVertexFiguresNumMeshVertices	= 0;
	aDirichletDomain->itsVertexFiguresNumMeshFaces		= 0;

	for (	theVertex = aDirichletDomain->itsVertexList;
			theVertex != NULL;
			theVertex = theVertex->itsNext)
	{
		//	Compute the vertex order.
		theVertexOrder = 0;
		theHalfEdge = theVertex->itsOutboundHalfEdge;
		do
		{
			theVertexOrder++;								//	count this edge
			theHalfEdge	= theHalfEdge->itsMate->itsCycle;	//	move on to the next edge
		} while (theHalfEdge != theVertex->itsOutboundHalfEdge);
	
		//	Increment the global counts.
		aDirichletDomain->itsVertexFiguresNumMeshVertices	+= 2*theVertexOrder + 2;
		aDirichletDomain->itsVertexFiguresNumMeshFaces		+= 2*theVertexOrder;
	}
}


void StayInDirichletDomain(
	DirichletDomain	*aDirichletDomain,	//	input
	Matrix			*aPlacement)		//	input and output
{
	HEFace			*theFace;
	double			theFaceValue;
	Matrix			theRestoringMatrix;
	unsigned int	i;

	if (aDirichletDomain == NULL)
		return;

	//	The object described by aPlacement is typically
	//	the user him/herself, but may also be the centerpiece
	//	(or anything else, for that matter).

	//	If the object strays out of the Dirichlet domain,
	//	use a face-pairing matrix to bring it back in.
	for (	theFace = aDirichletDomain->itsFaceList;
			theFace != NULL;
			theFace = theFace->itsNext)
	{
		//	Evaluate the halfspace equation on the image of the basepoint (0,0,0,1)
		//	under the action of aPlacement.
		theFaceValue = 0;
		for (i = 0; i < 4; i++)
			theFaceValue += theFace->itsHalfspace.v[i] * aPlacement->m[3][i];

		//	The value we just computed will be positive
		//	iff the user has gone past the given face plane.
		if (theFaceValue > RESTORING_EPSILON)
		{
			//	Apply the inverse of the face-pairing matrix
			//	to bring the user back closer to the origin.
			MatrixGeometricInverse(&theFace->itsMatrix, &theRestoringMatrix);
			MatrixProduct(aPlacement, &theRestoringMatrix, aPlacement);
		}
	}
}


ErrorText ConstructHoneycomb(
	MatrixList		*aHolonomyGroup,	//	input
	DirichletDomain	*aDirichletDomain,	//	input
	Honeycomb		**aHoneycomb)		//	output
{
	ErrorText		theErrorMessage	= NULL;
	unsigned int	theNumVertices;
	HEVertex		*theVertex;
	unsigned int	i,
					j;

	static Vector	theBasepoint		= {{0.0, 0.0, 0.0, 1.0}};

	if (aHolonomyGroup == NULL)
		return u"ConstructHoneycomb() received a NULL holonomy group.";

	if (*aHoneycomb != NULL)
		return u"ConstructHoneycomb() received a non-NULL output location.";

	//	Special case:  Allow a NULL Dirichlet domain,
	//	which occurs for the 3-sphere.  We'll need
	//	the 3-sphere to display Clifford parallels.
	//	(Confession:  This is a hack.  I hope it causes no trouble.)
//	if (aDirichletDomain == NULL)
//		return u"ConstructHoneycomb() received a NULL Dirichlet domain.";

	//	Count the Dirichlet domain's vertices.
	theNumVertices = 0;
	if (aDirichletDomain != NULL)
	{
		for (	theVertex = aDirichletDomain->itsVertexList;
				theVertex != NULL;
				theVertex = theVertex->itsNext)
		{
			theNumVertices++;
		}
	}

	//	Allocate memory for the honeycomb.
	*aHoneycomb = AllocateHoneycomb(aHolonomyGroup->itsNumMatrices, theNumVertices);
	if (*aHoneycomb == NULL)
	{
		theErrorMessage = u"Couldn't get memory for aHoneycomb in ConstructHoneycomb().";
		goto CleanUpConstructHoneycomb;
	}

	//	For each cell...
	for (i = 0; i < aHolonomyGroup->itsNumMatrices; i++)
	{
		//	Set the matrix.
		(*aHoneycomb)->itsCells[i].itsMatrix = aHolonomyGroup->itsMatrices[i];

		//	Compute the image of the basepoint (0,0,0,1).
		VectorTimesMatrix(	&theBasepoint,
							&aHolonomyGroup->itsMatrices[i],
							&(*aHoneycomb)->itsCells[i].itsCenter);

		//	Compute the image of the vertices.
		if (aDirichletDomain != NULL)
		{
			for (	theVertex = aDirichletDomain->itsVertexList, j = 0;
					theVertex != NULL && j < theNumVertices;
					theVertex = theVertex->itsNext, j++)
			{
				VectorTimesMatrix(	&theVertex->itsRawPosition,
									&aHolonomyGroup->itsMatrices[i],
									&(*aHoneycomb)->itsCells[i].itsVertices[j]);
			}
		}

		//	Record the number of vertices.
		(*aHoneycomb)->itsCells[i].itsNumVertices = theNumVertices;
	}

CleanUpConstructHoneycomb:

	//	The present code flags an error only when *aHoneycomb == NULL,
	//	but let's include clean-up code anyhow, for future robustness.
	if (theErrorMessage != NULL)
		FreeHoneycomb(aHoneycomb);

	return theErrorMessage;
}


static Honeycomb *AllocateHoneycomb(
	unsigned int	aNumCells,
	unsigned int	aNumVertices)
{
	Honeycomb		*theHoneycomb	= NULL;
	unsigned int	i;

	if ( aNumCells   > 0xFFFFFFFF / sizeof(Honeycell)	//	for safety
	 || aNumVertices > 0xFFFFFFFF / sizeof(Vector))
		goto CleanUpAllocateHoneycomb;

	theHoneycomb = (Honeycomb *) GET_MEMORY(sizeof(Honeycomb));
	if (theHoneycomb != NULL)
	{
		//	For safe error handling, immediately set all pointers to NULL.
		theHoneycomb->itsCells			= NULL;
		theHoneycomb->itsVisibleCells	= NULL;
	}
	else
		goto CleanUpAllocateHoneycomb;

	theHoneycomb->itsNumCells	= aNumCells;
	theHoneycomb->itsCells		= (Honeycell *) GET_MEMORY(aNumCells * sizeof(Honeycell));
	if (theHoneycomb->itsCells != NULL)
	{
		//	For safe error handling, immediately set all pointers to NULL.
		for (i = 0; i < aNumCells; i++)
			theHoneycomb->itsCells[i].itsVertices = NULL;

		for (i = 0; i < aNumCells; i++)
		{
			theHoneycomb->itsCells[i].itsNumVertices = aNumVertices;
			if (aNumVertices != 0)
			{
				theHoneycomb->itsCells[i].itsVertices = (Vector *) GET_MEMORY(aNumVertices * sizeof(Vector));
				if (theHoneycomb->itsCells[i].itsVertices == NULL)
					goto CleanUpAllocateHoneycomb;
			}
		}
	}
	else
		goto CleanUpAllocateHoneycomb;

	//	Allocate itsVisibleCells and initialize to an empty array.
	//	For simplicity allocate the maximal buffer size, even though
	//	we will never use all of it.
	theHoneycomb->itsNumVisibleCells	= 0;
	theHoneycomb->itsVisibleCells		= (Honeycell **) GET_MEMORY(aNumCells * sizeof(Honeycell *));
	if (theHoneycomb->itsVisibleCells != NULL)
	{
		for (i = 0; i < aNumCells; i++)
			theHoneycomb->itsVisibleCells[i] = NULL;
	}
	else
		goto CleanUpAllocateHoneycomb;

	//	Success
	return theHoneycomb;

CleanUpAllocateHoneycomb:

	//	We reach this point iff some memory allocation call failed.
	//	Free whatever memory may have been allocated.
	//	Other pointers should all be NULL.
	FreeHoneycomb(&theHoneycomb);
	
	//	Failure
	return NULL;
}


void FreeHoneycomb(Honeycomb **aHoneycomb)
{
	unsigned int	i;

	if (aHoneycomb != NULL
	 && *aHoneycomb != NULL)
	{
		if ((*aHoneycomb)->itsCells != NULL)
		{
			for (i = 0; i < (*aHoneycomb)->itsNumCells; i++)
				FREE_MEMORY_SAFELY((*aHoneycomb)->itsCells[i].itsVertices);
			FREE_MEMORY_SAFELY((*aHoneycomb)->itsCells);
		}
		FREE_MEMORY_SAFELY((*aHoneycomb)->itsVisibleCells);
		FREE_MEMORY_SAFELY(*aHoneycomb);
	}
}


ErrorText MakeDirichletVBO(
	GLuint			aVertexBufferName,
	GLuint			anIndexBufferName,
	DirichletDomain	*aDirichletDomain,
	double			anAperture,	//	in range [0.0, 1.0] (closed to open)
	bool			aColorCodingFlag,
	bool			aGreyscaleFlag)
{
	bool				theDirichletDomainIsPresentAndVisible;
	DirichletVBOData	*theVBOVertices	= NULL,
						*theVBOVertex;
	unsigned short		*theVBOIndices	= NULL,
						*theVBOIndex,
						theVBOVertexIndex;
	double				theTextureMultiple;
	HEFace				*theFace;
	float				theColor[4];
	Vector				*theFaceCenter;		//	normalized to the SpaceType
	HEHalfEdge			*theHalfEdge;
	bool				theParity;
	Vector				*theNearOuterVertex,//	normalized to the SpaceType
						theNearInnerVertex,	//	normalized to the SpaceType
						*theFarOuterVertex,	//	normalized to the SpaceType
						theFarInnerVertex;	//	normalized to the SpaceType
	double				theBaseTex,
						theAltitudeTex;

	static const Byte	theDummyByte = 0x00;

	theDirichletDomainIsPresentAndVisible = (aDirichletDomain != NULL && anAperture < 1.0);

	if (theDirichletDomainIsPresentAndVisible)
	{
		theVBOVertices	= (DirichletVBOData *) GET_MEMORY(  aDirichletDomain->itsDirichletNumMeshVertices * sizeof(DirichletVBOData));
		theVBOIndices	= ( unsigned short * ) GET_MEMORY( 3 * aDirichletDomain->itsDirichletNumMeshFaces * sizeof( unsigned short ));
		if (theVBOVertices == NULL
		 || theVBOIndices  == NULL)
		{
			return u"MakeDirichletVAO() couldn't get memory to construct vertex data.";
		}

		theTextureMultiple = (aColorCodingFlag ? FACE_TEXTURE_MULTIPLE_PLAIN : FACE_TEXTURE_MULTIPLE_WOOD);
		
		//	Keep a running pointer to the vertex currently under construction,
		//	and also keep track of its index in the array.  Keeping both these
		//	things is redundant, yet nevertheless convenient.
		theVBOVertex		= theVBOVertices;
		theVBOVertexIndex	= 0;
		
		//	Keep a running pointer to the current entry in the index buffer.
		theVBOIndex = theVBOIndices;

		//	Process each face in turn.
		for (	theFace = aDirichletDomain->itsFaceList;
				theFace != NULL;
				theFace = theFace->itsNext)
		{
			if (aColorCodingFlag && ! aGreyscaleFlag)
			{
				//	itsColorRGBA is already alpha-premultiplied
				theColor[0] = (float) theFace->itsColorRGBA.r;
				theColor[1] = (float) theFace->itsColorRGBA.g;
				theColor[2] = (float) theFace->itsColorRGBA.b;
				theColor[3] = (float) theFace->itsColorRGBA.a;
			}
			else
			{
				//	If the alpha component were less than 1.0,
				//	we'd need to premultiply the RGB components by it.
				theColor[0] = (float) theFace->itsColorGreyscale;
				theColor[1] = (float) theFace->itsColorGreyscale;
				theColor[2] = (float) theFace->itsColorGreyscale;
				theColor[3] = (float) 1.0;
			}

			theFaceCenter = &theFace->itsNormalizedCenter;

			//	After opening a window in the center of an n-sided face,
			//	an annulus-like shape remains, which we triangulate
			//	as n trapezoids, each with 4 vertices and 2 faces.
			//
			//	(An earlier version of this algorithm, archived 
			//	in the file "2n+2 vertices per Dirichlet face.c",
			//	used only 2n+2 vertices for the annulus,
			//	but got the texturing right only for regular faces,
			//	not irregular ones.  Furthermore it wasn't much faster.)
			
			//	Let the tangential texture coordinate run alternately
			//	forwards and backwards, so the texture coordinates will
			//	match up whenever possible.
			theParity = false;

			theHalfEdge = theFace->itsHalfEdge;
			do
			{
				//	Use outer vertices and face centers normalized to the SpaceType.
				//
				//	(Note:  This won't work if we later support vertices-at-infinity.
				//	For vertices-at-infinity, we'd have to use raw positions.
				//	For now let's stick with normalized vectors
				//	to facilitate texturing.  See details below.)

				theNearOuterVertex = &theHalfEdge->itsTip->itsNormalizedPosition;
				VectorInterpolate(	theFaceCenter,
									theNearOuterVertex,
									anAperture,
									&theNearInnerVertex);
				(void) VectorNormalize(&theNearInnerVertex, aDirichletDomain->itsSpaceType, &theNearInnerVertex);

				theFarOuterVertex = &theHalfEdge->itsCycle->itsTip->itsNormalizedPosition;
				VectorInterpolate(	theFaceCenter,
									theFarOuterVertex,
									anAperture,
									&theFarInnerVertex);
				(void) VectorNormalize(&theFarInnerVertex, aDirichletDomain->itsSpaceType, &theFarInnerVertex);
				
				//	Convert the triangle's dimensions from physical units
				//	to texture coordinate units.
				theBaseTex		= theTextureMultiple * theHalfEdge->itsCycle->itsBase;
				theAltitudeTex	= theTextureMultiple * theHalfEdge->itsCycle->itsAltitude;
				
				//	Get the proportions for the texturing exactly right 
				//	in the flat, regular case and approximately right otherwise.
				//
				//	Note.  Perspectively correct texture mapping
				//	is a real challenge in curved spaces.
				//	In the flat case, we're mapping a trapezoidal portion
				//	of a Dirichlet domain face onto a trapezoidal region
				//	in the texture, and we're guaranteed success just so
				//	we make sure the two trapezoids have the same shape
				//	(otherwise the final texturing will kink along the trapezoid's
				//	diagonal, where it splits into two triangles).
				//	In the spherical and hyperbolic cases, however,
				//	some residual distortion seems inevitable.
				//	Vertices-at-infinity would further complicate matters.

				//	near inner vertex
				theVBOVertex->pos[0] = (float) theNearInnerVertex.v[0];
				theVBOVertex->pos[1] = (float) theNearInnerVertex.v[1];
				theVBOVertex->pos[2] = (float) theNearInnerVertex.v[2];
				theVBOVertex->pos[3] = (float) theNearInnerVertex.v[3];
				theVBOVertex->tex[0] = (float) ( theBaseTex * ( theParity ? 0.5 - 0.5*anAperture : 0.5 + 0.5*anAperture ) );
				theVBOVertex->tex[1] = (float) ( theAltitudeTex * (1.0 - anAperture) );
				theVBOVertex->col[0] = theColor[0];
				theVBOVertex->col[1] = theColor[1];
				theVBOVertex->col[2] = theColor[2];
				theVBOVertex->col[3] = theColor[3];
				theVBOVertex++;

				//	near outer vertex
				theVBOVertex->pos[0] = (float) theNearOuterVertex->v[0];
				theVBOVertex->pos[1] = (float) theNearOuterVertex->v[1];
				theVBOVertex->pos[2] = (float) theNearOuterVertex->v[2];
				theVBOVertex->pos[3] = (float) theNearOuterVertex->v[3];
				theVBOVertex->tex[0] = (float) ( theBaseTex * ( theParity ? 0.0 : 1.0 ) );
				theVBOVertex->tex[1] = (float) 0.0;
				theVBOVertex->col[0] = theColor[0];
				theVBOVertex->col[1] = theColor[1];
				theVBOVertex->col[2] = theColor[2];
				theVBOVertex->col[3] = theColor[3];
				theVBOVertex++;

				//	far inner vertex
				theVBOVertex->pos[0] = (float) theFarInnerVertex.v[0];
				theVBOVertex->pos[1] = (float) theFarInnerVertex.v[1];
				theVBOVertex->pos[2] = (float) theFarInnerVertex.v[2];
				theVBOVertex->pos[3] = (float) theFarInnerVertex.v[3];
				theVBOVertex->tex[0] = (float) ( theBaseTex * ( theParity ? 0.5 + 0.5*anAperture : 0.5 - 0.5*anAperture ) );
				theVBOVertex->tex[1] = (float) ( theAltitudeTex * (1.0 - anAperture) );
				theVBOVertex->col[0] = theColor[0];
				theVBOVertex->col[1] = theColor[1];
				theVBOVertex->col[2] = theColor[2];
				theVBOVertex->col[3] = theColor[3];
				theVBOVertex++;

				//	far outer vertex
				theVBOVertex->pos[0] = (float) theFarOuterVertex->v[0];
				theVBOVertex->pos[1] = (float) theFarOuterVertex->v[1];
				theVBOVertex->pos[2] = (float) theFarOuterVertex->v[2];
				theVBOVertex->pos[3] = (float) theFarOuterVertex->v[3];
				theVBOVertex->tex[0] = (float) ( theBaseTex * ( theParity ? 1.0 : 0.0 ) );
				theVBOVertex->tex[1] = (float) 0.0;
				theVBOVertex->col[0] = theColor[0];
				theVBOVertex->col[1] = theColor[1];
				theVBOVertex->col[2] = theColor[2];
				theVBOVertex->col[3] = theColor[3];
				theVBOVertex++;
				
				//	Create a pair of triangles.

				*theVBOIndex++ = theVBOVertexIndex + 0;
				*theVBOIndex++ = theVBOVertexIndex + 1;
				*theVBOIndex++ = theVBOVertexIndex + 2;

				*theVBOIndex++ = theVBOVertexIndex + 2;
				*theVBOIndex++ = theVBOVertexIndex + 1;
				*theVBOIndex++ = theVBOVertexIndex + 3;
				
				//	Update theVBOVertexIndex.
				theVBOVertexIndex += 4;
				
				//	Let the tangential texture coordinate
				//	run the other way next time.
				theParity = ! theParity;
				
				//	Move on to the next HalfEdge.
				theHalfEdge	= theHalfEdge->itsCycle;

			} while (theHalfEdge != theFace->itsHalfEdge);
		}
		
		//	Did we write the correct number of entries into the arrays?
		if ((unsigned int)(theVBOVertex - theVBOVertices) != aDirichletDomain->itsDirichletNumMeshVertices
		 || (unsigned int)(theVBOIndex  - theVBOIndices ) != 3 * aDirichletDomain->itsDirichletNumMeshFaces
		 || theVBOVertexIndex != aDirichletDomain->itsDirichletNumMeshVertices)
		{
			return u"Wrong number of array entries written in MakeDirichletVAO().";
		}
	}

	//	Send the Dirichlet domain data to the GPU.
	//
	//	If MakeDirichletVAO() gets called when the Dirichlet domain is missing or invisible,
	//	provide dummy buffers so that glVertexAttribPointer() doesn't choke.
	//	Maybe it'd be OK with empty data, but why take chances?

	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	if (theDirichletDomainIsPresentAndVisible)
	{
		glBufferData(GL_ARRAY_BUFFER,
						aDirichletDomain->itsDirichletNumMeshVertices * sizeof(DirichletVBOData),
						theVBOVertices,
						GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, 1, &theDummyByte, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);
	if (theDirichletDomainIsPresentAndVisible)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						3 * aDirichletDomain->itsDirichletNumMeshFaces * sizeof(unsigned short),
						theVBOIndices,
						GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1, &theDummyByte, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//	Free the temporary buffers.
	FREE_MEMORY_SAFELY(theVBOVertices);
	FREE_MEMORY_SAFELY(theVBOIndices );
	
	//	Did any OpenGL errors occur?
	//	(SetUpGraphicsAsNeeded() wants us to check.)
	return GetErrorString();
}


void MakeDirichletVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(DirichletVBOData), (void *)offsetof(DirichletVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(DirichletVBOData), (void *)offsetof(DirichletVBOData, tex));

			glEnableVertexAttribArray(ATTRIBUTE_COLOR);
			glVertexAttribPointer(ATTRIBUTE_COLOR,     4, GL_FLOAT, GL_FALSE, sizeof(DirichletVBOData), (void *)offsetof(DirichletVBOData, col));

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindDirichletVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawDirichletVAO(
	GLuint			aDirichletTexture,
	DirichletDomain	*aDirichletDomain,
	Honeycomb		*aHoneycomb,
	Matrix			*aWorldPlacement,	//	the world's placement in eye space
	double			aCurrentAperture)
{
	unsigned int	i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aDirichletDomain == NULL || aHoneycomb == NULL || aCurrentAperture == 1.0)
		return;

	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, aDirichletTexture);

	//	Front-to-back drawing minimizes overdraw and makes a huge difference
	//	when drawing the Dirichlet domain's walls.  For example, 
	//	an earlier version of Curved Spaces, using the old fixed-function code
	//	on my Radeon X1600, rendered a 3-torus (at lesser depth than in the
	//	current shader-based code) at 295 frames/second with front-to-back drawing
	//	but only 43 frames/second with back-to-front drawing.

	for (i = 0; i < aHoneycomb->itsNumVisibleCells; i++)
	{
		//	Each element of the tiling group defines
		//	a placement of the Dirichlet domain in world space.
		theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

		//	Let front faces wind counterclockwise (resp. clockwise)
		//	when the Dirichlet domain's placement in eye space preserves (resp. reverses) parity.
		glFrontFace(theDirichletPlacement->itsParity == aWorldPlacement->itsParity ? GL_CCW : GL_CW);

		//	Compose theDirichletPlacement with aWorldPlacement
		//	and send the result to the shader.
		Matrix44Product(theDirichletPlacement->m, aWorldPlacement->m, theModelViewMatrix);
		SendModelViewMatrixToShader(theModelViewMatrix);

		//	Draw.
		glDrawElements(	GL_TRIANGLES,
						3 * aDirichletDomain->itsDirichletNumMeshFaces,
						GL_UNSIGNED_SHORT,
						0);
	}
}


void MakeVertexFiguresVBO(
	GLuint			aVertexBufferName,
	GLuint			anIndexBufferName,
	DirichletDomain	*aDirichletDomain)
{
	VertexFiguresVBOData	*theVBOVertices	= NULL,
							*theVBOVertex;
	unsigned short			*theVBOIndices	= NULL,
							*theVBOIndex,
							theVBOVertexIndex;
	HEVertex				*theVertex;
	HEHalfEdge				*theHalfEdge;
	unsigned int			theCount;

	static const Byte		theDummyByte = 0x00;

	if (aDirichletDomain != NULL)
	{
		theVBOVertices	= (VertexFiguresVBOData *) GET_MEMORY(  aDirichletDomain->itsVertexFiguresNumMeshVertices * sizeof(VertexFiguresVBOData));
		theVBOIndices	= (   unsigned short *   ) GET_MEMORY( 3 * aDirichletDomain->itsVertexFiguresNumMeshFaces * sizeof(   unsigned short   ));
		GEOMETRY_GAMES_ASSERT(theVBOVertices != NULL && theVBOIndices  != NULL,
			"MakeVertexFiguresVAO() couldn't get memory to construct vertex data.");
		
		//	Keep a running pointer to the vertex currently under construction,
		//	and also keep track of its index in the array.  Keeping both these
		//	things is redundant, yet nevertheless convenient.
		theVBOVertex		= theVBOVertices;
		theVBOVertexIndex	= 0;
		
		//	Keep a running pointer to the current entry in the index buffer.
		theVBOIndex = theVBOIndices;

		//	Process each vertex in turn.
		for (	theVertex = aDirichletDomain->itsVertexList;
				theVertex != NULL;
				theVertex = theVertex->itsNext)
		{
			//	For a closed loop we'll want to process theVertex->itsOutboundHalfEdge twice,
			//	once at the beginning of the loop and then once again at the end.
			for (	theHalfEdge = theVertex->itsOutboundHalfEdge, theCount = 0;
					theHalfEdge != theVertex->itsOutboundHalfEdge->itsMate->itsCycle || theCount == 1;
					theHalfEdge	= theHalfEdge->itsMate->itsCycle, theCount++)
			{
				//	outer vertex
				theVBOVertex->pos[0] = (float) theHalfEdge->itsOuterPoint.v[0];
				theVBOVertex->pos[1] = (float) theHalfEdge->itsOuterPoint.v[1];
				theVBOVertex->pos[2] = (float) theHalfEdge->itsOuterPoint.v[2];
				theVBOVertex->pos[3] = (float) theHalfEdge->itsOuterPoint.v[3];
				theVBOVertex->tex[0] = ( (theCount & 0x00000001) ? 0.00f : 1.00f );
				theVBOVertex->tex[1] = 0.0f;
				theVBOVertex++;

				//	inner vertex
				theVBOVertex->pos[0] = (float) theHalfEdge->itsInnerPoint.v[0];
				theVBOVertex->pos[1] = (float) theHalfEdge->itsInnerPoint.v[1];
				theVBOVertex->pos[2] = (float) theHalfEdge->itsInnerPoint.v[2];
				theVBOVertex->pos[3] = (float) theHalfEdge->itsInnerPoint.v[3];
				theVBOVertex->tex[0] = ( (theCount & 0x00000001) ? 0.15f : 0.85f );
				theVBOVertex->tex[1] = 1.0;
				theVBOVertex++;
				
				//	Create a pair of triangles for every pair of vertices
				//	except the first one.
				if (theCount != 0)
				{
					*theVBOIndex++ = theVBOVertexIndex - 2;
					*theVBOIndex++ = theVBOVertexIndex - 1;
					*theVBOIndex++ = theVBOVertexIndex    ;

					*theVBOIndex++ = theVBOVertexIndex    ;
					*theVBOIndex++ = theVBOVertexIndex - 1;
					*theVBOIndex++ = theVBOVertexIndex + 1;
				}
				
				//	Update theVBOVertexIndex.
				theVBOVertexIndex += 2;
			}
		}

		//	Did we write the correct number of entries into the arrays?
		GEOMETRY_GAMES_ASSERT(
				(unsigned int)(theVBOVertex - theVBOVertices) == aDirichletDomain->itsVertexFiguresNumMeshVertices
			 && (unsigned int)(theVBOIndex  - theVBOIndices ) == 3 * aDirichletDomain->itsVertexFiguresNumMeshFaces
			 && theVBOVertexIndex == aDirichletDomain->itsVertexFiguresNumMeshVertices,
				"Wrong number of array entries written in MakeVertexFiguresVBO().");
	}

	//	Send the vertex figure data to the GPU.
	//
	//	If MakeVertexFiguresVAOs() gets called when no Dirichlet domain is present,
	//	provide dummy buffers so that glVertexAttribPointer() doesn't choke.
	//	Maybe it'd be OK with empty data, but why take chances?

	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	if (aDirichletDomain != NULL)
	{
		glBufferData(GL_ARRAY_BUFFER,
						aDirichletDomain->itsVertexFiguresNumMeshVertices * sizeof(VertexFiguresVBOData),
						theVBOVertices,
						GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, 1, &theDummyByte, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);
	if (aDirichletDomain != NULL)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						3 * aDirichletDomain->itsVertexFiguresNumMeshFaces * sizeof(unsigned short),
						theVBOIndices,
						GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1, &theDummyByte, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//	Free the temporary buffers.
	FREE_MEMORY_SAFELY(theVBOVertices);
	FREE_MEMORY_SAFELY(theVBOIndices );
}


void MakeVertexFiguresVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(VertexFiguresVBOData), (void *)offsetof(VertexFiguresVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFiguresVBOData), (void *)offsetof(VertexFiguresVBOData, tex));

			glDisableVertexAttribArray(ATTRIBUTE_COLOR);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindVertexFiguresVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawVertexFiguresVAO(
	GLuint			aVertexFigureTexture,
	DirichletDomain	*aDirichletDomain,
	Honeycomb		*aHoneycomb,
	Matrix			*aWorldPlacement)	//	the world's placement in eye space
{
	unsigned int	thePass,
					i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aDirichletDomain == NULL || aHoneycomb == NULL)
		return;
	
	//	Design Note:  The vertex figures could be rendered in a single pass
	//	by using the built-in fragment shader variable gl_FrontFacing
	//	to detect back faces and dim them.
	//	However, keeping the current two-pass algorithm seems simpler
	//	than cluttering up the shader with a backface test that would
	//	go unused for the Dirichlet domain and the Earth,
	//	and would be a hinderance for the galaxy.
	//	And providing separate shaders for each kind of primitive,
	//	while easy to do, would also introduce more clutter than I'd like.

	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, aVertexFigureTexture);

	//	Draw interior and exterior faces in separate passes.
	for (thePass = 0; thePass < 2; thePass++)
	{
		if (thePass == 0)	//	draw exterior faces with full brightness
		{
			glCullFace(GL_BACK);
			glVertexAttrib4fv(ATTRIBUTE_COLOR, (float [4]) PREMULTIPLY_RGBA(1.0, 1.0, 1.0, 1.0));
		}
		else				//	draw interior faces with 1/4 brightness
		{
			glCullFace(GL_FRONT);
			glVertexAttrib4fv(ATTRIBUTE_COLOR, (float [4]) PREMULTIPLY_RGBA(0.25, 0.25, 0.25, 1.0));
		}

		for (i = 0; i < aHoneycomb->itsNumVisibleCells; i++)
		{
			//	Each element of the tiling group defines
			//	a placement of the Dirichlet domain in world space.
			theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

			//	Let front faces wind counterclockwise (resp. clockwise)
			//	when the Dirichlet domain's placement in eye space preserves (resp. reverses) parity.
			glFrontFace(theDirichletPlacement->itsParity == aWorldPlacement->itsParity ? GL_CCW : GL_CW);

			//	Compose theDirichletPlacement with aWorldPlacement
			//	and send the result to the shader.
			Matrix44Product(theDirichletPlacement->m, aWorldPlacement->m, theModelViewMatrix);
			SendModelViewMatrixToShader(theModelViewMatrix);

			//	Draw.
			glDrawElements(	GL_TRIANGLES,
							3 * aDirichletDomain->itsVertexFiguresNumMeshFaces,
							GL_UNSIGNED_SHORT,
							0);
		}
	}

	//	Tidy up.
	glCullFace(GL_BACK);
}


void SortVisibleCells(
	Honeycomb	*aHoneycomb,
	Matrix		*aViewProjectionMatrix,	//	composition of current modelview and projection matrices
	Matrix		*aViewMatrix,			//	current modelview  matrix
	double		aDrawingRadius)
{
	unsigned int	i;

	if (aHoneycomb != NULL)
	{
		//	Count the number of visible cells.
		aHoneycomb->itsNumVisibleCells = 0;

		//	In the hyperbolic mirrored dodecahedron test case,
		//	the frame rate almost doubles (on Carla) when we test
		//	the distance before the visibility rather than
		//	the other way around.
		for (i = 0; i < aHoneycomb->itsNumCells; i++)
		{
			aHoneycomb->itsCells[i].itsDistance
				= CellCenterDistance(&aHoneycomb->itsCells[i], aViewMatrix);

			if (aHoneycomb->itsCells[i].itsDistance <= aDrawingRadius)
			{
				if (CellMayBeVisible(&aHoneycomb->itsCells[i], aViewProjectionMatrix))
					aHoneycomb->itsVisibleCells[aHoneycomb->itsNumVisibleCells++] = &aHoneycomb->itsCells[i];
			}
		}

		//	Sort the visible cells in increasing distance
		//	from the observer.  The cells should be roughly sorted
		//	to begin with (because they are sorted in order of
		//	increasing distance from the basepoint (0,0,0,1))
		//	so it makes little difference whether we use qsort()
		//	or a bubble sort.  The important thing is that we're sorting
		//	only the visible cells, not the whole honeycomb.
		qsort(	aHoneycomb->itsVisibleCells,
				aHoneycomb->itsNumVisibleCells,
				sizeof(Honeycell *),
				CompareCellCenterDistances);
	}
}


static double CellCenterDistance(
	Honeycell	*aCell,
	Matrix		*aViewMatrix)
{
	Vector	theCellCenter;

	VectorTimesMatrix(	&aCell->itsCenter,
						aViewMatrix,
						&theCellCenter);

	return  VectorGeometricDistance(&theCellCenter);
}


static bool CellMayBeVisible(
	Honeycell	*aCell,
	Matrix		*aViewProjectionMatrix)	//	composition of modelview and projection matrices
{
	bool			thePosClipExcludesAllVertices[3],
					theNegClipExcludesAllVertices[3],
					theVertexIsVisible;
	Vector			theProjectedVertex;
	unsigned int	i,
					j;

	//	Special case:  Treat a cell with no vertices,
	//	which occurs for the 3-sphere, as visible.  
	//	We'll need the 3-sphere to display Clifford parallels.
	//	(Confession:  This is a hack.  I hope it causes no trouble.)
	if (aCell->itsNumVertices == 0)
		return true;

	//	Generic case:

	for (j = 0; j < 3; j++)
	{
		thePosClipExcludesAllVertices[j] = true;
		theNegClipExcludesAllVertices[j] = true;
	}

	for (i = 0; i < aCell->itsNumVertices; i++)
	{
		VectorTimesMatrix(	&aCell->itsVertices[i],
							aViewProjectionMatrix,
							&theProjectedVertex);

		theVertexIsVisible = true;

		for (j = 0; j < 3; j++)
		{
			//	Be tolerant on the boundary,
			//	so that the z < -w or z > +w hyperplanes
			//	don't falsely exclude lens space images.

			if (theProjectedVertex.v[j] < -theProjectedVertex.v[3])
				theVertexIsVisible = false;
			else
				theNegClipExcludesAllVertices[j] = false;

			if (theProjectedVertex.v[j] > +theProjectedVertex.v[3])
				theVertexIsVisible = false;
			else
				thePosClipExcludesAllVertices[j] = false;
		}

		//	If the given vertex lies within the clipping box,
		//	the cell is definitely visible, so return true.
		if (theVertexIsVisible)
			return true;
	}

	//	If a single clipping plane excludes all vertices,
	//	the cell is definitely not visible, so return false.
	for (j = 0; j < 3; j++)
	{
		if (thePosClipExcludesAllVertices[j]
		 || theNegClipExcludesAllVertices[j])
			return false;
	}

	//	We don't know whether the cell is visible or not,
	//	so return true to be safe.
	return true;
}


static __cdecl signed int CompareCellCenterDistances(
	const void	*p1,
	const void	*p2)
{
	double	theDifference;

	theDifference = (*((Honeycell **) p1))->itsDistance
				  - (*((Honeycell **) p2))->itsDistance;

	if (theDifference < 0.0)
		return -1;

	if (theDifference > 0.0)
		return +1;

	return 0;
}
