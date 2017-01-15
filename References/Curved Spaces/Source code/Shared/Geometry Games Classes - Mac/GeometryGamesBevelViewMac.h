//	GeometryGamesBevelViewMac.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Cocoa/Cocoa.h>

@interface GeometryGamesBevelView : NSView

- (id)initWithFrame:(NSRect)aFrame bevelWidth:(unsigned int)aBevelWidth red:(float)aRed green:(float)aGreen blue:(float)aBlue;
- (void)drawRect:(NSRect)dirtyRect;

@end
