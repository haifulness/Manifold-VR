//	CurvedSpacesClifford.c
//
//	Makes, binds and draws Vertex Buffer Objects for Clifford parallels.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>


//	What is the (approximate) radius of each Clifford parallel?
#define	R					0.01

//	How finely should we subdivide each meridian?
#define M					8

//	How finely should we subdivide each longitude?  (Must be even)
#define N					8

//	How many times should the longitudinal texture coordinate cycle
//	within each longitudinal segment?
#define TEXTURE_MULTIPLE	25

//	Colors for Clifford parallels.

//	single bicolor set
//
//	For non-anaglyphic use, we could use bolder centerline colors, e.g.
//
//		DARK_BLUE	((float [4]) PREMULTIPLY_RGBA(0.00, 0.00, 1.00, 1.00))
//		DARK_GREEN	((float [4]) PREMULTIPLY_RGBA(0.00, 0.50, 0.00, 1.00))
//
//	but for anaglyphic use we need to provide sufficient contrast for each eye,
//	and also make sure the intended channel remains stronger than
//	leakage from the other channel.
//
#define DARK_BLUE	((float [4]) PREMULTIPLY_RGBA(0.25, 0.25, 1.00, 1.00))
#define GREY_BLUE	((float [4]) PREMULTIPLY_RGBA(0.50, 0.50, 1.00, 1.00))
#define WHITE		((float [4]) PREMULTIPLY_RGBA(1.00, 1.00, 1.00, 1.00))
#define GREY_GREEN	((float [4]) PREMULTIPLY_RGBA(0.50, 1.00, 0.50, 1.00))
#define DARK_GREEN	((float [4]) PREMULTIPLY_RGBA(0.25, 1.00, 0.25, 1.00))

//	three mutually orthogonal monocolor sets 
//
//	Anaglyphic use requires unsaturated colors.
//

#define CLIFFORD_COLOR_A	((float [4]) PREMULTIPLY_RGBA(1.0, 0.5, 0.5, 1.0))
#define CLIFFORD_COLOR_B	((float [4]) PREMULTIPLY_RGBA(1.0, 1.0, 0.5, 1.0))
#define CLIFFORD_COLOR_C	((float [4]) PREMULTIPLY_RGBA(0.5, 1.0, 1.0, 1.0))

#define CLIFFORD_GREY_A		((float [4]) PREMULTIPLY_RGBA(0.75, 0.75, 0.75, 1.00))
#define CLIFFORD_GREY_B		((float [4]) PREMULTIPLY_RGBA(0.50, 0.50, 0.50, 1.00))
#define CLIFFORD_GREY_C		((float [4]) PREMULTIPLY_RGBA(1.00, 1.00, 1.00, 1.00))


//	The Clifford parallel Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2];	//	texture coordinates (u,v)
} CliffordVBOData;

//	The Clifford parallel Index Buffer Object (IBO) will contain
//	the following data for each of its faces.
typedef struct
{
	unsigned short	vtx[3];	//	three vertices
} CliffordIBOData;


//	Precompute a description of each Clifford parallel
//	in a standard bi-color set.

typedef enum
{
	CliffordNearCenterline,
	CliffordNearGeneric,
	CliffordHalfWay,
	CliffordFarGeneric,
	CliffordFarCenterline
} CliffordParallelType;

typedef struct
{
	Matrix					itsPlacement;
	CliffordParallelType	itsType;
} CliffordParallel;

//	Within a given set of Clifford parallels, how many Clifford parallels 
//	should each (coaxial, toroidal) layer contain?
//	Radially the layers are spaced π/12 units apart.
//	Both endpoints (at distance 0 and at distance π) are included,
//	so there are 13 levels total.
static const unsigned int	gNumParallelsInLayer[13] = {1, 4, 8, 11, 14, 16, 16, 16, 14, 11, 8, 4, 1};
#define NUM_PARALLELS_IN_SET	(1 + 4 + 8 + 11 + 14 + 16 + 16 + 16 + 14 + 11 + 8 + 4 + 1)

static bool					gStandardParallelsHaveBeenInitialized = false;
static CliffordParallel		gStandardParallels[NUM_PARALLELS_IN_SET];


static void	MakeTransformation(Matrix *aTransformation, double aTheta, double aPhi);
static void	DrawSetOfCliffordParallels(StereoMode aStereoMode, Matrix *aWorldPlacement, bool aUseDefaultColorFlag);
static void	DrawCliffordParallel(StereoMode aStereoMode, Matrix *aWorldPlacement, CliffordParallel *aCliffordParallel, bool aUseDefaultColorFlag);
static void SetColor(float aColor[4], bool aGreyscaleFlag);


void MakeCliffordVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	CliffordVBOData			theVertices[N][M];
	CliffordIBOData			theFaces[N][M][2];
	unsigned int			i,
							j;
	CliffordParallel		*theParallel;
	CliffordParallelType	theType;
	unsigned int			n,
							m;

	//	Create the vertices for a single Clifford parallel
	//	running along the axis {x² + y² = 0, w² + z² = 1}.

	//	Position the vertices like this
	//
	//		00--01--02--03--00					|
	//		 \  /\  /\  /\  /\					|
	//		  30--31--32--33--30				|
	//		 /  \/  \/  \/  \/					|
	//		20--21--22--23--20					|
	//		 \  /\  /\  /\  /\					|
	//		  10--11--12--13--10				|
	//		 /  \/  \/  \/  \/					|
	//		00--01--02--03--00					|
	//
	//	with a half-notch rotation from each meridian
	//	to the next.
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < M; j++)
		{
			theVertices[i][j].pos[0] = R*cos(2*PI*(((i & 0x00000001) ? 0.5 : 0.0) + j)/M);
			theVertices[i][j].pos[1] = R*sin(2*PI*(((i & 0x00000001) ? 0.5 : 0.0) + j)/M);
			theVertices[i][j].pos[2] =   cos(2*PI*i/N);
			theVertices[i][j].pos[3] =   sin(2*PI*i/N);

			theVertices[i][j].tex[0] = 0.0;	//	irrelevant
			theVertices[i][j].tex[1] = ((i & 0x00000001) ? TEXTURE_MULTIPLE : 0.0);
		}
	}
	
	//	List the faces.
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < M; j++)
		{
			if (i & 0x00000001)
			{
				//	odd-numbered row

				theFaces[i][j][1].vtx[0] = M*((i+1)%N) +    j   ;
				theFaces[i][j][1].vtx[1] = M*(   i   ) +    j   ;
				theFaces[i][j][1].vtx[2] = M*((i+1)%N) + (j+1)%M;
				
				theFaces[i][j][0].vtx[0] = M*((i+1)%N) + (j+1)%M;
				theFaces[i][j][0].vtx[1] = M*(   i   ) +    j   ;
				theFaces[i][j][0].vtx[2] = M*(   i   ) + (j+1)%M;
			}
			else
			{
				//	even-numbered row

				theFaces[i][j][1].vtx[0] = M*(   i   ) +    j   ;
				theFaces[i][j][1].vtx[1] = M*(   i   ) + (j+1)%M;
				theFaces[i][j][1].vtx[2] = M*((i+1)%N) +    j   ;
				
				theFaces[i][j][0].vtx[0] = M*((i+1)%N) +    j   ;
				theFaces[i][j][0].vtx[1] = M*(   i   ) + (j+1)%M;
				theFaces[i][j][0].vtx[2] = M*((i+1)%N) + (j+1)%M;
			}
		}
	}


	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	glBufferData(GL_ARRAY_BUFFER, sizeof(theVertices), theVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(theFaces), theFaces, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	
	//	Initialize a set of Clifford parallels (one-time initialization)
	if ( ! gStandardParallelsHaveBeenInitialized )
	{
		n = BUFFER_LENGTH(gNumParallelsInLayer) - 1;
		theParallel	= gStandardParallels;
		theType		= CliffordNearCenterline;
		for (i = 0; i <= n; i++)
		{
			if (i == 0)			theType = CliffordNearCenterline;
			if (i == 1)			theType = CliffordNearGeneric;
			if (i == n/2)		theType = CliffordHalfWay;
			if (i == n/2 + 1)	theType = CliffordFarGeneric;
			if (i == n)			theType = CliffordFarCenterline;

			m = gNumParallelsInLayer[i];
			for (j = 0; j < m; j++)
			{
				MakeTransformation(&theParallel->itsPlacement, i*PI/n, j*2*PI/m);
				theParallel->itsType = theType;
				theParallel++;
			}
		}
		
		gStandardParallelsHaveBeenInitialized = true;
	}
}

static void MakeTransformation(
	Matrix	*aTransformation,	//	output
	double	aTheta,				//	input:  angle down from pole, in range [0,π]
	double	aPhi)				//	input:  rotation angle about pole, in range [0,2π]
{
	//	Construct an isometry of S³ for which the corresponding
	//	isometry of S² takes the north pole to the indicated point.
	//	In S³ such a map takes the "central" Clifford parallel
	//	to an desired Clifford parallel.  Two matrices are possible,
	//	different by minus the identity, so choose an arbitrary one.

	Matrix	theFirstFactor =
			{
				{
					{ cos(aTheta/2),       0.0,            0.0,      -sin(aTheta/2)},
					{      0.0,       cos(aTheta/2),  sin(aTheta/2),       0.0     },
					{      0.0,      -sin(aTheta/2),  cos(aTheta/2),       0.0     },
					{ sin(aTheta/2),       0.0,            0.0,       cos(aTheta/2)}
				},
				ImagePositive
			},
			theSecondFactor =
			{
				{
					{  cos(aPhi/2),  sin(aPhi/2),      0.0,          0.0    },
					{ -sin(aPhi/2),  cos(aPhi/2),      0.0,          0.0    },
					{      0.0,          0.0,      cos(aPhi/2), -sin(aPhi/2)},
					{      0.0,          0.0,      sin(aPhi/2),  cos(aPhi/2)}
				},
				ImagePositive
			};

	MatrixProduct(&theFirstFactor, &theSecondFactor, aTransformation);
}


void MakeCliffordVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(CliffordVBOData), (void *)offsetof(CliffordVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(CliffordVBOData), (void *)offsetof(CliffordVBOData, tex));

			glDisableVertexAttribArray(ATTRIBUTE_COLOR);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindCliffordVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawCliffordVAO(
	GLuint			aCliffordTexture,
	CliffordMode	aCliffordMode,
	StereoMode		aStereoMode,
	Matrix			*aWorldPlacement)	//	the world's placement in eye space
{
				 Matrix	theRotatedWorldPlacement;
	static const Matrix	thePermutation1 =
						{
							{
								{0.0, 1.0, 0.0, 0.0},
								{0.0, 0.0, 1.0, 0.0},
								{1.0, 0.0, 0.0, 0.0},
								{0.0, 0.0, 0.0, 1.0}
							},
							ImagePositive
						},
						thePermutation2 =
						{
							{
								{0.0, 0.0, 1.0, 0.0},
								{1.0, 0.0, 0.0, 0.0},
								{0.0, 1.0, 0.0, 0.0},
								{0.0, 0.0, 0.0, 1.0}
							},
							ImagePositive
						};

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(aWorldPlacement->itsParity == ImagePositive ? GL_CCW : GL_CW);
	glBindTexture(GL_TEXTURE_2D, aCliffordTexture);

	switch (aCliffordMode)
	{
		case CliffordNone:
			break;

		case CliffordBicolor:
			DrawSetOfCliffordParallels(aStereoMode, aWorldPlacement, true);
			break;

		case CliffordCenterlines:
			DrawCliffordParallel(aStereoMode, aWorldPlacement, &gStandardParallels[0], true);
			DrawCliffordParallel(aStereoMode, aWorldPlacement, &gStandardParallels[NUM_PARALLELS_IN_SET - 1], true);
			break;

		//	For the 1, 2 or 3 sets of Clifford parallels,
		//	in StereoGreyscale use custom greys for good contrast.
		case CliffordThreeSets:
			MatrixProduct(&thePermutation2, aWorldPlacement, &theRotatedWorldPlacement);
			SetColor(aStereoMode == StereoNone ? CLIFFORD_COLOR_C : CLIFFORD_GREY_C, false);
			DrawSetOfCliffordParallels(aStereoMode, &theRotatedWorldPlacement, false);
			//	fall through to...
		case CliffordTwoSets:
			MatrixProduct(&thePermutation1, aWorldPlacement, &theRotatedWorldPlacement);
			SetColor(aStereoMode == StereoNone ? CLIFFORD_COLOR_B : CLIFFORD_GREY_B, false);
			DrawSetOfCliffordParallels(aStereoMode, &theRotatedWorldPlacement, false);
			//	fall through to...
		case CliffordOneSet:
			SetColor(aStereoMode == StereoNone ? CLIFFORD_COLOR_A : CLIFFORD_GREY_A, false);
			DrawSetOfCliffordParallels(aStereoMode, aWorldPlacement, false);
			break;
	}	
}

static void DrawSetOfCliffordParallels(
	StereoMode	aStereoMode,
	Matrix		*aWorldPlacement,
	bool		aUseDefaultColorFlag)
{
	unsigned int	i;

	for (i = 0; i < NUM_PARALLELS_IN_SET; i++)
		DrawCliffordParallel(aStereoMode, aWorldPlacement, &gStandardParallels[i], aUseDefaultColorFlag);
}

static void DrawCliffordParallel(
	StereoMode			aStereoMode,
	Matrix				*aWorldPlacement,
	CliffordParallel	*aCliffordParallel,
	bool				aUseDefaultColorFlag)	//	Use the Clifford parallel's default color?
												//	If not, assume the caller has set a custom color.
{
	double	theModelViewMatrix[4][4];

	//	Compose theTranslation with aWorldPlacement
	//	and send the result to the shader.
	Matrix44Product(aCliffordParallel->itsPlacement.m, aWorldPlacement->m, theModelViewMatrix);
	SendModelViewMatrixToShader(theModelViewMatrix);
	
	//	Send the color to the shader.
	if (aUseDefaultColorFlag)
	{
		switch (aCliffordParallel->itsType)
		{
			case CliffordNearCenterline:	SetColor(DARK_BLUE,		aStereoMode == StereoGreyscale);	break;
			case CliffordNearGeneric:		SetColor(GREY_BLUE,		aStereoMode == StereoGreyscale);	break;
			case CliffordHalfWay:			SetColor(WHITE,			aStereoMode == StereoGreyscale);	break;
			case CliffordFarGeneric:		SetColor(GREY_GREEN,	aStereoMode == StereoGreyscale);	break;
			case CliffordFarCenterline:		SetColor(DARK_GREEN,	aStereoMode == StereoGreyscale);	break;
		}
	}

	//	Draw one Clifford parallel.
	glDrawElements(	GL_TRIANGLES,
					3*2*N*M,	//	3 * (number of faces)
					GL_UNSIGNED_SHORT,
					0);
}

static void SetColor(
	float	aColor[4],
	bool	aGreyscaleFlag)
{
	float	theLuminance,
			theGreyscaleColor[4],
			*theColor;
	
	if ( ! aGreyscaleFlag )
	{
		theColor = aColor;
	}
	else
	{
		//	The greyscale conversion formula
		//
		//		luminance = 30% red + 59% green + 11% blue
		//
		//	appears widely on the internet, but with little explanation.
		//	Presumably its origins lie in human color perception.

		theLuminance = 0.30 * aColor[0]
					 + 0.59 * aColor[1]
					 + 0.11 * aColor[2];
		
		theGreyscaleColor[0] = theLuminance;
		theGreyscaleColor[1] = theLuminance;
		theGreyscaleColor[2] = theLuminance;
		
		theColor = theGreyscaleColor;
	}

	glVertexAttrib4fv(ATTRIBUTE_COLOR, theColor);
}
