//	GeometryGames-OpenGL.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#ifdef SUPPORT_OPENGL

//	On MacOS,
//		stdlib.h includes Availability.h
//		which includes AvailabilityInternal.h
//		which defines __MAC_OS_X_VERSION_MIN_REQUIRED
//		which is the preferred way to check whether
//			the code is compiling on MacOS.
#include <stdlib.h>


//	On MacOS or Win32, compile for desktop OpenGL.
#if defined(__WIN32__) || defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
#define SUPPORT_DESKTOP_OPENGL
#endif

//	On iOS or Android, compile for OpenGL ES 2.
#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) || defined(__ANDROID__)
#define SUPPORT_OPENGL_ES
#endif


//	Include appropriate OpenGL header files.

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	iOS - OpenGL ES 2
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "ExtensionsForOpenGLES.h"
#endif

#ifdef __ANDROID__						//	Android - OpenGL ES 3

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

//	The Android versions of the Geometry Games apps need no extensions.
//	The GLES3 code gets them as core functionality, while
//	the GLES2 code doesn't use them at all.
//
//	If I ever need to use them in Android, I'll need to add the line
//
//		LOCAL_SRC_FILES		+= ../../../../Shared/GL3/GLES/ExtensionsForOpenGLES.c
//
//	to each project's Android.mk, and also uncomment the following:
//#include "ExtensionsForOpenGLES.h"

#endif	//	__ANDROID__

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED	//	Mac OS
#define GL3_PROTOTYPES
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif

#ifdef __WIN32__						//	Windows
#include "GeometryGames-Win32-OpenGLEntryPoints.h"
#endif


#include "GeometryGamesUtilities-Common.h"


//	The GraphicsDataGL (like the ModelData) will be different for each application.
//	All the shared code needs to know is that the GraphicsDataGL is a structure.
typedef struct GraphicsDataGL GraphicsDataGL;

//	The shared code in GeometryGames-OpenGL.c accepts pointers
//	to ModelData and GraphicsDataGL structures, and passes them through
//	to an application-defined RenderFunction.
typedef ErrorText (*RenderFunction)(ModelData *md, GraphicsDataGL *gd, unsigned int aViewWidthPx, unsigned int aViewHeightPx, unsigned int *anElapsedTime);


//	OpenGL-related definitions shared by all Geometry Games software
//	(all application, all platforms)

//	For binding vertex attribute locations.  For example,
//
//		itsIndex = ATTRIBUTE_POSITION
//		itsName  = "atrPosition"
//
typedef struct
{
	GLuint			itsIndex;
	const GLchar	*itsName;
} VertexAttributeBinding;


//	OpenGL-related functions shared by all Geometry Games software
//	(all applications, all platforms)

extern ErrorText		ConfirmOpenGLVersion(void);
extern unsigned int		SizeOfGraphicsDataGL(void);

//	in <ProgramName>Init.c
extern void				ZeroGraphicsDataGL(GraphicsDataGL *gd);
extern ErrorText		SetUpGraphicsAsNeeded(ModelData *md, GraphicsDataGL *gd);
extern void				ShutDownGraphicsAsNeeded(ModelData *md, GraphicsDataGL *gd);

extern ErrorText		SetUpOneShaderProgram(GLuint *aShaderProgram,
							const Char16 *aVertexShaderFileName, const Char16 *aFragmentShaderFileName,
							unsigned int aNumVertexAttributeBindings,
							const VertexAttributeBinding *someVertexAttributeBindings,
							const GLchar *aCustomPrefix);

extern void				SetUpOneTexture(GLuint *aTextureName, const Char16 *aTextureFileName,
							GLint aWrapMode, GLint aMinificationMode,
							AnisotropicMode anAnisotropicMode, GreyscaleMode aGreyscaleMode,
							TextureFormat aTextureFormat, ErrorText *aFirstError);

extern ErrorText		RenderToBuffer(
							ModelData *md, GraphicsDataGL *gd,
							bool aMultisampleFlag, bool aDepthBufferFlag, 
							RenderFunction aRenderFunction, 
							unsigned int aViewWidthPx, unsigned int aViewHeightPx, 
							PixelRGBA *somePixels);

extern VersionNumber	GetVersionNumber(GLenum aName);
extern ErrorText		GetErrorString(void);
#ifdef DEBUG
extern void				GeometryGamesDebugMessage(const GLchar *message);
#endif

//	in <ProgramName>Graphics-OpenGL.c
extern ErrorText		Render(ModelData *md, GraphicsDataGL *gd, unsigned int aViewWidthPx, unsigned int aViewHeightPx, unsigned int *anElapsedTime);


//	Utility functions with platform-independent OpenGL-based implementations
//	appear in GeometryGames-OpenGL.c.
extern void				SetTextureImage(GLuint aTextureName,
							unsigned int aWidth, unsigned int aHeight, unsigned int aDepth,
							Byte *aPixelArray);

//	Utility functions with platform-dependent implementations
//	appear in GeometryGamesUtilities-iOS|Mac|Win.c.
extern void				SetAlphaTextureFromString(GLuint aTextureName, const Char16 *aString,
							unsigned int aWidthPx, unsigned int aHeightPx,
							const Char16 *aFontName, unsigned int aFontSize, unsigned int aFontDescent,
							bool aCenteringFlag, unsigned int aMargin, ErrorText *aFirstError);

#endif	//	SUPPORT_OPENGL
