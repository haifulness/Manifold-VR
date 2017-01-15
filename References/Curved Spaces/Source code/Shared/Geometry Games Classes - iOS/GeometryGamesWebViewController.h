//	GeometryGamesWebViewController.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>
#import "GeometryGamesPopover.h"

//	The app’s HelpChoiceController and this GeometryGamesWebViewController
//	must both see these values.  It's also possible for the size to vary, see
//
//		http://stackoverflow.com/questions/2752394/popover-with-embedded-navigation-controller-doesnt-respect-size-on-back-nav
//
#define HELP_PICKER_WIDTH	320
#define HELP_PICKER_HEIGHT	480

//	There's a bug in iOS 8 (all versions, apparently)
//	that prevents a WKWebView from loading local content,
//	apparently due to sandboxing issues.  See
//
//		http://openradar.appspot.com/20160687
//
#if (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_9_0)
#define USE_UIWEBVIEW_ON_IOS8
#else
#error WKWebView can load local content OK on iOS 9 and up, \
so we no longer need an alternative code path to use UIWebView on iOS 8. \
All code bracketed by USE_UIWEBVIEW_ON_IOS8 may now be deleted.
#endif

@interface GeometryGamesWebViewController : UIViewController
	<GeometryGamesPopover
#ifdef USE_UIWEBVIEW_ON_IOS8
	, UIWebViewDelegate
#endif
	>

- (id)initWithDirectory:(NSString *)aDirectoryName page:(NSString *)aFileName
		closeButton:(UIBarButtonItem *)aCloseButton
		showCloseButtonAlways:(BOOL)aShowCloseButtonAlways
		hideStatusBar:(BOOL)aPrefersStatusBarHidden;
#ifdef USE_UIWEBVIEW_ON_IOS8
- (void)dealloc;
#endif
- (void)loadView;
- (void)viewDidLoad;
- (void)viewWillAppear:(BOOL)animated;

//	GeometryGamesPopover
- (void)adaptNavBarForHorizontalSize:(UIUserInterfaceSizeClass)aHorizontalSizeClass;

#ifdef USE_UIWEBVIEW_ON_IOS8
//	UIWebViewDelegate
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void)webViewDidStartLoad:(UIWebView *)webView;
- (void)webViewDidFinishLoad:(UIWebView *)webView;
- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
#endif

@end
