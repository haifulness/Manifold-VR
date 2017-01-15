//	GeometryGamesWindowController.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Cocoa/Cocoa.h>


@class	GeometryGamesModel,
		GeometryGamesWindowMac,
		GeometryGamesViewMac;

@interface GeometryGamesWindowController : NSResponder <NSWindowDelegate>
{
	GeometryGamesModel		*itsModel;
	GeometryGamesWindowMac	*itsWindow;
	GeometryGamesViewMac	*itsMainView;	//	may be nil
}

- (id)init;
- (void)createToolbar;
- (void)createSubviews;
- (void)windowWillClose:(NSNotification *)aNotification;

- (BOOL)validateMenuItem:(NSMenuItem *)aMenuItem;
- (void)commandCopyImage:(id)sender;
- (void)commandSaveImage:(id)sender;

- (void)languageDidChange;

@end
