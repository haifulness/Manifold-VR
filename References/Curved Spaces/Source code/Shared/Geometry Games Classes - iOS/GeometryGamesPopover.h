//	GeometryGamesPopover.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>

//	If the UIKit ever starts notifying the presented view controller
//	(which for a popover-style view controller always has compact horizontal size)
//	about horizontal size class changes in the presenting view controller
//	(whose size may vary depending the split-screen arrangement),
//	then this GeometryGamesPopover protocol may be eliminated.
//
//		Note:  There are already some methods that would allow
//		the presented view controller to find out when the presentation mode
//		is changing, without requiring any code in the presenting view controller.
//		See for example
//
//			https://pspdfkit.com/blog/2015/presentation-controllers/
//			http://useyourloaf.com/blog/making-popovers-adapt-to-size-classes.html
//
//		but such a cure seems worse than the disease, and in any case
//		some of the methods are available only in iOS 8.3 and later.
//

@protocol GeometryGamesPopover
- (void)adaptNavBarForHorizontalSize:(UIUserInterfaceSizeClass)aHorizontalSizeClass;
@end
