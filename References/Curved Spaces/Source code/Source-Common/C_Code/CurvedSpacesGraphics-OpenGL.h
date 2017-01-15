//	CurvedSpacesGraphics-OpenGL.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#include "GeometryGames-OpenGL.h"
#include "CurvedSpaces-Common.h"

//	Assign the vertex attributes to well-defined locations in the vertex shader.
//
//	Desktop OpenGL promises at least 16 vertex attribute locations.
//	OpenGL ES 3.0 also promises 16.  OpenGL ES 2.0 promises only 8,
//	but the PowerVR SGX535 (and newer) provide 16 even with OpenGL ES 2.0,
//	so all iOS 7 compatible iDevices provide 16 vertex attribute locations.
//
//	Under the old glBegin/glEnd system, setting colors, texture coordinates, etc., 
//	would merely set those values, but setting the vertex position was special
//	in that it would also create the vertex.  This legacy convention persists
//	in a modified form:  vertex attribute 0 is special, and must be read
//	from an enabled vertex attribute array.  So, for example, if you call glDrawArrays() 
//	with an enabled array of values for vertex attribute 1, but with a constant value 
//	assigned to vertex attribute 0, nothing will get drawn.  That's not very logical, 
//	but that's how it works (at least under OpenGL 2 on my iMac's Radeon X1600 -- 
//	maybe future OpenGL drivers will abandon that legacy convention).
//
#define ATTRIBUTE_POSITION			0
#define ATTRIBUTE_TEX_COORD			1
#define ATTRIBUTE_COLOR				2
#define ATTRIBUTE_MV_MATRIX_ROW_0	3
#define ATTRIBUTE_MV_MATRIX_ROW_1	4
#define ATTRIBUTE_MV_MATRIX_ROW_2	5
#define ATTRIBUTE_MV_MATRIX_ROW_3	6

//	Keep an array of shader programs, each referenced by a GLuint
//	that glCreateProgram() provides to refer to the given program.
//	Each program contains a vertex shader and a fragment shader.
//	Here we define humanly meaningful synonyms for the array indices {0, 1, 2, … }.
typedef enum
{
	ShaderSph = 0,
	ShaderEuc,
	ShaderHyp,
	NumShaders
} ShaderIndex;

//	Keep an array of texture "names", each "name" being a GLuint
//	that glGenTextures() generates to refer to the given texture.  
//	Here we define humanly meaningful synonyms for the array indices {0, 1, 2, … }.
typedef enum
{
	TextureWallPaper = 0,
	TextureWallWood,
	TextureEarth,
	TextureGalaxy,
	TextureGyroscope,
	TextureObserver,
	TextureVertexFigures,
	TextureClifford,
	NumTextures
} TextureIndex;

//	Keep two parallel arrays of Vertex Buffer Object (VBO) names, one for 
//	the vertex buffer proper and the other for the so-called index buffer.
//	Each "name" is a GLuint that glGenBuffers() generates to refer to the given VBO.
//	Here we define humanly meaningful synonyms for the array indices {0, 1, 2, … }.
typedef enum
{
	VertexBufferDirichlet = 0,
	VertexBufferEarth,
	VertexBufferGalaxy,
	VertexBufferGyroscope,
	VertexBufferObserver,
	VertexBufferVertexFigures,
	VertexBufferClifford,
#ifdef HANTZSCHE_WENDT_AXES
	VertexBufferHantzscheWendt,
#endif
	NumVertexBuffers
} VertexBufferIndex;

//	Keep an array of Vertex Array Object (VAO) "names".
//	Each "name" is a GLuint that glGenVertexArrays() generates to refer to the given VAO.
//	Here we define humanly meaningful synonyms for the array indices {0, 1, 2, … }.
typedef enum
{
	VertexArrayObjectDirichlet = 0,
	VertexArrayObjectEarth,
	VertexArrayObjectGalaxy,
	VertexArrayObjectGyroscope,
	VertexArrayObjectObserver,
	VertexArrayObjectVertexFigures,
	VertexArrayObjectClifford,
#ifdef HANTZSCHE_WENDT_AXES
	VertexArrayObjectHantzscheWendt,
#endif
	NumVertexArrayObjects
} VertexArrayObjectIndex;

//	Keep an array of query "names", each "name" being a GLuint 
//	that glGenQueries() generates to refer to the given query.
//	Here we define humanly meaningful synonyms for the array indices {0, 1, 2, … }.
typedef enum
{
	QueryTotalRenderTime = 0,
	NumQueries
} QueryIndex;


struct GraphicsDataGL
{
	//	Have the various OpenGL elements been pre-initialized?
	bool	itsPreparedGLVersion,
			itsPreparedShaders,
			itsPreparedTextures,
			itsPreparedVBOs,
			itsPreparedVAOs,
			itsPreparedQueries;
	
	//	Whenever itsCurrentAperture changes,
	//	we must recompute the Dirichlet VBO.
	double	itsDirichletVBOAperture;
	
	//	OpenGL shaders, textures, vertex buffers, etc.
	GLuint	itsShaderPrograms[NumShaders],
			itsTextureNames[NumTextures],
			itsVertexBufferNames[NumVertexBuffers],
			itsIndexBufferNames [NumVertexBuffers],
			itsVertexArrayNames[NumVertexArrayObjects],
			itsQueryNames[NumQueries];
};

#endif	//	SUPPORT_OPENGL
