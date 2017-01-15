//	CurvedSpacesObserver.c
//
//	Makes, binds and draws Vertex Buffer Objects
//	for the colored dart that marks the observer's position.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>


//	How big should the dart be?
//
//	Note:  In a 3-sphere, the image of the dart at the user's origin
//	and/or antipodal point will display correctly iff the clipping distance
//	is at most about WIDTH/2.
#define HALF_LENGTH	0.050
#define RADIUS		0.017
#define WIDTH		0.004

//	What colors should the fletches be?
#define COLOR_FLETCH_LEFT	PREMULTIPLY_RGBA(1.00, 0.00, 0.00, 1.00)
#define COLOR_FLETCH_RIGHT	PREMULTIPLY_RGBA(0.00, 1.00, 0.00, 1.00)
#define COLOR_FLETCH_BOTTOM	PREMULTIPLY_RGBA(0.00, 0.00, 1.00, 1.00)
#define COLOR_FLETCH_TOP	PREMULTIPLY_RGBA(1.00, 1.00, 0.00, 1.00)
//#define COLOR_TAIL			PREMULTIPLY_RGBA(0.50, 0.25, 0.0625, 1.00)	//	brown
#define COLOR_TAIL			PREMULTIPLY_RGBA(0.50, 0.50, 0.50, 1.00)


//	The observer Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			col[4];	//	color (r,g,b,a)
} ObserverVBOData;

//	The observer Index Buffer Object (IBO) will contain
//	the following data for each of its faces.
typedef struct
{
	unsigned short	vtx[3];	//	three vertices
} ObserverIBOData;


//	Represent the observer's spaceship as a dart with four fletches.
static const ObserverVBOData	gVertices[4*4 + 8] =
								{
									//	left fletch
									{{-WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_LEFT	},
									{{-WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_LEFT	},
									{{-RADIUS,   0.0,   -HALF_LENGTH, 1.0}, COLOR_FLETCH_LEFT	},
									{{  0.0,     0.0,   +HALF_LENGTH, 1.0}, COLOR_FLETCH_LEFT	},

									//	right fletch
									{{+WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_RIGHT	},
									{{+WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_RIGHT	},
									{{+RADIUS,   0.0,   -HALF_LENGTH, 1.0}, COLOR_FLETCH_RIGHT	},
									{{  0.0,     0.0,   +HALF_LENGTH, 1.0}, COLOR_FLETCH_RIGHT	},

									//	bottom fletch
									{{-WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_BOTTOM	},
									{{+WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_BOTTOM	},
									{{  0.0,   -RADIUS, -HALF_LENGTH, 1.0}, COLOR_FLETCH_BOTTOM	},
									{{  0.0,     0.0,   +HALF_LENGTH, 1.0}, COLOR_FLETCH_BOTTOM	},

									//	top fletch
									{{+WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_TOP	},
									{{-WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_FLETCH_TOP	},
									{{  0.0,   +RADIUS, -HALF_LENGTH, 1.0}, COLOR_FLETCH_TOP	},
									{{  0.0,     0.0,   +HALF_LENGTH, 1.0}, COLOR_FLETCH_TOP	},
									
									//	tail
									{{-RADIUS,   0.0,   -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{-WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{  0.0,   -RADIUS, -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{+WIDTH,  -WIDTH,  -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{+RADIUS,   0.0,   -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{+WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{  0.0,   +RADIUS, -HALF_LENGTH, 1.0}, COLOR_TAIL			},
									{{-WIDTH,  +WIDTH,  -HALF_LENGTH, 1.0}, COLOR_TAIL			},
								};

static const ObserverIBOData	gFaces[4*2 + 6] =
								{
									//	left fletch
									{{ 2,  0,  3}},
									{{ 2,  3,  1}},

									//	right fletch
									{{ 6,  4,  7}},
									{{ 6,  7,  5}},

									//	bottom fletch
									{{10,  8, 11}},
									{{10, 11,  9}},

									//	top fletch
									{{14, 12, 15}},
									{{14, 15, 13}},
									
									//	“transom”
									{{16, 17, 23}},
									{{18, 19, 17}},
									{{20, 21, 19}},
									{{22, 23, 21}},
									{{17, 19, 21}},
									{{21, 23, 17}}
								};


void MakeObserverVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName,
	bool	aGreyscaleFlag)
{
	const ObserverVBOData	*theVertices;
	ObserverVBOData			theGreyscaleVertices[BUFFER_LENGTH(gVertices)];
	unsigned int			i;
	float					theLuminance;
	
	if ( ! aGreyscaleFlag )
	{
		//	For normal (non-anaglyphic) viewing,
		//	use gVertices exactly as they are.
		theVertices = gVertices;
	}
	else
	{
		//	For anaglyphic 3D, convert gVertices's colors to greyscale.

		for (i = 0; i < BUFFER_LENGTH(gVertices); i++)
		{
			theGreyscaleVertices[i] = gVertices[i];

			//	The greyscale conversion formula
			//
			//		luminance = 30% red + 59% green + 11% blue
			//
			//	appears widely on the internet, but with little explanation.
			//	Presumably its origins lie in human color perception.

			theLuminance = (float) (
					0.30 * theGreyscaleVertices[i].col[0]
				  + 0.59 * theGreyscaleVertices[i].col[1]
				  + 0.11 * theGreyscaleVertices[i].col[2] );

			theGreyscaleVertices[i].col[0] = theLuminance;
			theGreyscaleVertices[i].col[1] = theLuminance;
			theGreyscaleVertices[i].col[2] = theLuminance;
		}
		
		theVertices = theGreyscaleVertices;
	}


	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVertices), theVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gFaces), gFaces, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void MakeObserverVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(ObserverVBOData), (void *)offsetof(ObserverVBOData, pos));

			glDisableVertexAttribArray(ATTRIBUTE_TEX_COORD);

			glEnableVertexAttribArray(ATTRIBUTE_COLOR);
			glVertexAttribPointer(ATTRIBUTE_COLOR,     4, GL_FLOAT, GL_FALSE, sizeof(ObserverVBOData), (void *)offsetof(ObserverVBOData, col));

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindObserverVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawObserverVAO(
	GLuint		anObserverTexture,
	Honeycomb	*aHoneycomb,
	Matrix		*aWorldPlacement,		//	the world's placement in eye space
	Matrix		*anObserverPlacement)	//	the observer's placement in the Dirichlet domain
{
	ImageParity		thePartialParity;
	unsigned int	i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aHoneycomb == NULL)
		return;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//	It's simpler to bind a pure white texture for the observer
	//	than it would be to write a special-purpose texture-free shader for it.
	//	Using a flag to toggle texturing on and off would be the worst approach of all:
	//	on my Radeon X1600 it slowed KaleidoTile's frame rate by a factor of four!
	glBindTexture(GL_TEXTURE_2D, anObserverTexture);
	
	//	Set a pair of texture coordinates once and for all,
	//	to avoid having to pass them in arrays.
	//	(0.5, 0.5) points to the texture's center,
	//	which is as good a spot as any.
	glVertexAttrib2fv(ATTRIBUTE_TEX_COORD, (float [2]){0.5, 0.5});
	
	//	Compose the parity of the observer's placement in the Dirichlet domain
	//	with the parity of the world's placement in eye space.  
	//	Later on, as we traverse the tiling, we'll factor in the parity 
	//	of the Dirichlet domain's placement in world space, which may 
	//	be negative for some tiles and positive for others.
	thePartialParity = (aWorldPlacement->itsParity == anObserverPlacement->itsParity
						? ImagePositive : ImageNegative);
	
	//	Draw the images of the observer in near-to-far order.
	for (i = 0; i < aHoneycomb->itsNumVisibleCells; i++)
	{
		//	Each element of the tiling group defines
		//	a placement of the Dirichlet domain in world space.
		theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

		//	Let front faces wind counterclockwise (resp. clockwise)
		//	when the observer's placement in eye space preserves (resp. reverses) parity.
		glFrontFace(theDirichletPlacement->itsParity == thePartialParity ? GL_CCW : GL_CW);

		//	Compose anObserverPlacement, theDirichletPlacement and aWorldPlacement,
		//	and send the result to the shader.
		Matrix44Product(anObserverPlacement->m, theDirichletPlacement->m, theModelViewMatrix);
		Matrix44Product(theModelViewMatrix,     aWorldPlacement->m,       theModelViewMatrix);
		SendModelViewMatrixToShader(theModelViewMatrix);

		//	Draw.
		glDrawElements(	GL_TRIANGLES,
						3 * BUFFER_LENGTH(gFaces),
						GL_UNSIGNED_SHORT,
						0);
	}
}
