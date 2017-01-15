//	CurvedSpacesTiling.c
//
//	Given a Dirichlet domain, construct a tiling.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "GeometryGamesUtilities-Common.h"
#include <math.h>
#include <stdlib.h>	//	for qsort()


//	If a generator equals its inverse,
//	it should do so to pretty much full hardware precision
//	(so even 1e-8 is a looser bound than is really needed).
#define GENERATOR_EPSILON	1e-8

//	Comparing matrices within a tiling is trickier,
//	especially in the hyperbolic case where substantial
//	numerical error may accumulate.  But even a value of 1e-3
//	would be loose enough to accommodate the numerical error
//	yet still tight enough to distinguish different matrices.
#define TILING_EPSILON		1e-5

//	How accurately do we know our sort key?
//	For sure large hyperbolic tilings will be the worst case.
//	Happily, tests on a mirrored dodecahedron found sort key errors
//	running from ~10⁻¹⁴ at a tiling radius of 4.0,
//	to ~10⁻¹³ at a tiling radius of 6.0.
//	Of course other hyperbolic manifolds, with less precise
//	initial generators (and few zeros in the matrices!) will fare worse.
#define SORT_KEY_EPSILON	1e-8

//	The SORT_KEY_W_DEPENDENCE should be weak enough that it doesn't
//	disturb the symmetrical distribution of values on flat and hyperbolic
//	spaces, yet strong enough to reliably distinguish spherical points
//	differing only in w.
//	All these epsilons remind me why exact arithmetic is so appealing.  ☺
//	Yet exact arithmetic is surely too slow for present purposes.  ☹
#define SORT_KEY_W_DEPENDENCE	1e-4

//	For testing whether the antipodal matrix is present,
//	any reasonable value for ANTIPODAL_EPSILON will do.
#define ANTIPODAL_EPSILON		1e-8

//	__cdecl isn't defined or needed on MacOS X,
//	so make it disappear from our callback function prototypes.
#ifndef __cdecl
#define __cdecl
#endif


typedef struct Tile
{
	//	The matrix itself
	Matrix		itsMatrix;

	//	How far does it translate the origin (0,0,0,1) ?
	double		itsTranslationDistance;

	//	Support for the binary tree used during construction
	double		itsSortKey;
	struct Tile	*itsLeftChild,
				*itsRightChild;

	//	Support for the queue used during construction
	struct Tile	*itsQueueNext;

} Tile;


//	The TilingInProgress serves to group some variable together,
//	more for conceptual clarity than any profound algorithm reason.
typedef struct
{
	//	How many tiles do we have?
	unsigned int	itsNumTiles;

	//	Keep the Tiles on a binary tree during construction.
	//	To keep the tree balanced, the sort key should not correlate
	//	with the structure of the group.  When the construction
	//	is complete, destroy the tree and set itsTree to NULL.
	Tile			*itsTree;

	//	Keep Tiles waiting to be processed on a singly-linked list,
	//	as well as on the binary tree.  When the construction
	//	is complete, destroy the list and set itsQueueFirst and
	//	itsQueueLast to NULL.
	Tile			*itsQueueFirst,
					*itsQueueLast;

} TilingInProgress;


static void					FreeTree(Tile **aTree);
static double				MakeSortKey(Matrix *aMatrix);
static ErrorText			AddToTiling(TilingInProgress *aTiling, Matrix *aMatrix, double aSortKey, double aTranslationDistance);
static Tile					*GetTileFromQueue(TilingInProgress *aTiling);
static double				TranslationDistance(Matrix *aMatrix);
static bool					TreeContainsMatrix(Tile *aTree, Matrix *aMatrix, double aSortKey);
static void					CopyPointersToArray(Tile *aTree, Tile ***anArray);
static __cdecl signed int	CompareTranslationDistances(const void *p1, const void *p2);


ErrorText ConstructHolonomyGroup(
	MatrixList		*aGeneratorList,
	double			aTilingRadius,
	MatrixList		**aHolonomyGroup)			//	output
{
	ErrorText			theErrorMessage				= NULL;
	MatrixList			*theExtendedGeneratorList	= NULL;
	TilingInProgress	theTilingInProgress;
	Matrix				theIdentityMatrix,
						theInverse;
	Tile				*theTile;
	Matrix				theCandidate;
	double				theTranslationDistance,
						theSortKey;
	Tile				**theTileArray				= NULL,
						**theWriteLocation;
	unsigned int		i;

	if (*aHolonomyGroup != NULL)
		return u"ConstructHolonomyGroup() received a non-NULL output location.";

	//	Extend the list of generators to include explicit inverses.
	//
	//	Warning:  Error checking here is minimal.
	//	We assume the input generators do not contain
	//	the identity matrix and do not contain explicit inverses,
	//	and of course as always we assume they belong to one
	//	of O(4), Isom(E³) or O(3,1) and define a discrete group
	//	that doesn't fix the origin.

	//		Allow space for twice as many matrices as we were given.
	theExtendedGeneratorList = AllocateMatrixList( 2 * aGeneratorList->itsNumMatrices );
	if (theExtendedGeneratorList == NULL)
	{
		theErrorMessage = u"Couldn't get memory for theExtendedGeneratorList in ConstructHolonomyGroup().";
		goto CleanUpConstructHolonomyGroup;
	}

	//		Copy the generators and their inverses (when distinct)
	//		and count them as we go along.
	theExtendedGeneratorList->itsNumMatrices = 0;
	for (i = 0; i < aGeneratorList->itsNumMatrices; i++)
	{
		//	Always add the generator itself.
		theExtendedGeneratorList->itsMatrices[theExtendedGeneratorList->itsNumMatrices++]
			= aGeneratorList->itsMatrices[i];

		//	Add the generator's inverse iff it's distinct.
		MatrixGeometricInverse(&aGeneratorList->itsMatrices[i], &theInverse);
		if ( ! MatrixEquality(&aGeneratorList->itsMatrices[i], &theInverse, GENERATOR_EPSILON) )
			theExtendedGeneratorList->itsMatrices[theExtendedGeneratorList->itsNumMatrices++]
				= theInverse;
	}

	//	Initialize theTilingInProgress to an empty tiling.
	theTilingInProgress.itsNumTiles		= 0;
	theTilingInProgress.itsTree			= NULL;
	theTilingInProgress.itsQueueFirst	= NULL;
	theTilingInProgress.itsQueueLast	= NULL;

	//	Add the identity matrix to the tiling.
	MatrixIdentity(&theIdentityMatrix);
	theErrorMessage = AddToTiling(	&theTilingInProgress,
									&theIdentityMatrix,
									MakeSortKey(&theIdentityMatrix),
									0.0);
	if (theErrorMessage != NULL)
		goto CleanUpConstructHolonomyGroup;

	//	Process the queue.
	while ((theTile = GetTileFromQueue(&theTilingInProgress)) != NULL)
	{
		//	For each generator...
		for (i = 0; i < theExtendedGeneratorList->itsNumMatrices; i++)
		{
			//	...pre-multiplying (not post-multiplying!) a matrix
			//	by a generator yields a neighbor of the given tile.
			MatrixProduct(	&theExtendedGeneratorList->itsMatrices[i],
							&theTile->itsMatrix,
							&theCandidate);

			//	Note the candidate's translation distance.
			theTranslationDistance = TranslationDistance(&theCandidate);

			//	Reject candidates that translate too far.
			if (theTranslationDistance > aTilingRadius)
				continue;

			//	Note the candidate's sort key.
			theSortKey = MakeSortKey(&theCandidate);

			//	Reject candidates already found earlier.
			if (TreeContainsMatrix(theTilingInProgress.itsTree, &theCandidate, theSortKey))
				continue;

			//	Add the candidate to the tiling.
			theErrorMessage = AddToTiling(	&theTilingInProgress,
											&theCandidate,
											theSortKey,
											theTranslationDistance);
			if (theErrorMessage != NULL)
				goto CleanUpConstructHolonomyGroup;
		}
	}

	//	Sort the Tiles.
	//
	//	Note:  We'll sort the tiles again at render time,
	//	but I'm leaving this sort in place here as well,
	//	for future flexibility and robustness.
	//	In effect, this sort sorts relative to the original
	//	distances within the tiling, while the render-time sort
	//	sorts relative to the distance to the observer.

	//		Allocate an array to hold pointers to the Tiles.
	theTileArray = (Tile **) GET_MEMORY(theTilingInProgress.itsNumTiles * sizeof(Tile *));
	if (theTileArray == NULL)
	{
		theErrorMessage = u"Can't get memory to sort Tiles in ConstructHolonomyGroup().";
		goto CleanUpConstructHolonomyGroup;
	}

	//		Copy the pointers from the tree structure recursively,
	//		advancing the destination pointer theTile as we go.
	//		Afterwards, make sure we wrote exactly the right number of tiles.
	theWriteLocation = theTileArray;
	CopyPointersToArray(theTilingInProgress.itsTree, &theWriteLocation);
	if (theWriteLocation != theTileArray + theTilingInProgress.itsNumTiles)
	{
		theErrorMessage = u"Grave error while copying Tile addresses in ConstructHolonomyGroup().";
		goto CleanUpConstructHolonomyGroup;
	}

	//		Call the standard library's QuickSort implementation
	//		to sort the pointers.
	qsort(	theTileArray,
			theTilingInProgress.itsNumTiles,
			sizeof(Tile *),
			CompareTranslationDistances);

	//	Copy the matrices to aHolonomyGroup (our final output variable!).
	*aHolonomyGroup = AllocateMatrixList(theTilingInProgress.itsNumTiles);
	if (*aHolonomyGroup == NULL)
	{
		theErrorMessage = u"Couldn't get memory for aHolonomyGroup in ConstructHolonomyGroup().";
		goto CleanUpConstructHolonomyGroup;
	}
	for (i = 0; i < theTilingInProgress.itsNumTiles; i++)
		(*aHolonomyGroup)->itsMatrices[i] = theTileArray[i]->itsMatrix;

CleanUpConstructHolonomyGroup:

	//	As the code is now written, *aHolonomyGroup will be NULL
	//	until the last moment.  But best to leave this check in place
	//	in case I modify the code in the future.
	if (theErrorMessage != NULL)
		FreeMatrixList(aHolonomyGroup);

	//	The Tiles get stored on three independent data structures:
	//	the tree, the queue and the array.  Let's free the Tiles
	//	as part of the tree, because each Tile goes onto the tree
	//	immediately upon its creation and remains there thereafter.
	//	(By contrast, Tiles get taken off the queue, and the array
	//	doesn't even exist until late in the algorithm.)
	FreeTree(&theTilingInProgress.itsTree);
	theTilingInProgress.itsQueueFirst	= NULL;
	theTilingInProgress.itsQueueLast	= NULL;
	FREE_MEMORY_SAFELY(theTileArray);

	//	Free the extended generator list.
	FreeMatrixList(&theExtendedGeneratorList);

	return theErrorMessage;
}


static void FreeTree(Tile **aTree)
{
	if (*aTree != NULL)
	{
		FreeTree(&(*aTree)->itsLeftChild );
		FreeTree(&(*aTree)->itsRightChild);
		FREE_MEMORY_SAFELY(*aTree);
	}
}


static double MakeSortKey(Matrix *aMatrix)
{
	//	To store the tiles efficiently on a binary tree,
	//	we need a sort key that is likely to take distinct values
	//	for distinct matrices.  I don't see any way to achieve
	//	this goal with complete rigor (one can't map R³ into R
	//	injectively), but we should obtain fairly good results
	//	by taking the image of the origin (0,0,0,1) and projecting
	//	it onto an arbitrarily chosen axis.  If the chosen axis
	//	doesn't align with the group in any way, the sort keys
	//	should all be distinct.
	//
	//	A further benefit of this scheme is that the binary tree's
	//	root node, which will contain the identity matrix, will get
	//	a value of zero, exactly in the middle of the distribution!
	//	From there, one expects the remaining nodes to fill out
	//	a more-or-less balanced tree.
	//
	//	Uh-oh.  I forgot that in the spherical case different group
	//	elements will sit "one above the other", meaning their images
	//	of the origin differ only in the sign of the w-cooordinate.
	//	So let's throw in a weak dependence on w to resolve such
	//	degeneracies without making the sort key distribution
	//	too asymetrical in the flat and hyperbolic cases.
	//
	//	Of course, if one were using exact arithmetic then more robust
	//	schemes would be possible.

	static const double	theArbitraryAxis[4] =
						{
							0.16790445172382044311,
							0.31996944449851782048,
							0.93243104285444785608,
							SORT_KEY_W_DEPENDENCE
						};

	return theArbitraryAxis[0] * aMatrix->m[3][0]
		 + theArbitraryAxis[1] * aMatrix->m[3][1]
		 + theArbitraryAxis[2] * aMatrix->m[3][2]
		 + theArbitraryAxis[3] * aMatrix->m[3][3];
}


static ErrorText AddToTiling(
	TilingInProgress	*aTiling,
	Matrix				*aMatrix,
	double				aSortKey,
	double				aTranslationDistance)
{
	Tile	*theNewTile,
			**theNewLocation;

	//	Allocate a Tile.
	theNewTile = (Tile *) GET_MEMORY(sizeof(Tile));
	if (theNewTile == NULL)
		return u"Out of memory in AddToTiling().";

	//	Copy the basic data.
	theNewTile->itsMatrix				= *aMatrix;
	theNewTile->itsSortKey				= aSortKey;
	theNewTile->itsTranslationDistance	= aTranslationDistance;

	//	Add theNewTile to the tree.

	theNewTile->itsLeftChild	= NULL;
	theNewTile->itsRightChild	= NULL;

	theNewLocation = &aTiling->itsTree;
	while (*theNewLocation != NULL)
	{
		if (aSortKey < (*theNewLocation)->itsSortKey)
			theNewLocation = &(*theNewLocation)->itsLeftChild;
		else
			theNewLocation = &(*theNewLocation)->itsRightChild;
	}
	*theNewLocation = theNewTile;

	//	Put theNewTile onto the end of the to-be-processed queue.
	if (aTiling->itsQueueLast != NULL)
	{
		aTiling->itsQueueLast->itsQueueNext = theNewTile;
	}
	else
	{
		aTiling->itsQueueFirst = theNewTile;
	}
	theNewTile->itsQueueNext	= NULL;
	aTiling->itsQueueLast		= theNewTile;

	//	Update the tile count.
	aTiling->itsNumTiles++;

	return NULL;
}


static Tile *GetTileFromQueue(TilingInProgress *aTiling)
{
	//	Remove the first Tile from the to-be-processed queue,
	//	but do *not* remove it from the permanent data structure.

	Tile	*theTile;

	theTile = aTiling->itsQueueFirst;

	if (theTile != NULL)
	{
		aTiling->itsQueueFirst = theTile->itsQueueNext;
		if (aTiling->itsQueueFirst == NULL)
			aTiling->itsQueueLast = NULL;
		theTile->itsQueueNext = NULL;
	}

	return theTile;
}


static double TranslationDistance(Matrix *aMatrix)
{
	//	Spherical case O(4)
	if (aMatrix->m[3][3] <  1.0)
		return SafeAcos(aMatrix->m[3][3]);

	//	Flat case Isom(E³)
	//	(Would also work for elements of O(4) and O(3,1) that fix the origin,
	//	even though Curved Spaces allows no such elements except the identity.)
	if (aMatrix->m[3][3] == 1.0)
		return sqrt(aMatrix->m[3][0] * aMatrix->m[3][0]
				  + aMatrix->m[3][1] * aMatrix->m[3][1]
				  + aMatrix->m[3][2] * aMatrix->m[3][2]);

	//	Hyperbolic case O(3,1)
	if (aMatrix->m[3][3] >  1.0)
		return SafeAcosh(aMatrix->m[3][3]);

	return 0.0;	//	suppress compiler warnings
}


static bool TreeContainsMatrix(
	Tile	*aTree,
	Matrix	*aMatrix,
	double	aSortKey)
{
	Tile	*theTile;

	//	Does the given matrix already appear on the tree?
	//	This seemingly simple tree search is complicated by the fact
	//	that we know the sort key values only up to some numerical error,
	//	which may be substantial in the hyperbolic case.

	theTile = aTree;
	while (theTile != NULL)
	{
		if (aSortKey < theTile->itsSortKey - SORT_KEY_EPSILON)
			theTile = theTile->itsLeftChild;
		else
		if (aSortKey > theTile->itsSortKey + SORT_KEY_EPSILON)
			theTile = theTile->itsRightChild;
		else
		{
			//	The sort keys match to within SORT_KEY_EPSILON.

			//	If we've found the desired matrix, we're done.
			if (MatrixEquality(	&theTile->itsMatrix,
								aMatrix,
								TILING_EPSILON))
				return true;

			//	Otherwise the desired matrix could be on the left subtree,
			//	on the right subtree, or not on the tree at all.
			//	In practice the algorithm never reaches this point
			//	(at least not for the Poincaré dodecahedral space,
			//	the cubic 3-torus or a radius=6 hyperbolic tiling
			//	by mirrored dodecahedra), but it's good to have
			//	this code in place in case it's ever needed.
			//	One wouldn't want to call it often, though, because
			//	the forking could easily slow an otherwise fast search
			//	to a near halt.
			return TreeContainsMatrix(theTile->itsLeftChild,  aMatrix, aSortKey)
				|| TreeContainsMatrix(theTile->itsRightChild, aMatrix, aSortKey);
		}
	}

	return false;
}


static void CopyPointersToArray(
	Tile	*aTree,		//	read from here
	Tile	***anArray)	//	write to here
{
	if (aTree != NULL)
	{
		*(*anArray)++ = aTree;
		CopyPointersToArray(aTree->itsLeftChild,  anArray);
		CopyPointersToArray(aTree->itsRightChild, anArray);
	}
}


static __cdecl signed int CompareTranslationDistances(
	const void	*p1,
	const void	*p2)
{
	double	theDifference;

	theDifference = (*((Tile **) p1))->itsTranslationDistance
				  - (*((Tile **) p2))->itsTranslationDistance;

	if (theDifference < 0.0)
		return -1;

	if (theDifference > 0.0)
		return +1;

	return 0;
}


ErrorText NeedsBackHemisphere(
	MatrixList	*aHolonomyGroup,			//	input
	SpaceType	aSpaceType,					//	input
	bool		*aDrawBackHemisphereFlag)	//	output
{
	unsigned int	i;

	//	Test for missing input.
	if (aHolonomyGroup == NULL)
		return u"NULL matrix list passed to NeedsBackHemisphere().";

	if (aSpaceType == SpaceSpherical)
	{
		//	If the antipodal matrix is present the scenery will be
		//	antipodally symmetric and there'll be no need to draw
		//	the back hemisphere.

		*aDrawBackHemisphereFlag = true;

		for (i = 0; i < aHolonomyGroup->itsNumMatrices; i++)
			if (fabs(aHolonomyGroup->itsMatrices[i].m[3][3] - (-1.0)) < ANTIPODAL_EPSILON)
				*aDrawBackHemisphereFlag = false;
	}
	else
	{
		//	Flat and hyperbolic spaces never need to draw a "back hemisphere".
		*aDrawBackHemisphereFlag = false;
	}

	return NULL;
}
