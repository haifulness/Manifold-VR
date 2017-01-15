//	GeometryGamesModel.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import "GeometryGamesModel.h"
#import "GeometryGamesUtilities-Common.h"

//	On MacOS,
//		stdlib.h includes Availability.h
//		which includes AvailabilityInternal.h
//		which defines __MAC_OS_X_VERSION_MIN_REQUIRED
//		which is the preferred way to check whether
//			the code is compiling on MacOS.
#include <stdlib.h>
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#define REPORT_LAST_MODEL_CLOSED
#endif

#ifdef REPORT_LAST_MODEL_CLOSED
//	Normally when a user quits a Mac application
//	the system terminates it as quickly as possible,
//	without explicitly deallocating each block of memory.
//	However, if we want to confirm that there were no leaks
//	in the platform-independent C code, we must instead
//	close each window to free its ModelData, and then
//	once all the ModelData objects have been freed,
//	tell the app delegate that it's time to check for leaks
//	and terminate the application.
//
//		Caution:  If a window wants to replace its model, it must
//		of course create the new one before freeing the old one,
//		because otherwise the app would terminate!  Fortunately
//		creating the new model before freeing the old one
//		is standard practice anyhow, so that in case of error
//		while creating the new model, the old model may be kept.
//
static unsigned int	gNumActiveModels = 0;
#endif


@implementation GeometryGamesModel
{
	//	Mac OS
	//		The main thread and the CVDisplayLink thread both need 
	//		to access the ModelData, so use an NSLock to prevent conflicts.
	//
	//	iOS
	//		The CADisplayLink runs on the main thread, so in principle 
	//		no lock is needed.  Nevertheless, it seems worth using
	//		locking on iOS anyhow, to keep the Mac and iOS code 
	//		as similar as possible.
	//
	//	Only the -lockModelData method may directly access
	//	the instance variable itsModelDataPROTECTED.
	//	All other code should instead check out a pointer, use it, 
	//	and then check it back in again as soon as possible:
	//
	//		ModelData	md = NULL;
	//
	//		[theModel lockModelData:&md];
	//		...
	//		use md as desired
	//		...
	//		[theModel unlockModelData:&md];
	//
	ModelData	*itsModelDataPROTECTED;
	NSLock		*itsModelDataLock;
}


- (id)init
{
	self = [super init];
	if (self != nil)
	{
		//	Let the app-specific code allocate the ModelData.
		itsModelDataPROTECTED = (ModelData *)GET_MEMORY(SizeOfModelData());
		if (itsModelDataPROTECTED == NULL)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		
		//	Initialize the ModelData.
		SetUpModelData(itsModelDataPROTECTED);

		//	Create a lock to ensure that the main thread and 
		//	the CVDisplayLink never access the ModelData simultaneously.
		itsModelDataLock = [[NSLock alloc] init];

#ifdef REPORT_LAST_MODEL_CLOSED
		gNumActiveModels++;
#endif
	}
	return self;
}

- (void)dealloc
{
	//	If we're getting dealloc'd, that should mean that
	//	no objects hold a pointer to us, and no objects
	//	hold itsModelDataLock.
	
	ShutDownModelData(itsModelDataPROTECTED);
	FREE_MEMORY_SAFELY(itsModelDataPROTECTED);

#ifdef REPORT_LAST_MODEL_CLOSED

	gNumActiveModels--;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
	if (gNumActiveModels == 0)
		[NSApp sendAction:@selector(lastModelDidDeallocate:) to:nil from:self];
#pragma clang diagnostic pop

#endif	//	REPORT_LAST_MODEL_CLOSED
}


- (void)lockModelData:(ModelData **)aModelDataPtr
{
	GEOMETRY_GAMES_ASSERT(aModelDataPtr != NULL && *aModelDataPtr == NULL,
		"Bad input received in GeometryGamesModel -lockModelData");

	[itsModelDataLock lock];
	
	*aModelDataPtr = itsModelDataPROTECTED;
}

- (void)unlockModelData:(ModelData **)aModelDataPtr
{
	GEOMETRY_GAMES_ASSERT(aModelDataPtr != NULL && *aModelDataPtr != NULL,
		"Bad input received in GeometryGamesModel -unlockModelData");

	[itsModelDataLock unlock];
	
	*aModelDataPtr = NULL;
}


@end
