//	GeometryGamesWindowMac.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesWindowMac.h"


@implementation GeometryGamesWindowMac
{
}

- (BOOL)canBecomeKeyWindow
{
	//	"The NSWindow implementation returns YES if the window 
	//	has a title bar or a resize bar, or NO otherwise."
	//
	//	Geometry Games software wants the main window
	//	to be key even in fullscreen mode, so return YES always.
	return YES;
}

@end
