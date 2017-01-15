//	CurvedSpacesView.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"


double CharacteristicViewSize(
	double	aViewWidth,		//	typically in pixels or points
	double	aViewHeight)	//	typically in pixels or points
{
	//	As explained in the comment accompanying the definition 
	//	of itsCharacteristicSizeIU in CurvedSpaces-Common.h,
	//	the view has a characteristic size which corresponds
	//	to itsCharacteristicSizeIU intrinsic units.
	//	Here we compute that same characteristic size in pixels or points.
	//
	//	This is the *only* place that specifies the dependence 
	//	of the characteristic size on the view dimensions.
	//	If you want to change the definition, this is 
	//	the only place you need to do it.
	
	//	I prefer assigning a 90° field of view
	//	to the width or the height, whichever is bigger.
	//	This limits the field of view to 90°, which
	//	limits the perspective distortion caused by the fact
	//	that the user typically views the scene from a point
	//	much further back than the perspectively correct viewpoint.
	//	(If we rendered the scene from the user's true vantage point,
	//	the field of view would be far too narrow.)
	return (aViewWidth >= aViewHeight ? aViewWidth : aViewHeight);
}
