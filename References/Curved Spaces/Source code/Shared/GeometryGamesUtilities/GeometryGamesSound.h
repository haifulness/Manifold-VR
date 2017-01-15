//	GeometryGamesSound.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGames-Common.h"	//	for Char16


//	Enable sound effects?
extern bool	gPlaySounds;


extern void	SetUpAudio(void);
extern void	ShutDownAudio(void);

extern void	PlayTheSound(const Char16 *aSoundFileName);

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
extern void	ClearSoundCaches(void);	//	iOS only
#endif
