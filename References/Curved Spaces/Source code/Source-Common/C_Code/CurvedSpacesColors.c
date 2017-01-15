//	CurvedSpacesColors.c
//
//	HSLAtoRGBA()	converts hue-saturation-lightness-opacity
//					to red-green-blue-opacity.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"


void HSLAtoRGBA(
	HSLAColor	*anHSLAColor,	//	input
	RGBAColor	*anRGBAColor)	//	output
{
	double	h,	//	hue
			s,	//	saturation
			l,	//	lightness
			r,	//	red
			g,	//	green
			b,	//	blue
			a,	//	alpha (opaqueness)
			t,
			c;

	//	For easy readability.
	h = anHSLAColor->h;
	s = anHSLAColor->s;
	l = anHSLAColor->l;
	a = anHSLAColor->a;

	//	To understanding what's going on here, make yourself a sketch
	//	of the unit cube in red-green-blue (RGB) space.

	//	First create a "pure color" of the given hue.
	//	The "pure colors" lie along the hexagon with vertices at
	//		(1,0,0)		(pure red)
	//		(1,1,0)		(pure yellow)
	//		(0,1,0)		(pure green)
	//		(0,1,1)		(pure cyan)
	//		(0,0,1)		(pure blue)
	//		(1,0,1)		(pure magenta)
	//	on the color cube.

	h *= 6.0;	//	for convenience in the six different cases

	if (h < 1.0)
	{
		r = 1.0;
		g = h;
		b = 0.0;
	}
	else
	if (h < 2.0)
	{
		r = 2.0 - h;
		g = 1.0;
		b = 0.0;
	}
	else
	if (h < 3.0)
	{
		r = 0.0;
		g = 1.0;
		b = h - 2.0;
	}
	else
	if (h < 4.0)
	{
		r = 0.0;
		g = 4.0 - h;
		b = 1.0;
	}
	else
	if (h < 5.0)
	{
		r = h - 4.0;
		g = 0.0;
		b = 1.0;
	}
	else
	{
		r = 1.0;
		g = 0.0;
		b = 6.0 - h;
	}

	//	To take into account the saturation, average the pure hue
	//	with a medium grey (0.5, 0.5, 0.5).
	r = s*r + (1.0 - s)*0.5;
	g = s*g + (1.0 - s)*0.5;
	b = s*b + (1.0 - s)*0.5;

	//	To take into account the lightness, average the color
	//	with pure white or pure black.
	if (l > 0.5)
	{
		t = 2.0 * (1.0 - l);
		c = 1.0;
	}
	else
	{
		t = 2.0 * (l - 0.0);
		c = 0.0;
	}
	r = t*r + (1.0 - t)*c;
	g = t*g + (1.0 - t)*c;
	b = t*b + (1.0 - t)*c;
	
	//	ColorRGBA requires premultiplied alpha.
	r *= a;
	g *= a;
	b *= a;

	//	Package up the result.
	anRGBAColor->r = r;
	anRGBAColor->g = g;
	anRGBAColor->b = b;
	anRGBAColor->a = a;
}
