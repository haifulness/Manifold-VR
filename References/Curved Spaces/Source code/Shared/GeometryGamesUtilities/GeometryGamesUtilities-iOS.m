//	GeometryGamesUtilities-iOS.m
//
//	Implements
//
//		functions declared in GeometryGamesUtilities-Common.h that have 
//		platform-independent declarations but platform-dependent implementations
//	and
//		all functions declared in GeometryGamesUtilities-iOS.h.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesUtilities-Common.h"
#import "GeometryGamesUtilities-iOS.h"
#import "GeometryGamesUtilities-Mac-iOS.h"


//	How big should a thumbnail image be?
#define THUMBNAIL_IMAGE_SIZE_IPAD	128.0	//	in points, not pixels
#define THUMBNAIL_IMAGE_SIZE_IPHONE	 64.0	//	in points, not pixels


static void	ReleaseImagePixels(void *info, const void *data, size_t size);


#pragma mark -
#pragma mark functions with platform-independent declarations

#pragma mark -
#pragma mark error messages

static bool	gErrorMessaseIsActive	= false;

static void ShowMessage(ErrorText aMessage, ErrorText aTitle, bool anErrorFlag, bool aFatalFlag);

//	Use FatalError() for situations the user might conceivably encounter,
//	like an OpenGL setup or render error.  Otherwise use GeometryGamesAssertionFailed.
void FatalError(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ShowMessage(aMessage, aTitle, true, true);
}

void ErrorMessage(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ShowMessage(aMessage, aTitle, true, false);
}

void InfoMessage(
	ErrorText	aMessage,	//	UTF-16, may be NULL
	ErrorText	aTitle)		//	UTF-16, may be NULL
{
	ShowMessage(aMessage, aTitle, false, false);
}

static void ShowMessage(
	ErrorText	aMessage,		//	UTF-16, may be NULL
	ErrorText	aTitle,			//	UTF-16, may be NULL
	bool		anErrorFlag,	//	Pass true to
								//			- suppress rendering
								//			- suppress other messages
								//		while this message is up.
								//		(Warning:  This is dangerous in
								//		the Flandrau (science museum) version of Torus Games,
								//			whose timeout feature may dismiss presented views
								//	without simulating clicks in their OK buttons.)
	bool		aFatalFlag)		//	Exit the program after user dismisses the error alert?
{
	UIViewController	*thePresentingController;
	UIAlertController	*theAlertController;

	if ( (! anErrorFlag) || (! gErrorMessaseIsActive) )
	{
		//	Presenting theAlertController does *not* cancel any pending touches
		//	in the underlying view, so the underlying view could in principle
		//	continue to generate additional error messages even after theAlertController
		//	is up.  To keep things simple, ignore any such additional error messages
		//	while theAlertController is up.
		if (anErrorFlag)
			gErrorMessaseIsActive = true;

		//	The following code first finds the window's root view controller,
		//	and then walks up the stack of already presented view controllers (if any)
		//	to reach the topmost view controller.
		//
		//		Warning:  This approach is less than perfect,
		//		because if the code that posted the error message
		//		happens to dismiss thePresentingController,
		//		then theAlertController will get silently dismissed
		//		along with it, and the user will see the error message
		//		for only a small fraction of a second.  Worse still,
		//		gErrorMessaseIsActive would not get reset to false,
		//		nor would exit() get called in the case of a fatal error.
		//
		thePresentingController = [[[UIApplication sharedApplication] keyWindow] rootViewController];
		while ([thePresentingController presentedViewController] != nil)
			thePresentingController = [thePresentingController presentedViewController];

		theAlertController = [UIAlertController
			alertControllerWithTitle:	GetNSStringFromZeroTerminatedString(aTitle)
			message:					GetNSStringFromZeroTerminatedString(aMessage)
			preferredStyle:				UIAlertControllerStyleAlert];

		[theAlertController addAction:[UIAlertAction
			actionWithTitle:	@"OK"
			style:				UIAlertActionStyleDefault
			handler:^(UIAlertAction *action)
			{
				UNUSED_PARAMETER(action);

				if (anErrorFlag)
					gErrorMessaseIsActive = false;
				
				if (aFatalFlag)
					exit(1);
			}]];

		//	Note:  This call to presentViewController:animated:completion:
		//	returns immediately.  Thus even FatalError() calls get handled
		//	asynchronously, so some care is required in the rest of the app.
		//	User input will be blocked, but the run loop will keep running.
		//
		//	To avoid problems, GeometryGamesGraphicsViewController's animationTimerFired method
		//	suppresses rendering whenever IsShowingErrorAlert() returns true.

		[thePresentingController
			presentViewController:	theAlertController
			animated:				YES
			completion:				nil];
	}
}

bool IsShowingErrorAlert(void)
{
	return gErrorMessaseIsActive;
}


#pragma mark -
#pragma mark functions with iOS-specific declarations

#pragma mark -
#pragma mark popovers

void PresentPopoverFromToolbarButton(
	UIViewController	*aPresentingViewController,
	UIViewController	*aPresentedViewController,	//	OK to pass nil
	UIBarButtonItem		*anAnchorButton,
	NSArray<UIView *>	*somePassthroughViews)		//	OK to pass nil
{
	UIPopoverPresentationController	*thePresentationController;
	
	if (aPresentedViewController != nil)
	{
		//	According to Apple's official UIPopverPresentationController Class Reference
		//
		//		https://developer.apple.com/library/ios/documentation/UIKit/Reference/UIPopoverPresentationController_class/
		//
		//	we should present the popover first and then configure
		//	the UIPopoverPresentationController afterwards.
		//	Quoting the documentation:
		//
		//		"Configuring the popover presentation controller
		//		after calling presentViewController:animated:completion:
		//		might seem counter-intuitive but UIKit does not create
		//		a presentation controller until after you initiate a presentation.
		//		In addition, UIKit must wait until the next update cycle to display
		//		new content onscreen anyway. That delay gives you time to configure
		//		the presentation controller for your popover."
		//

		[aPresentedViewController setModalPresentationStyle:UIModalPresentationPopover];

		[aPresentingViewController
			presentViewController:	aPresentedViewController
			animated:				YES
			completion:				nil];
		
		//	Note:  The UIKit will automatically add all siblings of anAnchorButton
		//	to somePassthroughViews, but not anAnchorButton itself, which is
		//	just the behavior we want in most circumstances, so that a second tap
		//	on anAnchorButton automatically dismisses the popover.
		//
		thePresentationController = [aPresentedViewController popoverPresentationController];
		[thePresentationController setBarButtonItem:anAnchorButton];
		[thePresentationController setPassthroughViews:somePassthroughViews];
	}
}

void PresentPopoverFromRectInView(
	UIViewController		*aPresentingViewController,
	UIViewController		*aPresentedViewController,	//	OK to pass nil
	CGRect					anAnchorRect,
	UIView					*anAnchorView,
	UIPopoverArrowDirection	somePermittedArrowDirections,
	NSArray<UIView *>		*somePassthroughViews)		//	OK to pass nil
{
	UIPopoverPresentationController	*thePresentationController;
	
	if (aPresentedViewController != nil)
	{
		//	See comments in PresentPopoverFromToolbarButton() confirming that
		//	it really is correct to present the view controller first
		//	and then configure its controller afterwards.

		[aPresentedViewController setModalPresentationStyle:UIModalPresentationPopover];

		[aPresentingViewController
			presentViewController:	aPresentedViewController
			animated:				YES
			completion:				nil];
		
		thePresentationController = [aPresentedViewController popoverPresentationController];
		[thePresentationController setSourceRect:anAnchorRect];
		[thePresentationController setSourceView:anAnchorView];
		[thePresentationController setPermittedArrowDirections:somePermittedArrowDirections];
		[thePresentationController setPassthroughViews:somePassthroughViews];
	}
}


UIImage *ImageWithPixels(
	CGSize		anImageSize,
	PixelRGBA	**somePixels)	//	1.	The returned UIImage will take ownership of *somePixels,
								//		and not free them until it's deallocated.
								//	2.	Because the caller no longer owns *somePixels,
								//		the present function will set *somePixels to NULL.
								//	3.	somePixels should be in OpenGL's bottom-up row order.
								//		The code below will flip them.
{
	CGDataProviderRef	theDataProvider	= NULL;
	CGColorSpaceRef		theColorSpace	= NULL;
	CGImageRef			theImageRef		= NULL;
	UIImage				*theImage		= nil;
	
	if (somePixels == NULL || *somePixels == NULL)
		return nil;
	
	//	If we're using OpenGL ES we'll need to flip the image,
	//	but if we're using Metal we won't.
	//	Assume that if Metal is available, we'll be using it.
	if ( ! MetalIsAvailable() )
	{
		//	Flip the image top-to-bottom to convert it
		//	from OpenGL ES's bottom-up coordinates to Cocoa Touch's top-down coordinates.
		InvertRawImage(anImageSize.width, anImageSize.height, *somePixels);
	}

	theDataProvider = CGDataProviderCreateWithData(	NULL,
													*somePixels,
													anImageSize.width * anImageSize.height * sizeof(PixelRGBA),
													ReleaseImagePixels);
	*somePixels = NULL;	//	theDataProvider now owns *somePixels
	
	theColorSpace = CGColorSpaceCreateDeviceRGB();
	if (theColorSpace == NULL)
		goto CleanUpImageWithPixels;

	theImageRef = CGImageCreate(anImageSize.width, anImageSize.height, 8, 32, anImageSize.width * 4,
					theColorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault,
					theDataProvider, NULL, true, kCGRenderingIntentDefault);
	
	theImage = [[UIImage alloc] initWithCGImage:theImageRef];

CleanUpImageWithPixels:

	CGImageRelease(theImageRef);
	CGColorSpaceRelease(theColorSpace);
	CGDataProviderRelease(theDataProvider);
	
	return theImage;
}

static void ReleaseImagePixels(void *info, const void *data, size_t size)
{
	void	*thePixels;

	UNUSED_PARAMETER(info);
	UNUSED_PARAMETER(size);
	
	thePixels = (void *)data;
	
	FREE_MEMORY_SAFELY(thePixels);
}


NSString *GetFullFilePath(
	NSString	*aFileName,
	NSString	*aFileExtension)	//	includes the leading '.'
									//		for example ".txt" or ".png"
{
	NSURL		*theDirectory;
	NSString	*theExtendedFileName,
				*theFullFilePath;
	
	theDirectory = [[NSFileManager defaultManager]
		URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask
		appropriateForURL:nil create:YES error:nil];
		
	theExtendedFileName = [aFileName stringByAppendingString:aFileExtension];
	
	theFullFilePath = [[theDirectory URLByAppendingPathComponent:theExtendedFileName] path];
	
	return theFullFilePath;
}


UIImage *LoadScaledThumbnailImage(
	NSString	*aFileName)	//	file name without path and without ".png" extension
{
	UIImage	*theThumbnailRaw,
			*theThumbnailScaled;

	theThumbnailRaw	= [UIImage imageWithContentsOfFile:GetFullFilePath(aFileName, @".png")];
	
	if (theThumbnailRaw != nil)
	{
		theThumbnailScaled	= [UIImage	imageWithCGImage:	theThumbnailRaw.CGImage
										scale:				[[UIScreen mainScreen] scale]
										orientation:		theThumbnailRaw.imageOrientation];
	}
	else
	{
		theThumbnailScaled = nil;
	}

	if (theThumbnailScaled != nil)
		return theThumbnailScaled;
	else
		return [UIImage imageNamed:@"asterisk.png"];
}

CGFloat GetThumbnailSizePt(void)
{
	//	This could someday become an app-specific function,
	//	if different apps ever want to use different thumbnail sizes.
	
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
		return THUMBNAIL_IMAGE_SIZE_IPAD;
	else
		return THUMBNAIL_IMAGE_SIZE_IPHONE;
}

CGFloat GetThumbnailSizePx(void)
{
	return GetThumbnailSizePt()					//	thumbnail size in points
			* [[UIScreen mainScreen] scale];	//	pixels/point;
}
