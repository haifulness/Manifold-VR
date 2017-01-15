//	GeometryGamesAppDelegate-Mac.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Cocoa/Cocoa.h>
#import "GeometryGames-Common.h"	//	for Char16


typedef struct
{
	const Char16	*itsTitleKey,
					*itsDirectoryName,
					*itsFileBaseName;	//	just "Foo", not "Foo.html" or "Foo-xx.html"
	const bool		itsFileIsLocalized;
} HelpPageInfo;


@class GeometryGamesWindowController;

@interface GeometryGamesAppDelegate : NSObject <NSApplicationDelegate>
{
	NSMutableArray<GeometryGamesWindowController *>		*itsWindowControllers;
	CGSize												itsHelpPanelSize;
	unsigned int										itsNumHelpPages,		//	typically including a "null page"
														itsHelpPageIndex;		//	typically page 0 is the "null page"
	const HelpPageInfo									*itsHelpPageInfo;
	bool												itsQuitWhenAllWindowsHaveClosedFlag;
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (void)removeReferenceToWindowController:(id)sender;

- (NSMenu *)buildLocalizedMenuBar;
- (BOOL)validateMenuItem:(NSMenuItem *)aMenuItem;
- (void)commandHelp:(id)sender;
- (void)refreshHelpText;

- (void)lastModelDidDeallocate:(id)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)anApplication;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (void)applicationWillTerminate:(NSNotification *)aNotification;

@end
