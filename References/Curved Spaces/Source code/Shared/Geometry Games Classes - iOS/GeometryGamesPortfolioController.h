//	GeometryGamesPortfolioController.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>
#import "GeometryGamesThumbnail.h"


typedef enum
{
	TapToOpen,	//	tap opens drawing, press-and-hold opens menu
	TapForMenu	//	tap opens menu
} GeometryGamesTapMode;


//	The subclass will provide these app-specific functions.
extern void		CreateEmptyDrawingFile(NSString *aFileName);
extern void		CreateEmptyThumbnailFile(NSString *aFileName);
extern bool		ThumbnailImageWantsMultisampling(void);
extern bool		ThumbnailImageWantsDepthBuffer(void);


@class GeometryGamesGraphicsViewController;


@interface GeometryGamesPortfolioController : UIViewController
	<UIScrollViewDelegate, GeometryGamesThumbnailGestureTarget, UITextViewDelegate>
{
	//	Should a tap on a thumbnail open the drawing or just a menu?
	GeometryGamesTapMode	itsTapMode;
}

- (id)init;
- (void)dealloc;

- (void)viewDidLoad;
- (UIColor *)scrollViewBackgroundColor;

- (void)viewWillAppear:(BOOL)animated;
- (void)viewDidAppear:(BOOL)animated;

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection;

//	UIScrollViewDelegate
- (void)scrollViewDidScroll:(UIScrollView *)scrollView;

//	toolbar
- (UIViewController *)contentViewControllerForPreferencesPanel;

//	GeometryGamesThumbnailGestureTarget
- (void)userTappedFileIcon:(UITapGestureRecognizer *)aTapGestureRecognizer;
- (void)userLongPressedFileIcon:(UILongPressGestureRecognizer *)aLongPressGestureRecognizer;
- (void)userPannedFileIcon:(UIPanGestureRecognizer *)aPanGestureRecognizer;

//	UITextViewDelegate
- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text;
- (void)textViewDidBeginEditing:(UITextView *)textView;
- (void)textViewDidEndEditing:(UITextView *)textView;

- (GeometryGamesGraphicsViewController *)drawingControllerForFileNamed:(NSString *)aFileName;

- (unsigned int)maxExportMagnificationFactor;

@end
