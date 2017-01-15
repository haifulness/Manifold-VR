//	GeometryGames-Common.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include <stdbool.h>	//	for bool, true and false
#include <stdlib.h>		//	for NULL, malloc() and free()
#include <stdint.h>		//	for uint8_t, uint16_t, etc.


//	Acknowledge unused parameters.
#define UNUSED_PARAMETER(x)	((void)(x))

//	Define a byte.
typedef uint8_t		Byte;

//	Define our own UTF-16 type.
//	Our Char16 corresponds to a WCHAR on Win32 or to a UniChar on Mac OS.
//	We do not support surrogate pairs — in fact we often pass a character
//	as a single Char16 — so really this is more like UCS-2 than UTF-16.
//	Someday we can replace this Char16 with the C11 standard type char16_t,
//	but as of August 2014, char16_t is not yet supported in Xcode or CodeBlocks/GCC.
typedef uint16_t	Char16;

//	BUFFER_LENGTH() measures the number of items in an array,
//	not the number of bytes, and automatically adjusts to changes
//	in the number of elements or the size of each element.
#define BUFFER_LENGTH(a)	( sizeof(a) / sizeof((a)[0]) )

//	The ModelData will be different for each application.
//	All the shared code needs to know is that the ModelData is a structure.
typedef struct ModelData ModelData;

//	Internal functions pass error messages as pointers
//	to zero-terminated UTF-16 strings.  The strings are defined
//	statically, so the caller need not (must not!) free them.
typedef const Char16	*ErrorText;
typedef struct
{
	ErrorText	itsMessage,
				itsTitle;
} TitledErrorMessage;


//	Represent colors as (αR, αG, αB, α) instead of the traditional (R,G,B,α)
//	to facilitate blending and mipmap generation.
//
//	1.	Rigorously correct blending requires
//
//					   αs*(Rs,Gs,Bs) + (1 - αs)*αd*(Rd,Gd,Bd)
//			(R,G,B) = ----------------------------------------
//							 αs      +      (1 - αs)*αd
//
//				  α = αs + (1 - αs)*αd
//
//		Replacing the traditional (R,G,B,α) with the premultiplied (αR,αG,αB,α)
//		simplifies the formula to 
//
//			(αR, αG, αB) = (αs*Rs, αs*Gs, αs*Bs) + (1 - αs)*(αd*Rd, αd*Gd, αd*Bd)
//
//					   α = αs + (1 - αs)*αd
//
//		Because they share the same coefficients, 
//		we may merge the RGB and α parts into a single formula
//
//			(αR, αG, αB, α) = (αs*Rs, αs*Gs, αs*Bs, αs) + (1 - αs)*(αd*Rd, αd*Gd, αd*Bd, αd)
//
//
//	2.	When generating mipmaps, to average two (or more) pixels correctly
//		we must weight them according to their alpha values.
//		With traditional (R,G,B,α) the formula is a bit messy
//
//								 α0*(R0,G0,B0) + α1*(R1,G1,B1)
//			(Ravg, Gavg, Bavg) = -----------------------------
//								            α0 + α1
//
//								 α0 + α1
//						  αavg = -------
//									2
//
//		With premultiplied (αR,αG,αB,α) the formula becomes a simple average
//
//			(αavg*Ravg, αavg*Gavg, αavg*Bavg, αavg)
//
//				  (α0*R0, α0*G0, α0*B0, α0) + (α1*R1, α1*G1, α1*B1, α1)
//				= -----------------------------------------------------
//											2
//
//
#define PREMULTIPLY_RGBA(r,g,b,a)	{(a)*(r), (a)*(g), (a)*(b), (a)}

//	RGBA pixel
typedef struct
{
	Byte	r,	//	red,   premultiplied by alpha
			g,	//	green, premultiplied by alpha
			b,	//	blue,  premultiplied by alpha
			a;	//	alpha = opacity
} PixelRGBA;

//	RGBA image, either read from a .rgba file or constructed manually
typedef struct
{
	//	A width, a height and some pixels define the image.
	unsigned int	itsWidth,
					itsHeight;
	PixelRGBA		*itsPixels;	//	= itsRawBytes + 8 if the image comes from a file.

	//	If the image came from an RGBA file, 
	//		keep a pointer to the raw data, including the 8 header bytes,
	//		so it can eventually be freed.
	//	If the image was constructed manually,
	//		set itsRawBytes = (Byte *)itsPixels, with no header bytes.
	Byte			*itsRawBytes;
	
} ImageRGBA;

//	When the platform-dependent code passes the mouse or touch location
//	to the platform-independent code, it passes the view dimensions along with it.
typedef struct
{
	//	The horizontal coordinate runs left-to-right, from 0 to itsViewWidth.
	//	The  vertical  coordinate runs bottom-to-top, from 0 to itsViewHeight.
	//
	//	Measurements may be in pixels or points;
	//	either is fine just so they're consistent.
	
	float	itsX,
			itsY,
			itsViewWidth,
			itsViewHeight;
	
} DisplayPoint;

//	A DisplayPointMotion is just like a DisplayPoint,
//	except that it stores a relative motion instead of an absolute position.
typedef struct
{
	//	Measurements may be in pixels or points;
	//	either is fine just so they're consistent.
	
	float	itsDeltaX,		//	left-to-right motion is positive
			itsDeltaY,		//	bottom-to-top motion is positive
			itsViewWidth,
			itsViewHeight;

} DisplayPointMotion;

//	Stereoscopic 3D modes
typedef enum
{
	//	Full-color image for a single eye.
	StereoNone,
	
	//	Greyscale image pair for anaglyphic 3D.
	//	Left-eye image goes to red channel, while
	//	right-eye image goes to green and blue channels.
	StereoGreyscale,
	
	//	Full-color image pair for anaglyphic 3D.
	//	Left-eye image goes to red channel, while
	//	right-eye image goes to green and blue channels.
	//	Allows reasonable color perception,
	//	depending on the scene and the colors used.
	StereoColor

} StereoMode;


//	Platform-independent (but app-specific!) global variables
extern const Char16			gLanguages[][3];	//	for example {u"de", u"en", … , u"zs", u"zt"}
extern const unsigned int	gNumLanguages;
extern const Char16 * const	gLanguageFileBaseName;	//	just u"BaseName", not u"BaseName-xx.txt"


//	Platform-independent global functions

//	in <ProgramName>Init.c
extern unsigned int		SizeOfModelData(void);
extern void				SetUpModelData(ModelData *md);
extern void				ShutDownModelData(ModelData *md);

//	in <ProgramName>Simulation.c
extern bool				SimulationWantsUpdates(ModelData *md);
extern void				SimulationUpdate(ModelData *md, double aFramePeriod);

//	in <ProgramName>Drawing.c  (portfolio-based app only)
extern bool				ContentIsLocked(ModelData *md);
extern void				SetContentIsLocked(ModelData *md, bool aContentIsLocked);

//	in <ProgramName>FileIO.c
extern void				SaveDrawing(ModelData *md, const char *aPathName);
extern bool				OpenDrawing(ModelData *md, const char *aPathName);

