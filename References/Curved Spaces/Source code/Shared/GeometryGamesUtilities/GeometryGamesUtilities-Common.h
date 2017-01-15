//	GeometryGamesUtilities-Common.h
//
//	Declares Geometry Games types and functions with platform-independent declarations.
//	The file GeometryGamesUtilities-Common.c implements 
//	the functions whose implementation is platform-independent.
//	The files GeometryGamesUtilities-iOS|Mac|Win.c implement 
//	the functions whose implementation is platform-dependent.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include "GeometryGames-Common.h"

#if defined(__ANDROID__) || defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
//	The Android and Mac OS apps render in a separate thread,
//	so we must ensure that their access to gMemCount is threadsafe.
#define THREADSAFE_MEM_COUNT
#elif defined(__WIN32__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
//	The Windows and iOS apps render in the main thread,
//	so they may access gMemCount with no mutex.
#else
#error When adding support for a new operating system,\
 please indicate whether gMemCount access must be threadsafe or not.
#endif

#ifdef THREADSAFE_MEM_COUNT
#include <pthread.h>
#endif


//	Define GET_MEMORY and FREE_MEMORY macros to manage memory so
//	that in future we can easily change memory allocation schemes.
//	Use a variable gMemCount to test for memory leaks.
//	Define FREE_MEMORY_SAFELY() to test for and set NULL pointers.
#ifdef THREADSAFE_MEM_COUNT

//	compiling for Android or Mac OS
#define GET_MEMORY(n)			(											\
									pthread_mutex_lock(&gMemCountMutex),	\
									gMemCount++,							\
									pthread_mutex_unlock(&gMemCountMutex),	\
									malloc(n)								\
								)
#define FREE_MEMORY(p)			(											\
									pthread_mutex_lock(&gMemCountMutex),	\
									gMemCount--,							\
									pthread_mutex_unlock(&gMemCountMutex),	\
									free(p)									\
								)
extern pthread_mutex_t	gMemCountMutex;

#else

//	compiling for iOS or Win32
#define GET_MEMORY(n)			(											\
									gMemCount++,							\
									malloc(n)								\
								)
#define FREE_MEMORY(p)			(											\
									gMemCount--,							\
									free(p)									\
								)

#endif
#define RESIZE_MEMORY(p,n)		(realloc(p,n))
#define FREE_MEMORY_SAFELY(p)	{if ((p) != NULL) {FREE_MEMORY(p); (p) = NULL;}}
extern signed int		gMemCount;

//	Store the OpenGL, OpenGL ES or shading language version number 
//	in the two lower-order bytes of an unsigned int.
//	For example, OpenGL 3.2 would be encoded as 0x00000302 .
typedef unsigned int	VersionNumber;
#define VERSION_NUMBER(major, minor)	( ((major) << 8) | (minor) )

//	Assertions
#define GEOMETRY_GAMES_ASSERT(statement, description)						\
	do																		\
	{																		\
		if (!(statement))													\
			GeometryGamesAssertionFailed(__FILE__, __LINE__, __func__, description);	\
	} while (0)


//	Logically it would work fine to pass "true" or "false" 
//	to request anisotropic texture filtering or not,
//	but the enum values AnisotropicOn and AnisotropicOff
//	make the code more readable.
typedef enum
{
	AnisotropicOff,
	AnisotropicOn
} AnisotropicMode;

//	GreyscaleOn and GreyscaleOff make clearer code than "true" and "false".
typedef enum
{
	GreyscaleOff,
	GreyscaleOn
} GreyscaleMode;

//	Most textures will be RGBA, but a few will be alpha-only.
typedef enum
{
	TextureRGBA,
	TextureAlpha
} TextureFormat;


//	Functions with platform-independent non-OpenGL implementations
//	appear in GeometryGamesUtilities-Common.c.

extern bool			IsPowerOfTwo(unsigned int n);
extern bool			UTF8toUTF16(const char	*aInputStringUTF8, Char16 *anOutputBufferUTF16, unsigned int anOutputBufferLength);
extern bool			UTF16toUTF8(const Char16 *anInputStringUTF16, char *anOutputBufferUTF8, unsigned int anOutputBufferLength);
extern size_t		Strlen16(const Char16 *aString);
extern bool			Strcpy16(Char16 *aDstBuffer, size_t aDstBufferSize, const Char16 *aSrcString);
extern bool			Strcat16(Char16 *aDstBuffer, size_t aDstBufferSize, const Char16 *aSrcString);
extern bool			SameString16(const Char16 *aStringA, const Char16 *aStringB);
extern Char16		*AdjustKeyForNumber(Char16 *aKey, unsigned int aNumber);
extern ErrorText	ReadImageRGBA(const Char16 *aTextureFileName, GreyscaleMode aGreyscaleMode, ImageRGBA **anImageRGBA);
extern void			FreeImageRGBA(ImageRGBA **anImage);

extern ErrorText	GetFileContents(const Char16 *aDirectory, const Char16 *aFileName, unsigned int *aNumRawBytes, Byte **someRawBytes);
extern void			FreeFileContents(unsigned int *aNumRawBytes, Byte **someRawBytes);

extern void			InvertRawImage(unsigned int aWidth, unsigned int aHeight, PixelRGBA *aPixelBuffer);

extern bool			GetUserPrefBool(const Char16 *aKey);
extern void			SetUserPrefBool(const Char16 *aKey, bool aValue);
extern int			GetUserPrefInt(const Char16 *aKey);
extern void			SetUserPrefInt(const Char16 *aKey, int aValue);
extern float		GetUserPrefFloat(const Char16 *aKey);
extern void			SetUserPrefFloat(const Char16 *aKey, float aValue);
extern const Char16	*GetUserPrefString(const Char16 *aKey, Char16 *aBuffer, unsigned int aBufferLength);
extern void			SetUserPrefString(const Char16 *aKey, const Char16 *aString);

extern void			RandomInit(void);
extern void			RandomInitWithSeed(unsigned int aSeed);
extern bool			RandomBoolean(void);
extern unsigned int	RandomInteger(void);
extern float		RandomFloat(void);

extern void			StartNewThread(ModelData *md, void (*aStartFuction)(ModelData *));
extern void			SleepBriefly(void);

extern void			GetBevelBytes(
						Byte aBaseColor[3],
						unsigned int anImageWidthPx, unsigned int anImageHeightPx,
						unsigned int aBevelThicknessPx, unsigned int aScaleFactor,
#ifdef __APPLE__
						unsigned int anInitialPixelCount, unsigned int anInitialChannel,
						unsigned int theNumBytesRequested,
#endif
						Byte *aBuffer);

extern void			FatalError(ErrorText aMessage, ErrorText aTitle);
extern void			ErrorMessage(ErrorText aMessage, ErrorText aTitle);
extern void			InfoMessage(ErrorText aMessage, ErrorText aTitle);
extern bool			IsShowingErrorAlert(void);
extern void			GeometryGamesAssertionFailed(const char *aPathName, unsigned int aLineNumber, const char *aFunctionName, const char *aDescription);
