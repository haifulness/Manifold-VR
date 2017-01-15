//	GeometryGames-OpenGL.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#include "GeometryGames-OpenGL.h"
#include "GeometryGamesLocalization.h"	//	needed only for "missing shaders" error message
#include <string.h>	//	for strcmp()
#include <stdio.h>	//	for sscanf() and snprintf()


static bool				ExtensionIsAvailable(const char *anExtensionName);
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	compiling for iOS - OpenGL ES 2
static bool				WholeWordSubstring(const char *aString, const char *aPotentialSubstring);
#endif
#if defined(__WIN32__) && defined(DEBUG)
static void APIENTRY	GeometryGamesDebugCallbackAMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam);
static void APIENTRY	GeometryGamesDebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam);
#endif


#ifdef __APPLE__
#pragma mark ConfirmOpenGLVersion
#endif


ErrorText ConfirmOpenGLVersion(void)
{
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED	//	compiling for iOS - OpenGL ES 2
	
	//	Which extensions are present?
	
	//	From iOS 4.2 (and maybe earlier) onward, multisample framebuffers
	//	are available on all iOS devices.
	//
	//	The file ExtensionsForOpenGLES.c provides a wrapper glRenderbufferStorageMultisample()
	//	around glRenderbufferStorageMultisampleAPPLE(), and similarly provides a wrapper
	//	glBlitFramebuffer() around glResolveMultisampleFramebufferAPPLE().
	//
	GEOMETRY_GAMES_ASSERT(
		ExtensionIsAvailable("GL_APPLE_framebuffer_multisample"),
		"GL_APPLE_framebuffer_multisample not found");

	//	OpenGL ES 3 supports instanced arrays as a core feature,
	//	and is available on all devices with Apple A7 or newer processors.
	//	OpenGL ES 2 does not support instanced arrays as a core feature,
	//	but iOS provides the OpenGL ES 2 extension GL_EXT_instanced_arrays
	//	for all iOS7-capable devices (SGX 535, 543 and 554; Apple A7, A8, A9).
	//	When running on iOS, all Geometry Games apps currently use OpenGL ES 2.
	//	The file ExtensionsForOpenGLES.c provides wrappers around
	//	the extension functions' names.  That is, glVertexAttribDivisor()
	//	wraps glVertexAttribDivisorEXT() etc.
	//
	GEOMETRY_GAMES_ASSERT(
		ExtensionIsAvailable("GL_EXT_instanced_arrays"),
		"GL_EXT_instanced_arrays not found");

#elif defined(__ANDROID__)						//	compiling for Android
	
	//	Insist on OpenGL ES 3.0 or newer to get vertex array objects (VAO).
	//	While some OpenGL ES 2.x devices provide the GL_OES_vertex_array_object
	//	extension, there's no way for our app's manifest to request it.
	//	Also, some OpenGL ES 2.x devices have buggy GL_OES_vertex_array_object
	//	implementations.  So stick with OpenGL ES 3.0 for best reliability.
	//
	//	Note:  The tag
	//
	//		<uses-feature android:glEsVersion="0x00030000" android:required="true" />
	//
	//	in AndroidManifest.xml is informational only.  The Google Play store
	//	will use it to offer the app only to people with suitable devices.
	//	But if someone gets the app from some other source, Android will
	//	ignore the <uses-feature … > tags and attempt to run the app no matter what.
	//	So it's essential that we check the OpenGL version number here.
	//
	if (GetVersionNumber(GL_VERSION) < VERSION_NUMBER(3, 0))
	{
		//	Google Play will offer the app only to users
		//	whose devices support OpenGL ES 3.0 or newer,
		//	so the only way we can reach this point is
		//	if the user installs the app from some other source.
		return u"Your device's graphics processor lacks OpenGL ES 3.0 or newer, so this app cannot run.  Sorry.";
	}

#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)	//	compiling for Mac OS

	if (GetVersionNumber(GL_VERSION) < VERSION_NUMBER(3, 3))	//	OpenGL 3.2 or earlier
	{
		//	The code should never reach this point, because all hardware
		//	that runs Mac OS X 10.9 or later should provide OpenGL 3.3 or later.
		return u"OpenGL 3.3 not found.  (This should never happen on Mac OS X 10.9 or later.)";
	}

#elif defined(__WIN32__)	//	compiling for Windows

#ifdef DEBUG
	FILE	*fp = NULL;

	fp = fopen("debug log.txt", "a");
	if (fp != NULL)
	{
		fprintf(fp, "\nversion:  %s\nrenderer: %s\nvendor:   %s\n",
			glGetString(GL_VERSION),
			glGetString(GL_RENDERER),
			glGetString(GL_VENDOR));
		fclose(fp);
	}
#endif

	//	The setup code in GeometryGames-Win32-WndProc.c's SetUpDrawingPanel()
	//	will give us either an OpenGL 3.3 or newer core context (forward-compatible),
	//	or fail.  Check the OpenGL version number here anyhow, just to be safe.
	//
	if (GetVersionNumber(GL_VERSION) < VERSION_NUMBER(3, 3))	//	OpenGL 3.2 or earlier
	{
		//	In theory we could run on OpenGL 2.1 with the VAO extension, but
		//	in practice the drivers are often buggy, especially on Intel HD Graphics.
		//	Better to refer the user to the archived OpenGL 2 version of the program,
		//	which doesn't use VAOs and works fine, even on Intel HD Graphics.

		return GetLocalizedText(u"ErrorPleaseUseGL2");
	}

#ifdef DEBUG
	if (ExtensionIsAvailable("GL_AMD_debug_output"))
		glDebugMessageCallbackAMD(GeometryGamesDebugCallbackAMD, NULL);
	else
	if (ExtensionIsAvailable("GL_ARB_debug_output"))
		glDebugMessageCallbackARB(GeometryGamesDebugCallbackARB, NULL);
	else
		GeometryGamesDebugMessage("GL_..._debug_output is not available.");
#endif

#else
#error Compile-time error:  no OpenGL support.  
#endif

	//	Did any OpenGL errors occur?
	return GetErrorString();
}


//	As of April 2016, the Geometry Games require OpenGL ES 3.0
//	or desktop OpenGL 3.3 on all platforms except iOS,
//	so on all platforms except iOS we may use the easier extension checking mechanism.

#if ! defined(__IPHONE_OS_VERSION_MIN_REQUIRED)

static bool ExtensionIsAvailable(
	const char	*anExtensionName)	//	null-terminated ASCII string
{
	GLuint		theNumExtensions	= 0,
				i;

	glGetIntegerv(GL_NUM_EXTENSIONS, (GLint *)&theNumExtensions);
	for (i = 0; i < theNumExtensions; i++)
		if (strcmp(anExtensionName, (const char *)glGetStringi(GL_EXTENSIONS, i)) == 0)
			return true;

	return false;
}

#else	//	compiling for iOS - OpenGL ES 2

static bool ExtensionIsAvailable(
	const char	*anExtensionName)	//	null-terminated ASCII string
{
	const char	*theExtensionsString	= NULL;

	theExtensionsString = (const char *) glGetString(GL_EXTENSIONS);
	if (theExtensionsString != NULL)
		return WholeWordSubstring(theExtensionsString, anExtensionName);
	else
		return false;
}

static bool WholeWordSubstring(
	const char	*aString,				//	null-terminated
	const char	*aPotentialSubstring)	//	null-terminated
{
	const char	*thePotentialMatch,
				*s,
				*p;
	bool		theMatchFlag;

	//	Does aPotentialSubstring occur as a complete space-delimited word in aString?

	//	Irrelevant technical detail:  an empty aPotentialSubstring
	//	(consisting of a terminating zero alone) counts as a substring
	//	of any string, even an empty one.

	//	Consider the first character in each space-delimited word of aString
	//	as the starting point for a potential match.
	thePotentialMatch = aString;
	while (*thePotentialMatch == ' ')
		thePotentialMatch++;
	do
	{
		//	Compare each character in aPotentialSubstring
		//	(excluding the terminating zero) to thePotentialMatch.
		s = thePotentialMatch;
		p = aPotentialSubstring;
		theMatchFlag = true;
		while (theMatchFlag && *p != 0)
		{
			if (*p++ != *s++)
				theMatchFlag = false;
		}

		//	If a space or terminating 0 follows a match, return true.
		if (theMatchFlag
		 && (*s == ' ' || *s == 0))
			return true;

		//	Skip to the beginning of the next whole word in thePotentialMatch.
		//		Skip by non-spaces.
		while (*thePotentialMatch != ' ' && *thePotentialMatch != 0)
			thePotentialMatch++;
		//		Skip by intervening spaces.
		while (*thePotentialMatch == ' ')
			thePotentialMatch++;

		//	Continue with the loop to examine the next whole word (if any).

	} while (*thePotentialMatch != 0);

	return false;
}

#endif	//	two versions of ExtensionIsAvailable()


#if defined(__WIN32__) && defined(DEBUG)

static void APIENTRY GeometryGamesDebugCallbackAMD(
	GLuint			id,
	GLenum			category,
	GLenum			severity,
	GLsizei			length,
	const GLchar	*message,
	GLvoid			*userParam)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(category);
	UNUSED_PARAMETER(severity);
	UNUSED_PARAMETER(length);
	UNUSED_PARAMETER(userParam);

	GeometryGamesDebugMessage(message);
}

static void APIENTRY GeometryGamesDebugCallbackARB(
	GLenum			source,
	GLenum			type,
	GLuint			id,
	GLenum			severity,
	GLsizei			length,
	const GLchar	*message,
	GLvoid			*userParam)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(type);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(severity);
	UNUSED_PARAMETER(length);
	UNUSED_PARAMETER(userParam);
	
	GeometryGamesDebugMessage(message);
}

#endif


#ifdef __APPLE__
#pragma mark -
#pragma mark shaders
#endif


ErrorText SetUpOneShaderProgram(
	GLuint							*aShaderProgram,				//	output
	const Char16					*aVertexShaderFileName,			//	input,  zero-terminated UTF-16 string
	const Char16					*aFragmentShaderFileName,		//	input,  zero-terminated UTF-16 string
	unsigned int					aNumVertexAttributeBindings,	//	input
	const VertexAttributeBinding	*someVertexAttributeBindings,	//	input
	const GLchar					*aCustomPrefix)					//	input,  zero-terminated ASCII string,  may be NULL
{
	ErrorText		theErrorMessage					= NULL;
#ifdef SUPPORT_DESKTOP_OPENGL
//	VersionNumber	theShadingLanguageMaxVersion	= VERSION_NUMBER(0,0);
#endif
	unsigned int	theVertexShaderSourceLength		= 0,
					theFragmentShaderSourceLength	= 0;
	Byte			*theVertexShaderSource			= NULL,
					*theFragmentShaderSource		= NULL;
	const GLchar	*theVertexShaderPrefix			= NULL,
					*theFragmentShaderPrefix		= NULL,
					*theCustomPrefix				= NULL;
	GLuint			theVertexShader					= 0,
					theFragmentShader				= 0;
	GLint			theSuccessFlag;
	unsigned int	i;

	GLchar			theInfoLogAsASCII[1024] = {0};
	static Char16	theInfoLogAsUTF16[1024] = {0};	//	Static buffer persists after this function returns.

	//	Release any pre-existing shader.
	glDeleteProgram(*aShaderProgram);
	*aShaderProgram = 0;

#ifdef SUPPORT_DESKTOP_OPENGL
	//	Which version of the shading language is available?
//	theShadingLanguageMaxVersion = GetVersionNumber(GL_SHADING_LANGUAGE_VERSION);
#endif

	//	Load the vertex shader source code.
	if ((theErrorMessage = GetFileContents(	u"Shaders",
											aVertexShaderFileName,
											&theVertexShaderSourceLength,
											&theVertexShaderSource)) != NULL)
		goto CleanUpSetUpOneShaderProgram;
	
	//	Load the fragment shader source code.
	if ((theErrorMessage = GetFileContents(	u"Shaders",
											aFragmentShaderFileName,
											&theFragmentShaderSourceLength,
											&theFragmentShaderSource)) != NULL)
		goto CleanUpSetUpOneShaderProgram;
	
	//	Select appropriate prefixes.
#ifdef SUPPORT_OPENGL_ES

	//	Convert GLSL 1.3 storage qualifiers "in" and "out"
	//	to their GLSL ES 1.0 equivalents "attribute" and "varying".
	theVertexShaderPrefix	= "#version 100\n#define in attribute\n#define out varying\n";
	theFragmentShaderPrefix	= "#version 100\n#define in varying\n#define texture texture2D\nprecision highp float;\n";

#elif defined(SUPPORT_DESKTOP_OPENGL)

	//	OpenGL 3.2 guarantees GLSL 1.5 or later, which is all we need.
	//
	//	If we ever need to accommodate newer versions of GLSL, we can use code like
	//
	//		if (theShadingLanguageMaxVersion >= VERSION_NUMBER(1, 50))	//	OpenGL 3.2
	//		{
	//			theVertexShaderPrefix	= "#version 150\n";
	//			theFragmentShaderPrefix	= "#version 150\n#define gl_FragColor outColor\nout vec4 outColor;\nprecision highp float;\n";
	//		}
	//
	theVertexShaderPrefix	= "#version 150\n";
	theFragmentShaderPrefix	= "#version 150\n#define gl_FragColor outColor\nout vec4 outColor;\nprecision highp float;\n";

#else
#error Compile-time error:  no OpenGL support.  
#endif

	//	Let the caller pass custom #definitions to the shaders,
	//	so separate but similar shaders may share source code.
	if (aCustomPrefix != NULL)
		theCustomPrefix = aCustomPrefix;
	else
		theCustomPrefix = "\n";	//	Avoid an empty string, just to be safe.

	//	Create the vertex shader.
	theVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(	theVertexShader,
					3,
					(const GLchar *[3])	{	theVertexShaderPrefix,
											theCustomPrefix,
											(const GLchar *)theVertexShaderSource	},
					(GLint [3])			{	-1,
											-1,
											theVertexShaderSourceLength			});
	glCompileShader(theVertexShader);
	glGetShaderiv(theVertexShader, GL_COMPILE_STATUS, &theSuccessFlag);
	if ( ! theSuccessFlag )
	{
		glGetShaderInfoLog(theVertexShader, BUFFER_LENGTH(theInfoLogAsASCII), NULL, theInfoLogAsASCII);
		if (UTF8toUTF16(theInfoLogAsASCII, theInfoLogAsUTF16, BUFFER_LENGTH(theInfoLogAsUTF16)))
			theErrorMessage = theInfoLogAsUTF16;
		else
			theErrorMessage = u"Internal error:  failed to convert the vertex shader compiler's info log from ASCII to UTF-16";
		goto CleanUpSetUpOneShaderProgram;
	}

	//	Create the fragment shader.
	theFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(	theFragmentShader,
					3,
					(const GLchar *[3])	{	theFragmentShaderPrefix,
											theCustomPrefix,
											(const GLchar *)theFragmentShaderSource	},
					(GLint [3])			{	-1,
											-1,
											theFragmentShaderSourceLength			});
	glCompileShader(theFragmentShader);
	glGetShaderiv(theFragmentShader, GL_COMPILE_STATUS, &theSuccessFlag);
	if ( ! theSuccessFlag )
	{
		glGetShaderInfoLog(theFragmentShader, BUFFER_LENGTH(theInfoLogAsASCII), NULL, theInfoLogAsASCII);
		if (UTF8toUTF16(theInfoLogAsASCII, theInfoLogAsUTF16, BUFFER_LENGTH(theInfoLogAsUTF16)))
			theErrorMessage = theInfoLogAsUTF16;
		else
			theErrorMessage = u"Internal error:  failed to convert the fragment shader compiler's info log from ASCII to UTF-16";
		goto CleanUpSetUpOneShaderProgram;
	}

	//	Create the program and attach the vertex and fragment shaders.
	*aShaderProgram = glCreateProgram();
	if (*aShaderProgram == 0)
	{
		theErrorMessage = u"Couldn't create *aShaderProgram";
		goto CleanUpSetUpOneShaderProgram;
	}
	glAttachShader(*aShaderProgram, theVertexShader  );
	glAttachShader(*aShaderProgram, theFragmentShader);
	
	//	Assign locations to vertex shader attributes.
	//
	//	It's always safe to bind an attribute name, whether or not 
	//	the given shader uses it.  It's also OK to bind several different names 
	//	to the same location, just so the shader uses at most one of them.
	//
	for (i = 0; i < aNumVertexAttributeBindings; i++)
	{
		glBindAttribLocation(	*aShaderProgram,
								someVertexAttributeBindings[i].itsIndex,
								someVertexAttributeBindings[i].itsName);
	}

	//	Link the program.
	glLinkProgram(*aShaderProgram);
	glGetProgramiv(*aShaderProgram, GL_LINK_STATUS, &theSuccessFlag);
	if ( ! theSuccessFlag )
	{
		glGetProgramInfoLog(*aShaderProgram, BUFFER_LENGTH(theInfoLogAsASCII), NULL, theInfoLogAsASCII);
		if (UTF8toUTF16(theInfoLogAsASCII, theInfoLogAsUTF16, BUFFER_LENGTH(theInfoLogAsUTF16)))
			theErrorMessage = theInfoLogAsUTF16;
		else
			theErrorMessage = u"Internal error:  failed to convert the shader linker's info log from ASCII to UTF-16";
		goto CleanUpSetUpOneShaderProgram;
	}

CleanUpSetUpOneShaderProgram:

	//	Free our private copy of the source code.
	FreeFileContents(&theVertexShaderSourceLength,   &theVertexShaderSource  );
	FreeFileContents(&theFragmentShaderSourceLength, &theFragmentShaderSource);

	//	Release our temporary references (if any) to the vertex, geometry and fragment shaders.
	//	The shader program will keep its own references to the vertex, geometry and fragment shaders,
	//	and will automatically delete them when it itself gets deleted.  
	//	Passing 0 is harmless.
	glDeleteShader(theVertexShader);
	glDeleteShader(theFragmentShader);

	//	If an error occured, delete the shader program (if any).
	if (theErrorMessage != NULL)
	{
		glDeleteProgram(*aShaderProgram);	//	OK to pass 0
		*aShaderProgram = 0;
	}

	return theErrorMessage;
}


#ifdef __APPLE__
#pragma mark -
#pragma mark textures
#endif


//	Constants for the anisotropic texture sampling extension.
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF


void SetUpOneTexture(
	GLuint			*aTextureName,		//	output
	const Char16	*aTextureFileName,	//	zero-terminated UTF-16 string; pass NULL to set up an empty texture
	GLint			aWrapMode,			//	GL_REPEAT or GL_CLAMP_TO_EDGE
	GLint			aMinificationMode,	//	GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_LINEAR
	AnisotropicMode	anAnisotropicMode,
	GreyscaleMode	aGreyscaleMode,		//	ignored when aTextureFileName == NULL
	TextureFormat	aTextureFormat,		//	ignored when aTextureFileName == NULL
	ErrorText		*aFirstError)		//	may be NULL
{
	ErrorText		theErrorMessage		= NULL;
	float			theMaxAnisotropy	= 1.0;
	ImageRGBA		*theImageRGBA		= NULL;
	PixelRGBA		*theSrcPixel		= NULL;
	Byte			*theDstByte			= NULL;
	unsigned int	theCount;

	//	Release any pre-existing texture.
	glDeleteTextures(1, aTextureName);	//	silently ignores *aTextureName == 0
	*aTextureName = 0;
	
	//	Generate a new texture name.
	glGenTextures(1, aTextureName);

	//	Bind the texture.
	glBindTexture(GL_TEXTURE_2D, *aTextureName);

	//	Set wrapping or clamping.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, aWrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, aWrapMode);

	//	Set minification and magnification filters.
	//
	//	On desktop computers, mipmapping looks good and runs quickly.
	//
	//	On iOS devices, mipmapping may slow the frame rate.
	//	According to Section 6.6.1 Trilinear Filtering of the
	//	POWERVR SGX OpenGL ES 2.0 Application Development Recommendations
	//	( http://www.imgtec.com/factsheets/SDK/POWERVR%20SGX.OpenGL%20ES%202.0%20Application%20Development%20Recommendations.1.8f.External.pdf )
	//	the SGX samples a GL_LINEAR_MIPMAP_NEAREST texture in 1 cycle,
	//	but requires 2 cycles for a GL_LINEAR_MIPMAP_LINEAR texture.
	//	This is consistent with my own earlier experience:  
	//	on the PowerVR MBX Lite the torus Jigsaw puzzle's frame rate increased 
	//	from 25-27 fps with GL_LINEAR_MIPMAP_LINEAR to 40-42 fps with GL_LINEAR_MIPMAP_NEAREST.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aMinificationMode	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR			);

	//	Set anisotropic texture filtering.
	//
	//	On desktop computers, anisotropic texture filtering is fast.
	//
	//	On iOS devices, anisotropic texture filtering may slow the frame rate.
	//	For sure it is very expensive on PowerVR MBX  (for example, 
	//	disabling anisotropic texture filtering on the PowerVR MBX
	//	increased the Jigsaw puzzle's frame rate from 40-41 fps 
	//	to the maximum 60 fps -- and it might have gone higher given the chance).
	//	It may be expensive on PowerVR SGX as well (I haven't tested it),
	//	and in any case it goes only as far as 2x anisotropy.
	//	So on iOS leave it off unless absolutely necessary.
	//
	//	Note #1:  It seems that anisotropic texture filtering (GL_EXT_texture_filter_anisotropic)
	//	isn't part of core OpenGL or OpenGL ES because it's covered by a patent,
	//	and people naturally don't want to write any features into the core standard
	//	that would require implementors to pay licensing fees to a patent holder.
	//	(It was later suggested that relaxing the core standard to allow
	//	GL_MAX_TEXTURE_MAX_ANISOTROPY(_EXT) to be 1 would have let implementers
	//	not wanting to pay the licensing fee to nevertheless satisfy the standard,
	//	but that approach was not taken.)
	//
	//	Note #2:  On the one hand, it would be slightly more efficient to cache
	//	the value of ExtensionIsAvailable("GL_EXT_texture_filter_anisotropic").
	//	On the other hand, the overall app logic is simplified by not having
	//	to pass around an extra parameter.
	//
	if (ExtensionIsAvailable("GL_EXT_texture_filter_anisotropic"))
	{
		if (anAnisotropicMode == AnisotropicOn)
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &theMaxAnisotropy);	//	Allow the maximum.
		else
			theMaxAnisotropy = 1.0f;	//	Suppress anisotropic texture filtering.
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, theMaxAnisotropy);
	}
	
	//	If the caller didn't want to load a file, we're done.
	if (aTextureFileName == NULL)
		goto CleanUpSetUpOneTexture;

	//	Read the texture file.
	if ((theErrorMessage = ReadImageRGBA(aTextureFileName, aGreyscaleMode, &theImageRGBA)) != NULL)
		goto CleanUpSetUpOneTexture;
	
	//	What sort of texture does the caller want?
	switch (aTextureFormat)
	{
		case TextureRGBA:

			//	Pass the unmodified pixel data to OpenGL.
			SetTextureImage(*aTextureName,
							theImageRGBA->itsWidth, theImageRGBA->itsHeight, 4, 
							(Byte *) theImageRGBA->itsPixels);

			break;

		case TextureAlpha:

			//	Ignore the red, green and blue bytes, keeping only the alpha bytes.
			//	Note that we condense the array in place -- no new memory gets allocated.
			theSrcPixel =          theImageRGBA->itsPixels;
			theDstByte  = (Byte *) theImageRGBA->itsPixels;
			for (theCount = theImageRGBA->itsWidth * theImageRGBA->itsHeight; theCount-- > 0; )
				*theDstByte++ = (theSrcPixel++)->a;

			//	Pass the alpha values to OpenGL.
			SetTextureImage(*aTextureName,
							theImageRGBA->itsWidth, theImageRGBA->itsHeight, 1,
							(Byte *) theImageRGBA->itsPixels);

			break;
		
		default:
			theErrorMessage = u"Bad value for aTextureFormat";
			goto CleanUpSetUpOneTexture;
	}

CleanUpSetUpOneTexture:

	//	Free the image (if necessary).
	FreeImageRGBA(&theImageRGBA);

	if (theErrorMessage != NULL)
	{
		//	Substitute a pure red texture for the missing one.
		SetTextureImage(*aTextureName, 1, 1, 4, (Byte [4]){0xFF, 0x00, 0x00, 0xFF});
		
		//	Report this error iff an error report is desired
		//	and no earlier error has occurred.
		if (aFirstError != NULL
		 && *aFirstError == NULL)
		{
			*aFirstError = theErrorMessage;
		}

		//	Let the caller push on without the desired texture...
	}
}


void SetTextureImage(
	GLuint			aTextureName,
	unsigned int	aWidth,
	unsigned int	aHeight,
	unsigned int	aDepth,			//	Pass 4 for RGBA or 1 for alpha-only.
	Byte			*aPixelArray)	//	4 bytes per pixel or 1 byte per pixel
{
	unsigned int	theInternalPixelFormat,
					theExternalPixelFormat;

	if (aTextureName == 0
	 || ! IsPowerOfTwo(aWidth )
	 || ! IsPowerOfTwo(aHeight)
	 || (aDepth != 1 && aDepth != 4))
		return;

	//	Select an appropriate pixel format.
	if (aDepth == 4)	//	RGBA texture
	{
		theInternalPixelFormat = GL_RGBA;
		theExternalPixelFormat = GL_RGBA;
	}
	else	//	alpha-only texture
	{
#ifdef SUPPORT_OPENGL_ES

		//	Each texel carries a single value L representing that texel's opacity.
		//	The fragment shader receives the texel as (L,L,L,1);
		//	it reads the first ("red") component and ignores the other three.
		theInternalPixelFormat = GL_LUMINANCE;
		theExternalPixelFormat = GL_LUMINANCE;

#elif defined(SUPPORT_DESKTOP_OPENGL)

		//	OpenGL 3.2 doesn't support internal format GL_ALPHA
		//	or GL_LUMINANCE in a forward-compatible context.
		//	Instead we may create a 1-component texture
		//	with internal format GL_RED, and let the shader
		//	interpret the sample's red component as an opacity.
		theInternalPixelFormat = GL_RED;
		theExternalPixelFormat = GL_RED;

#else
#error Compile-time error:  no OpenGL support.
#endif

	}
	
	glBindTexture(GL_TEXTURE_2D, aTextureName);
	
	//	glTexImage2D()'s default 4-byte row alignment is fine for all RGBA textures
	//	as well as for alpha-only textures with power-of-two width at least 4,
	//	but we must relax the row alignment for alpha-only textures of width 1 or 2.
	if (aDepth == 1 && aWidth < 4)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					theInternalPixelFormat,
					aWidth,
					aHeight,
					0,
					theExternalPixelFormat,
					GL_UNSIGNED_BYTE,
					aPixelArray);

	glGenerateMipmap(GL_TEXTURE_2D);

	//	Restore the default row alignment, if necessary.
	if (aDepth == 1 && aWidth < 4)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}


#ifdef __APPLE__
#pragma mark -
#pragma mark RenderToBuffer
#endif


#ifdef SUPPORT_DESKTOP_OPENGL
#define RENDERBUFFER_RGBA	GL_RGBA
#elif defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
#define RENDERBUFFER_RGBA	GL_RGBA8_OES	//	iOS 4.2 supports the GL_OES_rgb8_rgba8 extension on all devices.
#elif defined(__ANDROID__)
#define RENDERBUFFER_RGBA	GL_RGBA8		//	valid in OpenGL ES 3
#else
#error Compile-time error:  no OpenGL support.
#endif

#ifdef SUPPORT_DESKTOP_OPENGL
//	Some hardware (for example the GeForce 9400M in my MacBook)
//	supports a 32-bit depth buffer but not a 16-bit depth buffer.
//	See comments below re GL_FRAMEBUFFER_UNSUPPORTED.
#define DEPTH_BUFFER_DEPTH	GL_DEPTH_COMPONENT32
#elif defined(SUPPORT_OPENGL_ES)
//	OpenGL ES supports GL_DEPTH_COMPONENT16 and GL_DEPTH_COMPONENT24_OES.
#define DEPTH_BUFFER_DEPTH	GL_DEPTH_COMPONENT16
#else
#error Compile-time error:  no OpenGL support.  
#endif

static ErrorText	RenderToMultisampleBuffer(ModelData *md, GraphicsDataGL *gd, bool aDepthBufferFlag, RenderFunction aRenderFunction, unsigned int aViewWidthPx, unsigned int aViewHeightPx, PixelRGBA *somePixels);
static ErrorText	RenderToPlainBuffer(ModelData *md, GraphicsDataGL *gd, bool aDepthBufferFlag, RenderFunction aRenderFunction, unsigned int aViewWidthPx, unsigned int aViewHeightPx, PixelRGBA *somePixels);


ErrorText RenderToBuffer(
	ModelData		*md,
	GraphicsDataGL	*gd,
	bool			aMultisampleFlag,
	bool			aDepthBufferFlag,
	RenderFunction	aRenderFunction,
	unsigned int	aViewWidthPx,	//	in pixels (not points)
	unsigned int	aViewHeightPx,	//	in pixels (not points)
	PixelRGBA		*somePixels)	//	RGBA with premultiplied alpha, rows go bottom-to-top
{
	if (aMultisampleFlag)
		return RenderToMultisampleBuffer(md, gd, aDepthBufferFlag, aRenderFunction, aViewWidthPx, aViewHeightPx, somePixels);
	else
		return RenderToPlainBuffer(md, gd, aDepthBufferFlag, aRenderFunction, aViewWidthPx, aViewHeightPx, somePixels);
}

static ErrorText RenderToMultisampleBuffer(
	ModelData		*md,
	GraphicsDataGL	*gd,
	bool			aDepthBufferFlag,
	RenderFunction	aRenderFunction,
	unsigned int	aViewWidthPx,	//	in pixels (not points)
	unsigned int	aViewHeightPx,	//	in pixels (not points)
	PixelRGBA		*somePixels)	//	RGBA with premultiplied alpha, rows go bottom-to-top
{
	ErrorText	theError						= NULL;
	GLuint		theSavedFramebuffer,	//	will be 0 on MacOS and Win32, but non-zero on iOS
				theMaxRenderbufferSize,
				theNumSamples,
				theMultisampleFramebuffer		= 0,
				theMultisampleColorRenderbuffer	= 0,
				theMultisampleDepthRenderbuffer	= 0,
				theResolveFramebuffer			= 0,
				theResolveColorRenderbuffer		= 0;
	
	char			theErrorTextUTF8[128]	= "";
	static Char16	theErrorTextUTF16[128]	= u"";	//	Static buffer persists after this function returns.

	glGetIntegerv(GL_FRAMEBUFFER_BINDING,	(int *)&theSavedFramebuffer		);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,	(int *)&theMaxRenderbufferSize	);
	glGetIntegerv(GL_MAX_SAMPLES,			(int *)&theNumSamples			);

	if (aViewWidthPx  == 0
	 || aViewHeightPx == 0
	 || aViewWidthPx  > theMaxRenderbufferSize
	 || aViewHeightPx > theMaxRenderbufferSize)
	{
		snprintf(
			theErrorTextUTF8, BUFFER_LENGTH(theErrorTextUTF8),
			"The Copy and Save commands cannot create an image larger than %d × %d.",
			theMaxRenderbufferSize, theMaxRenderbufferSize);
		if (UTF8toUTF16(theErrorTextUTF8, theErrorTextUTF16, BUFFER_LENGTH(theErrorTextUTF16)))
			theError = theErrorTextUTF16;
		else
			theError = u"Internal error:  failed to convert error message from UTF-8 to UTF-16";
		goto CleanUpRenderToMultisampleBuffer;
	}

	glGenFramebuffers(1, &theMultisampleFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, theMultisampleFramebuffer);

	glGenRenderbuffers(1, &theMultisampleColorRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, theMultisampleColorRenderbuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, theNumSamples, RENDERBUFFER_RGBA, aViewWidthPx, aViewHeightPx);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, theMultisampleColorRenderbuffer);

	if (aDepthBufferFlag)
	{
		glGenRenderbuffers(1, &theMultisampleDepthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, theMultisampleDepthRenderbuffer);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, theNumSamples, DEPTH_BUFFER_DEPTH, aViewWidthPx, aViewHeightPx);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, theMultisampleDepthRenderbuffer);
	}

	//	glCheckFramebufferStatus() may return GL_FRAMEBUFFER_UNSUPPORTED.
	//	According to www.opengl.org/wiki/Framebuffer_Object,
	//
	//		There's one more rule that can trip you up:
	//
	//		• The implementation likes your combination of attached image formats.
	//			(GL_FRAMEBUFFER_UNSUPPORTED when false).
	//
	//		OpenGL allows implementations to state that they do not support 
	//		some combination of image formats for the attached images; 
	//		they do this by returning GL_FRAMEBUFFER_UNSUPPORTED when you attempt 
	//		to use an unsupported format combinaton.
	//
	//		However, the OpenGL specification also requires that implementations 
	//		support certain formats; that is, if you use these formats, 
	//		implementations are forbidden to return GL_FRAMEBUFFER_UNSUPPORTED. 
	//		This list of required formats is also the list of required image formats 
	//		that all OpenGL implementations must support. These are most 
	//		of the useful formats. Basically, don't concern yourself 
	//		with GL_FRAMEBUFFER_UNSUPPORTED too much. Check for it, but you'll 
	//		be fine as long as you stick to the required formats.
	//
	//		Legacy Note: GL_FRAMEBUFFER_UNSUPPORTED was initially, 
	//		in the days of EXT_framebuffer_object, much less forgiving. 
	//		The specification didn't have a list of required image formats. 
	//		Indeed, the only guarantee that the EXT_FBO spec made was that there 
	//		was at least one combination of formats that implementations supported; 
	//		it provided no hints as to what that combination might be. 
	//		The core extension ARB_framebuffer_object does differ 
	//		from the core specification in one crucial way: 
	//		it uses the EXT wording for GL_FRAMEBUFFER_UNSUPPORTED. 
	//		So if you're using 3.0, you don't have to worry much about unsupported. 
	//		If you're using ARB_framebuffer_object, then you should be concerned 
	//		and do appropriate testing.
	//
	//	As a practical matter, my MacBook's GeForce 9400M
	//	fails with GL_FRAMEBUFFER_UNSUPPORTED for GL_DEPTH_COMPONENT16,
	//	but succeeds with GL_FRAMEBUFFER_COMPLETE for GL_DEPTH_COMPONENT32.
	//	So the moral of the story seems to be to stick 
	//	with a 32-bit depth buffer on desktop OpenGL.  
	//	By contrast, OpenGL ES supports GL_DEPTH_COMPONENT16
	//	and GL_DEPTH_COMPONENT24_OES.
	//
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		theError = u"The “multisample framebuffer” is incomplete.  Cannot copy/save image.";
		goto CleanUpRenderToMultisampleBuffer;
	}

	glGenFramebuffers(1, &theResolveFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, theResolveFramebuffer);

	glGenRenderbuffers(1, &theResolveColorRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, theResolveColorRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, RENDERBUFFER_RGBA, aViewWidthPx, aViewHeightPx);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, theResolveColorRenderbuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		theError = u"The “resolve framebuffer” is incomplete.  Cannot copy/save image.";
		goto CleanUpRenderToMultisampleBuffer;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, theMultisampleFramebuffer);
	aRenderFunction(md, gd, aViewWidthPx, aViewHeightPx, NULL);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, theMultisampleFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, theResolveFramebuffer);
	glBlitFramebuffer(	0, 0, aViewWidthPx, aViewHeightPx,
						0, 0, aViewWidthPx, aViewHeightPx,
						GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, theResolveFramebuffer);
	glReadPixels(	0, 0,
					aViewWidthPx, aViewHeightPx,
					GL_RGBA, GL_UNSIGNED_BYTE,
					somePixels);

CleanUpRenderToMultisampleBuffer:

	glDeleteFramebuffers(1, &theMultisampleFramebuffer);
	glDeleteRenderbuffers(1, &theMultisampleColorRenderbuffer);
	if (aDepthBufferFlag)
	{
		glDeleteRenderbuffers(1, &theMultisampleDepthRenderbuffer);
	}
	glDeleteFramebuffers(1, &theResolveFramebuffer);
	glDeleteRenderbuffers(1, &theResolveColorRenderbuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, theSavedFramebuffer);
	
	return theError;
}

static ErrorText RenderToPlainBuffer(
	ModelData		*md,
	GraphicsDataGL	*gd,
	bool			aDepthBufferFlag,
	RenderFunction	aRenderFunction,
	unsigned int	aViewWidthPx,	//	in pixels (not points)
	unsigned int	aViewHeightPx,	//	in pixels (not points)
	PixelRGBA		*somePixels)	//	RGBA with premultiplied alpha, rows go bottom-to-top
{
	ErrorText	theError				= NULL;
	GLuint		theSavedFramebuffer,	//	will be 0 on MacOS and Win32, but non-zero on iOS
				theMaxRenderbufferSize,
				theFramebuffer			= 0,
				theColorRenderbuffer	= 0,
				theDepthRenderbuffer	= 0;
	
	char			theErrorTextUTF8[128]	= "";
	static Char16	theErrorTextUTF16[128]	= u"";	//	Static buffer persists after this function returns.

	glGetIntegerv(GL_FRAMEBUFFER_BINDING,	(int *)&theSavedFramebuffer		);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,	(int *)&theMaxRenderbufferSize	);

	if (aViewWidthPx  == 0
	 || aViewHeightPx == 0
	 || aViewWidthPx  > theMaxRenderbufferSize
	 || aViewHeightPx > theMaxRenderbufferSize)
	{
		snprintf(
			theErrorTextUTF8, BUFFER_LENGTH(theErrorTextUTF8),
			"The Copy and Save commands cannot create an image larger than %d × %d.",
			theMaxRenderbufferSize, theMaxRenderbufferSize);
		if (UTF8toUTF16(theErrorTextUTF8, theErrorTextUTF16, BUFFER_LENGTH(theErrorTextUTF16)))
			theError = theErrorTextUTF16;
		else
			theError = u"Internal error:  failed to convert error message from UTF-8 to UTF-16";
		goto CleanUpRenderToPlainBuffer;
	}

	glGenFramebuffers(1, &theFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, theFramebuffer);

	glGenRenderbuffers(1, &theColorRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, theColorRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, RENDERBUFFER_RGBA, aViewWidthPx, aViewHeightPx);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, theColorRenderbuffer);

	if (aDepthBufferFlag)
	{
		glGenRenderbuffers(1, &theDepthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, theDepthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, DEPTH_BUFFER_DEPTH, aViewWidthPx, aViewHeightPx);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, theDepthRenderbuffer);
	}

	//	See extensive comment preceding glCheckFramebufferStatus()
	//	in -RenderToMultisampleBuffer immediately above.
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		theError = u"The plain framebuffer is incomplete.  Cannot copy/save image.";
		goto CleanUpRenderToPlainBuffer;
	}
	
	aRenderFunction(md, gd, aViewWidthPx, aViewHeightPx, NULL);
	glReadPixels(	0, 0,
					aViewWidthPx, aViewHeightPx,
					GL_RGBA, GL_UNSIGNED_BYTE,
					somePixels);

CleanUpRenderToPlainBuffer:

	glDeleteFramebuffers(1, &theFramebuffer);
	glDeleteRenderbuffers(1, &theColorRenderbuffer);
	if (aDepthBufferFlag)
	{
		glDeleteRenderbuffers(1, &theDepthRenderbuffer);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, theSavedFramebuffer);
	
	return theError;
}


#ifdef __APPLE__
#pragma mark -
#pragma mark Utilities
#endif


VersionNumber GetVersionNumber(
	GLenum aName)	//	GL_VERSION or GL_SHADING_LANGUAGE_VERSION
{
	const char		*theVersionString		= NULL;
	GLint			theMajorVersionNumber	= 0,
					theMinorVersionNumber	= 0;
	VersionNumber	theVersionNumber		= 0x00000000;

#if defined(SUPPORT_DESKTOP_OPENGL)
	//	The OpenGL version string or shading language version string
	//	has the format
	//
	//		"N.M vendor-specific information"
	//
	//	where N and M are the major and minor version numbers.
	//
	theVersionString = (const char *) glGetString(aName);
	if (theVersionString != NULL
	 && sscanf(theVersionString, "%d.%d", &theMajorVersionNumber, &theMinorVersionNumber) == 2)
	{
		theVersionNumber = VERSION_NUMBER(theMajorVersionNumber, theMinorVersionNumber);
	}
	else
	{
		theVersionNumber = VERSION_NUMBER(0, 0);
	}
#elif defined(SUPPORT_OPENGL_ES)
	//	The OpenGL ES version string has the format
	//
	//		"OpenGL ES N.M vendor-specific information"
	//
	//	while the shading language version string has the format
	//
	//		"OpenGL ES GLSL ES N.M vendor-specific information"
	//
	//	where N and M are the major and minor version numbers.
	//
	theVersionString = (const char *) glGetString(aName);
	if (theVersionString != NULL
	 && sscanf(	theVersionString,
	 			aName == GL_VERSION ? "OpenGL ES %d.%d" : "OpenGL ES GLSL ES %d.%d",
				&theMajorVersionNumber, &theMinorVersionNumber) == 2)
	{
		theVersionNumber = VERSION_NUMBER(theMajorVersionNumber, theMinorVersionNumber);
	}
	else
	{
		theVersionNumber = VERSION_NUMBER(0, 0);
	}
#else
#error No OpenGL support
#endif
	
	return theVersionNumber;
}


ErrorText GetErrorString(void)
{
	switch (glGetError())
	{
		case GL_NO_ERROR:
			return NULL;

		case GL_INVALID_ENUM:
			return u"GL_INVALID_ENUM:  GLenum argument out of range.";

		case GL_INVALID_VALUE:
			return u"GL_INVALID_VALUE:  Numeric argument out of range.";

		case GL_INVALID_OPERATION:
			return u"GL_INVALID_OPERATION:  Operation illegal in current state.";

		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return u"GL_INVALID_FRAMEBUFFER_OPERATION:  Framebuffer object is not complete.";

		case GL_OUT_OF_MEMORY:
			return u"GL_OUT_OF_MEMORY:  Not enough memory left to execute command.";

		default:
			return u"Unknown OpenGL error.";
	}
}


#ifdef DEBUG
void GeometryGamesDebugMessage(
	const GLchar	*message)
{
#ifdef __WIN32__
	FILE	*fp = NULL;

	fp = fopen("debug log.txt", "a");
	if (fp != NULL)
	{
		fprintf(fp, "%s\n", message);
		fclose(fp);
	}
#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
	printf("%s\n", message);	//	print to console
#else
	UNUSED_PARAMETER(message);
#endif
}
#endif


#ifdef __APPLE__
#pragma mark -
#pragma mark OpenGL extensions
#endif

#endif	//	SUPPORT_OPENGL

