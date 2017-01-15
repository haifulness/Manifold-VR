//	GeometryGamesWindowController.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesWindowController.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesWindowMac.h"
#import "GeometryGamesViewMac.h"
#import "GeometryGamesUtilities-Mac.h"
#import "GeometryGamesUtilities-Mac-iOS.h"


//	Privately-declared properties and methods
@interface GeometryGamesWindowController()
@end


@implementation GeometryGamesWindowController
{
}


- (id)init
{
	self = [super init];
	if (self != nil)
	{
		//	Create the model.
		itsModel = [[GeometryGamesModel alloc] init];

		//	Create the window.
		itsWindow = [[GeometryGamesWindowMac alloc]
			initWithContentRect:	(NSRect){{0, 0}, {512, 384}}	//	temporary size
			styleMask:				NSTitledWindowMask
									 | NSClosableWindowMask
									 | NSMiniaturizableWindowMask
									 | NSResizableWindowMask
			backing:				NSBackingStoreBuffered
			defer:					YES];
		[itsWindow setReleasedWhenClosed:NO];	//	ARC will release itsWindow
												//	when no object has a strong reference to it.
		[itsWindow setDelegate:self];
		[itsWindow setContentMinSize:(NSSize){256, 256}];	//	Subclass may override.
		[itsWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
		
		//	Give the subclass a chance to create a toolbar.
		//	Call -createToolbar before -createSubviews,
		//	so -createSubviews will see the expected content view bounds.
		[self createToolbar];

		//	Let the subclass populate itsWindow's contentView with subviews.
		//	The subclass may optionally assign a view to itsMainView
		//	to take advantage of this GeometryGamesWindowController's
		//	standard behaviour for copy, save, miniaturize/deminiaturize, etc.
		itsMainView = nil;
		[self createSubviews];
		
		//	Zoom the window.  As a happy side-effect,
		//	this triggers a call to -windowDidResize:,
		//	which gives the subclass a chance to arrange its subviews.
		[itsWindow zoom:self];
		
		//	Refresh all text in the current language.
		[self languageDidChange];
		
		//	Let itsMainView be itsWindow's first responder,
		//	so it will receive keystrokes.
		[itsWindow setInitialFirstResponder:itsMainView];

		//	Show itsWindow.
		[itsWindow makeKeyAndOrderFront:nil];

//		[itsWindow toggleFullScreen:self];
	}
	return self;
}

- (void)createToolbar
{
	//	Subclasses may override this method to create a toolbar.
}

- (void)createSubviews
{
	//	Subclasses may override this method
	//	to populate [itsWindow contentView] with subviews,
	//	and may optionally assign a view to itsMainView
	//	to take advantage of this GeometryGamesWindowController's
	//	standard behaviour for copy, save, miniaturize/deminiaturize, etc.

	//	If the subclass's -createSubviews method calls
	//	-setAutoresizingMask: for the subviews that it creates,
	//	then the subclass needn't implement -windowDidResize:.
	//
	//	If the subclass's -createSubviews method does not call
	//	-setAutoresizingMask: for the subviews that it creates,
	//	then the subclass should implement -windowDidResize:
	//	to arrange its subviews within the window's newly resized
	//	content view.
}

- (void)windowWillClose:(NSNotification *)aNotification
{
	UNUSED_PARAMETER(aNotification);

	//	Be careful: NSWindow does not use zeroing weak references,
	//	because it must run on Mac OS 10.7, which doesn't support them.
	//	So let's clear itsWindow's delegate manually.
	//	Someday, when NSWindow replaces
	//		@property (nullable, assign) id<NSWindowDelegate> delegate;
	//	with
	//		@property (nullable, weak) id<NSWindowDelegate> delegate;
	//	we'll probably be able to delete this call.
	//	(Or, in Swift, when
	//		unowned(unsafe) var delegate: NSWindowDelegate?
	//	gets replace with a safe version.)
	//
	[itsWindow setDelegate:nil];

	[itsWindow makeFirstResponder:nil];	//	maybe also unnecessary?

	//	The window holds no more pointers to this controller,
	//	so the app delegate may clear its reference to the controller,
	//	and let the controller get deallocated.  This will, in turn,
	//	clear the strong references to itsModel, itsWindow and itsMainView,
	//	along with any additional strong references that the subclass might hold.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
	[NSApp sendAction:@selector(removeReferenceToWindowController:) to:nil from:self];
#pragma clang diagnostic pop
}


- (BOOL)validateMenuItem:(NSMenuItem *)aMenuItem
{
	SEL	theAction;

	theAction = [aMenuItem action];

	if (theAction == @selector(commandCopyImage:))
		return YES;

	if (theAction == @selector(commandSaveImage:))
		return YES;

	return NO;
}

- (void)commandCopyImage:(id)sender
{
	NSData			*theTiff				= nil;
	NSPasteboard	*theGeneralPasteboard	= nil;
	
	UNUSED_PARAMETER(sender);

	theTiff = [itsMainView imageAsTIFF];
	
	if (theTiff != nil)
	{
		theGeneralPasteboard = [NSPasteboard generalPasteboard];
		
		[theGeneralPasteboard declareTypes:@[NSTIFFPboardType] owner:nil];
		[theGeneralPasteboard setData:theTiff forType:NSTIFFPboardType];
	}
}

- (void)commandSaveImage:(id)sender
{
	NSSavePanel				*theSavePanel			= nil;
	NSSize					theViewSize;
	NSView					*theImageSizeView		= nil;
	NSTextField				*theImageWidthLabel		= nil,
							*theImageWidthValue		= nil,
							*theImageHeightLabel	= nil,
							*theImageHeightValue	= nil;
	GeometryGamesViewMac	*theMainView			= nil;
	
	UNUSED_PARAMETER(sender);

	//	Pause the animation while the user selects a file name, 
	//	so the saved image will contain the desired frame.
	[itsMainView pauseAnimation];

	theSavePanel = [NSSavePanel savePanel];
	[theSavePanel setAllowedFileTypes:@[@"tiff"]];
//	[theSavePanel setTitle:GetLocalizedTextAsNSString(u"SavePanelTitle")];	Sheet shows no title
	[theSavePanel setNameFieldStringValue:GetLocalizedTextAsNSString(u"DefaultFileName")];
	
	//	Add a custom view to the Save panel
	//	so the user may specify the image width and height.
	MakeImageSizeView(	&theImageSizeView,
						&theImageWidthLabel,  &theImageWidthValue,
						&theImageHeightLabel, &theImageHeightValue);
	[theImageWidthLabel  setStringValue:GetLocalizedTextAsNSString(u"WidthLabel" )];
	[theImageHeightLabel setStringValue:GetLocalizedTextAsNSString(u"HeightLabel")];
	theViewSize = [itsMainView bounds].size;
	[theImageWidthValue  setIntValue:theViewSize.width];
	[theImageHeightValue setIntValue:theViewSize.height];
	[theSavePanel setAccessoryView:theImageSizeView];

	//	The completion handler is an Objective-C “block”.
	//	For a good discussion of how blocks make it easy
	//	to create strong reference cycles, see
	//
	//		https://blackpixel.com/writing/2014/03/capturing-myself.html
	//
	//	For more background on the intricacies of blocks,
	//	and how they retain the objects they refer to, see
	//
	//		developer.apple.com/library/ios/#documentation/cocoa/Conceptual/Blocks/Articles/00_Introduction.html
	//	and
	//		cocoawithlove.com/2009/10/how-blocks-are-implemented-and.html
	//
	
	//	Copy the instance variable itsMainView to the local variable theMainView,
	//	and pass it to the block.  This way the block will hold a strong reference
	//	to the view and not to "self".  (If the block accessed
	//	the instance variable "itsMainView" directly, it would need
	//	a strong reference to "self", which would create a strong reference cycle.
	//	Such a strong reference cycle might be harmless in this case --
	//	it would be broken when the user dismissed the modal sheet --
	//	but even still it makes me nervous.  At the very least it would
	//	require extra effort on the part of the programmer to think through
	//	what would happen if, say, the user closed the window before dismissing
	//	the sheet.)
	theMainView = itsMainView;
	
	[theSavePanel beginSheetModalForWindow:itsWindow
		completionHandler:^(NSInteger result)
		{
			NSSize	theNewViewSize;
			NSData	*theTiff		= nil;

			if (result == NSFileHandlingPanelOKButton)
			{
				theNewViewSize.width  = [theImageWidthValue  intValue];
				theNewViewSize.height = [theImageHeightValue intValue];

				theTiff = [theMainView imageAsTIFFofSize:theNewViewSize];

				if (theTiff != nil)
				{
					if ([theTiff writeToURL:[theSavePanel URL] atomically:YES] == NO)
						ErrorMessage(u"Couldn't save image under requested filename.", u"Save failed");
				}
			}

			[theMainView resumeAnimation];
		}
	];
}


- (void)languageDidChange
{
	[itsWindow setTitle:GetLocalizedTextAsNSString(u"WindowTitle")];
}


@end
