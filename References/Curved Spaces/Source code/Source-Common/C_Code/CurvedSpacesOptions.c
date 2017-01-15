//	CurvedSpacesOptions.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"


void SetCenterpiece(
	ModelData		*md,
	CenterpieceType	aCenterpieceChoice)
{
	md->itsCenterpiece			= aCenterpieceChoice;
	md->itsRedrawRequestFlag	= true;
}

void SetShowObserver(
	ModelData	*md,
	bool		aShowObserverChoice)
{
	md->itsShowObserver			= aShowObserverChoice;
	md->itsRedrawRequestFlag	= true;
}

void SetShowColorCoding(
	ModelData	*md,
	bool		aShowColorCodingChoice)
{
	md->itsShowColorCoding		= aShowColorCodingChoice;
	md->itsRedrawRequestFlag	= true;
}

void SetShowCliffordParallels(
	ModelData		*md,
	CliffordMode	aCliffordMode)
{
	md->itsCliffordMode			= aCliffordMode;
	md->itsRedrawRequestFlag	= true;
}

void SetShowVertexFigures(
	ModelData	*md,
	bool		aShowVertexFiguresChoice)
{
	md->itsShowVertexFigures	= aShowVertexFiguresChoice;
	md->itsRedrawRequestFlag	= true;
}

void SetFogFlag(
	ModelData	*md,
	bool		aFogFlag)
{
	md->itsFogFlag = aFogFlag;
}

void SetStereo3DMode(
	ModelData	*md,
	StereoMode	aStereo3DMode)
{
	md->itsStereoMode			= aStereo3DMode;
	md->itsRedrawRequestFlag	= true;
}

#ifdef HANTZSCHE_WENDT_AXES
void SetShowHantzscheWendtAxes(
	ModelData	*md,
	bool		aShowHantzscheWendtAxesChoice)
{
	md->itsShowHantzscheWendtAxes	= aShowHantzscheWendtAxesChoice;
	md->itsRedrawRequestFlag		= true;
}
#endif
