//	CurvedSpacesSafeMath.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include <math.h>


double SafeAcos(double x)
{
	if (x <= -1.0)
		return PI;

	if (x >= +1.0)
		return 0.0;

	return acos(x);
}


double SafeAcosh(double x)
{
	if (x <= +1.0)
		return 0.0;

	return acosh(x);
}
