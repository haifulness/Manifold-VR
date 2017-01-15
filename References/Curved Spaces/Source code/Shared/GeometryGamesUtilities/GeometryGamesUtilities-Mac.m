//	GeometryGamesUtilities-Mac.m
//
//	Implements
//
//		functions declared in GeometryGamesUtilities-Common.h that have 
//		platform-independent declarations but platform-dependent implementations
//	and
//		all functions declared in GeometryGamesUtilities-Mac.h.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesUtilities-Mac.h"
#import "GeometryGamesUtilities-Mac-iOS.h"	//	for GetLocalizedTextAsNSString()
#import "GeometryGamesUtilities-Common.h"
#import "GeometryGamesLocalization.h"


//	It's unsafe for a background thread to display an alert,
//	so ErrorMessage() will package up the error message and title
//	as an ErrorReporter object and ask the main thread
//	to display the message.
@interface ErrorReporter : NSObject
{
	ErrorText	itsMessage,
				itsTitle;
}
- (id)initWithMessage:(ErrorText)aMessage title:(ErrorText)aTitle;
- (void)displayMessage;
@end
@implementation ErrorReporter
- (id)initWithMessage:(ErrorText)aMessage title:(ErrorText)aTitle
{
	self = [super init];
	if (self != nil)
	{
		itsMessage	= aMessage;
		itsTitle	= aTitle;
	}
	return self;
}
- (void)displayMessage
{
	ErrorMessage(itsMessage, itsTitle);
}
@end


#pragma mark -
#pragma mark functions with platform-independent declarations

void FatalError(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ErrorMessage(aMessage, aTitle);
	exit(1);
}

void ErrorMessage(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ErrorReporter	*theErrorReporter;
	NSString		*theTitle,
					*theMessage;
	NSAlert			*theAlert;

	if ( ! [NSThread isMainThread] )
	{
		//	Using an NSAlert from a background thread is unsafe.
		//	Package up aMessage and aTitle as an ErrorReporter object,
		//	and let the main thread display them.
		theErrorReporter = [[ErrorReporter alloc] initWithMessage:aMessage title:aTitle];
		[theErrorReporter performSelectorOnMainThread:@selector(displayMessage) withObject:nil waitUntilDone:YES];
		theErrorReporter = nil;
	}
	else
	{
		//	We're on the main thread, so it's safe to use an NSAlert.
		
		//	Convert aTitle and aMessage to NSString objects.
		theTitle	= (aTitle   != NULL ? GetNSStringFromZeroTerminatedString(aTitle  ) : @"");
		theMessage	= (aMessage != NULL ? GetNSStringFromZeroTerminatedString(aMessage) : @"");

#ifdef DEBUG
		//	Log the message to the console.
		//	(This could be especially useful if an error occurs
		//	while the application is terminating.)
		NSLog(@"%@:  %@", theTitle, theMessage);
#endif
		
		theAlert = [[NSAlert alloc] init];
		[theAlert setMessageText:theTitle];
		[theAlert setInformativeText:theMessage];
		(void) [theAlert runModal];	//	blocks until user responds to alert
	}
}


#pragma mark -
#pragma mark functions with Mac-specific declarations

NSMenu *AddSubmenuWithTitle(
	NSMenu	*aParentMenu,
	Char16	*aSubmenuTitleKey)
{
	NSString	*theLocalizedTitle;
	NSMenuItem	*theNewParentMenuItem;
	NSMenu		*theNewMenu;

	//	Technical Note:  For top-level menus (i.e. those attached to the menu bar),
	//	theNewMenu's title gets displayed while theNewParentMenuItem's title gets ignored.
	//	For submenus, the reverse is true.  To be safe, pass the same title to both.

	//	Localize aSubmenuTitle to the current language.
	theLocalizedTitle = GetLocalizedTextAsNSString(aSubmenuTitleKey);

	//	Create a new item for aParentMenu, and a new menu to attach to it.
	theNewParentMenuItem	= [[NSMenuItem alloc] initWithTitle:theLocalizedTitle action:NULL keyEquivalent:@""];
	theNewMenu				= [[NSMenu alloc] initWithTitle:theLocalizedTitle];
	
	//	Attach theNewMenu to theNewParentMenuItem.
	[theNewParentMenuItem setSubmenu:theNewMenu];
	
	//	Add theNewParentMenuItem to aParentMenu.
	[aParentMenu addItem:theNewParentMenuItem];
	
	//	Return theNewMenu.
	return theNewMenu;
}

NSMenuItem *MenuItemWithTitleActionTag(
	NSString	*aTitle,
	SEL			anAction,
	NSInteger	aTag)
{
	return MenuItemWithTitleActionKeyTag(aTitle, anAction, 0, aTag);
}

NSMenuItem *MenuItemWithTitleActionKeyTag(
	NSString	*aTitle,
	SEL			anAction,
	unichar		aKeyEquivalent,	//	pass 0 if no key equivalent is needed
	NSInteger	aTag)
{
	NSString	*theKeyEquivalent;
	NSMenuItem	*theMenuItem;
	
	if (aKeyEquivalent != 0)
		theKeyEquivalent = [[NSString alloc] initWithCharacters:&aKeyEquivalent length:1];
	else
		theKeyEquivalent = [[NSString alloc] init];
	
	theMenuItem = [[NSMenuItem alloc] initWithTitle:aTitle action:anAction keyEquivalent:theKeyEquivalent];
	[theMenuItem setTag:aTag];
	
	return theMenuItem;
}


static void GetPixelFormatAttributes(
	NSOpenGLPixelFormatAttribute anAttributeArray[], unsigned int anAttributeArrayLength,
	bool aDepthBufferFlag, bool aMultisampleFlag, bool aStereoBuffersFlag, bool aStencilBufferFlag);

NSOpenGLPixelFormat *GetPixelFormat(
	bool	aDepthBufferFlag,
	bool	aMultisampleFlag,
	bool	aStereoBuffersFlag,
	bool	aStencilBufferFlag)
{
	NSOpenGLPixelFormatAttribute	thePixelFormatAttributes[64];
	NSOpenGLPixelFormat				*thePixelFormat;
	unsigned int					i;
	
	//	Attribute sets, in order of preference (best to worst).
	struct
	{
		bool	itsDepthBufferFlag,
				itsMultisampleFlag,
				itsStereoBuffersFlag,
				itsStencilBufferFlag;
	} theAttributeSets[] =
	{
		{aDepthBufferFlag,	aMultisampleFlag,	aStereoBuffersFlag,	aStencilBufferFlag	},
		{aDepthBufferFlag,	false,				aStereoBuffersFlag,	aStencilBufferFlag	},
		{aDepthBufferFlag,	aMultisampleFlag,	false,				false				},
		{aDepthBufferFlag,	false,				false,				false				}
	};
	
	GEOMETRY_GAMES_ASSERT(
		! (aStereoBuffersFlag && aStencilBufferFlag),
		"We never plan to use quad-buffered stereo and interlaced stereo at the same time.");
	
	//	Try to create a pixel format.
	//	Start with the most desirable attribute set
	//	and work towards the least desirable set.
	//	Return the first pixel format that the host supports.
	for (i = 0; i < sizeof(theAttributeSets); i++)
	{
		GetPixelFormatAttributes(	thePixelFormatAttributes,
									BUFFER_LENGTH(thePixelFormatAttributes),
									theAttributeSets[i].itsDepthBufferFlag,
									theAttributeSets[i].itsMultisampleFlag,
									theAttributeSets[i].itsStereoBuffersFlag,
									theAttributeSets[i].itsStencilBufferFlag);
		thePixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:thePixelFormatAttributes];
		if (thePixelFormat != nil)
			return thePixelFormat;
	}

	return nil;
}

static void GetPixelFormatAttributes(
	NSOpenGLPixelFormatAttribute	anAttributeArray[],
	unsigned int					anAttributeArrayLength,	//	max number of entries, including terminating zero
	bool							aDepthBufferFlag,
	bool							aMultisampleFlag,
	bool							aStereoBuffersFlag,
	bool							aStencilBufferFlag)
{
	unsigned int	theNumAttributes;

	theNumAttributes = 0;
	
	//	Profile attribute
	if (theNumAttributes + 2 > anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFAOpenGLProfile;
	anAttributeArray[theNumAttributes++] = NSOpenGLProfileVersion3_2Core;
	
	//	Basic attributes

	if (theNumAttributes == anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFAAccelerated;

	if (theNumAttributes == anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFANoRecovery;

	if (theNumAttributes == anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFADoubleBuffer;

	if (theNumAttributes + 2 > anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFAColorSize;
	anAttributeArray[theNumAttributes++] = 32;	//	Total RGBA bits.  Pass 32 for RGBA(8,8,8,8).

	if (theNumAttributes + 2 > anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFAAlphaSize;
	anAttributeArray[theNumAttributes++] = 8;
	
	//	Depth buffer attribute
	if (theNumAttributes + 2 > anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = NSOpenGLPFADepthSize;
	anAttributeArray[theNumAttributes++] = (aDepthBufferFlag ? 24 : 0);
	
	//	Multisample attributes
	if (aMultisampleFlag)
	{
		if (theNumAttributes + 5 > anAttributeArrayLength)
			goto GetPixelFormatAttributesFailed;
		anAttributeArray[theNumAttributes++] = NSOpenGLPFAMultisample;
		anAttributeArray[theNumAttributes++] = NSOpenGLPFASampleBuffers;
		anAttributeArray[theNumAttributes++] = 1;
		anAttributeArray[theNumAttributes++] = NSOpenGLPFASamples;
		anAttributeArray[theNumAttributes++] = 8;	//	Just guess a large number and let 
													//	the driver reduce it if necessary.
	}
	
	//	Stereo attribute
	if (aStereoBuffersFlag)
	{
		if (theNumAttributes == anAttributeArrayLength)
			goto GetPixelFormatAttributesFailed;
		anAttributeArray[theNumAttributes++] = NSOpenGLPFAStereo;
	}
	
	//	Stencil buffer attribute
	if (aStencilBufferFlag)
	{
		if (theNumAttributes + 2 > anAttributeArrayLength)
			goto GetPixelFormatAttributesFailed;
		anAttributeArray[theNumAttributes++] = NSOpenGLPFAStencilSize;
		anAttributeArray[theNumAttributes++] = 8;
	}
	
	//	Terminating zero
	if (theNumAttributes == anAttributeArrayLength)
		goto GetPixelFormatAttributesFailed;
	anAttributeArray[theNumAttributes++] = 0;

	return;	//	success

GetPixelFormatAttributesFailed:
	GEOMETRY_GAMES_ASSERT(0, "anAttributeArray[] isn't long enough.");
}


void MakeImageSizeView(
	NSView		* __strong *aView,			//	output
	NSTextField	* __strong *aWidthLabel,	//	output
	NSTextField	* __strong *aWidthValue,	//	output
	NSTextField	* __strong *aHeightLabel,	//	output
	NSTextField	* __strong *aHeightValue)	//	output
{
	NSTextField	*theWidthUnits,
				*theHeightUnits;

	//	Create a view
	//
	//		+--------------------------------------+
	//		|         +------+           +------+  |
	//		|   width:|012345|px  height:|012345|px|
	//		|         +------+           +------+  |
	//		+--------------------------------------+
	//
	//	that the caller may add to the Save Image… sheet,
	//	to let the user specify a custom image width and height.
	//
	//	Note:  An alternative approach would be to create the view
	//	in an XIB file and call -loadNibNamed:owner:topLevelObjects:
	//	to load it.  However, because we're in a plain C function
	//	here, we'd need to either
	//
	//		- create a temporary dummy object with IBOutlets
	//			to receive references to the various subviews
	//	or
	//		- iterate over the top-level object's subviews
	//			to recognize the various fields by their classes and tags.
	//
	//	The XIB-based approach -- when combined with autolayout --
	//	might ultimately offer more flexibility to accommodate
	//	different languages and different font sizes.
	//	For now, though, let's keep the follow code which
	//	creates the aView and its various subviews manually.
	
	*aView = [[NSView alloc] initWithFrame:(NSRect){{0.0, 0.0},{316.0, 38.0}}];
		
	*aWidthLabel = [[NSTextField alloc] initWithFrame:
		(NSRect){{-3.0, 11.0}, {79.0, 17.0}}];
	[*aView addSubview:*aWidthLabel];
	[*aWidthLabel setAlignment:NSRightTextAlignment];
	[*aWidthLabel setBezeled:NO];
	[*aWidthLabel setEditable:NO];
	[*aWidthLabel setDrawsBackground:NO];
	[*aWidthLabel setFont:[NSFont systemFontOfSize:13.0]];
		
	*aWidthValue = [[NSTextField alloc] initWithFrame:
		(NSRect){{81.0, 8.0}, {50.0, 22.0}}];
	[*aView addSubview:*aWidthValue];
	[*aWidthValue setAlignment:NSRightTextAlignment];
	[*aWidthValue setBezeled:YES];
	[*aWidthValue setEditable:YES];
	[*aWidthValue setDrawsBackground:YES];
	[*aWidthValue setFont:[NSFont systemFontOfSize:13.0]];
		
	theWidthUnits = [[NSTextField alloc] initWithFrame:
		(NSRect){{136.0, 11.0}, {21.0, 17.0}}];
	[*aView addSubview:theWidthUnits];
	[theWidthUnits setStringValue:@"px"];
	[theWidthUnits setAlignment:NSLeftTextAlignment];
	[theWidthUnits setBezeled:NO];
	[theWidthUnits setEditable:NO];
	[theWidthUnits setDrawsBackground:NO];
	[theWidthUnits setFont:[NSFont systemFontOfSize:13.0]];
		
	*aHeightLabel = [[NSTextField alloc] initWithFrame:
		(NSRect){{159.0, 11.0}, {79.0, 17.0}}];
	[*aView addSubview:*aHeightLabel];
	[*aHeightLabel setAlignment:NSRightTextAlignment];
	[*aHeightLabel setBezeled:NO];
	[*aHeightLabel setEditable:NO];
	[*aHeightLabel setDrawsBackground:NO];
	[*aHeightLabel setFont:[NSFont systemFontOfSize:13.0]];
		
	*aHeightValue = [[NSTextField alloc] initWithFrame:
		(NSRect){{243.0, 8.0}, {50.0, 22.0}}];
	[*aView addSubview:*aHeightValue];
	[*aHeightValue setAlignment:NSRightTextAlignment];
	[*aHeightValue setBezeled:YES];
	[*aHeightValue setEditable:YES];
	[*aHeightValue setDrawsBackground:YES];
	[*aHeightValue setFont:[NSFont systemFontOfSize:13.0]];
		
	theHeightUnits = [[NSTextField alloc] initWithFrame:
		(NSRect){{298.0, 11.0}, {21.0, 17.0}}];
	[*aView addSubview:theHeightUnits];
	[theHeightUnits setStringValue:@"px"];
	[theHeightUnits setAlignment:NSLeftTextAlignment];
	[theHeightUnits setBezeled:NO];
	[theHeightUnits setEditable:NO];
	[theHeightUnits setDrawsBackground:NO];
	[theHeightUnits setFont:[NSFont systemFontOfSize:13.0]];
}
