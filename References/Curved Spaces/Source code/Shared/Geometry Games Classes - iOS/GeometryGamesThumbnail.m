//	GeometryGamesThumbnail.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesThumbnail.h"
#import "GeometryGamesUtilities-iOS.h"


//	Use a smaller font on iPhone and iPod Touch,
//	and a larger font on iPad.
#define THUMBNAIL_LABEL_FONT_SIZE_IPAD		17.0
#define THUMBNAIL_LABEL_FONT_SIZE_IPHONE	11.0


@implementation GeometryGamesThumbnail


- (id)initWithName:(NSString *)aName nameEdited:(BOOL)aNameHasBeenEdited
{
	self = [super init];
	if (self != nil)
	{
		[self setItsName:				aName];	//	Copies aName.
		[self setItsNameHasBeenEdited:	aNameHasBeenEdited];
		[self setItsFrame:				CGRectZero];
		[self setItsIconFrame:			CGRectZero];
		[self setItsLabelFrame:			CGRectZero];
		[self setItsView:				nil];
		[self setItsIcon:				nil];
		[self setItsLabel:				nil];
	}
	return self;
}

- (void)dealloc
{
	[_itsView  removeFromSuperview];	//	OK if _itsView  is nil or if the superview is already nil.
	[_itsIcon  removeFromSuperview];	//	OK if _itsIcon  is nil or if the superview is already nil.
	[_itsLabel removeFromSuperview];	//	OK if _itsLabel is nil or if the superview is already nil.
}


- (void)loadIconAndLabelViewsIntoContainingView:(UIView *)aContainingView atIndex:(NSUInteger)anIndex
	withThumbnailTarget:(id<GeometryGamesThumbnailGestureTarget, UITextViewDelegate>)aThumbnailTarget
	dispatchQueue:(dispatch_queue_t)aDispatchQueue
{
	UIImageView	*theIcon;
	NSString	*theDrawingName;
	UIFont		*theLabelFont;
	UITextView	*theLabel;

	//	Caller should have already called
	//	-layOutThumbnailsBeginningAtIndex:animated:staggered:scroll:
	//	to set itsFrame, itsIconFrame and itsLabelFrame.

	//	Either _itsView, _itsIcon and _itsLabel should all be present,
	//	or none of them should.

	if (_itsIcon == nil)
	{
		//	We'll pass a local pointer (theIcon) into the dispatch_async() block
		//	instead of an instance variable (_itsIcon), so the block will
		//	retain only the UIImageView, not this GeometryGamesThumbnail.
		
		theIcon = [[UIImageView alloc] initWithImage:nil];	//	UIImageView disables user interaction by default
		[theIcon setUserInteractionEnabled:YES];			//	manually re-enable user interaction
		[self setItsIcon:theIcon];
		
		//	The AppKit will happily let the user drag several thumbnails
		//	at the same time, but the final placements are a bit arbitrary.
		//	To avoid such effects, insist on exclusiveTouch.
		[theIcon setExclusiveTouch:YES];

		theDrawingName = _itsName;	//	again, pass a local variable, not an instance variable
		dispatch_async(aDispatchQueue, ^(void)
		{
			UIImage	*theImage;
			
			//	Apple says
			//
			//		Although GCD dispatch queues have their own autorelease pools,
			//		they make no guarantees as to when those pools are drained.
			//
			//	so for maximal safety let's release any temporary objects
			//	that may get created.
			//
			@autoreleasepool
			{
				theImage = LoadScaledThumbnailImage(theDrawingName);

				dispatch_async(dispatch_get_main_queue(), ^(void)
				{
					[theIcon setImage:theImage];
				});
			}
		});

		[theIcon setFrame:_itsIconFrame];		//	caller should have already set _itsIconFrame
		[theIcon setTag:anIndex];

		[theIcon addGestureRecognizer:
			[[UITapGestureRecognizer alloc]
				initWithTarget:	aThumbnailTarget
				action:			@selector(userTappedFileIcon:)] ];

		[theIcon addGestureRecognizer:
			[[UILongPressGestureRecognizer alloc]
				initWithTarget:	aThumbnailTarget
				action:			@selector(userLongPressedFileIcon:)] ];

		[theIcon addGestureRecognizer:
			[[UIPanGestureRecognizer alloc]
				initWithTarget:	aThumbnailTarget
				action:			@selector(userPannedFileIcon:)] ];
	}

	if (_itsLabel == nil)
	{
		theLabelFont = [UIFont
						fontWithName:	@"Helvetica"
						size:			([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad ?
											THUMBNAIL_LABEL_FONT_SIZE_IPAD :
											THUMBNAIL_LABEL_FONT_SIZE_IPHONE)
						];
		
		//	Caller should have already set itsLabelFrame.
		theLabel = [[UITextView alloc] initWithFrame:_itsLabelFrame textContainer:nil];
		[self setItsLabel:theLabel];
		
		[theLabel setExclusiveTouch:YES];	//	safe and robust

		[theLabel setTag:anIndex];
		[theLabel setText:_itsName];
#if 0
#warning suppress this line (for use when making 4D Draw screenshots)
if ([_itsName isEqualToString:@"alternating vertices of hypercube"])
	[theLabel setText:@"alternating vertices\nof hypercube"];
#endif
		[theLabel setTextAlignment:NSTextAlignmentCenter];
		[theLabel setFont:theLabelFont];
		[theLabel setTextColor:[UIColor colorWithRed:0.00 green:0.25 blue:0.25 alpha:1.00]];
		[theLabel setBackgroundColor:[UIColor clearColor]];
		[theLabel setReturnKeyType:UIReturnKeyDone];
		[theLabel setDelegate:aThumbnailTarget];
	}

	if (_itsView == nil)
	{
		[self setItsView:[[UIView alloc] initWithFrame:[self itsFrame]]];
		
		[_itsView setFrame:_itsFrame];		//	caller should have already set _itsFrame
		[_itsView setTag:anIndex];

		[_itsView addSubview:_itsIcon ];
		[_itsView addSubview:_itsLabel];

		[aContainingView addSubview:_itsView];
	}
}

- (void)unloadIconAndLabelViews
{
	[_itsIcon  removeFromSuperview];	//	OK if _itsIcon  is nil or if the superview is already nil
	[_itsLabel removeFromSuperview];	//	OK if _itsLabel is nil or if the superview is already nil
	[_itsView  removeFromSuperview];	//	OK if _itsView  is nil or if the superview is already nil

	[self setItsIcon: nil];
	[self setItsLabel:nil];
	[self setItsView: nil];
}


@end
