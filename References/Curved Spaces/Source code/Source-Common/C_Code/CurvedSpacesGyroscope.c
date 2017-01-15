//	CurvedSpacesGyroscope.c
//
//	Makes, binds and draws Vertex Buffer Objects for spinning gyroscope.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>


//	How big should the ring of arrows be?
#define OUTER_RADIUS	0.050
#define OUTER_HEIGHT	0.025

//	How large should the central axle be?
#define INNER_RADIUS	0.017
#define INNER_HEIGHT	0.100

//	What colors should the gyroscope be?
#define COLOR_ARROW_OUTER	PREMULTIPLY_RGBA(0.2500, 0.6250, 1.0000, 1.0000)
#define COLOR_ARROW_INNER	PREMULTIPLY_RGBA(0.1250, 0.1875, 0.2500, 1.0000)
#define COLOR_AXLE_BOTTOM	PREMULTIPLY_RGBA(1.0000, 0.0000, 0.0000, 1.0000)
#define COLOR_AXLE_TOP		PREMULTIPLY_RGBA(1.0000, 1.0000, 1.0000, 1.0000)

//	For convenience, predefine cos(2πk/n) and sin(2πk/n).

#define ROOT_3_OVER_2	 0.86602540378443864676

#define COS0	 1.0
#define SIN0	 0.0

#define COS1	 0.5
#define SIN1	 ROOT_3_OVER_2

#define COS2	-0.5
#define SIN2	 ROOT_3_OVER_2

#define COS3	-1.0
#define SIN3	 0.0

#define COS4	-0.5
#define SIN4	-ROOT_3_OVER_2

#define COS5	 0.5
#define SIN5	-ROOT_3_OVER_2


//	The gyroscope Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			col[4];	//	color (r,g,b,a)
} GyroscopeVBOData;

//	The gyroscope Index Buffer Object (IBO) will contain
//	the following data for each of its faces.
typedef struct
{
	unsigned short	vtx[3];	//	three vertices
} GyroscopeIBOData;


static const GyroscopeVBOData	gVertices[2*6*3 + 2*(6+1)] =
{
	//	arrows, outer surface

	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0,  0.0,          1.0}, COLOR_ARROW_OUTER},

	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1,  0.0,          1.0}, COLOR_ARROW_OUTER},

	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2,  0.0,          1.0}, COLOR_ARROW_OUTER},

	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3,  0.0,          1.0}, COLOR_ARROW_OUTER},

	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4,  0.0,          1.0}, COLOR_ARROW_OUTER},

	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_OUTER},
	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5,  0.0,          1.0}, COLOR_ARROW_OUTER},

	//	arrows, inner surface

	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0,  0.0,          1.0}, COLOR_ARROW_INNER},

	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS1, OUTER_RADIUS*SIN1,  0.0,          1.0}, COLOR_ARROW_INNER},

	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS2, OUTER_RADIUS*SIN2,  0.0,          1.0}, COLOR_ARROW_INNER},

	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS3, OUTER_RADIUS*SIN3,  0.0,          1.0}, COLOR_ARROW_INNER},

	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS4, OUTER_RADIUS*SIN4,  0.0,          1.0}, COLOR_ARROW_INNER},

	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0, -OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS0, OUTER_RADIUS*SIN0, +OUTER_HEIGHT, 1.0}, COLOR_ARROW_INNER},
	{{OUTER_RADIUS*COS5, OUTER_RADIUS*SIN5,  0.0,          1.0}, COLOR_ARROW_INNER},

	//	bottom half of axle (red)
	{{INNER_RADIUS*COS0, INNER_RADIUS*SIN0, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{INNER_RADIUS*COS1, INNER_RADIUS*SIN1, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{INNER_RADIUS*COS2, INNER_RADIUS*SIN2, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{INNER_RADIUS*COS3, INNER_RADIUS*SIN3, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{INNER_RADIUS*COS4, INNER_RADIUS*SIN4, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{INNER_RADIUS*COS5, INNER_RADIUS*SIN5, 0.0,          1.0}, COLOR_AXLE_BOTTOM},
	{{0.0,               0.0,              -INNER_HEIGHT, 1.0}, COLOR_AXLE_BOTTOM},

	//	top half of axle (white)
	{{INNER_RADIUS*COS0, INNER_RADIUS*SIN0, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{INNER_RADIUS*COS1, INNER_RADIUS*SIN1, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{INNER_RADIUS*COS2, INNER_RADIUS*SIN2, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{INNER_RADIUS*COS3, INNER_RADIUS*SIN3, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{INNER_RADIUS*COS4, INNER_RADIUS*SIN4, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{INNER_RADIUS*COS5, INNER_RADIUS*SIN5, 0.0,          1.0}, COLOR_AXLE_TOP},
	{{0.0,               0.0,              +INNER_HEIGHT, 1.0}, COLOR_AXLE_TOP}
};

static const GyroscopeIBOData	gFaces[2*6 + 2*6] =
{
	//	arrows, outer surface
	{{ 0,  1,  2}},
	{{ 3,  4,  5}},
	{{ 6,  7,  8}},
	{{ 9, 10, 11}},
	{{12, 13, 14}},
	{{15, 16, 17}},

	//	arrows, inner surface
	{{18, 19, 20}},
	{{21, 22, 23}},
	{{24, 25, 26}},
	{{27, 28, 29}},
	{{30, 31, 32}},
	{{33, 34, 35}},
	
	//	bottom half of axle (red)
	{{36, 37, 42}},
	{{37, 38, 42}},
	{{38, 39, 42}},
	{{39, 40, 42}},
	{{40, 41, 42}},
	{{41, 36, 42}},

	//	top half of axle (white)
	{{44, 43, 49}},
	{{45, 44, 49}},
	{{46, 45, 49}},
	{{47, 46, 49}},
	{{48, 47, 49}},
	{{43, 48, 49}}
};


void MakeGyroscopeVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName,
	bool	aGreyscaleFlag)
{
	const GyroscopeVBOData	*theVertices;
	GyroscopeVBOData		theGreyscaleVertices[BUFFER_LENGTH(gVertices)];
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


void MakeGyroscopeVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(GyroscopeVBOData), (void *)offsetof(GyroscopeVBOData, pos));

			glDisableVertexAttribArray(ATTRIBUTE_TEX_COORD);

			glEnableVertexAttribArray(ATTRIBUTE_COLOR);
			glVertexAttribPointer(ATTRIBUTE_COLOR,     4, GL_FLOAT, GL_FALSE, sizeof(GyroscopeVBOData), (void *)offsetof(GyroscopeVBOData, col));

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindGyroscopeVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawGyroscopeVAO(
	GLuint		aGyroscopeTexture,
	Honeycomb	*aHoneycomb,
	Matrix		*aWorldPlacement,		//	the world's placement in eye space
	Matrix		*aGyroscopePlacement)	//	the gyroscope's placement in the Dirichlet domain
{
	ImageParity		thePartialParity;
	unsigned int	i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aHoneycomb == NULL)
		return;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//	It's simpler to bind a pure white texture for the gyroscope
	//	than it would be to write a special-purpose texture-free shader for it.
	//	Using a flag to toggle texturing on and off would be the worst approach of all:
	//	on my Radeon X1600 it slowed KaleidoTile's frame rate by a factor of four!
	glBindTexture(GL_TEXTURE_2D, aGyroscopeTexture);
	
	//	Set a pair of texture coordinates once and for all,
	//	to avoid having to pass them in arrays.
	//	(0.5, 0.5) points to the texture's center,
	//	which is as good as spot as any.
	glVertexAttrib2fv(ATTRIBUTE_TEX_COORD, (float [2]){0.5, 0.5});

	//	Compose the parity of the gyroscope's placement in the Dirichlet domain
	//	with the parity of the world's placement in eye space.  
	//	Later on, as we traverse the tiling, we'll factor in the parity 
	//	of the Dirichlet domain's placement in world space, which may 
	//	be negative for some tiles and positive for others.
	thePartialParity = (aWorldPlacement->itsParity == aGyroscopePlacement->itsParity
						? ImagePositive : ImageNegative);
	
	//	Draw the spinning gyroscopes in near-to-far order.
	for (i = 0; i < aHoneycomb->itsNumVisibleCells; i++)
	{
		//	Each element of the tiling group defines
		//	a placement of the Dirichlet domain in world space.
		theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

		//	Let front faces wind counterclockwise (resp. clockwise)
		//	when the gyroscope's placement in eye space preserves (resp. reverses) parity.
		glFrontFace(theDirichletPlacement->itsParity == thePartialParity ? GL_CCW : GL_CW);

		//	Compose aGyroscopePlacement, theDirichletPlacement and aWorldPlacement,
		//	and send the result to the shader.
		Matrix44Product(aGyroscopePlacement->m, theDirichletPlacement->m, theModelViewMatrix);
		Matrix44Product(theModelViewMatrix,     aWorldPlacement->m,       theModelViewMatrix);
		SendModelViewMatrixToShader(theModelViewMatrix);

		//	Draw.
		glDrawElements(	GL_TRIANGLES,
						3 * BUFFER_LENGTH(gFaces),
						GL_UNSIGNED_SHORT,
						0);
	}
}
