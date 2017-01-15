//	GeometryGamesPortfolioController.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesPortfolioController.h"
#import "GeometryGamesPortfolioView.h"
#import "GeometryGamesPopover.h"
#import "GeometryGamesGraphicsViewController.h"
#import "GeometryGamesModel.h"
#import "GeometryGamesUtilities-iOS.h"
#import "GeometryGamesUtilities-Mac-iOS.h"
#import "GeometryGamesLocalization.h"
#import "GeometryGames-OpenGL.h"	//	for -maxExportMagnificationFactor


#define NO_RECENTLY_OPENED_DRAWING	NSUIntegerMax


extern NSString	*gManifestFileName;	//	subclass must define the manifest file name


//	Privately-declared properties and methods
@interface GeometryGamesPortfolioController()

- (void)loadToolbarItems;

- (void)reloadMostRecentlyOpenedThumbnail;
- (void)maybeSelectLabelText;

- (void)showDuplicateDeleteMenuForThumbnail:(UIView *)aThumbnailIconView includeOpen:(bool)anIncludeOpenItemFlag;

- (bool)drawingContentIsLocked:(NSUInteger)aNameIndex;
- (void)unlockDrawingContent:(NSUInteger)aNameIndex;

- (void)refreshToolbarButtonEnabling;
- (void)userTappedToolbarButton:(id)sender;

- (void)maybeDeleteFile:(NSUInteger)aNameIndex;

- (void)keyboardDidShow:(NSNotification *)aNotification;
- (void)keyboardWillHide:(NSNotification *)aNotification;
- (float)keyboardHeight:(NSNotification *)aNotification withKey:(NSString *const)aKey;
- (void)setBottomInsets:(CGFloat)aBottomInset;

- (NSMutableArray<GeometryGamesThumbnail *> *)loadThumbnails;
- (void)createNewEmptyFile;
- (void)writeDrawingManifest;
- (void)openFile:(NSUInteger)aNameIndex;
- (void)duplicateFile:(NSUInteger)aNameIndex;
- (void)deleteFile:(NSUInteger)aNameIndex;

- (NSString *)makeUniqueUntitledName;
- (NSString *)makeUniqueCopyName:(NSString *)anOldName;
- (bool)nameIsAlreadyInUse:(NSString *)aFileNameCandidate;

@end


@implementation GeometryGamesPortfolioController
{
	GeometryGamesPortfolioView					*itsPortfolioView;

	UIBarButtonItem								*itsNewDrawingButton,
												*itsFlexibleSpace,
												*itsPreferencesButton;

	//	Remember which drawing the user opened,
	//	so when the user closes it we can load the revised thumbnail.
	NSUInteger									itsMostRecentlyOpenedDrawingIndex;
	
	//	When the user begins editing a file name,
	//	keep track of the UITextView in which the editing take place.
	UITextView __weak							*itsFirstResponderLabel;

	//	Maintain a list of available drawings, which should
	//	correspond to the .txt files in NSDocumentDirectory.
	NSMutableArray<GeometryGamesThumbnail *>	*itsThumbnails;
}


- (id)init
{
	self = [super initWithNibName:nil bundle:nil];
	if (self != nil)
	{
		[self setTitle:GetLocalizedTextAsNSString(u"PortfolioTitle")];

		itsTapMode							= GetUserPrefBool(u"tap opens drawing directly") ? TapToOpen : TapForMenu;
		itsMostRecentlyOpenedDrawingIndex	= NO_RECENTLY_OPENED_DRAWING;
		itsFirstResponderLabel				= nil;

		itsThumbnails = [self loadThumbnails];

		if ([itsThumbnails count] == 0)
			[self createNewEmptyFile];
		
		[[NSNotificationCenter defaultCenter]
			addObserver:	self
			selector:		@selector(keyboardDidShow:)
			name:			UIKeyboardDidShowNotification
			object:			nil];
		[[NSNotificationCenter defaultCenter]
			addObserver:	self
			selector:		@selector(keyboardWillHide:)
			name:			UIKeyboardWillHideNotification
			object:			nil];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}


- (BOOL)prefersStatusBarHidden
{
	return NO;
}


#pragma mark -
#pragma mark view load/unload

- (void)loadView
{
	//	Create the main UIScrollView to contain the icons and labels.
	itsPortfolioView = [[GeometryGamesPortfolioView alloc]
		initWithFrame:		CGRectZero	//	frame will get set automatically
		thumbnails:			itsThumbnails
		thumbnailTarget:	self];
	[itsPortfolioView setDelegate:self];
	[itsPortfolioView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
	[itsPortfolioView setBackgroundColor:[self scrollViewBackgroundColor]];

	[self setView:itsPortfolioView];

	[self setAutomaticallyAdjustsScrollViewInsets:YES];
}

- (void)viewDidLoad
{
	[super viewDidLoad];

	[self loadToolbarItems];
}

- (UIColor *)scrollViewBackgroundColor	//	subclass may override
{
	return [UIColor whiteColor];
}

- (void)loadToolbarItems
{
	itsNewDrawingButton = [[UIBarButtonItem alloc]
#if 1
		//	A text label for the New Drawing button should
		//	let the user immediately grasp its meaning.
		initWithTitle:	GetLocalizedTextAsNSString(u"NewButtonTitle")
		style:			UIBarButtonItemStylePlain
#elif 0
		initWithImage:	[UIImage imageNamed:@"NewDrawing.png"]	//	Provides consistent look with Preferences button.
		style:			UIBarButtonItemStylePlain
#else
		initWithBarButtonSystemItem:UIBarButtonSystemItemAdd	//	Thinner lines than Preferences button.
#endif
		target:			self
		action:			@selector(createNewEmptyFile)];

	itsFlexibleSpace = [[UIBarButtonItem alloc]
		initWithBarButtonSystemItem:	UIBarButtonSystemItemFlexibleSpace
		target:							nil
		action:							nil];

	itsPreferencesButton = [[UIBarButtonItem alloc]
		initWithImage:	[UIImage imageNamed:@"Preferences.png"]
		style:			UIBarButtonItemStylePlain
		target:			self
		action:			@selector(userTappedToolbarButton:)];

	[self setToolbarItems:	@[
								itsNewDrawingButton,
								itsFlexibleSpace,
								itsPreferencesButton
							]
				animated:	NO];
	
	[self refreshToolbarButtonEnabling];
}


#pragma mark -
#pragma mark view appearance/disappearance

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];

	//	If the user just finishing editing a drawing
	//	and returned to this portfolio controller...
	if (itsMostRecentlyOpenedDrawingIndex != NO_RECENTLY_OPENED_DRAWING)
	{
		//	Reload the most recently opened thumbnail image.
		[self reloadMostRecentlyOpenedThumbnail];

		//	The user might not realize that the drawing title is editable.
		//	To give him/her a hint, select the label text the first time
		//	s/he closes the drawing, but then never again.
		[self maybeSelectLabelText];

		//	Clear itsMostRecentlyOpenedDrawingIndex
		itsMostRecentlyOpenedDrawingIndex = NO_RECENTLY_OPENED_DRAWING;
	}
	else
	{
		//	The view just appeared for the first time.

		//	At first launch ...
		if (GetUserPrefBool(u"is first launch"))
		{
			if ([itsThumbnails count] == 1)
			{
				//	New users will have only the default empty drawing,
				//	whose .txt and .png files should have already been created in -init.
				//	Auto-open that default empty drawing now.
				//
				//	The drawing view controller's -viewDidAppear method will
				//	open the Help panel, and then set the "is first launch"
				//	user pref to false.
				//
				[self openFile:0];
			}
			else
			{
				SetUserPrefBool(u"is first launch", false);
			}
		}
	}
}


#pragma mark -
#pragma mark size change

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	CGFloat	theOldCurrentDisplacement,
			theOldMaximumDisplacement,
			theScrollFraction;
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
	
	//	Remember what fraction of the way the user has scrolled through the content,
	//	so we can restore that same fraction in the resized view.
	theOldCurrentDisplacement	= [itsPortfolioView contentOffset].y - (-[itsPortfolioView contentInset].top);
	theOldMaximumDisplacement	= [itsPortfolioView contentSize].height
								- (	  [itsPortfolioView bounds].size.height
									- (
										  [itsPortfolioView contentInset].top
										+ [itsPortfolioView contentInset].bottom
									  )
								  );
	if (theOldMaximumDisplacement > 0.0)
		theScrollFraction = theOldCurrentDisplacement / theOldMaximumDisplacement;
	else
		theScrollFraction = 0.0;
	
	[coordinator
		animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinator> context)
		{
			UNUSED_PARAMETER(context);

			//	Place code here to perform animations during the transition.
		}
	 	completion:^(id<UIViewControllerTransitionCoordinator> context)
		{
			CGFloat	theNewCurrentDisplacement,
					theNewMaximumDisplacement;
			
			UNUSED_PARAMETER(context);
					
			//	Place code here to run before the transition ends.

			//	By the time this code runs, itsPortfolioView will have its new size.
			//
			//	If itsPortfolioView is visible (at the top of the navigation stack,
			//	not covered by a drawing view), then layoutSubviews will have been
			//	called (twice in fact!) and [itsPortfolioView contentSize] will be correct.
			//
			//	If itsPortfolioView isn't visible (because it's covered by a drawing view)
			//	then layoutSubviews won't get called until the drawing view gets dismissed,
			//	and so the value of [itsPortfolioView contentSize] will be incorrect
			//	and theScrollFraction won't be accurately preserved.  (We could try
			//	to force a layout immediately, but I hesitate to make subviews come and go
			//	at a moment when the UIKit is still managing a transition.)

			//	Implicit strong reference to self is harmless here
			//	because GeometryGamesPortfolioController lives
			//	as long as the app does.

			//	Apply theScrollFraction to the new view size.
			theNewMaximumDisplacement	= [itsPortfolioView contentSize].height
										- (	  [itsPortfolioView bounds].size.height
											- (
												  [itsPortfolioView contentInset].top
												+ [itsPortfolioView contentInset].bottom
											  )
										  );
			theNewCurrentDisplacement	= theScrollFraction * theNewMaximumDisplacement;
			[itsPortfolioView setContentOffset:(CGPoint)
				{
					0.0,
					(-[itsPortfolioView contentInset].top) + theNewCurrentDisplacement
				}];
		}
	];
}

//	This same identical traitCollectionDidChange: code appears
//	in both GeometryGamesPortfolioController and GeometryGamesGraphicsViewController.
//	If these two classes ever get any other methods in common,
//	we can pull it out into common GeometryGamesViewController superclass.
//	However at the moment it doesn't seem worth creating a superclass
//	for this one small method alone.
//
- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
	UIViewController	*thePresentedViewController,
						*theContentViewController;
	
	[super traitCollectionDidChange:previousTraitCollection];
	
	//	To let a presented view adapt to a new size class,
	//	we must check the new size class here
	//	in the full-window *presenting* view controller,
	//	not in the *presented* view's own view controller.
	//	The presented view's own view controller gets told
	//	only the size class of its own view, which
	//	for a popover-style view is always horizontally compact.
	
	if ([[self traitCollection] horizontalSizeClass] != [previousTraitCollection horizontalSizeClass])
	{
		thePresentedViewController = [self presentedViewController];

		if ([thePresentedViewController isKindOfClass:[UINavigationController class]])
		{
			//	Calling adaptNavBarForHorizontalSize: for the topViewController
			//	is good enough.  If the user later taps the back button
			//	to return to an earlier view controller on the navigation controller's stack,
			//	that earlier view controller's viewWillAppear: method
			//	will call adaptNavBarForHorizontalSize.
			
			theContentViewController = [((UINavigationController *)thePresentedViewController) topViewController];
		
			if ([theContentViewController conformsToProtocol:@protocol(GeometryGamesPopover)])
			{
				[((id<GeometryGamesPopover>)theContentViewController)
					adaptNavBarForHorizontalSize:[[self traitCollection] horizontalSizeClass]];
			}
		}
	}
}


#pragma mark -
#pragma mark UIScrollViewDelegate

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
	UNUSED_PARAMETER(scrollView);

	//	As well as getting called during a scroll,
	//	this -scrollViewDidScroll method also gets called at launch,
	//	before -layoutSubviews, so any methods we call from here
	//	must keep in mind that the thumbnails positions have
	//	not yet been set.
	//
	[itsPortfolioView installVisibleSubviews];
}


#pragma mark -
#pragma mark thumbnails

- (void)reloadMostRecentlyOpenedThumbnail
{
	GeometryGamesThumbnail	*theThumbnail;
	UIImageView				*theIconView;
	UIImage					*theNewImage;

	if (itsMostRecentlyOpenedDrawingIndex != NO_RECENTLY_OPENED_DRAWING	//	may fail if no drawing has been opened
	 && itsMostRecentlyOpenedDrawingIndex < [itsThumbnails count])		//	should never fail
	{
		theThumbnail	= [itsThumbnails objectAtIndex:itsMostRecentlyOpenedDrawingIndex];
		theIconView		= [theThumbnail itsIcon];

		theNewImage		= LoadScaledThumbnailImage([theThumbnail itsName]);
		[theIconView setImage:theNewImage];
	}
}

- (void)maybeSelectLabelText
{
	GeometryGamesThumbnail	*theThumbnail;

	if (itsMostRecentlyOpenedDrawingIndex != NO_RECENTLY_OPENED_DRAWING	//	may fail if no drawing has been opened
	 && itsMostRecentlyOpenedDrawingIndex < [itsThumbnails count])		//	should never fail
	{
		theThumbnail = [itsThumbnails objectAtIndex:itsMostRecentlyOpenedDrawingIndex];

		//	If the user hasn't yet had a chance to edit the file name...
		if ( ! [theThumbnail itsNameHasBeenEdited] )
		{
			//	...give him/her a chance now.
			[[theThumbnail itsLabel] selectAll:self];
			
			//	Force-selecting the label text once will let the user know
			//	that it can be edited.  Thereafter the user can select it
			//	manually only when desired.  No need to force-select each time.
			[theThumbnail setItsNameHasBeenEdited:YES];
		}
	}
}


#pragma mark -
#pragma mark GeometryGamesThumbnailGestureTarget

- (void)userTappedFileIcon:(UITapGestureRecognizer *)aTapGestureRecognizer
{
	UITextView	*theFirstResponderLabel;
	UIView		*theThumbnailIconView;
	NSUInteger	theNameIndex;
	
	//	Dismiss the keyboard if it's up, to not risk deleting a file while editing its name.
	//	The gestureRecognizer remains in state UIGestureRecognizerStateRecognized,
	//	even if -resignFirstResponder: triggers a name-already-in-use error message.
	theFirstResponderLabel = itsFirstResponderLabel;	//	convert weak reference to strong reference
	if (theFirstResponderLabel != nil)
	{
		[theFirstResponderLabel resignFirstResponder];
		
		//	gestureRecognizer will remain in the state UIGestureRecognizerStateRecognized:.
		//
		//	If itsTapMode == TapToOpen, then obviously opening the drawing would be
		//		a terrible idea if a name-already-in-use error message appears.
		//
		//	If itsTapMode == TapForMenu, then
		//		on iOS 8 we could go ahead and put up the action sheet, which would
		//			wait patiently if a name-already-in-use error message appears,
		//		but on iOS 5, after dismissing the name-already-in-use error message,
		//			it becomes impossible to dismiss the action sheet (all touches are ignored !).
		//
		//	So in all cases, let's ignore the tap.
		return;
	}

	theThumbnailIconView	= [aTapGestureRecognizer view];
	theNameIndex			= [theThumbnailIconView tag];

	switch ([aTapGestureRecognizer state])
	{
		case UIGestureRecognizerStateRecognized:

			switch (itsTapMode)
			{
				case TapToOpen:
					[self openFile:theNameIndex];
					break;
				
				case TapForMenu:
					//	Display an action sheet to let the user
					//	open, duplicate or delete the drawing.
					[self showDuplicateDeleteMenuForThumbnail:theThumbnailIconView includeOpen:true];
					break;
			}

			break;
		
		default:
			//	We don't expect any other messages,
			//	but if any did arrive we'd ignore them.
			break;
	}
}

- (void)userLongPressedFileIcon:(UILongPressGestureRecognizer *)aLongPressGestureRecognizer
{
	UITextView	*theFirstResponderLabel;
	UIView		*theThumbnailIconView;
	
	//	See comments in -userPannedFileIcon: below.
	theFirstResponderLabel = itsFirstResponderLabel;	//	convert weak reference to strong reference
	if (theFirstResponderLabel != nil)
		[theFirstResponderLabel resignFirstResponder];

	theThumbnailIconView = [aLongPressGestureRecognizer view];
	
	//	Respond to a press-and-hold in TapToOpen mode only.
	if (itsTapMode == TapToOpen)
	{
		switch ([aLongPressGestureRecognizer state])
		{
			case UIGestureRecognizerStateBegan:

				//	Display an action sheet to let the user
				//	duplicate or delete the drawing.
				//
				//		Note:  The long-press gesture continues normally
				//		even after the Duplicate/Delete menu appears.
				//		The UIKit interprets all touch events as part
				//		of the long-press gesture until the user lifts all fingers
				//		and the gesture ends, at which point the user
				//		may tap a button in the Duplicate/Delete menu.
				//
				[self showDuplicateDeleteMenuForThumbnail:theThumbnailIconView includeOpen:false];

				break;

			case UIGestureRecognizerStateChanged:
				break;

			case UIGestureRecognizerStateEnded:
				break;

			case UIGestureRecognizerStateCancelled:
				break;
			
			default:
				//	We don't expect any other messages,
				//	but if any did arrive we'd ignore them.
				break;
		}
	}
}

- (void)showDuplicateDeleteMenuForThumbnail:(UIView *)aThumbnailIconView includeOpen:(bool)anIncludeOpenItemFlag
{
	NSUInteger			theNameIndex;
	UIAlertController	*theAlertController;

	theNameIndex = [aThumbnailIconView tag];

	theAlertController = [UIAlertController
							alertControllerWithTitle:	nil
							message:					nil
							preferredStyle:				UIAlertControllerStyleActionSheet];

	[theAlertController addAction:[UIAlertAction
		actionWithTitle:	GetLocalizedTextAsNSString(u"Cancel")
		style:				UIAlertActionStyleCancel
		handler:^(UIAlertAction *action)
		{
			UNUSED_PARAMETER(action);
		}]];

	if (anIncludeOpenItemFlag)
	{
		[theAlertController addAction:[UIAlertAction
			actionWithTitle:	GetLocalizedTextAsNSString(u"Open")
			style:				UIAlertActionStyleDefault
			handler:^(UIAlertAction *action)
			{
				UNUSED_PARAMETER(action);

				//	Strong reference to self is harmless here
				//	because GeometryGamesPortfolioController lives
				//	as long as the app does.

				[self openFile:theNameIndex];
			}]];
	}

	[theAlertController addAction:[UIAlertAction
		actionWithTitle:	GetLocalizedTextAsNSString(u"Duplicate")
		style:				UIAlertActionStyleDefault
		handler:^(UIAlertAction *action)
		{
			UNUSED_PARAMETER(action);
			
			//	Strong reference to self is harmless here
			//	because GeometryGamesPortfolioController lives
			//	as long as the app does.

			[self duplicateFile:theNameIndex];
		}]];

	if ([self drawingContentIsLocked:theNameIndex])
	{
		[theAlertController addAction:[UIAlertAction
			actionWithTitle:	GetLocalizedTextAsNSString(u"Unlock")
			style:				UIAlertActionStyleDefault
			handler:^(UIAlertAction *action)
			{
				UNUSED_PARAMETER(action);
				
				//	Strong reference to self is harmless here
				//	because GeometryGamesPortfolioController lives
				//	as long as the app does.

				[self unlockDrawingContent:theNameIndex];
			}]];
	}
	else
	{
		[theAlertController addAction:[UIAlertAction
			actionWithTitle:	GetLocalizedTextAsNSString(u"Delete")
			style:				UIAlertActionStyleDefault
			handler:^(UIAlertAction *action)
			{
				UNUSED_PARAMETER(action);
				
				//	Strong reference to self is harmless here
				//	because GeometryGamesPortfolioController lives
				//	as long as the app does.

				[self maybeDeleteFile:theNameIndex];
			}]];
	}

	PresentPopoverFromRectInView(	self,
									theAlertController,
									[aThumbnailIconView bounds],
									aThumbnailIconView,
									UIPopoverArrowDirectionAny,
									nil);
}

- (void)userPannedFileIcon:(UIPanGestureRecognizer *)aPanGestureRecognizer
{
	UITextView						*theFirstResponderLabel;
	UIImageView						*theIconView;
	UIView							*theThumbnailView;
	CGPoint							theThumbnailCenter,
									theTranslation;
	CGFloat							theImageSizePt;		//	image  size   in points, not pixels
	NSUInteger						theNumThumbnailsPerRow,
									theStride,			//	same stride horizontally and vertically
									theMargin;
	NSInteger						theNewRow,
									theNewCol;
	CGFloat							theDeltaX;
	UIUserInterfaceLayoutDirection	theLayoutDirection;
	NSUInteger						theOldIndex,
									theNewIndex;
	GeometryGamesThumbnail			*theThumbnail;
	
	theFirstResponderLabel = itsFirstResponderLabel;	//	convert weak reference to strong reference
	if (theFirstResponderLabel != nil)
	{
		//	It would seem like a bad idea to let the user
		//	drag a thumbnail while editing a label of either
		//	the same or a different thumbnail.
		//
		//	The exact sequence of events depends on the situation.
		//	We call -resignFirstResponder here and then
		//
		//		if a name-already-in-use error message appears,
		//		we get two StateCancelled calls (the first one is the call
		//		that started off as StateBegan before we called resignFirstResponder
		//		and the second is an explicit StateCancelled that the AppKit then sends us),
		//
		//	otherwise
		//
		//		we proceed normally with
		//		StateBegan, StateChanged, ..., StateChanged, StateEnded.
		//
		[theFirstResponderLabel resignFirstResponder];
	}

	if ([[aPanGestureRecognizer view] isKindOfClass:[UIImageView class]])	//	should never fail
	{
		theIconView			= (UIImageView *) [aPanGestureRecognizer view];
		theThumbnailView	= [theIconView superview];
	}
	else
	{
		return;
	}

	switch ([aPanGestureRecognizer state])
	{
		case UIGestureRecognizerStateBegan:

			//	Let the selected thumbnail appear on top of all other thumbnails.
			[itsPortfolioView bringSubviewToFront:theThumbnailView];

			break;

		case UIGestureRecognizerStateChanged:

			theThumbnailCenter		= [theThumbnailView center];
			theTranslation			= [aPanGestureRecognizer translationInView:itsPortfolioView];
			theThumbnailCenter.x	+= theTranslation.x;
			theThumbnailCenter.y	+= theTranslation.y;
			[theThumbnailView setCenter:theThumbnailCenter];
			
			//	Zero out the gesture recognizer's translation --
			//	otherwise it's cumulative.
			[aPanGestureRecognizer setTranslation:CGPointZero inView:itsPortfolioView];

			break;

		case UIGestureRecognizerStateEnded:

			//	Figure out at where theThumbnailView currently sits in the layout,
			//	to decide at what index to insert it into itsThumbnails.
			
			[itsPortfolioView	getStride:				&theStride
								imageSizePt:			&theImageSizePt
								margin:					&theMargin
								numThumbnailsPerRow:	&theNumThumbnailsPerRow];

			theThumbnailCenter		= [theThumbnailView center];
			theThumbnailCenter.y	-= (CGFloat) theMargin;
			theThumbnailCenter.x	/= (CGFloat) theStride;
			theThumbnailCenter.y	/= (CGFloat) theStride;
			theNewRow				= (NSInteger) floor(theThumbnailCenter.y);
			theNewCol				= (NSInteger) floor(theThumbnailCenter.x);
			if (theNewRow < 0)
				theNewRow = 0;
			if (theNewCol < 0)
				theNewCol = 0;
			if (theNewCol > (NSInteger)theNumThumbnailsPerRow - 1)
				theNewCol = (NSInteger)theNumThumbnailsPerRow - 1;

			theDeltaX = theThumbnailCenter.x - (theNewCol + 0.5);

			theLayoutDirection = [[UIApplication sharedApplication] userInterfaceLayoutDirection];
			if (theLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft)
			{
				theNewCol = ((NSInteger)theNumThumbnailsPerRow - 1) - theNewCol;
				theDeltaX = - theDeltaX;
			}
			
			theOldIndex = [theThumbnailView tag];
			theNewIndex = theNewRow * theNumThumbnailsPerRow + theNewCol;
			if (theNewIndex > [itsThumbnails count] - 1)
				theNewIndex = [itsThumbnails count] - 1;

			//	If the user places theThumbnailView over the center of another thumbnail...
			if (fabs(theDeltaX) < 0.125)
			{
				//	...assume s/he want to put it into that cell.
				//	(So our choice of theNewCol is already correct.)
			}
			else	//	But if s/he places it between two thumbnails...
			{
				//	...assume s/he wants to place it between the those two.
				//	Which way we move it depends on which direction
				//	the thumbnail came from, because that's where there's
				//	a free space.
				if (theDeltaX > 0.0 && theOldIndex > theNewIndex)
					theNewIndex++;
				if (theDeltaX < 0.0 && theOldIndex < theNewIndex)
					theNewIndex--;
			}

			theThumbnail = [itsThumbnails objectAtIndex:theOldIndex];
			
			if (theNewIndex != theOldIndex)
			{
				[itsThumbnails removeObjectAtIndex:theOldIndex];
				[itsThumbnails insertObject:theThumbnail atIndex:theNewIndex];

				//	Update the manifest file.
				[self writeDrawingManifest];

				//	Animate all icons into the positions appropriate for their tags.
				[itsPortfolioView layOutThumbnailsBeginningAtIndex:MIN(theOldIndex, theNewIndex) animated:YES staggered:YES scroll:NO];
				
				//	Install those and only those views that are visible or nearly so.
				[itsPortfolioView installVisibleSubviews];

				//	Update each drawing's subviews' tags to match its index in itsThumbnails.
				[itsPortfolioView refreshViewTags];
			}
			else
			{
				//	The user left theThumbnailView more-or-less at its original position.
				//	Align it exactly.
				[theThumbnailView setFrame:[theThumbnail itsFrame]];
			}

			break;

		case UIGestureRecognizerStateCancelled:

			[theThumbnailView setFrame:
				[[itsThumbnails objectAtIndex:[theThumbnailView tag]] itsFrame]];

			break;
		
		default:
			//	We don't expect any other messages,
			//	but if any did arrive we'd ignore them.
			break;
	}
}


#pragma mark -
#pragma mark drawing lock

- (bool)drawingContentIsLocked:(NSUInteger)aNameIndex
{
	const char			*theFilePath				= NULL;
	GeometryGamesModel	*theModel					= nil;
	ModelData			*md							= NULL;
	bool				theDrawingContentIsLocked	= false;

	theFilePath = [GetFullFilePath([[itsThumbnails objectAtIndex:aNameIndex] itsName], @".txt") UTF8String];
	
	theModel = [[GeometryGamesModel alloc] init];
	[theModel lockModelData:&md];
	if (OpenDrawing(md, theFilePath))
		theDrawingContentIsLocked = ContentIsLocked(md);
	[theModel unlockModelData:&md];
	
	return theDrawingContentIsLocked;
}

- (void)unlockDrawingContent:(NSUInteger)aNameIndex
{
	const char			*theFilePath	= NULL;
	GeometryGamesModel	*theModel		= nil;
	ModelData			*md				= NULL;

	theFilePath = [GetFullFilePath([[itsThumbnails objectAtIndex:aNameIndex] itsName], @".txt") UTF8String];
	
	theModel = [[GeometryGamesModel alloc] init];
	[theModel lockModelData:&md];
	if (OpenDrawing(md, theFilePath))
	{
		SetContentIsLocked(md, false);
		SaveDrawing(md, theFilePath);
	}
	[theModel unlockModelData:&md];
}


#pragma mark -
#pragma mark toolbar

- (void)refreshToolbarButtonEnabling
{
	//	The two toolbar buttons (New drawing and Preferences)
	//	are enabled at all times.
}

- (void)userTappedToolbarButton:(id)sender
{
	UIViewController		*theContentViewController;
	UINavigationController	*theNavigationController;

	//	On iOS 8 and later, the UIPopoverPresentationController's setBarButtonItem
	//	automatically adds all of a given button's siblings to the list
	//	of passthrough views, but does not add the given button itself.
	//	So a second tap on the same button will automatically dismiss the popover,
	//	without generating a call to this userTappedToolbarButton method.
	//
	//	Thus when we do get a call to userTappedToolbarButton, we know that
	//	if a popover is already up, it's not this one.  So we may safely dismiss
	//	the already-up popover (if present) and display the newly requested one,
	//	with no risk that it might be the same popover.

	//	If a popover is already present, dismiss it.
	[self dismissViewControllerAnimated:YES completion:nil];
	
	//	Create a view controller whose content corresponds
	//	to the button the user touched.
	if (sender == itsPreferencesButton)
		theContentViewController = [self contentViewControllerForPreferencesPanel];
	else
		theContentViewController = nil;

	if (theContentViewController != nil)
	{
		//	Present theContentViewController in a navigation controller.
		//
		//	Note #1:  Don't include [self navigationController] in the array.
		//	If there were a back button, then if the user tapped it
		//	while a popover was up, the app would crash.
		//
		//	Note #2:  UIKit tests the passthrough views for hits
		//	in the order in which they appear in the array below,
		//	ignoring the views' stacking order in the window itself.
		//
		//	Note #3:  UIPopoverPresentationController's setBarButtonItem
		//	automatically adds all of a given button's siblings
		//	to the *end* of the list of passthrough views.
		//
		//	As a corollary of those last two notes, if a passthrough view
		//	underlaps the toolbar, then that view would steal taps
		//	from the toolbar buttons, because even though the view
		//	sits lower than the toolbar in the window's stacking order,
		//	it would come before the toolbar buttons on the list of passthrough views.

		//	Note #4:  Even the Preferences Picker gets wrapped in a navigation controller,
		//	to give it a header that doesn't scroll along with the table's contents
		//	(unlike a table header view, which does scroll along with the table's contents).
		//	On iPhone, the nav bar will get a Close button
		//	which we want to keep visible at all times, regardless of scrolling.
		theNavigationController = [[UINavigationController alloc]
			initWithRootViewController:theContentViewController];
		[[theNavigationController navigationBar] setTranslucent:NO];	//	so content won't underlap nav bar

		PresentPopoverFromToolbarButton(self, theNavigationController, sender, nil);
	}
}

- (UIViewController *)contentViewControllerForPreferencesPanel
{
	return nil;	//	subclass must override
}

- (void)maybeDeleteFile:(NSUInteger)aNameIndex	//	index of file in itsThumbnails
{
	GeometryGamesThumbnail	*theThumbnail;
	UIAlertController		*theAlertController;
	UIView					*theThumbnailIconView;

	theThumbnail			= [itsThumbnails objectAtIndex:aNameIndex];
	theThumbnailIconView	= [theThumbnail itsIcon];

	theAlertController = [UIAlertController
							alertControllerWithTitle:	nil
							message:					nil
							preferredStyle:				UIAlertControllerStyleActionSheet];
		
	[theAlertController addAction:[UIAlertAction
		actionWithTitle:	GetLocalizedTextAsNSString(u"Cancel")
		style:				UIAlertActionStyleCancel
		handler:^(UIAlertAction *action)
		{
			UNUSED_PARAMETER(action);
		}]];

	[theAlertController addAction:[UIAlertAction
		actionWithTitle:	[NSString stringWithFormat:
								GetLocalizedTextAsNSString(u"DeleteDrawing"),
								[theThumbnail itsName]]
		style:				UIAlertActionStyleDestructive
		handler:^(UIAlertAction *action)
		{
			UNUSED_PARAMETER(action);

			//	Strong reference to self is harmless here
			//	because GeometryGamesPortfolioController lives
			//	as long as the app does.

			[self deleteFile:aNameIndex];
		}]];

	PresentPopoverFromRectInView(	self,
									theAlertController,
									[theThumbnailIconView bounds],
									theThumbnailIconView,
									UIPopoverArrowDirectionAny,
									nil);
}


#pragma mark -
#pragma mark UITextViewDelegate

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
	UNUSED_PARAMETER(range);

	if ([text isEqualToString:@"\n"])		//	User hit return key?
	{
		//	Dismiss the keyboard.
		[textView resignFirstResponder];
		
		//	Discard the newline character itself.
		return NO;
	}
	else
	if ([text rangeOfString:@"/"].location != NSNotFound)
	{
		//	Unix file names may not contain '/'.
		return NO;
	}
	else
	if ([text hasPrefix:@"."] && range.location == 0)
	{
		//	Strictly speaking a leading '.' is harmless, and the file
		//	shows up normally in iTunes when the user connects the device
		//	to a Mac.  However... if the user ever copied the file onto a Mac,
		//	it would become a "hidden file".  To avoid that unpleasant situation,
		//	let's prohibit file names starting with '.' .
		return NO;
	}
	else
	{
		//	Process all other characters normally.
		return YES;
	}
}

- (void)textViewDidBeginEditing:(UITextView *)textView
{
	itsFirstResponderLabel = textView;
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
	NSUInteger				theIndex;
	GeometryGamesThumbnail	*theThumbnail;
	UITextView				*theLabelView;
	NSString				*theOldName,
							*theNewName;
	UIAlertController		*theAlertController;

	//	Clear itsFirstResponderLabel
	itsFirstResponderLabel = nil;

	//	Make sure the proposed new name is distinct from all existing names
	//	and then rename both the drawing file and the thumbnail file.

	theIndex = (NSUInteger)[textView tag];
	if (theIndex >= [itsThumbnails count])	//	should never occur
		return;

	theThumbnail	= [itsThumbnails objectAtIndex:theIndex];
	theLabelView	= [theThumbnail itsLabel];
	theOldName		= [theThumbnail itsName];
	theNewName		= [textView text];
	
	GEOMETRY_GAMES_ASSERT(textView == theLabelView, "internal error:  incorrectly indexed label view");

	if ( ! [theNewName isEqualToString:theOldName] )
	{
		if ([self nameIsAlreadyInUse:theNewName] )
		{
			[textView setText:theOldName];

			theAlertController = [UIAlertController
				alertControllerWithTitle:	GetLocalizedTextAsNSString(u"NameInUse-Title")
				message:					[NSString stringWithFormat:
												GetLocalizedTextAsNSString(u"NameInUse-Message"),
												theNewName]
				preferredStyle:				UIAlertControllerStyleAlert];

			[theAlertController addAction:[UIAlertAction
				actionWithTitle:	GetLocalizedTextAsNSString(u"OK")
				style:				UIAlertActionStyleDefault
				handler:			nil]];

			[self
				presentViewController:	theAlertController
				animated:				YES
				completion:				nil];
		}
		else
		{
			//	theNewName is distinct from all existing names so...
			
			//	rename the files
			[[NSFileManager defaultManager]
				moveItemAtPath:	GetFullFilePath(theOldName, @".txt")
				toPath:			GetFullFilePath(theNewName, @".txt")
				error:			nil];
			[[NSFileManager defaultManager]
				moveItemAtPath:	GetFullFilePath(theOldName, @".png")
				toPath:			GetFullFilePath(theNewName, @".png")
				error:			nil];
			
			//	update theThumbnail, and
			[theThumbnail setItsName:theNewName];
			[theThumbnail setItsNameHasBeenEdited:YES];
	
			//	update the manifest file.
			[self writeDrawingManifest];
		}
	}
}


#pragma mark -
#pragma mark keyboard management

- (void)keyboardDidShow:(NSNotification *)aNotification
{
	CGFloat		theKeyboardHeight;
	UITextView	*theFirstResponderLabel;
	
	theKeyboardHeight = [self keyboardHeight:aNotification withKey:UIKeyboardFrameEndUserInfoKey];
	
	//	The messages -keyboardDidShow: and -keyboardWillHide: don't always
	//	arrive in matched pairs.
	//
	//	On the iPad:
	//		If the user changes the keyboard while its up (for example
	//		from English to Chinese) and the new keyboard's height differs
	//		from the old one's height, we'll get a second -keyboardDidShow:
	//		message with no intervening -keyboardWillHide: message.
	//
	//	On the iPhone:
	//		The Chinese Pinyin keyboard begins with a QWERTY layout only.
	//		Later, once the user starts typing, it displays its row of proposed hanzi
	//		and at that moment we'll get a second -keyboardDidShow: message
	//		with no intervening -keyboardWillHide: message.
	//
	//	If the user rotates the device while the keyboard is visible,
	//	we'll get a -keyboardWillHide: message for the old keyboard
	//	followed immediately by a -keyboardDidShow: message for the new keyboard.
	//
	//	We don't use automaticallyAdjustsScrollViewInsets, but even if we did
	//	we could still manually change the insets' bottom values.

	[self setBottomInsets:theKeyboardHeight];
	
	//	Scroll the label's frame rectangle into view.
	theFirstResponderLabel = itsFirstResponderLabel;	//	convert weak reference to strong reference
	if (theFirstResponderLabel != nil)
		[itsPortfolioView scrollRectToVisible:[[theFirstResponderLabel superview] frame] animated:YES];
}

- (void)keyboardWillHide:(NSNotification *)aNotification
{
	UNUSED_PARAMETER(aNotification);

	[self setBottomInsets:[[self bottomLayoutGuide] length]];
}

- (float)keyboardHeight:(NSNotification *)aNotification
	withKey:(NSString *const)aKey	//	aKey ∈ {UIKeyboardFrameBeginUserInfoKey, UIKeyboardFrameEndUserInfoKey}
{
	NSDictionary	*theNotificationInfo;
	CGRect			theKeyboardRectInScreenCoords,
					theKeyboardRectInWindowCoords,
					theKeyboardRectInViewCoords;
	CGFloat			theKeyboardHeight;

	theNotificationInfo = [aNotification userInfo];
	
	theKeyboardRectInScreenCoords = [[theNotificationInfo objectForKey:aKey] CGRectValue];
	
	//	Apple's documentation for UIKeyboardFrameBeginUserInfoKey
	//	and UIKeyboardFrameEndUserInfoKey says
	//
	//		These coordinates do not take into account any rotation factors
	//		applied to the window’s contents as a result of interface orientation changes.
	//		Thus, you may need to convert the rectangle
	//		to window coordinates (using the convertRect:fromWindow: method) or
	//		to view coordinates (using the convertRect:fromView: method) before using it.
	//
	//	Note:  In the context of an OpenGL ES view, "base coordinates" sometimes
	//	implies pixel coordinates as opposed to the usual coordinates measured in points.
	//	Here, however, when the documentation for UIView's -convertRect:fromView: says
	//
	//		If view is nil, this method instead converts from window base coordinates.
	//
	//	it seems to mean window coordinates in points, not pixels.
	//
	theKeyboardRectInWindowCoords	= [[itsPortfolioView window] convertRect:theKeyboardRectInScreenCoords fromView:nil];
	theKeyboardRectInViewCoords		= [itsPortfolioView convertRect:theKeyboardRectInWindowCoords fromView:nil];

	theKeyboardHeight = theKeyboardRectInViewCoords.size.height;

	return theKeyboardHeight;
}

- (void)setBottomInsets:(CGFloat)aBottomInset
{
	UIEdgeInsets	theContentInset,
					theScrollIndicatorInset;

	//	Change only the bottom values and leave the other sides untouched.

	theContentInset = [itsPortfolioView contentInset];
	theContentInset.bottom = aBottomInset;
	[itsPortfolioView setContentInset:theContentInset];
	
	theScrollIndicatorInset = [itsPortfolioView scrollIndicatorInsets];
	theScrollIndicatorInset.bottom = aBottomInset;
	[itsPortfolioView setScrollIndicatorInsets:theScrollIndicatorInset];
}


#pragma mark -
#pragma mark file management

- (NSMutableArray<GeometryGamesThumbnail *> *)loadThumbnails
{
	NSMutableArray<NSString *>					*theManifestNameList;
	NSMutableOrderedSet<NSString *>				*theManifestNameSet;
	NSURL										*theDocumentDirectory;
	NSArray<NSString *>							*theRawFileList,		//	Names of all files in the directory
												*theTextFileList;		//	Names of all .txt files in the directory
	NSMutableArray<NSString *>					*theDirectoryNameList;	//	Same as theTextFileList, but with the ".txt" extension removed
	NSMutableOrderedSet<NSString *>				*theDirectoryNameSet;
	NSMutableArray<NSString *>					*theNameList;
	NSMutableArray<GeometryGamesThumbnail *>	*theThumbnails;			//	Final array of GeometryGamesThumbnails.
	NSString									*theName;
	GeometryGamesThumbnail						*theThumbnail;
	
	//	Read the manifest file, which is an XML file
	//	listing all the user's drawings.
	theManifestNameList = [NSMutableArray<NSString *> arrayWithContentsOfFile:
							GetFullFilePath(gManifestFileName, @".xml")];
	
	//	If the manifest file was missing or invalid,
	//	create an empty array.
	if (theManifestNameList == nil)
		theManifestNameList = [NSMutableArray<NSString *> arrayWithCapacity:1];
	
	//	Get a list of all available drawing files.
	//	Typically this will agree with theManifestNameList,
	//	but the user might have added or removed files via iTunes.

	theDocumentDirectory = [[NSFileManager defaultManager]
						URLForDirectory:	NSDocumentDirectory
						inDomain:			NSUserDomainMask
						appropriateForURL:	nil
						create:				YES
						error:				nil];

	theRawFileList = [[NSFileManager defaultManager]
						contentsOfDirectoryAtPath:	[theDocumentDirectory path]
						error:						nil];
	
	theTextFileList = [theRawFileList filteredArrayUsingPredicate:
						[NSPredicate predicateWithFormat:@"pathExtension == 'txt'"]];
	
	//	The manifest stores drawing names in the same order
	//	in which they appear on the screen (typically newest-to-oldest).
	//	So let's reverse the order of theTextFileList,
	//	so that any drawings that the user may add via iTunes
	//	will appear in a consistent bottom-to-top order.
	theTextFileList = [[theTextFileList reverseObjectEnumerator] allObjects];

	theDirectoryNameList = [NSMutableArray<NSString *> arrayWithCapacity:[theTextFileList count]];
	for (theName in theTextFileList)
	{
		//	Even though NSString looks like a single class, internally it's implemented
		//	as a "class cluster", which is a collection of classes, each of which
		//	implements the NSString interface but is optimized for some purpose.
		//	As it turns out, -stringByDeletingPathExtension returns an NSPathStore2
		//	implementation of NSString.  The NSPathStore2 works correctly in the app itself,
		//	but unfortunately the LLDB debugger often misreads its contents
		//	(strings of length four confuse it -- the debugger seems to be expecting
		//	the NSPathStore2's internal buffer to end with a terminating zero,
		//	which of course it usually doesn't).  To avoid future confusion, I've replaced
		//	the -stringByDeletingPathExtension call with an equivalent -substringToIndex: call.
		//	The -substringToIndex: call returns an __NSCFString implementation of NSString,
		//	which the LLDB debugger reads correctly.
		//
	//	[theDirectoryNameList addObject:[theName stringByDeletingPathExtension]];
		[theDirectoryNameList addObject:[theName substringToIndex:([theName length] - [@".txt" length])]];
	}
	
	//	Delete from theManifestNameList any files that are no longer available.
	//
	//		Design note:  We're not afraid of linear-time operations,
	//		but we'd like to avoid potentially quadratic-time operations
	//		such as comparing every element in one array to every element in another.
	//		So let's use NSMutableOrderedSets and trust that Apple's engineers
	//		have written efficient implementations of the set operations.
	//

	theManifestNameSet = [NSMutableOrderedSet<NSString *> orderedSetWithCapacity:[theManifestNameList count]];
	[theManifestNameSet addObjectsFromArray:theManifestNameList];

	theDirectoryNameSet = [NSMutableOrderedSet<NSString *> orderedSetWithCapacity:[theDirectoryNameList count]];
	[theDirectoryNameSet addObjectsFromArray:theDirectoryNameList];

	//	Remove from theManifestNameSet the names of any drawings
	//	that are no longer present in the Document Directory.
	[theManifestNameSet intersectOrderedSet:theDirectoryNameSet];
	
	//	Remove from theDirectoryNameSet the names of all drawings
	//	that are already present in the manifest.
	//	Unless the user has added drawings via iTunes,
	//	the result will be an empty set.
	[theDirectoryNameSet minusOrderedSet:theManifestNameSet];
	
	//	Append theManifestNameSet to theDirectoryNameSet.
	//	The result is the same as the original directory name set;
	//	what's changed is that we respect the order
	//	given in the manifest.
	[theDirectoryNameSet unionOrderedSet:theManifestNameSet];
	
	//	Convert from an NSMutableOrderedSet to an NSMutableArray.
	//
	//		Caution:  [theDirectoryNameSet array] returns a "proxy object",
	//		so it's essential that we make a copy here.
	//		Another independent reason to copy it is to make it mutable.
	//
	theNameList = [NSMutableArray<NSString *> arrayWithArray:[theDirectoryNameSet array]];
	
	//	Update the manifest file for next time.
	//	(Typically it won't have changed, but sometimes it might.)
	[theNameList writeToFile:GetFullFilePath(gManifestFileName, @".xml") atomically:YES];

	//	Create a GeometryGamesThumbnail for each name in theManifestNameList.
	theThumbnails = [NSMutableArray<GeometryGamesThumbnail *> arrayWithCapacity:([theNameList count] + 32)];	//	NSMutableArray will expand as needed.
	for (theName in theNameList)
	{
		theThumbnail = [[GeometryGamesThumbnail alloc]
							initWithName:	theName
							nameEdited:		YES];	//	assume names of pre-existing drawings are OK
		[theThumbnails addObject:theThumbnail];
	}
	
	return theThumbnails;
}

- (void)createNewEmptyFile
{
	NSString				*theNewUntitledName;
	GeometryGamesThumbnail	*theThumbnail;
	
	//	Choose a name.
	theNewUntitledName = [self makeUniqueUntitledName];

	//	Create empty drawing and thumbnail files.
	CreateEmptyDrawingFile(theNewUntitledName);
	CreateEmptyThumbnailFile(theNewUntitledName);
	
	//	Create the corresponding GeometryGamesThumbnail and
	//	add it to itsThumbnails, at the beginning of the list.
	theThumbnail = [[GeometryGamesThumbnail alloc]
						initWithName:	theNewUntitledName
						nameEdited:		NO];
	[itsThumbnails insertObject:theThumbnail atIndex:0];
	
	//	Update the manifest file.
	[self writeDrawingManifest];
	
	if (itsPortfolioView != nil)	//	true except when called from -init
	{
		//	Animate all icons into the positions appropriate for their tags.
		[itsPortfolioView layOutThumbnailsBeginningAtIndex:0 animated:YES staggered:YES scroll:YES];

		//	Install those and only those views that are visible or nearly so.
		//	Note that even though we just asked
		//	-layOutThumbnailsBeginningAtIndex:animated:staggered:scroll:
		//	to scroll itsPortfolioView to the top, it won't have arrived there yet,
		//	so we must manually ask -installVisibleSubviewsAtTop: to prepare
		//	thumbnails for the top of itsPortfolioView.
		[itsPortfolioView installVisibleSubviewsAtTop:YES];

		//	Update each drawing's subviews' tags to match its index in itsThumbnails.
		[itsPortfolioView refreshViewTags];
	}
}

- (void)writeDrawingManifest
{
	NSMutableArray<NSString *>	*theNameList;
	GeometryGamesThumbnail		*theThumbnail;
	
	theNameList = [[NSMutableArray<NSString *> alloc] initWithCapacity:[itsThumbnails count]];
	
	for (theThumbnail in itsThumbnails)
		[theNameList addObject:[theThumbnail itsName]];
	
	[theNameList writeToFile:GetFullFilePath(gManifestFileName, @".xml") atomically:YES];
}

- (void)openFile:(NSUInteger)aNameIndex	//	index of file in itsThumbnails
{
	NSString							*theFileName;
	GeometryGamesGraphicsViewController	*theDrawingController;
	
	itsMostRecentlyOpenedDrawingIndex = aNameIndex;

	theFileName				= [[itsThumbnails objectAtIndex:aNameIndex] itsName];
	theDrawingController	= [self drawingControllerForFileNamed:theFileName];
	if (theDrawingController != nil)
		[[self navigationController] pushViewController:theDrawingController animated:YES];
}

- (GeometryGamesGraphicsViewController *)drawingControllerForFileNamed:(NSString *)aFileName
{
	UNUSED_PARAMETER(aFileName);

	//	The subclass must override this function to return an instance
	//	of an app-specific subclass of GeometryGamesGraphicsViewController.
	return nil;
}

- (void)duplicateFile:(NSUInteger)aNameIndex	//	index of file in itsThumbnails
{
	GeometryGamesThumbnail	*theOldThumbnail,
							*theNewThumbnail;
	NSString				*theOldName,
							*theNewName;
	
	//	Note the drawing to be duplicated.
	theOldThumbnail = [itsThumbnails objectAtIndex:aNameIndex];
	
	//	Choose a name.
	theOldName = [theOldThumbnail itsName];
	theNewName = [self makeUniqueCopyName:theOldName];

	//	Duplicate the .txt and .png files.
	[[NSFileManager defaultManager]
		copyItemAtPath:	GetFullFilePath(theOldName, @".txt")
		toPath:			GetFullFilePath(theNewName, @".txt")
		error:			nil];
	[[NSFileManager defaultManager]
		copyItemAtPath:	GetFullFilePath(theOldName, @".png")
		toPath:			GetFullFilePath(theNewName, @".png")
		error:			nil];

	//	Create the corresponding GeometryGamesThumbnail and
	//	add it to itsThumbnails, at position aNameIndex + 1.
	theNewThumbnail = [[GeometryGamesThumbnail alloc]
							initWithName:	theNewName
							nameEdited:		[theOldThumbnail itsNameHasBeenEdited]];
	[itsThumbnails insertObject:theNewThumbnail atIndex:(aNameIndex + 1)];
	
	//	Update the manifest file.
	[self writeDrawingManifest];

	//	Animate all icons into the positions appropriate for their tags.
	[itsPortfolioView layOutThumbnailsBeginningAtIndex:(aNameIndex + 1) animated:YES staggered:YES scroll:YES];
	
	//	Install those and only those views that are visible or nearly so.
	//	This will always include the new drawing.
	[itsPortfolioView installVisibleSubviews];

	//	Update each drawing's subviews' tags to match its index in itsThumbnails.
	[itsPortfolioView refreshViewTags];
}

- (void)deleteFile:(NSUInteger)aNameIndex	//	index of file in itsThumbnails
{
	NSString	*theDeadFileName;
	
	if (aNameIndex >= [itsThumbnails count])	//	should never occur
		return;

	//	Note the name.
	theDeadFileName = [[itsThumbnails objectAtIndex:aNameIndex] itsName];
	
	//	Delete the .txt and .png files.
	[[NSFileManager defaultManager] removeItemAtPath:GetFullFilePath(theDeadFileName, @".txt") error:nil];
	[[NSFileManager defaultManager] removeItemAtPath:GetFullFilePath(theDeadFileName, @".png") error:nil];

	//	Remove the name from itsThumbnails.
	//	GeometryGamesThumbnail's -dealloc method will remove the icon and label views
	//	from the main UIScrollView, which releases them.
	[itsThumbnails removeObjectAtIndex:aNameIndex];
	
	//	Update the manifest file.
	[self writeDrawingManifest];

	//	Animate affected icons into the positions appropriate for their tags.
	[itsPortfolioView layOutThumbnailsBeginningAtIndex:aNameIndex animated:YES staggered:YES scroll:YES];
	
	//	Install those and only those views that are visible or nearly so.
	[itsPortfolioView installVisibleSubviews];

	//	Update each drawing's subviews' tags to match its index in itsThumbnails.
	[itsPortfolioView refreshViewTags];
}


#pragma mark -
#pragma mark file name creation

- (NSString *)makeUniqueUntitledName
{
	NSString		*theBaseName,
					*theNewFileName;
	unsigned int	theCount;
	
	//	To pick a name, first try a localized version of "Untitled", with no number.
	//	If that name is already in use, try "Untitled 2", "Untitled 3", etc.
	//	until an unused name is found.

	theBaseName		= GetLocalizedTextAsNSString(u"Untitled");
	theNewFileName	= theBaseName;
	theCount		= 1;
	
	while ([self nameIsAlreadyInUse:theNewFileName])
	{
		theCount++;
		theNewFileName = [NSString stringWithFormat:@"%@ %d", theBaseName, theCount];
	}

	return theNewFileName;
}

- (NSString *)makeUniqueCopyName:(NSString *)anOldName
{
	NSArray<NSString *>	*theNameComponents;
	NSString			*theLastComponent,
						*theBaseName,
						*theCopyName;
	unsigned int		theOldNumber,
						theNewNumber;

	//	If theFileName ends with a space followed by a natural number,
	//	then theBaseName will be theFileName with the space and the number removed;
	//	otherwise theBaseName will be theFileName itself.
	theNameComponents	= [anOldName componentsSeparatedByString:@" "];
	theLastComponent	= [theNameComponents lastObject];
	if ([theNameComponents count] > 1
	 && [theLastComponent rangeOfCharacterFromSet:
			[[NSCharacterSet characterSetWithCharactersInString:@"0123456789"] invertedSet]
		].location == NSNotFound )
	{
		theBaseName		= [anOldName substringToIndex:
							([anOldName length]
							- 1								//	omit separating space
							- [theLastComponent length])];	//	omit final number
		theOldNumber	= (unsigned int) [theLastComponent integerValue];
	}
	else
	{
		theBaseName		= anOldName;
		theOldNumber	= 1;	//	The original file is implicitly copy #1.
	}
	
	//	Let theCopyName be theBaseName followed by a space and
	//	the smallest number greater than theOldNumber that's
	//	not already in use with theBaseName.
	theNewNumber = theOldNumber + 1;
	do
	{
		theCopyName = [NSString stringWithFormat:@"%@ %d", theBaseName, theNewNumber];
		theNewNumber++;
		
	} while ([self nameIsAlreadyInUse:theCopyName]);
	
	return theCopyName;
}

- (bool)nameIsAlreadyInUse:(NSString *)aFileNameCandidate
{
	GeometryGamesThumbnail	*theThumbnail;
	
	for (theThumbnail in itsThumbnails)
	{
		if ([[theThumbnail itsName] isEqualToString:aFileNameCandidate])
			return true;
	}
	
	return false;
}


#pragma mark -
#pragma mark OpenGL capabilities

- (unsigned int)maxExportMagnificationFactor
{
	unsigned int	theMaxExportMagnificationFactor	= 1;	//	safe default value in case of error
	CGSize			theViewSize2D;
	unsigned int	theViewSize,
					theMaxRenderbufferSize;
	
	//	Compute theViewLength
	theViewSize2D	= [itsPortfolioView bounds].size;
	theViewSize		= (unsigned int) ceil(MAX(theViewSize2D.width, theViewSize2D.height));

	theMaxRenderbufferSize = theViewSize;	//	fallback value in case of error

	if (MetalIsAvailable())
	{
#ifdef SUPPORT_METAL
#warning Still need to write code to get maze size of Metal renderbuffer.
		theMaxRenderbufferSize = theViewSize;	//	TEMPORARY CODE!
#endif
	}
	else
	{
#ifdef SUPPORT_OPENGL
		EAGLContext	*theContext	= nil;
		
		//	Set up an OpenGL ES 2 context and use it
		//	to check the maximum framebuffer size.

		theContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		if (theContext == nil)
			goto CleanUpMaxExportMagnificationFactor;

		if ( ! [EAGLContext setCurrentContext:theContext] )
			goto CleanUpMaxExportMagnificationFactor;
		
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,	(int *)&theMaxRenderbufferSize);
		if (glGetError() != GL_NO_ERROR)
			goto CleanUpMaxExportMagnificationFactor;

CleanUpMaxExportMagnificationFactor:

		//	Shut down the OpenGL ES context.
		[EAGLContext setCurrentContext:nil];
#endif
	}
	
	//	Compute theMaxExportMagnificationFactor.
	//	The integer division will discard the fractional part.
	theMaxExportMagnificationFactor = theMaxRenderbufferSize / theViewSize;
	
	return theMaxExportMagnificationFactor;
}


@end

