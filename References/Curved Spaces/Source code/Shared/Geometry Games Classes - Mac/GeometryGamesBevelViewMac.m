//	GeometryGamesBevelViewMac.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesBevelViewMac.h"
#import "GeometryGames-Common.h"
#import "GeometryGamesUtilities-Mac-iOS.h"


@implementation GeometryGamesBevelView
{
	unsigned int	itsBevelWidth;
	float			itsRed,
					itsGreen,
					itsBlue;
}


- (id)initWithFrame:(NSRect)aFrame bevelWidth:(unsigned int)aBevelWidth red:(float)aRed green:(float)aGreen blue:(float)aBlue
{
	self = [super initWithFrame:aFrame];
	if (self != nil)
	{
		itsBevelWidth = aBevelWidth;

		itsRed		= aRed;
		itsGreen	= aGreen;
		itsBlue		= aBlue;
	}
	return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSSize		theViewSize;
	BevelInfo	theBevelInfo;
	CGImageRef	theImageRef		= NULL;
	
	UNUSED_PARAMETER(dirtyRect);

	//	Get the view's size in pixels (not points).
	theViewSize = [self convertSizeToBacking:[self bounds].size];

	theBevelInfo.itsBaseColor[0]	= itsRed;
	theBevelInfo.itsBaseColor[1]	= itsGreen;
	theBevelInfo.itsBaseColor[2]	= itsBlue;
	theBevelInfo.itsImageWidthPx	= theViewSize.width;
	theBevelInfo.itsImageHeightPx	= theViewSize.height;
	theBevelInfo.itsBevelWidthPx	= itsBevelWidth;
	theBevelInfo.itsScale			= 1.0;

	theImageRef = BevelImageCreate(&theBevelInfo);

	CGContextDrawImage(	[[NSGraphicsContext currentContext] graphicsPort],
						NSRectToCGRect([self bounds]),
						theImageRef);

	CGImageRelease(theImageRef);
}

@end
