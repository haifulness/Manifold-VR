//	GeometryGamesPortfolioBasedAppDelegate.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesPortfolioBasedAppDelegate.h"
#import "GeometryGamesPortfolioController.h"
#import "GeometryGamesUtilities-Mac-iOS.h"
#import "GeometryGamesLocalization.h"


//	Privately-declared methods and properties
@interface GeometryGamesPortfolioBasedAppDelegate()
@end


@implementation GeometryGamesPortfolioBasedAppDelegate
{
	UIWindow	*itsWindow;	//	strong reference keeps window alive
}


#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	NSString							*theTwoLetterLanguageCodeAsNSString;
	Char16								theTwoLetterLanguageCode[3];
	GeometryGamesPortfolioController	*theRootContentController;
	UINavigationController				*theNavigationController;

	UNUSED_PARAMETER(application);
	UNUSED_PARAMETER(launchOptions);

#if TARGET_IPHONE_SIMULATOR
NSLog(@"Documents directory:  %@",
	NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0]);
#endif

	[[NSUserDefaults standardUserDefaults] registerDefaults:
		@{
			//	At first launch we'll want to open an empty drawing
			//	and show the Help window.
			@"is first launch" : @YES,
			
			//	Should a tap on a thumbnail open a menu or open the drawing itself?
			//	For first-time users leave this set to No, so that they
			//	may immediately see a way to duplicate and delete files.
			@"tap opens drawing directly" : @NO,
			
			//	Should exported images be rendered
			//	at single, double or quadruple resolution?
			@"exported image magnification factor" : @1
		}
	];

	//	Get the user's preferred language and convert it to a zero-terminated UTF-16 string.
	theTwoLetterLanguageCodeAsNSString = GetPreferredLanguage();
	[theTwoLetterLanguageCodeAsNSString getCharacters:theTwoLetterLanguageCode range:(NSRange){0,2}];
	theTwoLetterLanguageCode[2] = 0;

	//	Initialize the language code and dictionary to the user's default language.
	SetCurrentLanguage(theTwoLetterLanguageCode);
	
	//	Let the subclass perform additional app-specific initializations.
	[self performAdditionalLaunchInitializations];

	//	Create itsWindow.
	itsWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	
	//	Create theNavigationController and assign it to the window.
	//	theNavigationController's own root controller is a KaleidoPaintPortfolioController.
	//
	//		Note that the UIKit uses the word "root" in two different ways:
	//		The root view controller for the app as a whole is theNavigationController,
	//		while the root view controller within theNavigationController's
	//		own stack is theRootContentController.
	//
	theRootContentController	= [self newPortfolioController];
	theNavigationController		= [[UINavigationController alloc]
									initWithRootViewController:theRootContentController];
	[itsWindow setRootViewController:theNavigationController];
	[theNavigationController setToolbarHidden:NO];
	[[theNavigationController navigationBar] setTranslucent:NO];

	//	Show the window.
	[itsWindow makeKeyAndVisible];

	return YES;
}

- (void)performAdditionalLaunchInitializations
{
	//	The subclass may override this method.
}

- (GeometryGamesPortfolioController *)newPortfolioController
{
	//	The subclass may override this method.
	return [[GeometryGamesPortfolioController alloc] init];
}


- (void)applicationWillResignActive:(UIApplication *)application
{
	//	Sent when the application is about to move from active to inactive state. 
	//	This can occur for certain types of temporary interruptions 
	//	(such as an incoming phone call or SMS message) or when the user quits 
	//	the application and it begins the transition to the background state.
	//	Use this method to pause ongoing tasks, disable timers, and throttle down 
	//	OpenGL ES frame rates. Games should use this method to pause the game.
	
	UNUSED_PARAMETER(application);
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	//	Use this method to release shared resources, save user data, 
	//	invalidate timers, and store enough application state information 
	//	to restore your application to its current state in case it is terminated later. 
	//	If your application supports background execution, called instead 
	//	of applicationWillTerminate: when the user quits.

	UNUSED_PARAMETER(application);
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	//	Called as part of transition from the background to the inactive state: 
	//	here you can undo many of the changes made on entering the background.

	UNUSED_PARAMETER(application);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	//	Restart any tasks that were paused (or not yet started) 
	//	while the application was inactive. If the application was previously 
	//	in the background, optionally refresh the user interface.

	UNUSED_PARAMETER(application);
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	//	Called only when the app is about to get terminated
	//	while actively running in the background.
	//	For the Geometry Games apps, which do no background processing,
	//	-applicationWillTerminate: will likely never get called.
	
	//	Note:  When a suspended app is terminated, it disappears immediately.
	//	The system does *not* call -applicationWillTerminate:.
	//	So better to save data in -applicationDidEnterBackground:.

	UNUSED_PARAMETER(application);
}


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	//	Apparently background apps don't get memory warnings,
	//	only foreground apps do.  Our GeometryGamesGraphicsViewController's
	//	-applicationDidEnterBackground calls [self shutDownAllGraphics] to release
	//	all OpenGL ES objects (including framebuffers, texture memory, etc.)
	//	as soon as we enter the background, to keep memory usage 
	//	to a minimum whether we receive a memory warning or not.

	UNUSED_PARAMETER(application);
}


@end
