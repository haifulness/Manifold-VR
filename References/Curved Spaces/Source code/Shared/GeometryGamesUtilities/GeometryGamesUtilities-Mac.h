//	GeometryGamesUtilities-Mac.h
//
//	Declares Geometry Games functions with Mac-specific declarations.
//	The file GeometryGamesUtilities-Mac.c implements these functions.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#pragma once

#import <Cocoa/Cocoa.h>
#import "GeometryGames-Common.h"	//	for Char16


extern NSMenu				*AddSubmenuWithTitle(NSMenu *aParentMenu, Char16 *aSubmenuTitleKey);
extern NSMenuItem			*MenuItemWithTitleActionTag(NSString *aTitle, SEL anAction, NSInteger aTag);
extern NSMenuItem			*MenuItemWithTitleActionKeyTag(NSString *aTitle, SEL anAction, unichar aKeyEquivalent, NSInteger aTag);

extern NSOpenGLPixelFormat	*GetPixelFormat(bool aDepthBufferFlag, bool aMultisampleFlag, bool aStereoBuffersFlag, bool aStencilBufferFlag);

extern void					MakeImageSizeView(
								NSView		* __strong *aView,
								NSTextField	* __strong *aWidthLabel,
								NSTextField	* __strong *aWidthValue,
								NSTextField	* __strong *aHeightLabel,
								NSTextField	* __strong *aHeightValue);
