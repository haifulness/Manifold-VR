//	GeometryGamesPortfolioView.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>
#import "GeometryGamesThumbnail.h"


@interface GeometryGamesPortfolioView : UIScrollView
{
}

- (id)initWithFrame:(CGRect)aFrame
	thumbnails:(NSMutableArray<GeometryGamesThumbnail *> *)someThumbnails
	thumbnailTarget:(id<GeometryGamesThumbnailGestureTarget, UITextViewDelegate>)aThumbnailTarget;
- (void)dealloc;

- (void)layoutSubviews;

- (void)layOutThumbnailsBeginningAtIndex:(NSUInteger)aNameIndex
		animated:(BOOL)anAnimationFlag staggered:(BOOL)aStaggerFlag
		scroll:(BOOL)aScrollToMakeVisibleFlag;
- (void)getStride:(NSUInteger *)aStride imageSizePt:(CGFloat *)anImageSizePt
		margin:(NSUInteger *)aMargin numThumbnailsPerRow:(NSUInteger *)aNumThumbnailsPerRow;
- (void)installVisibleSubviews;
- (void)installVisibleSubviewsAtTop:(BOOL)aScrollingToTopFlag;
- (void)refreshViewTags;

@end
