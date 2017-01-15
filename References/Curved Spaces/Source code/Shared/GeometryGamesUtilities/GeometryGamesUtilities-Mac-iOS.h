//	GeometryGamesUtilities-Mac-iOS.h
//
//	Declares Geometry Games functions with the same implementation on Mac OS and iOS.
//	The file GeometryGamesUtilities-Mac-iOS.c implements these functions.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#import "GeometryGames-Common.h"	//	for Char16
#import <simd/simd.h>				//	for matrix_float4x4


typedef struct
{
	float			itsBaseColor[3];	//	{R,G,B}
	unsigned int	itsImageWidthPx,	//	in pixels, not points
					itsImageHeightPx,	//	in pixels, not points
					itsBevelWidthPx;	//	in pixels, not points
	float			itsScale;			//	1.0 (normal) or 2.0 (double resolution)
} BevelInfo;

extern bool				MetalIsAvailable(void);
extern matrix_float4x4	ConvertMatrixToSIMD(double aMatrix[4][4]);
extern NSString			*GetPreferredLanguage(void);
extern NSString			*GetLocalizedTextAsNSString(const Char16 *aKey);
extern NSString			*GetNSStringFromZeroTerminatedString(const Char16 *anInputString);
extern Char16			*GetZeroTerminatedStringFromNSString(NSString *anInputString, Char16 *anOutputBuffer, unsigned int anOutputBufferLength);
extern CGImageRef		BevelImageCreate(BevelInfo *aBevelRequest);
extern void				LogMemoryUsage(void);
