//	GeometryGamesPortfolioView.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesPortfolioView.h"
#import "GeometryGamesUtilities-iOS.h"


//	Allow THUMBNAIL_IMAGE_MIN_PADDING_* points of padding
//		between each icon and the edge of the view, and
//	2.0 * THUMBNAIL_IMAGE_MIN_PADDING_* points of padding
//		between any two adjacent icons.
//
//	Note:  To accommodate three icons in each row on the iPhone,
//	make sure that
//
//		3 * (  THUMBNAIL_IMAGE_MIN_PADDING_IPHONE
//		     + THUMBNAIL_IMAGE_SIZE_IPHONE
//		     + THUMBNAIL_IMAGE_MIN_PADDING_IPHONE)
//		≤ 320
//
//	2015/09/12  The original iPhone could accommodate
//	three icons per row with THUMBNAIL_IMAGE_SIZE_IPHONE = 64pt
//	and THUMBNAIL_IMAGE_MIN_PADDING_IPHONE = 21pt,
//	so I originally set THUMBNAIL_IMAGE_MIN_PADDING_IPHONE to 21pt.
//	However, with that minimum padding, the iPhone 6 Plus
//	also accommodates only three icons per row, giving
//	a layout that looks too sparse to my eye.
//	By reducing the minimum padding from 21pt to 19pt,
//	the iPhone 6 Plus can accommdodate four icons per row
//	instead of three, and nothing changes on the original iPhone.
//	To accommodate four icons per row on the regular iPhone 6
//	(that is, the non-Plus model) would require reducing
//	the minimum padding to 14pt, which looks OK for the icons
//	but leaves the labels looking cramped (especially if a label
//	fills two or three lines).
//
#define THUMBNAIL_IMAGE_MIN_PADDING_IPAD	 32.0	//	in points, not pixels
#define THUMBNAIL_IMAGE_MIN_PADDING_IPHONE	 19.0	//	in points, not pixels


//	Privately-declared methods
@interface GeometryGamesPortfolioView()
@end


@implementation GeometryGamesPortfolioView
{
	NSMutableArray<GeometryGamesThumbnail *>							*itsThumbnails;
	id<GeometryGamesThumbnailGestureTarget, UITextViewDelegate> __weak	itsThumbnailTarget;
	
	//	Maintain a GCD dispatch queue for fetching and decompressing thumbnail images.
	dispatch_queue_t													itsDispatchQueue;
}

- (id)initWithFrame:(CGRect)aFrame
	thumbnails:(NSMutableArray<GeometryGamesThumbnail *> *)someThumbnails
	thumbnailTarget:(id<GeometryGamesThumbnailGestureTarget, UITextViewDelegate>)aThumbnailTarget
{
	self = [super initWithFrame:aFrame];
	if (self != nil)
	{
		itsThumbnails		= someThumbnails;
		itsThumbnailTarget	= aThumbnailTarget;
		itsDispatchQueue	= dispatch_queue_create(
								"org.geometrygames.shared.thumbnailQueue",
								DISPATCH_QUEUE_CONCURRENT);	//	Blocks gets dequeued in FIFO order,
															//	but may run concurrently if resources permit.
	}
	return self;
}

- (void)dealloc
{
	//	ARC releases itsDispatchQueue automatically.  No need to manually release it.
	//
//	dispatch_release(itsDispatchQueue);	//	itsDispatchQueue will get deallocated
//										//		*after* all pending blocks have completed.
}


- (void)layoutSubviews
{
	//	Lay out the thumbnails here rather than in viewWillLayoutSubviews,
	//	to avoid laying out the views over and over while scrolling.
	//	(Given that we pass "animated:NO" it might not be a big deal
	//	to lay out the thumbnails over and over while scrolling,
	//	but if we ever passed "animated:YES" it could bog things down.)
	//
	[self layOutThumbnailsBeginningAtIndex:0 animated:NO staggered:NO scroll:NO];
	
	//	Install those and only those views that are visible or nearly so.
	[self installVisibleSubviews];

	//	For each installed thumbnail, set itsView's, itsIcon's and itsLabel's
	//	view tags to match the drawing's index in itsThumbnails.
	[self refreshViewTags];
}

- (void)layOutThumbnailsBeginningAtIndex:	(NSUInteger)aNameIndex
								animated:	(BOOL)anAnimationFlag
								staggered:	(BOOL)aStaggerFlag
								scroll:		(BOOL)aScrollToMakeVisibleFlag	//	scroll to show icon for aNameIndex?
{
	CGFloat							theImageSizePt;		//	image  size   in points, not pixels
	NSUInteger						theNumThumbnailsPerRow,
									theStride,			//	same stride horizontally and vertically
									theMargin,
									theNumDrawings,
									theNumRows;
	UIUserInterfaceLayoutDirection	theLayoutDirection;
	NSUInteger						theDelayFactor,
									theIndex,
									theRow,
									theCol;
	GeometryGamesThumbnail			*theThumbnail;
	CGRect							theThumbnailFrame,	//	placement of theThumbnailView in scrollview
									theIconFrame,		//	placement of theIconView  in theThumbnailView
									theLabelFrame,		//	placement of theLabelView in theThumbnailView
									theExtendedThumbnailFrame;
	UIView							*theThumbnailView;
	UIImageView						*theIconView;
	UITextView						*theLabelView;
	CGFloat							theLabelWidthPt,	//	label width  in points, not pixels
									theLabelHeightPt;	//	label height in points, not pixels

	//	Each thumbnail gets a square cell in the view, of size theStride × theStride.
	//	The icon fills a smaller square, of size theImageSizePt × theImageSizePt,
	//	that sits flush with the cell's top edge, but centered horizontally.
	//	The margin on either side of the icon has width theMargin.
	//	The label fills the bottom part of the cell, occupying the cell's full width
	//	and the remaining 2*theMargin of its height.
	//
	//		⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅⋅
	//		+--------------+--------------+--------------+--------------+
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		+--------------+--------------+--------------+--------------+
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		+--------------+--------------+--------------+--------------+
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|   ********   |   ********   |   ********   |   ********   |
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|LLLLLLLLLLLLLL|
	//		+--------------+--------------+--------------+--------------+
	//
	//	Adjecent cells are packed tightly, and in each row the first and last cells
	//	sits flush against the sides of the scroll view's content area.
	//	Adjecent cells are packed tightly in the columns as well.
	//	However a margin, of height theMargin, sits above the first row.

	[self	getStride:				&theStride
			imageSizePt:			&theImageSizePt
			margin:					&theMargin
			numThumbnailsPerRow:	&theNumThumbnailsPerRow];

	theNumDrawings		= [itsThumbnails count];
	theNumRows			= (theNumDrawings + (theNumThumbnailsPerRow - 1)) / theNumThumbnailsPerRow;	//	integer division discards remainder
	theLabelWidthPt		= theStride;	//	UITextView seems to include a built-in margin (maybe for scrollbar?)
	theLabelHeightPt	= 2.0 * theMargin;

	[self setContentSize:(CGSize)
		{
			[self bounds].size.width,
			theMargin  +  theNumRows * theStride
				+ theMargin	//	Allow a little extra space beneath the labels in the bottom row.
		}];

	theLayoutDirection = [[UIApplication sharedApplication] userInterfaceLayoutDirection];
	
	theDelayFactor = 0;
	for (theIndex = aNameIndex; theIndex < theNumDrawings; theIndex++)
	{
		theRow = theIndex / theNumThumbnailsPerRow;	//	integer division discards remainder
		theCol = theIndex % theNumThumbnailsPerRow;
		if (theLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft)
			theCol = (theNumThumbnailsPerRow - 1) - theCol;

		theThumbnail = [itsThumbnails objectAtIndex:theIndex];
		
		theThumbnailFrame = (CGRect)
		{
			{
				theCol * theStride,
				theRow * theStride + theMargin
			},
			{
				theStride,
				theStride
			}
		};
		theIconFrame	= (CGRect){{theMargin, 0}, {theImageSizePt, theImageSizePt}};
		theLabelFrame	= (CGRect){{0.0, theImageSizePt}, {theLabelWidthPt, theLabelHeightPt}};

		[theThumbnail setItsFrame:		theThumbnailFrame	];	//	Set itsFrame within scrollview's content rect
		[theThumbnail setItsIconFrame:	theIconFrame		];	//	Set itsIconFrame  within the thumbnail's bounds
		[theThumbnail setItsLabelFrame:	theLabelFrame		];	//	Set itsLabelFrame within the thumbnail's bounds
		
		if (aScrollToMakeVisibleFlag && theIndex == aNameIndex)
		{
			//	The icon sits flush with the top of theThumbnailFrame,
			//	so when scrolling to the first row it looks better
			//	if we provide a little extra margin.
			theExtendedThumbnailFrame = theThumbnailFrame;
			theExtendedThumbnailFrame.origin.y		-= theMargin;
			theExtendedThumbnailFrame.size.height	+= theMargin;
			[self scrollRectToVisible:theExtendedThumbnailFrame animated:YES];
		}

		theThumbnailView	= [theThumbnail itsView ];
		theIconView			= [theThumbnail itsIcon ];
		theLabelView		= [theThumbnail itsLabel];

		if (theThumbnailView	!= nil	//	Either none or all three should exist.
		 && theIconView			!= nil
		 && theLabelView		!= nil)
		{
			//	Place theIconView and theLabelView within theThumbnailView.
			[theIconView  setFrame:theIconFrame ];
			[theLabelView setFrame:theLabelFrame];

			//	Place theThumbnailView within the scrollview's content rect.
			if (anAnimationFlag)
			{
				[UIView animateWithDuration:	0.25
						delay:					theDelayFactor * 0.03125
						options:				0
						animations:^
						{
							[theThumbnailView setFrame:theThumbnailFrame];
						}
						completion:				NULL];
				
				if (aStaggerFlag)
					theDelayFactor++;
			}
			else
			{
				[theThumbnailView setFrame:theThumbnailFrame];
			}
		}
	}
}

- (void)getStride:(NSUInteger *)aStride
		imageSizePt:(CGFloat *)anImageSizePt	//	image size in points, not pixels
		margin:(NSUInteger *)aMargin
		numThumbnailsPerRow:(NSUInteger *)aNumThumbnailsPerRow
{
	CGFloat	theImagePaddingPt,	//	image padding in points, not pixels
			theScrollViewWidth;

	//	For layout details, please see the comment and ASCII art
	//	in -layOutThumbnailsBeginningAtIndex immediately above.

	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
		theImagePaddingPt	= THUMBNAIL_IMAGE_MIN_PADDING_IPAD;
	else
		theImagePaddingPt	= THUMBNAIL_IMAGE_MIN_PADDING_IPHONE;

	*anImageSizePt			= GetThumbnailSizePt();
	theScrollViewWidth		= [self bounds].size.width;
	*aNumThumbnailsPerRow	= floor(theScrollViewWidth / (*anImageSizePt + 2.0*theImagePaddingPt));
	if (*aNumThumbnailsPerRow == 0)	//	could occur only if the thumbnail+padding is wider than itsPortfolioView
		*aNumThumbnailsPerRow = 1;
	*aStride				= (NSUInteger)theScrollViewWidth / *aNumThumbnailsPerRow;	//	integer division discards remainder
	*aMargin				= (*aStride - (NSUInteger)*anImageSizePt) / 2;	//	integer division discards remainder
}

- (void)installVisibleSubviews
{
	[self installVisibleSubviewsAtTop:NO];
}

- (void)installVisibleSubviewsAtTop:(BOOL)aScrollingToTopFlag
{
	CGFloat					theMinY,
							theMaxY;
	NSUInteger				theIndex;
	GeometryGamesThumbnail	*theThumbnail;
	CGRect					theThumbnailFrame;
	CGFloat					theAdjustedTop,
							theAdjustedBottom;
	
	theMinY = (aScrollingToTopFlag ?
				-[self contentInset].top :
				[self bounds].origin.y);
	theMaxY = theMinY + [self bounds].size.height;

	theIndex = 0;
	for (theThumbnail in itsThumbnails)
	{
		theThumbnailFrame = [theThumbnail itsFrame];

		//	As well as getting called during a scroll
		//	and when the user adds or removes thumbnails,
		//	this method also gets called at launch,
		//	before the thumbnails positions have been set,
		//	in which case we shouldn't try to load any views.
		//
		if (CGRectEqualToRect(theThumbnailFrame, CGRectZero))
			continue;
		
		//	Base the decision to load or unload the views
		//	not on the vertical coordinate y, but rather
		//	on the adjusted vertical coordinate y ± x/4,
		//	so the four or five thumbnails in a given row
		//	don't all need to get loaded at once.
		//
		//	This adjustment also lets the thumbnail image
		//	start loading in a background thread slightly
		//	before the icon becomes visible.
		//
		theAdjustedTop		= theThumbnailFrame.origin.y
							- 0.25 * theThumbnailFrame.origin.x;
		theAdjustedBottom	= theThumbnailFrame.origin.y
							+ theThumbnailFrame.size.height
							+ 0.25 * theThumbnailFrame.origin.x;

#if 0
#warning Disable this code for public release.
		//	During testing, enable these two lines
		//	if you want to watch the thumbnails come and go
		//	as you scroll the portfolio view.
		theAdjustedTop += 256.0;
		theAdjustedBottom -= 256.0;
#endif

		if (theAdjustedTop > theMaxY		//	 top   edge too low ?
		 || theAdjustedBottom < theMinY)	//	bottom edge too high?
		{
			[theThumbnail unloadIconAndLabelViews];
		}
		else
		{
			[theThumbnail	loadIconAndLabelViewsIntoContainingView:self
							atIndex:								theIndex
							withThumbnailTarget:					itsThumbnailTarget
							dispatchQueue:							itsDispatchQueue];
		}
		
		theIndex++;
	}
}

- (void)refreshViewTags
{
	NSUInteger				theNumFiles,
							theIndex;
	GeometryGamesThumbnail	*theThumbnail;
	
	theNumFiles = [itsThumbnails count];
	for (theIndex = 0; theIndex < theNumFiles; theIndex++)
	{
		theThumbnail = [itsThumbnails objectAtIndex:theIndex];
		[[theThumbnail itsView ] setTag:theIndex];	//	OK if itsView  == nil
		[[theThumbnail itsIcon ] setTag:theIndex];	//	OK if itsIcon  == nil
		[[theThumbnail itsLabel] setTag:theIndex];	//	OK if itsLabel == nil
	}
}

@end
