//	CurvedSpacesWindowController.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "CurvedSpacesWindowController.h"
#import "CurvedSpacesView.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesWindowMac.h"
#import "GeometryGamesUtilities-Mac-iOS.h"
#import "GeometryGamesLocalization.h"


//	Space Selection Sheet layout
#define SSS_TITLE_HEIGHT		 36	//	includes space between title and body
#define SSS_TITLE_MARGIN_V		 12	//	space above title
#define SSS_HEADER_HEIGHT		(SSS_TITLE_HEIGHT + SSS_TITLE_MARGIN_V)
#define SSS_BODY_HEIGHT			288
#define SSS_FOOTER_HEIGHT		 60
#define SSS_TOTAL_HEIGHT		(SSS_HEADER_HEIGHT  + SSS_BODY_HEIGHT + SSS_FOOTER_HEIGHT)
#define SSS_BODY_WIDTH			288
#define SSS_MARGIN_H			 14
#define SSS_TOTAL_WIDTH			(SSS_BODY_WIDTH + 2*SSS_MARGIN_H)
#define SSS_BUTTON_WIDTH		 96
#define SSS_BUTTON_HEIGHT		 32
#define SSS_BUTTON_TO_FRAME_V	 12
#define SSS_BUTTON_TO_BUTTON_H	  0	//	Visually there's still a space.


static NSArray<NSObject *>	*CreateFileTree(NSString *aDirectoryPath);
static bool					OpenContainingDirectoriesAndSelectItem(NSOutlineView *anOutlineView, NSArray<NSObject *> *aDirectoryItem, NSString *aGeneratorFilePath);


//	Privately-declared properties and methods
@interface CurvedSpacesWindowController()

- (void)showSpaceSelectionWindow;
- (NSFont *)spaceSelectionTitleFont;
- (void)refreshSpaceSelectionLanguage;
- (void)spaceSelectionWindowOK:(id)sender;
- (void)spaceSelectionWindowCancel:(id)sender;
- (void)spaceSelectionWindowDoubleClick:(id)sender;
- (void)readGeneratorFile;
- (void)dismissSpaceSelectionWindow;
- (void)openGeneratorFile:(NSString *)aFilePath;

@end


@implementation CurvedSpacesWindowController
{
	CurvedSpacesView	*itsCurvedSpacesView;

	//	-showSpaceSelectionWindow creates a temporary tree
	//	that represents the structure of the Sample Spaces directory,
	//	and displays it in a temporary outline view in a temporary sheet.
	NSArray<NSObject *>	*itsFileTree;
	NSOutlineView		*itsOutlineView;
	NSWindow			*itsSpaceSelectionWindow;

	//	Full path to matrix file for current space.
	//	(Similar to NSWindow's representedFilename, 
	//	but fully under our control.)
	NSString	*itsGeneratorFilePath;	//	keeps a copy
}


- (id)init
{
	self = [super init];
	if (self != nil)
	{
		//	No space selector is present.
		itsFileTree				= nil;
		itsOutlineView			= nil;
		itsSpaceSelectionWindow	= nil;

		//	The superclass has created the window.

#ifdef SAVE_ANIMATION
		//	Override the standard window placement
		//	to match high-def (1920×1080) or low-def (720×480)
		//	video standards.
#ifdef HIGH_DEF_ANIMATION
		//	Ultimately we'll render offscreen at 1920×1080,
		//	so for now run at half those dimensions to get
		//	the aspect ratio right.
		[itsWindow setContentSize:(NSSize){1920/2, 1080/2}];	//	high-def
#else
		[itsWindow setContentSize:(NSSize){720, 480}];			//	low-def
#endif //HIGH_DEF_ANIMATION
		[itsWindow center];
#endif //SAVE_ANIMATION

#if defined(CENTERPIECE_DISPLACEMENT) || defined(START_OUTSIDE)
//	Note:  For START_STILL we want to wait a few seconds, long enough to say
//
//		"Here we're inside a cube that contains a single Earth."
//
//	before manually going to fullscreen.  That way the audience
//	sees that it really is a cube.
		[itsWindow toggleFullScreen:self];
#endif

		//	Delay the call to -commandChooseSpace, 
		//	so the user sees the sheet animate gracefully into place.
		[self performSelector:@selector(commandChooseSpace:) withObject:self afterDelay:0.125];
	}
	return self;
}

- (void)createSubviews
{
	//	Create the view.
	itsCurvedSpacesView = [[CurvedSpacesView alloc]
		initWithModel:	itsModel
		frame:			[[itsWindow contentView] bounds]];
	[[itsWindow contentView] addSubview:itsCurvedSpacesView];
	[itsCurvedSpacesView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

	//	Let the superclass treat itsCurvedSpacesView as itsMainView.
	itsMainView = itsCurvedSpacesView;
}

- (void)windowWillClose:(NSNotification *)aNotification
{
	//	Stop the CVDisplayLink and wait for it to finish drawing.
	[itsCurvedSpacesView pauseAnimation];
	
	//	The superclass will finish cleaning up
	//	and then release this controller.
	[super windowWillClose:aNotification];
}


- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)newFrame
{
	NSRect	theContentRect;
	
	//	When the windows zooms, Cocoa calls this -windowWillUseStandardFrame:defaultFrame:.
	
	//	Constrain the aspect ratio for windowed-mode zooms,
	//	but let fullscreen mode use the full screen.
	if ( ! ([itsWindow styleMask] & NSFullScreenWindowMask) )
	{
		//	Convert the newFrame to theContentRect.
		theContentRect = [window contentRectForFrameRect:newFrame];
		
		//	Constrain to a square.
		if (theContentRect.size.width  > theContentRect.size.height)
			theContentRect.size.width  = theContentRect.size.height;
		if (theContentRect.size.height > theContentRect.size.width )
			theContentRect.size.height = theContentRect.size.width;
		
		//	Convert theContentRect back to the newFrame.
		newFrame = [window frameRectForContentRect:theContentRect];
	}
	
	return newFrame;
}


- (BOOL)validateMenuItem:(NSMenuItem *)aMenuItem
{
	SEL			theAction;
	ModelData	*md				= NULL;
	bool		theEnableFlag;

	theAction = [aMenuItem action];

	if (theAction == @selector(commandChooseSpace:))
		return (itsFileTree == nil && itsOutlineView == nil && itsSpaceSelectionWindow == nil);

	if (theAction == @selector(commandCenterpiece:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:((CenterpieceType)[aMenuItem tag] == md->itsCenterpiece ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}

	if (theAction == @selector(commandObserver:))
	{
		[itsModel lockModelData:&md];
		//	Enable the spaceship only in monoscopic 3D,
		//	to avoid the problem that in stereoscopic 3D
		//	the observer would see the two sides of the spaceship
		//	the way you see both sides of your nose in real life.
		theEnableFlag = (md->itsStereoMode == StereoNone);
		[aMenuItem setState:(theEnableFlag && md->itsShowObserver ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return theEnableFlag;
	}

	if (theAction == @selector(commandColorCoding:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:(md->itsShowColorCoding ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}

#ifdef HANTZSCHE_WENDT_AXES
	if (theAction == @selector(commandHantzscheWendt:))
	{
		[itsModel lockModelData:&md];
		theEnableFlag = md->itsHantzscheWendtSpaceIsLoaded;
		[aMenuItem setState:(theEnableFlag && md->itsShowHantzscheWendtAxes ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return theEnableFlag;
	}
#endif

	if (theAction == @selector(commandCliffordParallels:))
	{
		[itsModel lockModelData:&md];
#ifdef CLIFFORD_FLOWS_FOR_TALKS
		theEnableFlag = true;
#else
		theEnableFlag = md->itsThreeSphereFlag;
#endif
		[aMenuItem setState:(theEnableFlag
			 && (CliffordMode)[aMenuItem tag] == md->itsCliffordMode ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return theEnableFlag;
	}

#ifdef CLIFFORD_FLOWS_FOR_TALKS
	if (theAction == @selector(commandCliffordFlowXY:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:(md->itsCliffordFlowXYEnabled ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}
	if (theAction == @selector(commandCliffordFlowZW:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:(md->itsCliffordFlowZWEnabled ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}
#endif

	if (theAction == @selector(commandVertexFigures:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:(md->itsShowVertexFigures ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}

	if (theAction == @selector(commandFog:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:(md->itsFogFlag ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}

	if (theAction == @selector(commandStereoscopic3D:))
	{
		[itsModel lockModelData:&md];
		[aMenuItem setState:((StereoMode)[aMenuItem tag] == md->itsStereoMode ? NSOnState : NSOffState)];
		[itsModel unlockModelData:&md];

		return YES;
	}

#ifdef SAVE_ANIMATION
	if (theAction == @selector(qtStartRecording:))
		return _itsFlightPathFile == nil;
	if (theAction == @selector(qtStopRecording:))
		return _itsFlightPathFile != nil;
	if (theAction == @selector(qtPreviewMovie:))
		return _itsFlightPathFile == nil;
	if (theAction == @selector(qtRenderMovie:))
		return _itsFlightPathFile == nil;
#endif

	return [super validateMenuItem:aMenuItem];
}

- (void)commandChooseSpace:(id)sender
{
	if (itsFileTree != nil || itsOutlineView != nil || itsSpaceSelectionWindow != nil)
		return;	//	should never occur

#ifdef CURVED_SPACES_MOUSE_INTERFACE
	[itsCurvedSpacesView exitNavigationalMode];
#endif

	[self showSpaceSelectionWindow];
}

- (void)commandSaveImage:(id)sender
{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
	[itsCurvedSpacesView exitNavigationalMode];
#else
#warning If I ever permanently eliminate the mouse interface,	\
	I can eliminate this override of -commandSaveImage: .
#endif
	
	[super commandSaveImage:sender];
}

- (void)commandCenterpiece:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetCenterpiece(md, (CenterpieceType)[sender tag]);

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}

- (void)commandObserver:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetShowObserver(md, ! md->itsShowObserver );

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}

- (void)commandColorCoding:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetShowColorCoding(md, ! md->itsShowColorCoding );

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}

#ifdef HANTZSCHE_WENDT_AXES
- (void)commandHantzscheWendt:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetShowHantzscheWendtAxes(md, ! md->itsShowHantzscheWendtAxes );

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}
#endif

- (void)commandCliffordParallels:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetShowCliffordParallels(md, (CliffordMode)[sender tag]);

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}

#ifdef CLIFFORD_FLOWS_FOR_TALKS
- (void)commandCliffordFlowXY:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];
	md->itsCliffordFlowXYEnabled = ! md->itsCliffordFlowXYEnabled;
	[itsModel unlockModelData:&md];
}
- (void)commandCliffordFlowZW:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];
	md->itsCliffordFlowZWEnabled = ! md->itsCliffordFlowZWEnabled;
	[itsModel unlockModelData:&md];
}
#endif

- (void)commandVertexFigures:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetShowVertexFigures(md, ! md->itsShowVertexFigures );

	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}

- (void)commandFog:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];
	SetFogFlag(md, ! md->itsFogFlag );
	[itsModel unlockModelData:&md];
}

- (void)commandStereoscopic3D:(id)sender
{
	ModelData	*md				= NULL;

	[itsModel lockModelData:&md];

	SetStereo3DMode(md, (StereoMode)[sender tag]);
	
	//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
	//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
	//	in a consistent state.
	[itsCurvedSpacesView requestTextureUpdate];
	[itsCurvedSpacesView requestVBOUpdate];

	[itsModel unlockModelData:&md];
}


- (void)languageDidChange
{
	//	The superclass will set the generic window title
	//	("Curved Spaces") in the current language.
	[super languageDidChange];
	
	//	Typically a space has been loaded, in which case
	//	we should overwrite the generic window title
	//	with the space's file name.
	if (itsGeneratorFilePath != nil)
		[itsWindow setTitle:[[itsGeneratorFilePath
								lastPathComponent]
								stringByDeletingPathExtension]];
	
	//	If the Space Selection Sheet is open,
	//	re-localize its labels in the new language.
	if (itsSpaceSelectionWindow != nil)
		[self refreshSpaceSelectionLanguage];
}


#pragma mark Space Selection Sheet

- (void)showSpaceSelectionWindow
{
	NSScrollView	*theScrollView		= nil;
	NSTableColumn	*theOutlineColumn	= nil;
	NSButton		*theOKButton		= nil,
					*theCancelButton	= nil;
	NSTextField		*theTitle			= nil;
	
	//	Parse the Sample Spaces directory, building a tree
	//	containing an NSArray for each subdirectory
	//	and an NSString for each generator (.gen) file.
	//	Ignore non-generator files.
	itsFileTree = CreateFileTree([[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/Sample Spaces/"]);

	//	Create itsSpaceSelectionWindow.
	itsSpaceSelectionWindow = [[NSWindow alloc]
		initWithContentRect:	(NSRect){{0, 0}, {SSS_TOTAL_WIDTH, SSS_TOTAL_HEIGHT}}
		styleMask:				NSTitledWindowMask
		backing:				NSBackingStoreBuffered
		defer:					NO];

	//	itsSpaceSelectionWindow shouldn't prevent the user from quitting.
	[itsSpaceSelectionWindow setPreventsApplicationTerminationWhenModal:NO];

	//	Add an an NSTableColumn within an NSOutlineView within an NSScrollView.
	theScrollView = [[NSScrollView alloc] initWithFrame:
						(NSRect){{SSS_MARGIN_H, SSS_FOOTER_HEIGHT}, {SSS_BODY_WIDTH, SSS_BODY_HEIGHT}}];
	[[itsSpaceSelectionWindow contentView] addSubview:theScrollView];
	[theScrollView setHasHorizontalScroller:NO];
	[theScrollView setHasVerticalScroller:YES];
	itsOutlineView = [[NSOutlineView alloc] initWithFrame:NSZeroRect];
	[theScrollView setDocumentView:itsOutlineView];
	[itsOutlineView setDataSource:self];
	[itsOutlineView setTarget:self];
	[itsOutlineView setDoubleAction:@selector(spaceSelectionWindowDoubleClick:)];
	[itsOutlineView setHeaderView:nil];
	theOutlineColumn = [[NSTableColumn alloc] initWithIdentifier:@"only column"];
	[itsOutlineView addTableColumn:theOutlineColumn];
	[theOutlineColumn setEditable:NO];	//	NO ⇒ double-click sends double-click action to target
	[itsOutlineView setOutlineTableColumn:theOutlineColumn];
	[itsOutlineView sizeLastColumnToFit];
	if (itsGeneratorFilePath == nil)
	{
		//	No generator file has yet been selected.
		//	Expand the root directory "Sample Spaces"
		//	and the "Basic" subdirectory within it,
		//	then tenatively select the first space "3-Torus".
		[itsOutlineView expandItem:itsFileTree];
#if   defined(START_STILL)				//	pre-select Basic : 3-Torus
		[itsOutlineView expandItem:[itsFileTree objectAtIndex:1]];
		[itsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:2] byExtendingSelection:NO];
#elif defined(CENTERPIECE_DISPLACEMENT)	//	pre-select Flat : 3-Torus
		[itsOutlineView expandItem:[itsFileTree objectAtIndex:2]];
		[itsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:13] byExtendingSelection:NO];
#elif defined(START_OUTSIDE)			//	pre-select Basic : Poincaré Dodecahedral Space
		[itsOutlineView expandItem:[itsFileTree objectAtIndex:1]];
		[itsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:4] byExtendingSelection:NO];
#elif defined(CLIFFORD_FLOWS_FOR_TALKS)		//	pre-select Spherical : Binary Dihedral 12 L
		[itsOutlineView expandItem:[itsFileTree objectAtIndex:4]];
		[itsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:6] byExtendingSelection:NO];
#else									//	pre-select Basic : 3-Torus
		[itsOutlineView expandItem:[itsFileTree objectAtIndex:1]];
		[itsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:2] byExtendingSelection:NO];
#endif
	}
	else
	{
		//	Open the directories leading to the current file,
		//	and then tentatively select that file.
		OpenContainingDirectoriesAndSelectItem(itsOutlineView, itsFileTree, itsGeneratorFilePath);
	}
	
	//	Add OK and Cancel buttons.

	theOKButton = [[NSButton alloc] initWithFrame:(NSRect)
	{
		{
			SSS_TOTAL_WIDTH - SSS_BUTTON_WIDTH - SSS_MARGIN_H,
			SSS_BUTTON_TO_FRAME_V
		},
		{
			SSS_BUTTON_WIDTH,
			SSS_BUTTON_HEIGHT
		}
	}];
	[[itsSpaceSelectionWindow contentView] addSubview:theOKButton];
	[theOKButton setButtonType:NSMomentaryPushInButton];
	[theOKButton setTarget:self];
	[theOKButton setAction:@selector(spaceSelectionWindowOK:)];
	[theOKButton setTitle:GetLocalizedTextAsNSString(u"OK")];
	[theOKButton setBordered:YES];
	[theOKButton setBezelStyle:NSRoundedBezelStyle];
	[itsSpaceSelectionWindow setDefaultButtonCell:[theOKButton cell]];

	theCancelButton = [[NSButton alloc] initWithFrame:(NSRect)
	{
		{
			SSS_TOTAL_WIDTH - 2*SSS_BUTTON_WIDTH - SSS_MARGIN_H - SSS_BUTTON_TO_BUTTON_H,
			SSS_BUTTON_TO_FRAME_V
		},
		{
			SSS_BUTTON_WIDTH,
			SSS_BUTTON_HEIGHT
		}
	}];
	[[itsSpaceSelectionWindow contentView] addSubview:theCancelButton];
	[theCancelButton setButtonType:NSMomentaryPushInButton];
	[theCancelButton setTarget:self];
	[theCancelButton setAction:@selector(spaceSelectionWindowCancel:)];
	[theCancelButton setTitle:GetLocalizedTextAsNSString(u"Cancel")];
	[theCancelButton setBordered:YES];
	[theCancelButton setBezelStyle:NSRoundedBezelStyle];
	
	theTitle = [[NSTextField alloc] initWithFrame:(NSRect)
	{
		{
			SSS_MARGIN_H,
			SSS_FOOTER_HEIGHT + SSS_BODY_HEIGHT
		},
		{
			SSS_BODY_WIDTH,
			SSS_TITLE_HEIGHT
		}
	}];
	[[itsSpaceSelectionWindow contentView] addSubview:theTitle];
	[theTitle setBezeled:NO];
	[theTitle setDrawsBackground:NO];
	[theTitle setSelectable:NO];
	[theTitle setFont:[self spaceSelectionTitleFont]];
	[theTitle setAlignment:NSCenterTextAlignment];
	[theTitle setStringValue:GetLocalizedTextAsNSString(u"Choose a Space")];

	//	Display itsSpaceSelectionWindow as a sheet.  
	//	-beginSheet:completionHandler: returns immediately.
	//	When the user clicks the OK or Cancel button or double-clicks a space name,
	//	it will call a method which in turn calls -dismissSpaceSelectionWindow.
	[itsWindow beginSheet:itsSpaceSelectionWindow completionHandler:^(NSModalResponse returnCode){}];
}

- (NSFont *)spaceSelectionTitleFont
{
	NSString	*theFontName	= nil;

	if (IsCurrentLanguage(u"ja"))
		theFontName = @"Hiragino Kaku Gothic Pro W3";
	else
	if (IsCurrentLanguage(u"ko"))
		theFontName = @"AppleGothic Regular";
	else
		theFontName = @"Helvetica";

	return [NSFont fontWithName:theFontName size:18.0];
}

- (void)refreshSpaceSelectionLanguage
{
	NSView				*theContentView;
	NSArray<NSView *>	*theSubviews;
	NSView				*theSubview;
	NSButton			*theButton;
	SEL					theAction;
	NSTextField			*theTitle;

	if (itsSpaceSelectionWindow != nil)
	{
		theContentView	= [itsSpaceSelectionWindow contentView];
		theSubviews		= [[theContentView subviews] copy];	//	NSView documentation says to copy before using.

		for (theSubview in theSubviews)
		{
			//	Re-localize the OK and Cancel buttons using the new language.
			if ([theSubview isKindOfClass:[NSButton class]])
			{
				theButton = (NSButton *)theSubview;
				theAction = [theButton action];
				
				if (theAction == @selector(spaceSelectionWindowOK:))
					[theButton setTitle:GetLocalizedTextAsNSString(u"OK")];

				if (theAction == @selector(spaceSelectionWindowCancel:))
					[theButton setTitle:GetLocalizedTextAsNSString(u"Cancel")];
			}

			//	Re-localize the title using the new language.
			if ([theSubview isKindOfClass:[NSTextField class]])
			{
				theTitle = (NSTextField *)theSubview;
				[theTitle setFont:[self spaceSelectionTitleFont]];
				[theTitle setStringValue:GetLocalizedTextAsNSString(u"Choose a Space")];
			}
		}
	}
}

static NSArray<NSObject *> *CreateFileTree(
	NSString	*aDirectoryPath)	//	full pathname
{
	NSFileManager				*theFileManager;
	BOOL						theDirectoryExists,
								theIsDirectoryFlag;
	NSMutableArray<NSObject *>	*theFileTree;	//	will contain an NSString as object 0,
												//		followed by zero or more NSArrays
												//		as objects 1 through n
	NSArray<NSString *>			*theDirectoryContents;
	NSString					*theItemName,
								*theItemPath;
	NSArray<NSObject *>			*theSubtree;

	theFileManager = [NSFileManager defaultManager];

	theDirectoryExists = [theFileManager fileExistsAtPath:aDirectoryPath isDirectory:&theIsDirectoryFlag]; 
	if ( ! theDirectoryExists || ! theIsDirectoryFlag )
		return nil;	//	should never occur
	
	theFileTree = [[NSMutableArray<NSObject *> alloc] initWithCapacity:32];

	//	theFileTree stores its own directory path as object 0.
	[theFileTree addObject:aDirectoryPath];

	//	theFileTree stores its n children as objects 1 through n.
	theDirectoryContents = [theFileManager contentsOfDirectoryAtPath:aDirectoryPath error:NULL];
	for (theItemName in theDirectoryContents)
	{
		theItemPath = [aDirectoryPath stringByAppendingPathComponent:theItemName];
		
		[theFileManager fileExistsAtPath:theItemPath isDirectory:&theIsDirectoryFlag];
		if (theIsDirectoryFlag)
		{
			theSubtree = CreateFileTree(theItemPath);
			[theFileTree addObject:theSubtree];
		}
		else
		{
			if ([theItemPath hasSuffix:@".gen"])
				[theFileTree addObject:theItemPath];
		}
	}
	
	return theFileTree;
}

static bool OpenContainingDirectoriesAndSelectItem(
	NSOutlineView		*anOutlineView,
	NSArray<NSObject *>	*aDirectoryItem,
	NSString			*aGeneratorFilePath)
{
	id	theItem;
	
	//	Subdirectories ignore -expandItem unless parent items
	//	are already expanded.  So we must tenatively expand aDirectoryItem
	//	and then close it before returning if aGeneratorFilePath
	//	doesn't lie within.
	[anOutlineView expandItem:aDirectoryItem];
	
	for (theItem in aDirectoryItem)
	{
		if ([theItem isKindOfClass:[NSArray class]])
		{
			if (OpenContainingDirectoriesAndSelectItem(
					anOutlineView, theItem, aGeneratorFilePath))
			{
				return true;
			}
		}
		
		if ([theItem isKindOfClass:[NSString class]])
		{
			if ([theItem isEqualToString:aGeneratorFilePath])
			{
				[anOutlineView
					selectRowIndexes:		[NSIndexSet indexSetWithIndex:
												[anOutlineView rowForItem:theItem]]
					byExtendingSelection:	NO];

				return true;
			}
		}
	}
	
	[anOutlineView collapseItem:aDirectoryItem];

	return false;
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
	//	The "parent of the root" has one child, namely the root.
	if (item == nil)
		return 1;

	//	A directory with n children has n+1 objects,
	//	because object 0 is the directory's own path.
	if ([item isKindOfClass:[NSArray class]])
		return ([item count] - 1);
	
	//	A .gen file has no children.
	if ([item isKindOfClass:[NSString class]])
		return 0;
	
	//	If we've gotten this far, we're in trouble.
	return 0;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
	//	The "parent of the root" is expandable, to see the root.
	if (item == nil)
		return YES;
	
	//	A directory is expandable, even if it's empty.
	if ([item isKindOfClass:[NSArray class]])
		return YES;
	
	//	A .gen file is not expandable.
	if ([item isKindOfClass:[NSString class]])
		return NO;
	
	//	If we've gotten this far, we're in trouble.
	return NO;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
	//	The "parent of the root" has one child, namely the root.
	if (item == nil)
		return itsFileTree;
	
	//	A directory stores children 0 though (n-1) as objects 1 through n,
	//	because object 0 is the directory's own path.
	if ([item isKindOfClass:[NSArray class]])
		return [item objectAtIndex:(index + 1)];
	
	//	If we've gotten this far, we're in trouble.
	return nil;
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	//	The "parent of the root" never gets displayed.
	if (item == nil)
		return @"/";
	
	//	Display a directory as its file name (not a full path).
	if ([item isKindOfClass:[NSArray class]])
		return [[item objectAtIndex:0] lastPathComponent];
	
	//	Display a .gen file as the extensionless file name.
	if ([item isKindOfClass:[NSString class]])
		return [[item lastPathComponent] stringByDeletingPathExtension];
	
	//	If we've gotten this far, we're in trouble.
	return @"internal error";
}

- (void)spaceSelectionWindowOK:(id)sender
{
	NSButton	*theOKButton	= nil;

	if ([sender isKindOfClass:[NSButton class]])	//	expect the OK button
	{
		theOKButton = (NSButton *) sender;
		
		if ([theOKButton window] == itsSpaceSelectionWindow)
		{
			[self readGeneratorFile];
			[self dismissSpaceSelectionWindow];
		}
	}
}

- (void)spaceSelectionWindowCancel:(id)sender
{
	NSButton	*theCancelButton	= nil;

	if ([sender isKindOfClass:[NSButton class]])	//	expect the Cancel button
	{
		theCancelButton = (NSButton *) sender;

		if ([theCancelButton window] == itsSpaceSelectionWindow)
		{
			[self dismissSpaceSelectionWindow];
		}
	}
}

- (void)spaceSelectionWindowDoubleClick:(id)sender
{
	//	The user has double-clicked a space 
	//	in the space selection sheet's NSOutlineView.

	if (sender == itsOutlineView)
	{
		[self readGeneratorFile];
		[self dismissSpaceSelectionWindow];
	}
}

- (void)readGeneratorFile
{
	id	theSelectedItem	= nil;

	//	Locate the selected item.
	theSelectedItem = [itsOutlineView itemAtRow:[itsOutlineView selectedRow]];
	
	//	Accept file names, but ignore directory names.
	if ([theSelectedItem isKindOfClass:[NSString class]])
	{
		[self openGeneratorFile:(NSString *)theSelectedItem];
	}
}

- (void)dismissSpaceSelectionWindow
{
	//	Disconnect itsOutlineView's data source
	//	and clear our strong reference to it.
	[itsOutlineView setDataSource:nil];
	itsOutlineView = nil;

	//	Clear our strong reference to the data.
	//	Releasing itsFileTree's root should release the whole tree.
	itsFileTree = nil;

	//	Dismiss the sheet.
	[itsWindow endSheet:itsSpaceSelectionWindow];
	itsSpaceSelectionWindow	= nil;
}

- (void)openGeneratorFile:(NSString *)aFilePath	//	full pathname
{
	ErrorText		theError		= NULL;
	NSData			*theRawData		= nil;
	unsigned int	theFileSize		= 0;
	Byte			*theRawBytes	= NULL;
	ModelData		*md				= NULL;

	//	Read the file's raw bytes.
	theRawData = [NSData dataWithContentsOfFile:aFilePath];
	if (theRawData == NULL)
	{
		theError = u"Matrix file is missing or empty.";
		goto CleanUpOpenGeneratorFile;
	}
	
	//	Append a terminating zero to the raw data.
	theFileSize = (unsigned int)[theRawData length];
	theRawBytes = (Byte *) GET_MEMORY(theFileSize + 1);	//	allow room for a terminating zero
	if (theRawBytes == NULL)
	{
		theError = u"Could get memory to copy matrices.";
		goto CleanUpOpenGeneratorFile;
	}
	[theRawData getBytes:theRawBytes length:theFileSize];
	theRawBytes[theFileSize] = 0;	//	terminating zero

	//	Try to parse the file.
	[itsModel lockModelData:&md];
	theError = LoadGeneratorFile(md, theRawBytes);
		//	Test for theErrorMessage != NULL follows below.
		//
		//	First, though, whether we succeed or fail,
		//	we need to update the shaders, VBOs and VAOs.
		//
		//	Request updates to OpenGL(ES) resources before unlocking the ModelData,
		//	to ensure that the rendering code finds the ModelData and the GraphicsDataGL
		//	in a consistent state.
	[itsCurvedSpacesView requestShaderUpdate];
	[itsCurvedSpacesView requestVBOUpdate];		//	triggers a VAO refresh as well
	[itsModel unlockModelData:&md];
	if (theError != NULL)
		goto CleanUpOpenGeneratorFile;

	//	Set the window title.
//	[itsWindow setTitleWithRepresentedFilename:aFilePath];	//	window name includes .gen suffix
	itsGeneratorFilePath = [aFilePath copy];
	[itsWindow setTitle:[[itsGeneratorFilePath lastPathComponent] stringByDeletingPathExtension]];

#ifdef HANTZSCHE_WENDT_AXES
	[itsModel lockModelData:&md];
	md->itsHantzscheWendtSpaceIsLoaded = [[aFilePath lastPathComponent] isEqualToString:@"Hantzsche Wendt.gen"];
	[itsModel unlockModelData:&md];
#endif

CleanUpOpenGeneratorFile:

	FREE_MEMORY_SAFELY(theRawBytes);
	
	if (theError != NULL)
	{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
		[itsCurvedSpacesView errorWithTitle:u"Error reading matrix file" message:theError];
#else
		ErrorMessage(theError, u"Error reading matrix file");
#endif
	}
}

#ifdef SAVE_ANIMATION

#pragma mark QuickTime Recording

- (void)qtStartRecording:(id)sender
{
	NSSavePanel	*theSavePanel;
	NSString	*theFilePath;

#ifdef CURVED_SPACES_MOUSE_INTERFACE
	[itsCurvedSpacesView exitNavigationalMode];
#endif
	
	if (_itsFlightPathFile != nil)
	{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
		[itsCurvedSpacesView errorWithTitle:u"Internal Error" message:u"Flight path file unexpectedly busy in qtStartRecording."];
#else
		ErrorMessage(	u"Flight path file unexpectedly busy in qtStartRecording.",
						u"Internal Error");
#endif
		return;
	}

	//	Pause the animation while the user selects a flight file name, 
	//	so the movie will start exactly at the desired moment.
	[itsCurvedSpacesView pauseAnimation];
	
	theSavePanel = [NSSavePanel savePanel];
	[theSavePanel setNameFieldLabel:@"Save Flight As"];
	if ([theSavePanel runModal] == NSFileHandlingPanelOKButton)
	{
		theFilePath = [theSavePanel filename];
		[[NSFileManager defaultManager] createFileAtPath:theFilePath contents:nil attributes:nil];
		[self setItsFlightPathFile:[NSFileHandle fileHandleForWritingAtPath:theFilePath]];
		if (_itsFlightPathFile == nil)
		{
#ifdef CURVED_SPACES_MOUSE_INTERFACE
			[itsCurvedSpacesView errorWithTitle:u"I/O Error" message:u"Couldn't open output file in qtStartRecording."];
#else
			ErrorMessage(	u"Couldn't open output file in qtStartRecording.",
							u"I/O Error");
#endif
		}
	}

	[itsCurvedSpacesView resumeAnimation];
}

- (void)qtStopRecording:(id)sender
{
	[_itsFlightPathFile closeFile];
	[self setItsFlightPathFile:nil];
}

- (void)qtPreviewMovie:(id)sender
{
	[itsCurvedSpacesView qtPlayMovieToFile:false];
}

- (void)qtRenderMovie:(id)sender
{
	[itsCurvedSpacesView qtPlayMovieToFile:true];
}

#endif	//	SAVE_ANIMATION


@end
