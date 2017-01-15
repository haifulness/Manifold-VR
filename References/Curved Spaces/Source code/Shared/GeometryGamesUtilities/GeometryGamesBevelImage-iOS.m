//	GeometryGamesBevelImage-iOS.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesBevelImage-iOS.h"
#import "GeometryGamesUtilities-Mac-iOS.h"


UIImage *StretchableBevelImage(
	const float		aBaseColor[3],	//	{r,g,b}, with components in range [0.0, 1.0]
	unsigned int	aBevelWidth,	//	in points, not pixels
	float			aScale)			//	= [[UIScreen mainScreen] scale] = 1.0, 2.0 or 3.0,
									//		but never equals a non-integer "native scale"
{
	unsigned int	theImageSizePx			= 0,
					theBevelWidthPx			= 0;
	BevelInfo		theBevelInfo;
	CGImageRef		theCGImage				= NULL;
	UIImage			*theUnstretchableImage	= nil;
	UIImage			*theStretchableImage	= nil;	//	Initialized to nil in case we return early.

	//	aScale should be 1.0, 2.0 or 3.0.  Never a non-integer "native scale".
	if (aScale != floor(aScale))
		return nil;

	//	Create an image that's almost all bevel, with just 
	//	a 1pt × 1pt (maybe 2px × 2px) square center region.
	theImageSizePx	= (aBevelWidth + 1 + aBevelWidth) * (unsigned int)aScale;
	theBevelWidthPx	= aBevelWidth * (unsigned int)aScale;

	theBevelInfo.itsBaseColor[0]	= aBaseColor[0];
	theBevelInfo.itsBaseColor[1]	= aBaseColor[1];
	theBevelInfo.itsBaseColor[2]	= aBaseColor[2];
	theBevelInfo.itsImageWidthPx	= theImageSizePx;
	theBevelInfo.itsImageHeightPx	= theImageSizePx;
	theBevelInfo.itsBevelWidthPx	= theBevelWidthPx;
	theBevelInfo.itsScale			= aScale;

	theCGImage = BevelImageCreate(&theBevelInfo);
	if (theCGImage == NULL)
		goto CleanUpStretchableBevelImage;
	
	theUnstretchableImage = (
		[UIImage instancesRespondToSelector:@selector(initWithCGImage:scale:orientation:)] ?
			[[UIImage alloc] initWithCGImage:theCGImage scale:aScale orientation:UIImageOrientationUp] :
			[[UIImage alloc] initWithCGImage:theCGImage] );
	if (theUnstretchableImage == nil)
		goto CleanUpStretchableBevelImage;
	
	theStretchableImage = [theUnstretchableImage stretchableImageWithLeftCapWidth:aBevelWidth topCapHeight:aBevelWidth];

CleanUpStretchableBevelImage:

	if (theCGImage != NULL)
	{
		CGImageRelease(theCGImage);
		theCGImage = NULL;
	}

	return theStretchableImage;
}
