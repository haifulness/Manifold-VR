//	CurvedSpacesHantzscheWendt.c
//
//	Makes, binds and draws Vertex Buffer Objects
//	for the corkscrew axes in the Hantzsche- Wendt space.
//
//	WARNING:  THIS CODE IS JUST A QUICK-AND-DIRTY HACK.
//	NOT FOR PUBLIC RELEASE!
//
//	IF I EVER CLEAN THIS UP, NOTE THAT THE MESH WITH AN "INDEX BUFFER OBJECT",
//	WHILE NECESSARY IN THE 3-SPHERE, IS OVERKILL IN FLAT SPACE.
//	A SIMPLE TRIANGLE STRIP WOULD DO.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"

//	If we're not showing the Hantzsche-Wendt axes,
//	this file remains empty!
#ifdef HANTZSCHE_WENDT_AXES

#include <stddef.h>	//	for offsetof()
#include <math.h>


//	What is the (approximate) radius of each axis?
#define	R					0.02

//	How finely should we subdivide each meridian?
#define M					8

//	How finely should we subdivide each longitude?  (Must be even)
#define N					2

//	How many times should the longitudinal texture coordinate cycle
//	within each longitudinal segment?
#define TEXTURE_MULTIPLE	12


//	The Hantzsche-Wendt Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2];	//	texture coordinates (u,v)
} HantzscheWendtVBOData;

//	The HantzscheWendt parallel Index Buffer Object (IBO) will contain
//	the following data for each of its faces.
typedef struct
{
	unsigned short	vtx[3];	//	three vertices
} HantzscheWendtIBOData;


#define NUM_AXIS_PLACEMENTS	6
static Matrix	gAxisPlacements[NUM_AXIS_PLACEMENTS];
static float	gAxisColors[NUM_AXIS_PLACEMENTS][4];


static void	MakeTransformation(Matrix *aTransformation, double aTheta, double aPhi, double dx, double dy, double dz);


void MakeHantzscheWendtVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	HantzscheWendtVBOData	theVertices[N+1][M];
	HantzscheWendtIBOData	theFaces[N][M][2];
	unsigned int			i,
							j;
	RGBAColor				theRGBAColor;

	//	Create the vertices for a single Hantzsche-Wendt axis
	//	running along the axis {x = 0, y = 0, -1 ≤ z ≤ +1, w = 1}.

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
	for (i = 0; i <= N; i++)
	{
		for (j = 0; j < M; j++)
		{
			theVertices[i][j].pos[0] = R*cos(2*PI*(((i & 0x00000001) ? 0.5 : 0.0) + j)/M);
			theVertices[i][j].pos[1] = R*sin(2*PI*(((i & 0x00000001) ? 0.5 : 0.0) + j)/M);
			theVertices[i][j].pos[2] = -0.5 + ((double)i)/N;
			theVertices[i][j].pos[3] = 1.0;

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

				theFaces[i][j][1].vtx[0] = M*(i+1) +    j   ;
				theFaces[i][j][1].vtx[1] = M*( i ) +    j   ;
				theFaces[i][j][1].vtx[2] = M*(i+1) + (j+1)%M;
				
				theFaces[i][j][0].vtx[0] = M*(i+1) + (j+1)%M;
				theFaces[i][j][0].vtx[1] = M*( i ) +    j   ;
				theFaces[i][j][0].vtx[2] = M*( i ) + (j+1)%M;
			}
			else
			{
				//	even-numbered row

				theFaces[i][j][1].vtx[0] = M*( i ) +    j   ;
				theFaces[i][j][1].vtx[1] = M*( i ) + (j+1)%M;
				theFaces[i][j][1].vtx[2] = M*(i+1) +    j   ;
				
				theFaces[i][j][0].vtx[0] = M*(i+1) +    j   ;
				theFaces[i][j][0].vtx[1] = M*( i ) + (j+1)%M;
				theFaces[i][j][0].vtx[2] = M*(i+1) + (j+1)%M;
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

	
	//	Initialize the translation matrices and colors.

	MakeTransformation(&gAxisPlacements[0], 0.0, 0.0, 0.5, 0.0, 0.0);
	HSLAtoRGBA(&(HSLAColor){5.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[0][0] = (float)theRGBAColor.r;
	gAxisColors[0][1] = (float)theRGBAColor.g;
	gAxisColors[0][2] = (float)theRGBAColor.b;
	gAxisColors[0][3] = (float)theRGBAColor.a;

	MakeTransformation(&gAxisPlacements[1], 0.0, 0.0, -0.5, 0.0, 0.0);
	HSLAtoRGBA(&(HSLAColor){0.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[1][0] = (float)theRGBAColor.r;
	gAxisColors[1][1] = (float)theRGBAColor.g;
	gAxisColors[1][2] = (float)theRGBAColor.b;
	gAxisColors[1][3] = (float)theRGBAColor.a;

	MakeTransformation(&gAxisPlacements[2], PI/2, PI/2, 0.0, 0.0, 0.5);
	HSLAtoRGBA(&(HSLAColor){3.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[2][0] = (float)theRGBAColor.r;
	gAxisColors[2][1] = (float)theRGBAColor.g;
	gAxisColors[2][2] = (float)theRGBAColor.b;
	gAxisColors[2][3] = (float)theRGBAColor.a;

	MakeTransformation(&gAxisPlacements[3], PI/2, PI/2, 0.0, 0.0, -0.5);
	HSLAtoRGBA(&(HSLAColor){1.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[3][0] = (float)theRGBAColor.r;
	gAxisColors[3][1] = (float)theRGBAColor.g;
	gAxisColors[3][2] = (float)theRGBAColor.b;
	gAxisColors[3][3] = (float)theRGBAColor.a;

	MakeTransformation(&gAxisPlacements[4], PI/2, 0.0, 0.0, 0.5, 0.0);
	HSLAtoRGBA(&(HSLAColor){2.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[4][0] = (float)theRGBAColor.r;
	gAxisColors[4][1] = (float)theRGBAColor.g;
	gAxisColors[4][2] = (float)theRGBAColor.b;
	gAxisColors[4][3] = (float)theRGBAColor.a;

	MakeTransformation(&gAxisPlacements[5], PI/2, 0.0, 0.0, -0.5, 0.0);
	HSLAtoRGBA(&(HSLAColor){4.0/6.0, 0.6, 0.5, 1.0}, &theRGBAColor);
	gAxisColors[5][0] = (float)theRGBAColor.r;
	gAxisColors[5][1] = (float)theRGBAColor.g;
	gAxisColors[5][2] = (float)theRGBAColor.b;
	gAxisColors[5][3] = (float)theRGBAColor.a;

}

static void MakeTransformation(
	Matrix	*aTransformation,	//	output
	double	aTheta,				//	input:  rotation from z-axis towards x-axis, in range [0,π]
	double	aPhi,				//	input:  rotation from x-axis towards y-axis, in range [0,2π]
	double	dx,					//	input:	translation
	double	dy,
	double	dz)
{
	Matrix	theFirstFactor =
			{
				{
					{ cos(aTheta),     0.0,      -sin(aTheta),       0.0     },
					{    0.0,          1.0,          0.0,            0.0     },
					{ sin(aTheta),     0.0,       cos(aTheta),       0.0     },
					{    0.0,          0.0,          0.0,            1.0     }
				},
				ImagePositive
			},
			theSecondFactor =
			{
				{
					{  cos(aPhi),  sin(aPhi),      0.0,          0.0    },
					{ -sin(aPhi),  cos(aPhi),      0.0,          0.0    },
					{    0.0,        0.0,          1.0,          0.0    },
					{     dx,         dy,           dz,          1.0    }
				},
				ImagePositive
			};

	MatrixProduct(&theFirstFactor, &theSecondFactor, aTransformation);
}


void MakeHantzscheWendtVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(HantzscheWendtVBOData), (void *)offsetof(HantzscheWendtVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(HantzscheWendtVBOData), (void *)offsetof(HantzscheWendtVBOData, tex));

			glDisableVertexAttribArray(ATTRIBUTE_COLOR);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindHantzscheWendtVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawHantzscheWendtVAO(
	GLuint 		aHantzscheWendtTexture,
	Honeycomb	*aHoneycomb,
	Matrix		*aWorldPlacement)	//	the world's placement in eye space
{
	unsigned int	i,
					j;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
//	glFrontFace(aWorldPlacement->itsParity == ImagePositive ? GL_CCW : GL_CW);
//NOT SURE WHY THIS HAD TO BE FLIPPED.  LIKE I SAID, IT'S A QUICK-AND-DIRTY HACK!
	glFrontFace(aWorldPlacement->itsParity == ImagePositive ? GL_CW : GL_CCW);
	
	glBindTexture(GL_TEXTURE_2D, aHantzscheWendtTexture);

	for (i = 0; i < NUM_AXIS_PLACEMENTS; i++)
	{
		glVertexAttrib4fv(ATTRIBUTE_COLOR, gAxisColors[i]);

		for (j = 0; j < aHoneycomb->itsNumVisibleCells; j++)
		{
			//	Each element of the tiling group defines
			//	a placement of the Dirichlet domain in world space.
			theDirichletPlacement = &aHoneycomb->itsVisibleCells[j]->itsMatrix;

			//	Let front faces wind counterclockwise (resp. clockwise)
			//	when the Dirichlet domain's placement in eye space preserves (resp. reverses) parity.
		//Needed only in a non-orientable space. 
		//	glFrontFace(theDirichletPlacement->itsParity == aWorldPlacement->itsParity ? GL_CCW : GL_CW);

			//	Compose gAxisPlacements[i], theDirichletPlacement and aWorldPlacement,
			//	and send the result to the shader.
			Matrix44Product(theDirichletPlacement->m, aWorldPlacement->m, theModelViewMatrix);
			Matrix44Product(gAxisPlacements[i].m,     theModelViewMatrix, theModelViewMatrix);
			SendModelViewMatrixToShader(theModelViewMatrix);

			//	Draw one Hantzsche-Wendt axis.
			glDrawElements(	GL_TRIANGLES,
							3*2*N*M,	//	3 * (number of faces)
							GL_UNSIGNED_SHORT,
							0);
		}
	}
}

#endif	//	HANTZSCHE_WENDT_AXES
