//	GeometryGamesModel.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#import <Foundation/Foundation.h>
#import "GeometryGames-Common.h"


@interface GeometryGamesModel : NSObject

- (id)init;
- (void)dealloc;

- (void)lockModelData:(ModelData **)aModelDataPtr;
- (void)unlockModelData:(ModelData **)aModelDataPtr;

@end
