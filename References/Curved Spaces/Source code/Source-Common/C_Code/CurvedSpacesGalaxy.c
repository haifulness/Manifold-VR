//	CurvedSpacesGalaxy.c
//
//	Makes, binds and draws Vertex Buffer Objects for the rotating galaxy.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>


//	The galaxy's corners sit at (±GALAXY_SIZE, ±GALAXY_SIZE, 0).
#define GALAXY_SIZE		0.25


//	The galaxy Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2];	//	texture coordinates (u,v)
} GalaxyVBOData;


void MakeGalaxyVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)	//	ignored
{
	//	The galaxy is so simple (a single quad!) that it's hardly worth
	//	creating a Vertex Buffer Object for it.  I'm making one anyhow,
	//	though, for consistency with the other centerpieces (the Earth
	//	and the gyroscope) which do merit Vertex Buffer Objects.
	//	[That turned out to be a good move:  forward-compatible OpenGL 3.0
	//	requires Vertex Array Objects and Vertex Buffer Objects.]

	static const GalaxyVBOData	theVertices[4] =
								{
									{{-GALAXY_SIZE, -GALAXY_SIZE, 0.0, 1.0}, {0.0, 0.0}},
									{{+GALAXY_SIZE, -GALAXY_SIZE, 0.0, 1.0}, {1.0, 0.0}},
									{{+GALAXY_SIZE, +GALAXY_SIZE, 0.0, 1.0}, {1.0, 1.0}},
									{{-GALAXY_SIZE, +GALAXY_SIZE, 0.0, 1.0}, {0.0, 1.0}}
								};

	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GalaxyVBOData), theVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	UNUSED_PARAMETER(anIndexBufferName);
}


void MakeGalaxyVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)	//	ignored
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(GalaxyVBOData), (void *)offsetof(GalaxyVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(GalaxyVBOData), (void *)offsetof(GalaxyVBOData, tex));

			glDisableVertexAttribArray(ATTRIBUTE_COLOR);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		UNUSED_PARAMETER(anIndexBufferName);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void BindGalaxyVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawGalaxyVAO(
	GLuint		aGalaxyTexture,
	Honeycomb	*aHoneycomb,
	Matrix		*aWorldPlacement,	//	the world's placement in eye space
	Matrix		*aGalaxyPlacement)	//	the galaxy's placement in the Dirichlet domain
{
	unsigned int	i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aHoneycomb == NULL)
		return;

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, aGalaxyTexture);
	glVertexAttrib4fv(ATTRIBUTE_COLOR, (float [4]) PREMULTIPLY_RGBA(1.0, 1.0, 1.0, 1.0));
	
	//	Draw the spinning galaxies in far-to-near order,
	//	to get the transparency right.
	for (i = aHoneycomb->itsNumVisibleCells; i-- > 0; )
	{
#ifdef HIGH_RESOLUTION_SCREENSHOT
		//	Suppress the centerpiece image nearest the camera.
		if (i == 0)
			break;
#endif

		//	Each element of the tiling group defines
		//	a placement of the Dirichlet domain in world space.
		theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

		//	Compose aGalaxyPlacement, theDirichletPlacement and aWorldPlacement,
		//	and send the result to the shader.
		Matrix44Product(aGalaxyPlacement->m, theDirichletPlacement->m, theModelViewMatrix);
		Matrix44Product(theModelViewMatrix,  aWorldPlacement->m,       theModelViewMatrix);
		SendModelViewMatrixToShader(theModelViewMatrix);

		//	Draw.
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glDisable(GL_BLEND);
}
