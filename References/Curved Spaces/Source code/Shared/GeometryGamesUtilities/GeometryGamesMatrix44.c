//	GeometryGamesMatrix44.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesMatrix44.h"


void Matrix44Identity(double m[4][4])
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			m[i][j] = (i == j ? 1.0 : 0.0);
}

void Matrix44Copy(
	          double	dst[4][4],	//	output destination matrix
	/*const*/ double	src[4][4])	//	input  source matrix
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			dst[i][j] = src[i][j];
}

void Matrix44Product(
	/*const*/ double	m1[4][4],		//	input first  factor
	/*const*/ double	m2[4][4],		//	input second factor
	          double	product[4][4])	//	output product, which may coincide with either or both inputs
{
	unsigned int	i,
					j,
					k;
	double			temp[4][4];

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			temp[i][j] =  0.0;
			for (k = 0; k < 4; k++)
				temp[i][j] += m1[i][k] * m2[k][j];
		}
	}

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			product[i][j] = temp[i][j];
}

void Matrix44GeometricInverse(
	/*const*/ double	m[4][4],		//	input
	          double	mInverse[4][4])	//	output (may coincide with input)
{
	//	Invert a matrix in O(4), Isom(E³) or O(3,1).
	//	Work geometrically for better precision than
	//	row-reduction methods would provide.

	double			theInverse[4][4];
	unsigned int	i,
					j;

	//	Spherical case O(4)
	if (m[3][3] < 1.0)
	{
		//	The inverse is the transpose.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse[i][j] = m[j][i];
	}
	else
	//	Flat case Isom(E³)
	//	(Also works for elements of O(4) and O(3,1) that fix the origin.)
	if (m[3][3] == 1.0)
	{
		//	The upper-left 3×3 block is the transpose of the original.
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				theInverse[i][j] = m[j][i];

		//	The right-most column is mostly zeros.
		for (i = 0; i < 3; i++)
			theInverse[i][3] = 0.0;

		//	The bottom row is the negative of a matrix product.
		for (i = 0; i < 3; i++)
		{
			theInverse[3][i] = 0.0;
			for (j = 0; j < 3; j++)
				theInverse[3][i] -= m[3][j] * m[i][j];
		}

		//	The bottom right entry is 1.
		theInverse[3][i] = 1.0;
	}
	else
	//	Hyperbolic case O(3,1)
	if (m[3][3] > 1.0)
	{
		//	The inverse is the transpose,
		//	but with a few minus signs thrown in.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse[i][j] = ((i == 3) == (j == 3)) ? m[j][i] : -m[j][i];
	}
	else
	//	Can never occur.
	{
		Matrix44Identity(theInverse);
	}

	//	Copy the result to the output variable.
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			mInverse[i][j] = theInverse[i][j];
}

void Matrix44DoubleToFloat(
	          float		dst[4][4],	//	output destination matrix
	/*const*/ double	src[4][4])	//	input  source matrix
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			dst[i][j] = (float) src[i][j];
}

void Matrix44RowVectorTimesMatrix(
	  const   double	v[4],		//	input
	/*const*/ double	m[4][4],	//	input
	          double	vm[4])		//	output, OK to pass same array as v
{
	unsigned int	i,
					j;
	double			temp[4];
	
	for (i = 0; i < 4; i++)
	{
		temp[i] = 0.0;
		for (j = 0; j < 4; j++)
			temp[i] += v[j] * m[j][i];
	}

	for (i = 0; i < 4; i++)
		vm[i] = temp[i];
}

void Matrix44TimesColumnVector(
	/*const*/ double	m[4][4],	//	input
	  const   double	v[4],		//	input
	          double	mv[4])		//	output, OK to pass same array as v
{
	unsigned int	i,
					j;
	double			temp[4];
	
	for (i = 0; i < 4; i++)
	{
		temp[i] = 0.0;
		for (j = 0; j < 4; j++)
			temp[i] += m[i][j] * v[j];
	}

	for (i = 0; i < 4; i++)
		mv[i] = temp[i];
}


//	Functions with single-precision float arguments.

void Matrix44fIdentity(float m[4][4])
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			m[i][j] = (i == j ? 1.0 : 0.0);
}

void Matrix44fCopy(
	          float	dst[4][4],	//	output destination matrix
	/*const*/ float	src[4][4])	//	input  source matrix
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			dst[i][j] = src[i][j];
}

void Matrix44fProduct(
	/*const*/ float	m1[4][4],		//	input first  factor
	/*const*/ float	m2[4][4],		//	input second factor
	          float	product[4][4])	//	output product, which may coincide with either or both inputs
{
	unsigned int	i,
					j,
					k;
	double			temp[4][4];	//	double precision is good here

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			temp[i][j] =  0.0;
			for (k = 0; k < 4; k++)
				temp[i][j] += m1[i][k] * m2[k][j];
		}
	}

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			product[i][j] = temp[i][j];
}

void Matrix44fGeometricInverse(
	/*const*/ float	m[4][4],		//	input
	          float	mInverse[4][4])	//	output (may coincide with input)
{
	//	Invert a matrix in O(4), Isom(E³) or O(3,1).
	//	Work geometrically for better precision than
	//	row-reduction methods would provide.

	double			theInverse[4][4];	//	double precision is OK here
	unsigned int	i,
					j;

	//	Spherical case O(4)
	if (m[3][3] < 1.0)
	{
		//	The inverse is the transpose.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse[i][j] = m[j][i];
	}

	//	Flat case Isom(E³)
	//	(Also works for elements of O(4) and O(3,1) that fix the origin.)
	if (m[3][3] == 1.0)
	{
		//	The upper-left 3×3 block is the transpose of the original.
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				theInverse[i][j] = m[j][i];

		//	The right-most column is mostly zeros.
		for (i = 0; i < 3; i++)
			theInverse[i][3] = 0.0;

		//	The bottom row is the negative of a matrix product.
		for (i = 0; i < 3; i++)
		{
			theInverse[3][i] = 0.0;
			for (j = 0; j < 3; j++)
				theInverse[3][i] -= m[3][j] * m[i][j];
		}

		//	The bottom right entry is 1.
		theInverse[3][i] = 1.0;
	}

	//	Hyperbolic case O(3,1)
	if (m[3][3] > 1.0)
	{
		//	The inverse is the transpose,
		//	but with a few minus signs thrown in.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse[i][j] = ((i == 3) == (j == 3)) ? m[j][i] : -m[j][i];
	}

	//	Copy the result to the output variable.
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			mInverse[i][j] = theInverse[i][j];
}

void Matrix44fRowVectorTimesMatrix(
	  const   float	v[4],		//	input
	/*const*/ float	m[4][4],	//	input
	          float	vm[4])		//	output, OK to pass same array as v
{
	unsigned int	i,
					j;
	double			temp[4];	//	double precision is good here
	
	for (i = 0; i < 4; i++)
	{
		temp[i] = 0.0;
		for (j = 0; j < 4; j++)
			temp[i] += v[j] * m[j][i];
	}

	for (i = 0; i < 4; i++)
		vm[i] = temp[i];
}

void Matrix44fTimesColumnVector(
	/*const*/ float	m[4][4],	//	input
	  const   float	v[4],		//	input
	          float	mv[4])		//	output, OK to pass same array as v
{
	unsigned int	i,
					j;
	double			temp[4];	//	double precision is good here
	
	for (i = 0; i < 4; i++)
	{
		temp[i] = 0.0;
		for (j = 0; j < 4; j++)
			temp[i] += m[i][j] * v[j];
	}

	for (i = 0; i < 4; i++)
		mv[i] = temp[i];
}
