//	GeometryGamesPortfolioBasedAppDelegate.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <UIKit/UIKit.h>


@class GeometryGamesPortfolioController;


@interface GeometryGamesPortfolioBasedAppDelegate : UIResponder <UIApplicationDelegate>
{
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions;
- (void)performAdditionalLaunchInitializations;
- (GeometryGamesPortfolioController *)newPortfolioController;

- (void)applicationWillResignActive:(UIApplication *)application;
- (void)applicationDidEnterBackground:(UIApplication *)application;
- (void)applicationWillEnterForeground:(UIApplication *)application;
- (void)applicationDidBecomeActive:(UIApplication *)application;
- (void)applicationWillTerminate:(UIApplication *)application;

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application;

@end
