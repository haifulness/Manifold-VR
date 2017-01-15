//	CurvedSpacesFileIO.c
//
//	Accept files in either UTF-8 or Latin-1,
//	subject to the condition that non-ASCII characters
//	may appear only in comments.  In other words, assume
//	the matrix entries are written using plain 7-bit ASCII only.
//	If using UTF-8, allow but do not require a byte-order-mark.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#ifdef HIGH_RESOLUTION_SCREENSHOT
#include <math.h>
#endif

//	A quick-and-dirty hack tiles the mirrored dodecahedron
//	and the Seifert-Weber space (which have relatively large volumes)
//	more deeply than the smaller-volume hyperbolic spaces.
//	A more robust algorithm would examine the size
//	of the fundamental domain.
typedef enum
{
	HyperbolicSpaceGeneric,
	HyperbolicSpaceMirroredDodecahedron,
	HyperbolicSpaceSeifertWeber
} HyperbolicSpaceType;


static bool			StringBeginsWith(Byte *anInputText, Byte *aPossibleBeginning);
static void			RemoveComments(Byte *anInputText);
static ErrorText	ReadMatrices(Byte *anInputText, MatrixList **aMatrixList);
static bool			ReadOneNumber(Byte *aString, double *aValue, Byte **aStoppingPoint, ErrorText *anError);
static ErrorText	LoadGenerators(ModelData *md, MatrixList *aGeneratorList, HyperbolicSpaceType aHyperbolicSpaceType);
static ErrorText	DetectSpaceType(MatrixList *aGeneratorList, SpaceType *aSpaceType);


ErrorText LoadGeneratorFile(
	ModelData	*md,
	Byte		*anInputText)	//	zero-terminated, and hopefully UTF-8 or Latin-1
{
	ErrorText			theErrorMessage	= NULL;
	HyperbolicSpaceType	theHyperbolicSpaceType;
	MatrixList			*theGenerators	= NULL;

	//	Make sure we didn't get UTF-16 data by mistake.
	if ((anInputText[0] == 0xFF && anInputText[1] == 0xFE)
	 || (anInputText[0] == 0xFE && anInputText[1] == 0xFF))
	{
		theErrorMessage = u"The matrix file is in UTF-16 format.  Please convert to UTF-8.";
		goto CleanUpLoadGeneratorFile;
	}
	
	//	If a UTF-8 byte-order-mark is present, skip over it.
	if (anInputText[0] == 0xEF && anInputText[1] == 0xBB && anInputText[2] == 0xBF)
		anInputText += 3;

	//	As special cases, check whether anInputText begins with
	//
	//		#	Mirrored Right-Angled Dodecahedron
	//	or
	//		#	Seifert-Weber Dodecahedral Space
	//
	if (StringBeginsWith(anInputText, (Byte *)"#	Mirrored Right-Angled Dodecahedron"))
		theHyperbolicSpaceType = HyperbolicSpaceMirroredDodecahedron;
	else
	if (StringBeginsWith(anInputText, (Byte *)"#	Seifert-Weber Dodecahedral Space"))
		theHyperbolicSpaceType = HyperbolicSpaceSeifertWeber;
	else
		theHyperbolicSpaceType = HyperbolicSpaceGeneric;

	//	Remove comments.
	//	What remains should be plain 7-bit ASCII (common to both UTF-8 and Latin-1).
	RemoveComments(anInputText);

	//	Parse the input text into 4×4 matrices.
	theErrorMessage = ReadMatrices(anInputText, &theGenerators);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGeneratorFile;
		
	//	Load theGenerators.
	theErrorMessage = LoadGenerators(md, theGenerators, theHyperbolicSpaceType);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGeneratorFile;

CleanUpLoadGeneratorFile:

	FreeMatrixList(&theGenerators);

	return theErrorMessage;
}

static bool StringBeginsWith(
	Byte	*anInputText,			//	zero-terminated, UTF-8 or Latin-1
	Byte	*aPossibleBeginning)	//	zero-terminated, UTF-8 or Latin-1
{
	Byte	*a,
			*b;
	
	a = anInputText;
	b = aPossibleBeginning;
	
	while (*b != 0)
	{
		if (*a++ != *b++)
			return false;
	}
	
	return true;
}

static void RemoveComments(
	Byte *anInputText)	//	Zero-terminated input string also serves for output.
{
	//	Remove comments in place.
	//	A comment begins with a '#' character and runs to the end of the line,
	//	which may be marked by '\r' or '\n' or both.
	//	The input text must be zero-terminated.
	//	Both UTF-8 and Latin-1 are acceptable.

	Byte	*r,	//	read  location
			*w;	//	write location

	r = anInputText;
	w = anInputText;

	do
	{
		if (*r == '#')
		{
			do
			{
				r++;
			} while (*r != '\r' && *r != '\n' && *r != 0);
		}
	} while ((*w++ = *r++) != 0);
}


static ErrorText ReadMatrices(
	Byte			*anInputText,	//	zero-terminated comment-free input string
	MatrixList		**aMatrixList)
{
	ErrorText		theErrorMessage		= NULL;
	unsigned int	theNumNumbers		= 0,
					theNumGenerators	= 0;
	Byte			*theMarker			= NULL;
	unsigned int	i,
					j,
					k;

	//	Check the input parameters.
	if (*aMatrixList != NULL)
		return u"ReadMatrices() was passed a non-NULL output pointer.";

	//	Count the number of numbers in anInputText.
	theNumNumbers	= 0;
	theMarker		= anInputText;
	while (ReadOneNumber(theMarker, NULL, &theMarker, &theErrorMessage))
	{
		theNumNumbers++;
	}
	if (theErrorMessage != NULL)
		goto CleanUpReadMatrices;

	//	If anInputText contains a set of 4×4 matrices,
	//	the number of numbers should be a positive multiple of 16.
	if (theNumNumbers % 16 != 0)
	{
		theErrorMessage = u"A matrix generator file should contain a list of 4×4 matrices and nothing else.\nUnfortunately the number of entries in the present file is not a multiple of 16.";
		goto CleanUpReadMatrices;
	}

	//	How many generators?
	theNumGenerators = theNumNumbers / 16;

	//	Allocate space for the matrices.
	*aMatrixList = AllocateMatrixList(theNumGenerators);
	if (*aMatrixList == NULL)
	{
		theErrorMessage = u"Couldn't allocate memory for matrix generators.";
		goto CleanUpReadMatrices;
	}

	//	Reread the string, writing the numbers directly into the matrices,
	//	and then compute the determinant to determine the parity.
	theMarker = anInputText;
	for (i = 0; i < theNumGenerators; i++)
	{
		for (j = 0; j < 4; j++)
			for (k = 0; k < 4; k++)
				if ( ! ReadOneNumber(theMarker, &(*aMatrixList)->itsMatrices[i].m[j][k], &theMarker, NULL) )
				{
					theErrorMessage = u"\"Impossible\" error in ReadMatrices().";
					goto CleanUpReadMatrices;
				}
		(*aMatrixList)->itsMatrices[i].itsParity =
			(MatrixDeterminant(&(*aMatrixList)->itsMatrices[i]) > 0.0) ?
			ImagePositive : ImageNegative;
	}

CleanUpReadMatrices:

	if (theErrorMessage != NULL)
		FreeMatrixList(aMatrixList);

	return theErrorMessage;
}


static bool ReadOneNumber(
	Byte		*aString,			//	input, null-terminated string
	double		*aValue,			//	output, may be null
	Byte		**aStoppingPoint,	//	output, may be null, *aStoppingPoint may equal aString
	ErrorText	*anError)			//	output, may be null
{
	char		*theStartingPoint	= NULL,
				*theStoppingPoint	= NULL;
	double		theNumber			= 0.0;

	theStartingPoint = (char *) aString;

	//	The strtod() documentation defines whitespace as spaces and tabs only.
	//	In practice strtod() also skips over newlines, but one hates
	//	to rely on undocumented behavior, so skip over all whitespace
	//	before calling strtod().
	while (*theStartingPoint == ' '
		|| *theStartingPoint == '\t'
		|| *theStartingPoint == '\r'
		|| *theStartingPoint == '\n')
	{
		theStartingPoint++;
	}

	//	Try to read a number.
	theNumber = strtod(theStartingPoint, &theStoppingPoint);

	//	Did we get one?
	//
	//	Note:  When strtod() fails it returns 0.0, making it awkward to distinguish
	//	failure from a successfully read 0.0.   The documentation doesn't say exactly
	//	what to expect from errno when no conversion can be done.
	//	The practical (and safe) approach seems to be to check whether
	//	theStoppingPoint differs from the theStartingPoint.

	if (theStoppingPoint != theStartingPoint)	//	success
	{
		if (aValue != NULL)
			*aValue = theNumber;

		if (aStoppingPoint != NULL)
			*aStoppingPoint = (Byte *) theStoppingPoint;

		if (anError != NULL)
			*anError = NULL;

		return true;
	}
	else										//	failure
	{
		if (aValue != NULL)
			*aValue = 0.0;

		if (aStoppingPoint != NULL)
			*aStoppingPoint = aString;

		//	The only valid reason not to get a number is reaching the end of the string.
		if (anError != NULL)
			*anError = (*theStoppingPoint != 0) ?
				u"Matrix file contains text other than numbers." :	//	bad characters
				NULL;												//	no error

		return false;
	}
}


static ErrorText LoadGenerators(
	ModelData			*md,
	MatrixList			*aGeneratorList,
	HyperbolicSpaceType	aHyperbolicSpaceType)
{
	ErrorText	theErrorMessage		= NULL;
	MatrixList	*theHolonomyGroup	= NULL;
#ifdef HIGH_RESOLUTION_SCREENSHOT
	Matrix		theRotation,
				theTranslation;
#endif

	//	Delete any pre-existing Dirichlet domain and honeycomb,
	//	reset the user's placement and speed, and reset the centerpiece.
	md->itsSpaceType = SpaceNone;
	FreeDirichletDomain(&md->itsDirichletDomain);
	FreeHoneycomb(&md->itsHoneycomb);
	MatrixIdentity(&md->itsUserPlacement);
	md->itsUserSpeed = USER_SPEED_INCREMENT;	//	slow forward motion
#ifdef CENTERPIECE_DISPLACEMENT
	MatrixIdentity(&md->itsCenterpiecePlacement);
#endif

	//	Detect the new geometry and make sure it's consistent.
	theErrorMessage = DetectSpaceType(aGeneratorList, &md->itsSpaceType);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGenerators;
	
	//	Set itsTilingRadius and itsDrawingRadius according to the SpaceType.
	//
	//	A more sophisticated approach would take into account
	//	the translation distances of the generators (assuming
	//	the generators have been efficiently chosen) to tile 
	//	more/less deeply when the fundamental domain is likely 
	//	to be large/small, but the present code doesn't do that.
	switch (md->itsSpaceType)
	{
		case SpaceSpherical:
			//	Any value greater than π will suffice to tile all of S³.
			md->itsTilingRadius		= 3.15;
			md->itsDrawingRadius	= 3.15;
			break;

		case SpaceFlat:
#if defined(START_STILL) || defined(CENTERPIECE_DISPLACEMENT) || defined(START_OUTSIDE)
#warning If I ever run this on a newer computer, I can probably use the deeper tilings.
			//	The deeper tilings run plenty smoothly on my 2008 MacBook
			//	when Curved Spaces runs alone on the machine.  But when
			//	Safari and Torus Games are running simultaneously with it,
			//	the Curved Spaces animation sometimes gets a little jerky,
			//	and I hear what may be fan noises.
			md->itsTilingRadius		=  8.0;
			md->itsDrawingRadius	=  7.5;
#else
			//	The number of tiles grows cubicly with the radius,
			//	so we can afford to tile deeper in the flat case
			//	than in the hyperbolic case.
#if defined(__WIN32__) || defined(__MAC_OS_X_VERSION_MIN_REQUIRED)	//	desktop
			md->itsTilingRadius		= 12.0;
			md->itsDrawingRadius	= 11.5;
#else																//	mobile
			md->itsTilingRadius		=  8.0;
			md->itsDrawingRadius	=  7.5;
#endif

#endif	//	for talks or for public release

			break;

		case SpaceHyperbolic:
			//	The number of tiles grows exponentially with the radius,
			//	so we can't tile too deep in the hyperbolic case.

#ifdef HIGH_RESOLUTION_SCREENSHOT

			UNUSED_PARAMETER(aHyperbolicSpaceType);

			//	For a static screenshot, speed isn't an issue,
			//	and neither is popping.
			md->itsTilingRadius		= 6.5;
			md->itsDrawingRadius	= 9.0;

#else	//	normal resolution

#if defined(__WIN32__) || defined(__MAC_OS_X_VERSION_MIN_REQUIRED)	//	desktop
			if (aHyperbolicSpaceType != HyperbolicSpaceGeneric)
			{
				//	Tile deeper for larger spaces like the mirrored dodecahedron 
				//	or the Seifert-Weber space.  Setting
				//
				//		md->itsTilingRadius		= 6.5;
				//		md->itsDrawingRadius	= 6.0;
				//
				//	looks best, but it's still a little slow
				//	on integrated graphics from 2008.
				//	In a few more years I can use those radii.
				//	For now be satisfied with less impressive,
				//	but less demanding, radii.
				md->itsTilingRadius		= 5.5;
				md->itsDrawingRadius	= 5.0;
			}
			else
			{
				//	Tile less deep for other hyperbolic spaces,
				//	typically the lowest-volume ones.
				md->itsTilingRadius		= 4.5;
				md->itsDrawingRadius	= 4.0;
			}
#else																//	mobile
			//	On older iOS or Android, use less demanding radii
			//	for all hyperbolic spaces.
			md->itsTilingRadius		= 4.5;
			md->itsDrawingRadius	= 4.0;
#endif	//	desktop or mobile

#endif	//	HIGH_RESOLUTION_SCREENSHOT or not

			break;
		
		default:
			md->itsTilingRadius		= 0.0;
			md->itsDrawingRadius	= 0.0;
			break;
	}

	//	Use the generators to construct the holonomy group
	//	out to the desired tiling radius.
	//	Assume the group is discrete and no element fixes the origin.
	theErrorMessage = ConstructHolonomyGroup(	aGeneratorList,
												md->itsTilingRadius,
												&theHolonomyGroup);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGenerators;

	//	In the case of a spherical space, we'll want to draw the back hemisphere
	//	if and only if the holonomy group does not contain the antipodal matrix.
	theErrorMessage = NeedsBackHemisphere(theHolonomyGroup, md->itsSpaceType, &md->itsDrawBackHemisphere);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGenerators;
	
	//	The space is a 3-sphere iff theHolonomyGroup 
	//	contains the identity matrix alone.
	md->itsThreeSphereFlag = (theHolonomyGroup->itsNumMatrices == 1);

	//	Use the holonomy group to construct a Dirichlet domain.
	theErrorMessage = ConstructDirichletDomain(	theHolonomyGroup,
												&md->itsDirichletDomain);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGenerators;

	//	Use the holonomy group and the Dirichlet domain
	//	to construct a honeycomb.
	theErrorMessage = ConstructHoneycomb(	theHolonomyGroup,
											md->itsDirichletDomain,
											&md->itsHoneycomb);
	if (theErrorMessage != NULL)
		goto CleanUpLoadGenerators;

#ifdef CENTERPIECE_DISPLACEMENT
	//	For ad hoc convenience in the Shape of Space lecture,
	//	move the user back a bit, move the centerpiece forward a bit,
	//	and set the speed to zero.
	//	This will look good when the fundamental domain is a unit cube.
	//
	//	Technical note:  When the aperture is closed and 
	//	only the central Dirichlet domain is drawn, it's crucial that 
	//	we place the user at -1/2 + ε rather that at -1/2, so the user
	//	doesn't land at +1/2 instead.  Also, we want to have at least
	//	a near clipping distance's margin between the user and the back wall,
	//	in case s/he turns around!
	MatrixTranslation(&md->itsUserPlacement, md->itsSpaceType, 0.0, 0.0, -0.49);
	MatrixTranslation(&md->itsCenterpiecePlacement, md->itsSpaceType, 0.0, 0.0, 0.25);
	md->itsUserSpeed = 0.0;
#endif
#ifdef START_STILL
	//	For ad hoc convenience in the Shape of Space lecture,
	//	move the user back a bit, move the centerpiece forward a bit,
	//	and set the speed to zero.
	MatrixTranslation(&md->itsUserPlacement, md->itsSpaceType, 0.0, 0.0, -0.3);
	md->itsUserSpeed = 0.0;
#endif
#ifdef HIGH_RESOLUTION_SCREENSHOT
#if 1
#warning ad hoc placement for viewing mirrored dodecahedron
	MatrixRotation(&theRotation, 0.0, SafeAcos(cos(PI/3)/sin(PI/5)), 0.0);
	MatrixTranslation(&theTranslation, md->itsSpaceType, 0.0, 0.0, -0.125);
	//	Ultimately theViewMatrix will be the inverse of itsUserPlacement,
	//	so we must multiply the factors here in a possibly unexpected order.
	MatrixProduct(&theTranslation, &theRotation, &md->itsUserPlacement);
#endif	//	ad hoc placement
	md->itsUserSpeed = 0.0;
#endif	//	HIGH_RESOLUTION_SCREENSHOT

CleanUpLoadGenerators:

	if (theErrorMessage != NULL)
	{
		FreeDirichletDomain(&md->itsDirichletDomain);
		FreeHoneycomb(&md->itsHoneycomb);
	}

	md->itsRedrawRequestFlag = true;

	FreeMatrixList(&theHolonomyGroup);

	return theErrorMessage;
}


static ErrorText DetectSpaceType(
	MatrixList		*aGeneratorList,
	SpaceType		*aSpaceType)
{
	SpaceType		theSpaceType	= SpaceNone;
	unsigned int	i;
	
	//	Special case:
	//	If no generators are present, the space is a 3-sphere.
	if (aGeneratorList->itsNumMatrices == 0)
	{
		*aSpaceType = SpaceSpherical;
		return NULL;
	}

	//	Generic case:
	//	Set *aSpaceType to the type of the first generator,
	//	then make sure all the rest agree.

	*aSpaceType = SpaceNone;

	for (i = 0; i < aGeneratorList->itsNumMatrices; i++)
	{
		if (aGeneratorList->itsMatrices[i].m[3][3] <  1.0)
			theSpaceType = SpaceSpherical;
		if (aGeneratorList->itsMatrices[i].m[3][3] == 1.0)
			theSpaceType = SpaceFlat;
		if (aGeneratorList->itsMatrices[i].m[3][3] >  1.0)
			theSpaceType = SpaceHyperbolic;

		if (*aSpaceType == SpaceNone)
			*aSpaceType = theSpaceType;
		else
		{
			if (*aSpaceType != theSpaceType)
			{
				*aSpaceType = SpaceNone;
				return u"Matrix generators have inconsistent geometries (spherical, flat, hyperbolic), or perhaps an unneeded identity matrix is present.";
			}
		}
	}

	return NULL;
}
