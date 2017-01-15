//	CurvedSpacesMouse.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include <math.h>


void MouseMoved(
	ModelData			*md,
	DisplayPoint		aMouseLocation,
	DisplayPointMotion	aMouseMotion,
	bool				aShiftKeyIsDown,
	bool				aCtrlKeyIsDown,
	bool				anAltKeyIsDown)
{
	double	theCharacteristicSize,
			theConversionFactor,	//	number of intrinsic units per point or pixel
			theXIU,
			theYIU,
			theDeltaXIU,
			theDeltaYIU,
			theRotationAngle[2];	//	(dα, dβ)
	Matrix	theIncrement;

	if (aMouseMotion.itsViewWidth <= 0.0
	 || aMouseMotion.itsViewHeight <= 0.0
	 || md->itsViewingDistanceIU <= 0.0)
	{
		return;
	}

	theCharacteristicSize	= CharacteristicViewSize(aMouseMotion.itsViewWidth, aMouseMotion.itsViewHeight);
	theConversionFactor		= md->itsCharacteristicSizeIU / theCharacteristicSize;
	theXIU					= theConversionFactor * (aMouseLocation.itsX  -  0.5 * aMouseMotion.itsViewWidth );
	theYIU					= theConversionFactor * (aMouseLocation.itsY  -  0.5 * aMouseMotion.itsViewHeight);
	theDeltaXIU				= theConversionFactor * aMouseMotion.itsDeltaX;
	theDeltaYIU				= theConversionFactor * aMouseMotion.itsDeltaY;

	//	The user typically uses the mouse to navigate.
	//	As an exceptional case, if the alt key is down
	//	mouse motion instead serves to move the centerpiece.
	if (anAltKeyIsDown)
	{
#ifdef CENTERPIECE_DISPLACEMENT
		//	Interpret the motion relative to the centerpiece's
		//	own local coordinate system.
		
		//	Fudge alert:
		//	Because aMouseMotion is measured along the plane of the display,
		//	while the centerpiece typically sits deeper into the scene than that,
		//	we must amplify the motion to get the centerpiece to move quickly enough.
		//	The exact factor is just a rough guess -- no effort is made
		//	to track the centerpiece exactly.
		theDeltaXIU *= 3.0;
		theDeltaYIU *= 3.0;
		
		if (aCtrlKeyIsDown)
		{
			MatrixTranslation(	&theIncrement,
								md->itsSpaceType,
								0.0,
								0.0,
								theDeltaYIU);
		}
		else
		{
			MatrixTranslation(	&theIncrement,
								md->itsSpaceType,
								theDeltaXIU,
								theDeltaYIU,
								0.0);
		}

		//	Pre-multiply by theIncrement.
		MatrixProduct(&theIncrement, &md->itsCenterpiecePlacement, &md->itsCenterpiecePlacement);
		
		//	Stay in the fundamental domain.
		if (md->itsDirichletDomain != NULL)
			StayInDirichletDomain(md->itsDirichletDomain, &md->itsCenterpiecePlacement);

		//	Keep numerical errors from accumulating, so we stay
		//	in Isom(S³) = O(4), Isom(E³) or Isom(H³) = O(3,1).
		FastGramSchmidt(&md->itsCenterpiecePlacement, md->itsSpaceType);
#endif
	}
	else
	{
#ifdef CURVED_SPACES_TOUCH_INTERFACE
		//	With the traditional mouse interface,
		//	users feel as if they are steering a spaceship,
		//	but with the touch interface they feel
		//	as if they are dragging the whole world.
		//	So in the former case, negative dx and dy.
		theDeltaXIU = -theDeltaXIU;
		theDeltaYIU = -theDeltaYIU;
#endif
		//	Allow full six-degrees-of-freedom navigation.
		if (aShiftKeyIsDown)
		{
			//	Translate
		
			//	Fudge alert:
			//	Because aMouseMotion is measured along the plane of the display,
			//	while the user typically focuses on more distant objects,
			//	let's amplify aMouseMotion to better match the user's expectations.
			theDeltaXIU *= 4.0;
			theDeltaYIU *= 4.0;

			if (aCtrlKeyIsDown)
			{
				MatrixTranslation(	&theIncrement,
									md->itsSpaceType,
									0.0,
									0.0,
									theDeltaYIU);
			}
			else
			{
				MatrixTranslation(	&theIncrement,
									md->itsSpaceType,
									theDeltaXIU,
									theDeltaYIU,
									0.0);
			}
		}
		else
		{
			//	Rotate
			
			//	Use similar triangles to get a first-order approximation
			//	to the small rotations about the x- and y-axes.
			theRotationAngle[0] = - theDeltaYIU * md->itsViewingDistanceIU / (theYIU * theYIU + md->itsViewingDistanceIU * md->itsViewingDistanceIU);
			theRotationAngle[1] =   theDeltaXIU * md->itsViewingDistanceIU / (theXIU * theXIU + md->itsViewingDistanceIU * md->itsViewingDistanceIU);
			
			if (aCtrlKeyIsDown)
			{
				MatrixRotation(	&theIncrement,
								0.0,
								0.0,
								- theRotationAngle[1]);
			}
			else
			{
				MatrixRotation(	&theIncrement,
								theRotationAngle[0],
								theRotationAngle[1],
								0.0);
			}
		}

		//	Pre-multiply by theIncrement.
		MatrixProduct(&theIncrement, &md->itsUserPlacement, &md->itsUserPlacement);

		//	Keep numerical errors from accumulating, so we stay
		//	in Isom(S³) = O(4), Isom(E³) or Isom(H³) = O(3,1).
		FastGramSchmidt(&md->itsUserPlacement, md->itsSpaceType);
	}
	
	//	Ask the idle-time routine to redraw the scene.
	md->itsRedrawRequestFlag = true;
}
