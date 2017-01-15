//	GeometryGamesGraphicsDataGL.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#ifdef SUPPORT_OPENGL

#import <Foundation/Foundation.h>
#import "GeometryGames-OpenGL.h"


@interface GeometryGamesGraphicsDataGL : NSObject

- (id)init;
- (void)dealloc;

- (void)lockGraphicsDataGL:(GraphicsDataGL **)aGraphicsDataGLPtr;
- (void)unlockGraphicsDataGL:(GraphicsDataGL **)aGraphicsDataGLPtr;

@end

#endif	//	SUPPORT_OPENGL
