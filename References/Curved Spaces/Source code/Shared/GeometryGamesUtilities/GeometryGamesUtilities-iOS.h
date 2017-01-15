//	GeometryGamesUtilities-iOS.h
//
//	Declares Geometry Games functions with iOS-specific declarations.
//	The file GeometryGamesUtilities-iOS.c implements these functions.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#include <UIKit/UIKit.h>
#include "GeometryGamesUtilities-Common.h"


#define PREFERRED_POPOVER_WIDTH	320.0


extern void		PresentPopoverFromToolbarButton(
					UIViewController *aPresentingViewController, UIViewController *aPresentedViewController,
					UIBarButtonItem *anAnchorButton, NSArray<UIView *> *somePassthroughViews);
extern void		PresentPopoverFromRectInView(
					UIViewController *aPresentingViewController, UIViewController *aPresentedViewController,
					CGRect anAnchorRect, UIView *anAnchorView, UIPopoverArrowDirection somePermittedArrowDirections,
					NSArray<UIView *> *somePassthroughViews);
extern UIImage	*ImageWithPixels(CGSize anActualSize, PixelRGBA **somePixels);
extern NSString *GetFullFilePath(NSString *aFileName, NSString *aFileExtension);
extern UIImage	*LoadScaledThumbnailImage(NSString *aFileName);
extern CGFloat	GetThumbnailSizePt(void);
extern CGFloat	GetThumbnailSizePx(void);
