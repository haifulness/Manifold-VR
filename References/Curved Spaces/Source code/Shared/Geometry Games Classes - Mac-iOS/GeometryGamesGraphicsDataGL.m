//	GeometryGamesGraphicsDataGL.m
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#import "GeometryGamesGraphicsDataGL.h"
//#import "GeometryGamesUtilities-Common.h"


@implementation GeometryGamesGraphicsDataGL
{
	//	Mac OS
	//		The main thread and the CVDisplayLink thread both need 
	//		to access the GraphicsDataGL, so use an NSLock to prevent conflicts.
	//
	//	iOS
	//		The CADisplayLink runs on the main thread, so in principle 
	//		no lock is needed.  Nevertheless, it seems worth using
	//		locking on iOS anyhow, to keep the Mac and iOS code 
	//		as similar as possible.
	//
	//	Only the -lockGraphicsDataGL method may directly access
	//	the instance variable itsGraphicsDataPROTECTED.
	//	All other code should instead check out a pointer, use it, 
	//	and then check it back in again as soon as possible:
	//
	//		GraphicsDataGL	*gd = NULL;
	//
	//		[theGraphicsDataGL lockGraphicsDataGL:&gd];
	//		...
	//		use gd as desired
	//		...
	//		[theGraphicsDataGL unlockGraphicsDataGL:&gd];
	//
	GraphicsDataGL	*itsGraphicsDataPROTECTED;
	NSLock			*itsGraphicsDataLock;
}


- (id)init
{
	self = [super init];
	if (self != nil)
	{
		//	Let the app-specific code allocate the GraphicsDataGL.
		itsGraphicsDataPROTECTED = (GraphicsDataGL *)GET_MEMORY(SizeOfGraphicsDataGL());
		if (itsGraphicsDataPROTECTED == NULL)
		{
			self = nil;	//	might not be necessary to set self = nil
			return nil;
		}
		
		//	Initialize the GraphicsDataGL,
		//	without assuming that any OpenGL context is active.
		ZeroGraphicsDataGL(itsGraphicsDataPROTECTED);

		//	Create a lock to ensure that the main thread and 
		//	the CVDisplayLink never access the GraphicsDataGL simultaneously.
		itsGraphicsDataLock = [[NSLock alloc] init];
	}
	return self;
}

- (void)dealloc
{
	//	If we're getting dealloc'd, that should mean that
	//	no objects hold a pointer to us, and no objects
	//	hold itsGraphicsDataLock.
	
	//	Before letting GeometryGamesGraphicsDataGL get deallocated,
	//	the caller should have either manually released our OpenGL(ES) resources,
	//	or destroyed the OpenGL(ES) context entirely.
	
	FREE_MEMORY_SAFELY(itsGraphicsDataPROTECTED);
}


- (void)lockGraphicsDataGL:(GraphicsDataGL **)aGraphicsDataGLPtr
{
	GEOMETRY_GAMES_ASSERT(aGraphicsDataGLPtr != NULL && *aGraphicsDataGLPtr == NULL,
		"Bad input received in GeometryGamesGraphicsDataGL -lockGraphicsDataGL");

	[itsGraphicsDataLock lock];
	
	*aGraphicsDataGLPtr = itsGraphicsDataPROTECTED;
}

- (void)unlockGraphicsDataGL:(GraphicsDataGL **)aGraphicsDataGLPtr
{
	GEOMETRY_GAMES_ASSERT(aGraphicsDataGLPtr != NULL && *aGraphicsDataGLPtr != NULL,
		"Bad input received in GeometryGamesGraphicsDataGL -unlockGraphicsDataGL");

	[itsGraphicsDataLock unlock];
	
	*aGraphicsDataGLPtr = NULL;
}


@end

#endif	//	SUPPORT_OPENGL
