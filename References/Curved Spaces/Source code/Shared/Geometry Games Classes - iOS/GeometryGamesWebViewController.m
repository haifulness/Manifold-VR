//	GeometryGamesWebViewController.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <WebKit/WebKit.h>
#import "GeometryGamesWebViewController.h"


//	DISABLE_TEXT_SELECTION_AT_FLANDRAU is useful for the Flandrau version of Torus Games,
//	to prevent exhibit visitors from getting access to the iOS Notes app.
//	Otherwise it's best to leave text selection enabled,
//	mainly to allow ordinary users to copy text from the Help pages,
//	but also to keep JavaScript disabled for better performance.
//#define DISABLE_TEXT_SELECTION_AT_FLANDRAU
#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
#warning DISABLE_TEXT_SELECTION_AT_FLANDRAU is enabled (should be enabled only for Flandrau Torus Games)
#endif


#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU

@interface GeometryGamesWebViewController() <WKNavigationDelegate>
- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler;
- (void)webView:(WKWebView *)webView decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler;
@end

#else

@interface GeometryGamesWebViewController()
@end

#endif


@implementation GeometryGamesWebViewController
{
	NSString		*itsDirectoryName,	//	keeps a private copy
					*itsFileName;		//	keeps a private copy
	UIBarButtonItem	*itsCloseButton;
	BOOL			itsShowCloseButtonAlways;
	BOOL			itsPrefersStatusBarHidden;
	WKWebView		*itsWebView;

#ifdef USE_UIWEBVIEW_ON_IOS8
	bool		itsUseLegacyUIWebViewFlag;
	UIWebView	*itsWebView_FOR_IOS8;	//	must be strong reference -- see comment in -dealloc
#endif
}

- (id)	initWithDirectory:		(NSString *)aDirectoryName
		page:					(NSString *)aFileName
		closeButton:			(UIBarButtonItem *)aCloseButton
		showCloseButtonAlways:	(BOOL)aShowCloseButtonAlways	//	So the user can close a popover that has passthrough views.
																//		Currently always "YES", so parameter could be eliminated.
		hideStatusBar:			(BOOL)aPrefersStatusBarHidden
{
	self = [super init];
	if (self != nil)
	{
		itsDirectoryName			= [aDirectoryName copy];
		itsFileName					= [aFileName copy];
		itsCloseButton				= aCloseButton;
		itsShowCloseButtonAlways	= aShowCloseButtonAlways;
		
		itsPrefersStatusBarHidden = aPrefersStatusBarHidden;
		
		//	The web page itself will contain a title,
		//	so leave the navigation bar's title area empty.
		[self setTitle:@""];

		[self setPreferredContentSize:(CGSize){HELP_PICKER_WIDTH, HELP_PICKER_HEIGHT}];
	
#ifdef USE_UIWEBVIEW_ON_IOS8
		itsUseLegacyUIWebViewFlag = ([[NSProcessInfo processInfo] operatingSystemVersion].majorVersion < 9);
//itsUseLegacyUIWebViewFlag = true;	//	for testing only
#endif
	}
	return self;
}

#ifdef USE_UIWEBVIEW_ON_IOS8
- (void)dealloc
{
	if (itsUseLegacyUIWebViewFlag)
	{
		//	The UIWebViewDelegate documentation says
		//
		//		Important: Before releasing an instance of UIWebView 
		//		for which you have set a delegate, you must first set 
		//		the UIWebView delegate property to nil before disposing
		//		of the UIWebView instance.  This can be done, for example,
		//		in the dealloc method where you dispose of the UIWebView.
		//
		//	To comply with this bizarre request, we've kept our own
		//	private reference to the UIWebView, which we now use
		//	to clear its delegate.  ARC will then automatically
		//	clear the itsWebView_FOR_IOS8 reference itself.
		//
		[itsWebView_FOR_IOS8 setDelegate:nil];
	}
}
#endif	//	USE_UIWEBVIEW_ON_IOS8


- (BOOL)prefersStatusBarHidden
{
	return itsPrefersStatusBarHidden;
}

- (void)loadView
{
	WKPreferences			*thePreferences;
	WKWebViewConfiguration	*theConfiguration;
#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
	NSString				*theUserScriptSource;
	WKUserScript			*theUserScript;
	WKUserContentController	*theUserContentController;
#endif

	//	-loadView merely decides what subclass of UIView to load, and then loads it.
	//	-viewDidLoad will handle all subsequent view setup.

#ifdef USE_UIWEBVIEW_ON_IOS8
	if (itsUseLegacyUIWebViewFlag)
	{
		itsWebView_FOR_IOS8 = [[UIWebView alloc] initWithFrame:CGRectZero];	//	frame will get set automatically
		[itsWebView_FOR_IOS8 setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
		[self setView:itsWebView_FOR_IOS8];

		//	Suppress incremental rendering to avoid the re-rendering caused
		//	by the "viewport width" hack in -webViewDidFinishLoad: .
		//	(Hmmm... hard to judge whether this really has much of an effect.)
//		[itsWebView_FOR_IOS8 setSuppressesIncrementalRendering:YES];
	}
	else
#endif
	{
		thePreferences = [[WKPreferences alloc] init];
#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
		[thePreferences setJavaScriptEnabled:YES];	//	Enable JavaScript for Flandrau Torus Games
#else
		[thePreferences setJavaScriptEnabled:NO];	//	Otherwise leave JavaScript disabled
#endif

#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
		//	The following code was adapted from
		//
		//		https://paulofierro.com/blog/2015/7/13/disabling-callouts-in-wkwebview
		//
		theUserScriptSource =
			@"var style = document.createElement('style'); \
			style.type = 'text/css'; \
			style.innerText = '* { -webkit-user-select: none; -webkit-touch-callout: none; }'; \
			var head = document.getElementsByTagName('head')[0]; \
			head.appendChild(style);";
		theUserScript = [[WKUserScript alloc]
			initWithSource:		theUserScriptSource
			injectionTime:		WKUserScriptInjectionTimeAtDocumentEnd
			forMainFrameOnly:	YES];
		theUserContentController = [[WKUserContentController alloc] init];
		[theUserContentController addUserScript:theUserScript];
#endif

		theConfiguration = [[WKWebViewConfiguration alloc] init];
		[theConfiguration setPreferences:thePreferences];
#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
		[theConfiguration setUserContentController:theUserContentController];
#endif
		itsWebView = [[WKWebView alloc]
			initWithFrame:	CGRectZero	//	frame will get set automatically
			configuration:	theConfiguration];
#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU
		[itsWebView setNavigationDelegate:self];
#endif
		[itsWebView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
		[self setView:itsWebView];
	}
}

- (void)viewDidLoad
{
	NSString	*theResourcePath,
				*theDirectoryPath,
				*theFilePath;

	[super viewDidLoad];
	
#ifdef USE_UIWEBVIEW_ON_IOS8
	if (itsUseLegacyUIWebViewFlag)
	{
		[itsWebView_FOR_IOS8 setDelegate:self];
		[itsWebView_FOR_IOS8
			loadData:			[NSData dataWithContentsOfFile:[[NSBundle mainBundle]
									pathForResource:itsFileName ofType:nil inDirectory:itsDirectoryName]]
			MIMEType:			@"text/html"
			textEncodingName:	@"utf-8"
			baseURL:			[NSURL	fileURLWithPath:	[[[NSBundle mainBundle] resourcePath]
																stringByAppendingPathComponent:itsDirectoryName]
										isDirectory:		YES]
		];
	}
	else	//	We're running on iOS 9 or higher, and so WKWebView's
			//	-loadFileURL:allowingReadAccessToURL: is available...
#endif
	{
		theResourcePath		= [[NSBundle mainBundle] resourcePath];
		theDirectoryPath	= [theResourcePath  stringByAppendingPathComponent:itsDirectoryName];
		theFilePath			= [theDirectoryPath stringByAppendingPathComponent:itsFileName		];

		[itsWebView
			loadFileURL:				[NSURL fileURLWithPath:theFilePath      isDirectory:NO ]
			allowingReadAccessToURL:	[NSURL fileURLWithPath:theDirectoryPath isDirectory:YES]];
	}
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];

	[self adaptNavBarForHorizontalSize:
		[[[[UIApplication sharedApplication] keyWindow] traitCollection] horizontalSizeClass]];
}


#ifdef DISABLE_TEXT_SELECTION_AT_FLANDRAU

#pragma mark -
#pragma mark WKNavigationDelegate (Flandrau only)

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
	//	In the Flandrau version of the Torus Games,
	//	allow navigation only to
	//
	//		- the app's own internal Help files
	//		- external files at www.geometrygames.org
	//
	if ([[[navigationAction request] URL] isFileURL]
	 || [[[[navigationAction request] URL] host] hasPrefix:@"www.geometrygames.org"])
	{
		decisionHandler(WKNavigationActionPolicyAllow);
	}
	else
	{
		decisionHandler(WKNavigationActionPolicyCancel);
	}
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler
{
	//	In the Flandrau version of the Torus Games,
	//	allow navigation only to files of MIME type "text/html",
	//	never to other files (for example, of type "application/zip").
	//
	if ([[[navigationResponse response] MIMEType] isEqualToString:@"text/html"])
		decisionHandler(WKNavigationResponsePolicyAllow);
	else
		decisionHandler(WKNavigationResponsePolicyCancel);
}

#endif	//	DISABLE_TEXT_SELECTION_AT_FLANDRAU


#pragma mark -
#pragma mark GeometryGamesPopover

- (void)adaptNavBarForHorizontalSize:(UIUserInterfaceSizeClass)aHorizontalSizeClass
{
	if (itsShowCloseButtonAlways)
	{
		//	The Help popover apparently includes the main game area
		//	in its set of passthrough views, to let the user play
		//	with the game while reading the Help page.
		//	So it's asked us to show itsCloseButton
		//	even in a horizontally regular layout.
		[[self navigationItem] setRightBarButtonItem:itsCloseButton animated:NO];
	}
	else
	{
		//	Show itsCloseButton only in a horizontally compact layout,
		//	where the Help panel will cover the whole window.
		[[self navigationItem]
			setRightBarButtonItem:	(aHorizontalSizeClass == UIUserInterfaceSizeClassCompact ?
										itsCloseButton : nil)
			animated:				NO];
	}
}


#ifdef USE_UIWEBVIEW_ON_IOS8

#pragma mark -
#pragma mark UIWebViewDelegate

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
//	[itsActivityIndicator startAnimating];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
	unsigned int	theWebViewWidth;
	NSString		*theJavaScriptCommand;
	
//	This workaround is still needed in the iOS 8.1 simulator (and presumably on the real device too).

	//	When a web page includes
	//
	//		<meta name="viewport" content="width=device-width">
	//
	//	then in a popover on an iPad, UIWebView unfortunatley
	//	reports the device-width as the full width of the iPad,
	//	not just the width of the popover.  Fortunately the page
	//
	//		http://stackoverflow.com/questions/7808579/displaying-wiki-mobile-page-in-uiwebview-within-uipopovercontroller
	//
	//	provides the following workaround, using JavaScript.
	//	That page in fact provides code that goes a little further
	//	and provides a "viewport" tag if the page doesn't already have one,
	//	but that seems unnecessary in the present context
	//	because UIWebView does fine with pages that
	//	don't specify "width=device-width".
	//
	//	If some future version of UIWebView interprets the device-width
	//	as the width of the popover rather the width of the whole device,
	//	then I can remove this code entirely, along with any possible
	//	call to -setSuppressesIncrementalRendering: in -loadView: .
	//
//NSLog(@"URL: %@", [[[webView request] URL] absoluteString]);
	if
	(
		//	Only the iPad supports popovers, so no need to waste time with this on iPhones.
		//	(Note that this legacy UIWebKit-based code gets called only on iOS 8,
		//	so there's no possibility of split-screen multitasking here.
		//	If we're running on an iPad, the web views are being presented as popovers.)
		//
		[[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad
	 &&
//	Even on our internal help pages I've replaced
//
//		<meta name="viewport" content="width=320">
//	with
//		<meta name="viewport" content="width=device-width">
//
//	because it's the clean way to do things, and WKWebView is fine with it
//	(this GeometryGamesWebViewController uses WKWebView instead of UIWebView
//	on iOS 9 and later;  on iOS 8 there's a bug that WKWebView can't load
//	local content).
//
//		//	Let's reset the width only on external pages (with URLs "http://..."),
//		//	not on our own internal help pages (with URLs "file:///...").
//		//	It would be harmless to reset the width on all pages,
//		//	but I'd rather not do so for two reasons:
//		//		- to avoid wasting time needlessly re-rendering all help pages, and
//		//		- for future robustness (if anything changes in the future,
//		//			the internal help files won't be affected)
//	 	[[[[webView request] URL] absoluteString] rangeOfString:@"http://"].location != NSNotFound
//	 &&
		//	If a page contains several frames, -webViewDidFinishLoad:
		//	will get called after each frame finishes loading.
		//	So let's test explicitly whether the whole page has finished loading.
		! [webView isLoading]
	)
	{
		theWebViewWidth			= (unsigned int) [webView frame].size.width;
		theJavaScriptCommand	= [NSString stringWithFormat:
			@"var viewport = document.querySelector('meta[name=viewport]');if (viewport != null) {viewport.setAttribute('content','width=%dpx');} else {}",
			theWebViewWidth];
		
		[webView stringByEvaluatingJavaScriptFromString:theJavaScriptCommand];
	}
	
//	[itsActivityIndicator stopAnimating];
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
	UIAlertController	*theAlertController;

//	[itsActivityIndicator stopAnimating];

	//	This method never gets called, even if the requested HTML file is missing.
	
	if (error != nil)
	{
		theAlertController = [UIAlertController
			alertControllerWithTitle:	[error localizedDescription]
			message:					[error localizedFailureReason]
			preferredStyle:				UIAlertControllerStyleAlert];

		[theAlertController addAction:[UIAlertAction
			actionWithTitle:	@"OK"
			style:				UIAlertActionStyleDefault
			handler:			nil]];

		[self
			presentViewController:	theAlertController
			animated:				YES
			completion:				nil];
	}
}

#endif	//	USE_UIWEBVIEW_ON_IOS8


@end
