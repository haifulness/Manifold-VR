//	GeometryGamesThumbnail.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>


@protocol GeometryGamesThumbnailGestureTarget
- (void)userTappedFileIcon:(UITapGestureRecognizer *)aTapGestureRecognizer;
- (void)userLongPressedFileIcon:(UILongPressGestureRecognizer *)aLongPressGestureRecognizer;
- (void)userPannedFileIcon:(UIPanGestureRecognizer *)aPanGestureRecognizer;
@end


@interface GeometryGamesThumbnail : NSObject

//	The drawing name, as seen by the user.
//	The same name is used for the drawing file "<name>.txt"
//	and the thumbnail file "<name>.png".
@property (nonatomic, copy)		NSString	*itsName;

//	Is itsName still a default name ("Untitled"),
//	or has the user edited it?
@property (nonatomic)			BOOL		itsNameHasBeenEdited;

//	Keep track of where the thumbnail's frame would sit
//	in the scroll view's content view, whether or not
//	the thumbnail is currently loaded. Typically only
//	the thumbnails that fall in or near the visible part
//	of the content area are loaded.  All other thumbnails
//	are unloaded to keep memory demands light
//	even if the user has a thousand drawings.
@property (nonatomic)			CGRect		itsFrame,		//	placement of itsView  in scrollview
											itsIconFrame,	//	placement of itsIcon  in itsView
											itsLabelFrame;	//	placement of itsLabel in itsView

//	itsView represents the thumbnail as a whole,
//	and serves as a container view to hold itsIcon and itsLabel.
//	itsIcon displays a thumbnail image of the drawing
//	while itsLabel display the drawing's name as editable text.
//	UITextField doesn't support multi-line text,
//	so let itsLabel be a UITextView instead.
@property (nonatomic, strong)	UIView		*itsView;	//	= nil when view isn't loaded
@property (nonatomic, strong)	UIImageView	*itsIcon;	//	= nil when view isn't loaded
@property (nonatomic, strong)	UITextView	*itsLabel;	//	= nil when view isn't loaded

- (id)initWithName:(NSString *)aName nameEdited:(BOOL)aNameHasBeenEdited;
- (void)dealloc;

- (void)loadIconAndLabelViewsIntoContainingView:(UIView *)aContainingView atIndex:(NSUInteger)anIndex
	withThumbnailTarget:(id<GeometryGamesThumbnailGestureTarget, UITextViewDelegate>)aThumbnailTarget
	dispatchQueue:(dispatch_queue_t)aDispatchQueue;
- (void)unloadIconAndLabelViews;

@end
