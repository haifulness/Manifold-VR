//	CurvedSpacesEarth.c
//
//	Makes, binds and draws Vertex Buffer Objects for the spinning Earth.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "CurvedSpacesGraphics-OpenGL.h"
#include <stddef.h>	//	for offsetof()
#include <math.h>


//	NUM_REFINEMENTS tells how finely the triangulation will be subdivided.
//
//		If NUM_REFINEMENTS is 1 the Earth will be
//			triangulated as a plain octahedron (8 faces) only.
//
//		If NUM_REFINEMENTS is 2 the Earth will be
//			triangulated as the first subdivision (32 faces) as well.
//
//		If NUM_REFINEMENTS is 3 the Earth will be
//			triangulated as the second subdivision (128 faces) as well.
//
//		etc.
//
//	The present implementation uses NUM_REFINEMENTS == 5 and
//	provides the following levels:
//
//		level 0		   8 faces
//		level 1		  32 faces
//		level 2		 128 faces
//		level 3		 512 faces
//		level 4		2048 faces
//
//	Don't push the refinement level too ridiculously high, or 
//	the vertex indices will overflow the 16-bit ints used to store them.
#define NUM_REFINEMENTS		5
								
//	How big should the Earth be?
#define EARTH_RADIUS		0.1


//	The Earth Vertex Buffer Object (VBO) will contain
//	the following data for each of its vertices.
typedef struct
{
	float	pos[4],	//	position (x,y,z,w)
			tex[2];	//	texture coordinates (u,v)
} EarthVBOData;

//	The Earth Index Buffer Object (IBO) will contain
//	the following data for each of its faces.
typedef struct
{
	unsigned short	vtx[3];	//	three vertices
} EarthIBOData;


typedef struct
{
	unsigned int	itsNumVertices;
	EarthVBOData	*itsVertices;
	
	unsigned int	itsNumEdges;

	unsigned int	itsNumFaces;
	EarthIBOData	*itsFaces;

} Triangulation;


//	When we compute the Earth triangulation's various subdivisions,
//	we'll find out how many vertices and faces they have.
static unsigned int	gNumEarthVertices[NUM_REFINEMENTS],
					gNumEarthFaces[NUM_REFINEMENTS],
					gStartEarthFaces[NUM_REFINEMENTS];	//	offset within concatenated array (see below)


static void	InitOctahedron(Triangulation *aTriangulation);
static void	SubdivideTriangulation(Triangulation *aTriangulation, Triangulation *aSubdivision);
static void	ProjectToSphere(Triangulation *aTriangulation);


void MakeEarthVBO(
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	Triangulation	theSubdivisions[NUM_REFINEMENTS];
	unsigned int	theTotalNumFaces,
					i,
					j;
	EarthIBOData	*theFaces		= NULL,
					*theFace;

	//	For robust error handling, initialize all pointers to NULL.
	for (i = 0; i < NUM_REFINEMENTS; i++)
	{
		theSubdivisions[i].itsVertices	= NULL;
		theSubdivisions[i].itsFaces		= NULL;
	}

	//	Construct an octahedron for the base level.
	//	The octahedron works better than an icosahedron 
	//	because no face straddles the equator.
	//	The texture mapping projects the octahedron orthogonally 
	//	onto the equatorial plane, rotates 45°, and then swings 
	//	the northern hemisphere around to sit beside the southern hemisphere.
	//	The texture itself, of course, has been precomputed with this mapping in mind.
	//
	//	2006/10/23  Each pixel on the perimeter of the southern texture map has been
	//	averaged with the corresponding pixel on the perimeter of the northern hemisphere,
	//	with the average replacing both original pixels.  This avoids rendering artifacts
	//	along the equator when drawing the Earth close up.  When several rows of pixels
	//	corresponding to only one row of texels, the GL_CLAMP_TO_EDGE method fills
	//	in the last half-a-texel-row's worth of pixels with the uninterpolated color
	//	of the last texel.  The ideal solution would be to interpolate betweeen the southern
	//	and northern texture maps, but that's not possible (OpenGL will accept a row of border
	//	texels, but renders them in softare -- ugh) so the next best thing is to have
	//	the southern and northern perimeters agree, to avoid an obvious visual discontinuity.
	//
	InitOctahedron(&theSubdivisions[0]);

	//	Subdivide each triangulation to get the next one in the series.
	//	At this point the subdivisions all lie on the octahedron itself,
	//	not on the Earth's spherical surface.
	for (i = 0; i < NUM_REFINEMENTS - 1; i++)
		SubdivideTriangulation(&theSubdivisions[i], &theSubdivisions[i+1]);
	
	//	Normalize all vertices to lie on the Earth's spherical surface.
	for (i = 0; i < NUM_REFINEMENTS; i++)
		ProjectToSphere(&theSubdivisions[i]);
	
	//	Record the number of vertices and faces in each subdivision,
	//	for use at render time.
	for (i = 0; i < NUM_REFINEMENTS; i++)
	{
		gNumEarthVertices[i]	= theSubdivisions[i].itsNumVertices;
		gNumEarthFaces[i]		= theSubdivisions[i].itsNumFaces;
		if (i == 0)
			gStartEarthFaces[0] = 0;
		else
			gStartEarthFaces[i] = gStartEarthFaces[i-1] + gNumEarthFaces[i-1];
	}

	//	Concatenate the face information for the various subdivisions
	//	into a single long index array.

	theTotalNumFaces	= gStartEarthFaces[NUM_REFINEMENTS - 1]
						+ gNumEarthFaces[NUM_REFINEMENTS - 1];
	theFaces			= (EarthIBOData *) GET_MEMORY(theTotalNumFaces * sizeof(EarthIBOData));
	GEOMETRY_GAMES_ASSERT(	theFaces != NULL,
							"Couldn't get memory to assemble combined face information in MakeEarthVBO().");

	theFace = theFaces;
	for (i = 0; i < NUM_REFINEMENTS; i++)
		for (j = 0; j < theSubdivisions[i].itsNumFaces; j++)
			*theFace++ = theSubdivisions[i].itsFaces[j];

	//	Prepare the Vertex Buffer Objects.

	//	Each subdivision's vertex list begins with the preceding subdivision's
	//	vertex list.  So we can send the most refined list to the GPU, and then 
	//	use however much of it we need according to the desired level-of-detail.
	glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);
	glBufferData(GL_ARRAY_BUFFER,
					theSubdivisions[NUM_REFINEMENTS - 1].itsNumVertices * sizeof(EarthVBOData),
					theSubdivisions[NUM_REFINEMENTS - 1].itsVertices,
					GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);
	glBufferData(	GL_ELEMENT_ARRAY_BUFFER,
					theTotalNumFaces * sizeof(EarthIBOData),
					theFaces,
					GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	//	Free temporary memory.
	for (i = 0; i < NUM_REFINEMENTS; i++)
	{
		FREE_MEMORY_SAFELY(theSubdivisions[i].itsVertices);
		FREE_MEMORY_SAFELY(theSubdivisions[i].itsFaces   );
	}
	FREE_MEMORY_SAFELY(theFaces);
}

static void InitOctahedron(Triangulation *aTriangulation)
{
	unsigned int	i;
	
	//	Triangulate the octahedron as
	//
	//		0-----3-----4
	//		| \ / | \ / |
	//		|  6  |  7  |
	//		| / \ | / \ |
	//		1-----2-----5
	//
	//	Vertices 0 and 4 will coincide in space but carry different
	//	texture coordinates, and similarly for vertices 1 and 5.
	//	Some such duplication of vertices is unavoidable, because otherwise
	//	it would be impossible to map the octahedron (a topological sphere)
	//	into the (u,v) texture plane without self-overlap.

	static const EarthVBOData	v[8] =
								{
									{{ 1.0,  0.0,  0.0,  1.0 }, {0.00, 1.00}},	//	equator "southern"
									{{ 0.0,  1.0,  0.0,  1.0 }, {0.00, 0.00}},	//	equator "southern"
									{{-1.0,  0.0,  0.0,  1.0 }, {0.50, 0.00}},	//	equator shared
									{{ 0.0, -1.0,  0.0,  1.0 }, {0.50, 1.00}},	//	equator shared
									{{ 1.0,  0.0,  0.0,  1.0 }, {1.00, 1.00}},	//	equator "northern"
									{{ 0.0,  1.0,  0.0,  1.0 }, {1.00, 0.00}},	//	equator "northern"
									{{ 0.0,  0.0, -1.0,  1.0 }, {0.25, 0.50}},	//	south pole
									{{ 0.0,  0.0,  1.0,  1.0 }, {0.75, 0.50}}	//	north pole
								};
	static const EarthIBOData	f[8] =
								{
									//	southern faces
									{{6, 0, 1}},
									{{6, 1, 2}},
									{{6, 2, 3}},
									{{6, 3, 0}},

									//	northern faces
									{{7, 5, 4}},
									{{7, 2, 5}},
									{{7, 3, 2}},
									{{7, 4, 3}}
								};

	aTriangulation->itsNumVertices			= 8;
	aTriangulation->itsNumEdges				= 15;
	aTriangulation->itsNumFaces				= 8;
	aTriangulation->itsVertices				= (EarthVBOData *) GET_MEMORY(8 * sizeof(EarthVBOData));
	aTriangulation->itsFaces				= (EarthIBOData *) GET_MEMORY(8 * sizeof(EarthIBOData));

	GEOMETRY_GAMES_ASSERT(	aTriangulation->itsVertices != NULL
						 && aTriangulation->itsFaces	!= NULL,
					"Failed to allocate memory for initial icosahedron.");

	for (i = 0; i < 8; i++)
		aTriangulation->itsVertices[i] = v[i];

	for (i = 0; i < 8; i++)
		aTriangulation->itsFaces[i] = f[i];
}

static void SubdivideTriangulation(
	Triangulation	*aTriangulation,
	Triangulation	*aSubdivision)
{
	//	Subdivide the triangulation, replacing
	//	each old triangle with four new ones.
	//
	//		        /\				//
	//		       /  \				//
	//		      /____\			//
	//		     /\    /\			//
	//		    /  \  /  \			//
	//		   /____\/____\			//
	//

	unsigned short	v0,
					v1,
					*v,
					vv[3],
					*theTable		= NULL,
					theVertexCount;
	unsigned int	i,
					j,
					k;
	
	aSubdivision->itsNumVertices	= aTriangulation->itsNumVertices + aTriangulation->itsNumEdges;
	aSubdivision->itsNumEdges		= 2 * aTriangulation->itsNumEdges + 3 * aTriangulation->itsNumFaces;
	aSubdivision->itsNumFaces		= 4 * aTriangulation->itsNumFaces;
	aSubdivision->itsVertices		= (EarthVBOData *) GET_MEMORY(aSubdivision->itsNumVertices * sizeof(EarthVBOData));
	aSubdivision->itsFaces			= (EarthIBOData *) GET_MEMORY(aSubdivision->itsNumFaces    * sizeof(EarthIBOData));
	GEOMETRY_GAMES_ASSERT(	aSubdivision->itsVertices	!= NULL
						 && aSubdivision->itsFaces		!= NULL,
					"Failed to allocate memory for new subdivision.");

	//	Copy the vertices from the previous level.
	for (i = 0; i < aTriangulation->itsNumVertices; i++)
		aSubdivision->itsVertices[i] = aTriangulation->itsVertices[i];
	theVertexCount = aTriangulation->itsNumVertices;

	//	Create one new vertex on each edge.
	//	Make a table to index them, so two triangles sharing
	//	an edge can share the same vertex.  (The memory for the
	//	table grows as the square of the number of old vertices.
	//	For modest triangulations this won't be a problem.
	//	For large triangulations a fancier algorithm could be used.)

	//	In spirit theTable is a 2-dimensional array.
	//	But such an array is a hassle to allocate in C,
	//	so instead we just replace theTable[i][j] with
	//	theTable[i*aTriangulation->itsNumVertices + j].
	theTable = (unsigned short *) GET_MEMORY(aTriangulation->itsNumVertices * aTriangulation->itsNumVertices * sizeof(unsigned short));
	GEOMETRY_GAMES_ASSERT(	theTable != NULL,
							"Failed to allocate temporary table for new subdivision.");

	for (i = 0; i < aTriangulation->itsNumVertices; i++)
		for (j = 0; j < aTriangulation->itsNumVertices; j++)
			theTable[i*aTriangulation->itsNumVertices + j] = 0xFFFF;

	for (i = 0; i < aTriangulation->itsNumFaces; i++)
	{
		for (j = 0; j < 3; j++)
		{
			v0 = aTriangulation->itsFaces[i].vtx[   j   ];
			v1 = aTriangulation->itsFaces[i].vtx[(j+1)%3];

			if (theTable[v0*aTriangulation->itsNumVertices + v1] == 0xFFFF)
			{
				GEOMETRY_GAMES_ASSERT(	theVertexCount < aSubdivision->itsNumVertices,
										"Internal error #1 in SubdivideTriangulation.");
				
				//	The new vertex will be midway between vertices v0 and v1,
				//	as computed directly on the octahedron (not on the sphere).
				for (k = 0; k < 4; k++)
					aSubdivision->itsVertices[theVertexCount].pos[k]
						= 0.5 * (aSubdivision->itsVertices[v0].pos[k] + aSubdivision->itsVertices[v1].pos[k]);
				for (k = 0; k < 2; k++)
					aSubdivision->itsVertices[theVertexCount].tex[k]
						= 0.5 * (aSubdivision->itsVertices[v0].tex[k] + aSubdivision->itsVertices[v1].tex[k]);

				theTable[v0*aTriangulation->itsNumVertices + v1] = theVertexCount;
				theTable[v1*aTriangulation->itsNumVertices + v0] = theVertexCount;

				theVertexCount++;
			}
		}
	}
	GEOMETRY_GAMES_ASSERT(	theVertexCount == aSubdivision->itsNumVertices,
							"Internal error #2 in SubdivideTriangulation.");

	//	Create the new faces.
	for (i = 0; i < aTriangulation->itsNumFaces; i++)
	{
		//	The old vertices incident to this face will be v[0], v[1] and v[2].
		v = aTriangulation->itsFaces[i].vtx;

		//	The new vertices -- which sit at the midpoints of the old edges --
		//	will be vv[0], vv[1], and vv[2].
		//	Each vv[j] sits opposite the corresponding v[j].
		for (j = 0; j < 3; j++)
			vv[j] = theTable[v[(j+1)%3]*aTriangulation->itsNumVertices + v[(j+2)%3]];

		//	Create the new faces.

		aSubdivision->itsFaces[4*i + 0].vtx[0] = vv[0];
		aSubdivision->itsFaces[4*i + 0].vtx[1] = vv[1];
		aSubdivision->itsFaces[4*i + 0].vtx[2] = vv[2];

		aSubdivision->itsFaces[4*i + 1].vtx[0] = v[0];
		aSubdivision->itsFaces[4*i + 1].vtx[1] = vv[2];
		aSubdivision->itsFaces[4*i + 1].vtx[2] = vv[1];

		aSubdivision->itsFaces[4*i + 2].vtx[0] = v[1];
		aSubdivision->itsFaces[4*i + 2].vtx[1] = vv[0];
		aSubdivision->itsFaces[4*i + 2].vtx[2] = vv[2];

		aSubdivision->itsFaces[4*i + 3].vtx[0] = v[2];
		aSubdivision->itsFaces[4*i + 3].vtx[1] = vv[1];
		aSubdivision->itsFaces[4*i + 3].vtx[2] = vv[0];
	}

	FREE_MEMORY_SAFELY(theTable);
}


static void ProjectToSphere(
	Triangulation	*aTriangulation)
{
	unsigned int	i,
					j;
	double			theLengthSquared,
					theFactor;

	for (i = 0; i < aTriangulation->itsNumVertices; i++)
	{
		theLengthSquared = 0.0;
		for (j = 0; j < 3; j++)		//	ignore fourth coordinate
			theLengthSquared += aTriangulation->itsVertices[i].pos[j] * aTriangulation->itsVertices[i].pos[j];

		if (theLengthSquared > 0.3)	//	theLengthSquared should always be at least 1/3
		{
			theFactor = EARTH_RADIUS / sqrt(theLengthSquared);
			for (j = 0; j < 3; j++)	//	ignore fourth coordinate
				aTriangulation->itsVertices[i].pos[j] *= theFactor;
		}
	}
}


void MakeEarthVAO(
	GLuint	aVertexArrayName,
	GLuint	aVertexBufferName,
	GLuint	anIndexBufferName)
{
	glBindVertexArray(aVertexArrayName);

		glBindBuffer(GL_ARRAY_BUFFER, aVertexBufferName);

			glEnableVertexAttribArray(ATTRIBUTE_POSITION);
			glVertexAttribPointer(ATTRIBUTE_POSITION,  4, GL_FLOAT, GL_FALSE, sizeof(EarthVBOData), (void *)offsetof(EarthVBOData, pos));

			glEnableVertexAttribArray(ATTRIBUTE_TEX_COORD);
			glVertexAttribPointer(ATTRIBUTE_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(EarthVBOData), (void *)offsetof(EarthVBOData, tex));

			glDisableVertexAttribArray(ATTRIBUTE_COLOR);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndexBufferName);

	glBindVertexArray(0);
}

void BindEarthVAO(
	GLuint	aVertexArrayName)
{
	glBindVertexArray(aVertexArrayName);
}

void DrawEarthVAO(
	GLuint		anEarthTexture,
	Honeycomb	*aHoneycomb,
	Matrix		*aWorldPlacement,	//	the world's placement in eye space
	Matrix		*anEarthPlacement)	//	the Earth's placement in the Dirichlet domain
{
	ImageParity		thePartialParity;
	unsigned int	theLevel,
					i;
	Matrix			*theDirichletPlacement;	//	the (translated) Dirichlet domain's placement in world space
	double			theModelViewMatrix[4][4];

	if (aHoneycomb == NULL)
		return;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glBindTexture(GL_TEXTURE_2D, anEarthTexture);
	glVertexAttrib4fv(ATTRIBUTE_COLOR, (float [4]) PREMULTIPLY_RGBA(1.0, 1.0, 1.0, 1.0));

	//	Compose the parity of the Earth's placement in the Dirichlet domain
	//	with the parity of the world's placement in eye space.  
	//	Later on, as we traverse the tiling, we'll factor in the parity 
	//	of the Dirichlet domain's placement in world space, which may 
	//	be negative for some tiles and positive for others.
	thePartialParity = (aWorldPlacement->itsParity == anEarthPlacement->itsParity
						? ImagePositive : ImageNegative);

	//	Start at the best level of detail.
	theLevel = NUM_REFINEMENTS - 1;
	
	//	Draw the spinning Earths in near-to-far order.
	for (i = 0; i < aHoneycomb->itsNumVisibleCells; i++)
	{
		//	Each element of the tiling group defines
		//	a placement of the Dirichlet domain in world space.
		theDirichletPlacement = &aHoneycomb->itsVisibleCells[i]->itsMatrix;

		//	In the spherical case, stick with the best level of detail
		//	for the whole drawing, because the number of cells is typically
		//	not too large, and the spinning Earths near the antipodal point
		//	appear large and require best quality.
		//
		//	In the flat and hyperbolic cases, decrease the level-of-detail
		//	based on the number of translates rendered so far, to produce 
		//	a reasonable image while keeping the workload under control.
		//	In the flat case, this also insures that the image quality
		//	does not depend on the scale of the manifold.
		if (theDirichletPlacement->m[3][3] >= 1.0)	//	flat or hyperbolic
		{
			//	Gradually decrease theLevel as we go.
			//	The current implementation starts at level 4, then...
			if
			(
				i ==    1	//	...drops down to level 3
			 || i ==   64	//	...drops down to level 2
			 || i ==  256	//	...drops down to level 1
			)				//	Level 0 remains unused because it's too coarse.
			{
				theLevel--;
				if (theLevel >= NUM_REFINEMENTS)	//	unnecessary but safe guard against underflow
					theLevel = 0;
			}
		}
		
		//	Let front faces wind counterclockwise (resp. clockwise)
		//	when the Earth's placement in eye space preserves (resp. reverses) parity.
		glFrontFace(theDirichletPlacement->itsParity == thePartialParity ? GL_CCW : GL_CW);

		//	Compose anEarthPlacement, theDirichletPlacement and aWorldPlacement,
		//	and send the result to the shader.
		Matrix44Product(anEarthPlacement->m, theDirichletPlacement->m, theModelViewMatrix);
		Matrix44Product(theModelViewMatrix,  aWorldPlacement->m,       theModelViewMatrix);
		SendModelViewMatrixToShader(theModelViewMatrix);

		//	Draw.
		glDrawElements(	GL_TRIANGLES,
						3 * gNumEarthFaces[theLevel],
						GL_UNSIGNED_SHORT,
						(void *)( gStartEarthFaces[theLevel] * sizeof(EarthIBOData) ));
	}
}
