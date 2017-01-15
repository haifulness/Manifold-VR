//	CurvedSpacesMatrices.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "CurvedSpaces-Common.h"
#include "GeometryGamesUtilities-Common.h"
#include <math.h>


static void	RawMatrixSum(double a[4][4], double b[4][4], double sum[4][4]);
static void	ConstantTimesRawMatrix(double c, double m[4][4], double cm[4][4]);


void MatrixIdentity(Matrix *aMatrix)
{
	unsigned int	i,
					j;

	if (aMatrix != NULL)
	{
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				aMatrix->m[i][j] = (i == j) ? 1.0 : 0.0;

		aMatrix->itsParity = ImagePositive;
	}
}


bool MatrixIsIdentity(Matrix *aMatrix)
{
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (aMatrix->m[i][j] != ((i == j) ? 1.0 : 0.0))
				return false;

	return true;
}


void MatrixAntipodalMap(Matrix *aMatrix)
{
	unsigned int	i,
					j;

	if (aMatrix != NULL)
	{
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				aMatrix->m[i][j] = (i == j) ? -1.0 : 0.0;

		aMatrix->itsParity = ImagePositive;
	}
}


void MatrixTranslation(
	Matrix			*aMatrix,	//	output
	SpaceType		aSpaceType,	//	input
	double			dx,			//	input
	double			dy,			//	input
	double			dz)			//	input
{
	double	theLength,
			theFactor;
	Matrix	m,
			m2;
	double	c1,
			c2;

	//	Normalize (dx, dy, dz) to unit length.
	theLength = sqrt(dx*dx + dy*dy + dz*dz);
	if (theLength == 0.0)
	{
		MatrixIdentity(aMatrix);
		return;
	}
	theFactor = 1.0 / theLength;
	dx *= theFactor;
	dy *= theFactor;
	dz *= theFactor;

	//	Initialize the geometry-independent entries of the velocity matrix.
	//	The geometry-dependent right-hand column will get set below.
	m.m[0][0] = 0.0;	m.m[0][1] = 0.0;	m.m[0][2] = 0.0;	m.m[0][3] = 0.0;
	m.m[1][0] = 0.0;	m.m[1][1] = 0.0;	m.m[1][2] = 0.0;	m.m[1][3] = 0.0;
	m.m[2][0] = 0.0;	m.m[2][1] = 0.0;	m.m[2][2] = 0.0;	m.m[2][3] = 0.0;
	m.m[3][0] =  dx;	m.m[3][1] =  dy;	m.m[3][2] =  dz;	m.m[3][3] = 0.0;

	switch (aSpaceType)
	{
		case SpaceSpherical:
			//	In the spherical case, the small motion (dx, dy, dz) defines a matrix
			//
			//			(  0   0   0 -dx )
			//		m = (  0   0   0 -dy )
			//			(  0   0   0 -dz )
			//		    ( dx  dy  dz   0 )
			//
			//	Miraculously, m³ = -(dx² + dy² + dz²)m,
			//	so the nominal infinite series for
			//
			//		exp(m) = 1 + m + m²/(2!) + m³/(3!) + ...
			//
			//	compresses to a quadratic polynomial in m,
			//
			//		exp(m) = 1 + (...)m + (...)m²
			//
			//	Letting L = sqrt(dx² + dy² + dz²),
			//
			//		exp(t m) = 1 + t m + t²m²/(2!) + t³m³/(3!) + ...
			//
			//				 = 1 + (t - t³L²/(3!) + ...) m + (t²/(2!) - ...)m²
			//
			//				 = 1  +  sin(t L) (m/L)  +  (1 - cos(t L)) (m²/L²)
			//
			//	In the present case t = 1.

			m.m[0][3] = -dx;
			m.m[1][3] = -dy;
			m.m[2][3] = -dz;

			c1 = sin(theLength);
			c2 = 1.0 - cos(theLength);

			break;

		case SpaceFlat:
			//	In the flat case, the small motion (dx, dy, dz) defines a matrix
			//
			//			(  0   0   0   0 )
			//		m = (  0   0   0   0 )
			//			(  0   0   0   0 )
			//		    ( dx  dy  dz   0 )
			//
			//	Trivially m² = 0, so the nominal infinite series for
			//
			//		exp(m) = 1 + m + m²/(2!) + m³/(3!) + ...
			//
			//	compresses to the simple sum
			//
			//		exp(m) = 1 + m
			//	or
			//		exp(t m) = 1 + t L (m/L)

			m.m[0][3] = 0.0;
			m.m[1][3] = 0.0;
			m.m[2][3] = 0.0;

			c1 = theLength;
			c2 = 0.0;

			break;

		case SpaceHyperbolic:
			//	In the hyperbolic case, the small motion (dx, dy, dz) defines a matrix
			//
			//			(  0   0   0  dx )
			//		m = (  0   0   0  dy )
			//			(  0   0   0  dz )
			//		    ( dx  dy  dz   0 )
			//
			//	Miraculously, m³ = +(dx² + dy² + dz²)m,
			//	so the nominal infinite series for
			//
			//		exp(m) = 1 + m + m²/(2!) + m³/(3!) + ...
			//
			//	compresses to a quadratic polynomial in m,
			//
			//		exp(m) = 1 + (...)m + (...)m²
			//
			//	Letting L = sqrt(dx² + dy² + dz²),
			//
			//		exp(t m) = 1 + t m + t²m²/(2!) + t³m³/(3!) + ...
			//
			//				 = 1 + (t + t³L²/(3!) + ...) m + (t²/(2!) + ...)m²
			//
			//				 = 1  +  sinh(t L) (m/L)  +  (cosh(t L) - 1) (m²/L²)
			//
			//	In the present case t = 1.

			m.m[0][3] = +dx;
			m.m[1][3] = +dy;
			m.m[2][3] = +dz;

			c1 = sinh(theLength);
			c2 = cosh(theLength) - 1.0;

			break;

		default:	//	should never occur
			MatrixIdentity(aMatrix);
			return;
	}

	//	In all three cases, the final translation matrix will
	//	be 1 + c1 m + c2 m².  The parity is always ImagePositive.

	MatrixProduct(&m, &m, &m2);
	ConstantTimesRawMatrix(c1,  m.m,  m.m);
	ConstantTimesRawMatrix(c2, m2.m, m2.m);
	MatrixIdentity(aMatrix);	//	This call sets aMatrix->itsParity = ImagePositive.
	RawMatrixSum(aMatrix->m,  m.m, aMatrix->m);
	RawMatrixSum(aMatrix->m, m2.m, aMatrix->m);
}


void MatrixRotation(
	Matrix			*aMatrix,	//	output
	double			da,			//	input = dα
	double			db,			//	input = dβ
	double			dc)			//	input = dγ
{
	double	theLength,
			theFactor;
	Matrix	m,
			m2;
	double	c1,
			c2;

	//	The small rotation (dα, dβ, dγ) defines a matrix
	//
	//			(  0  dγ -dβ   0 )
	//		m = (-dγ   0  dα   0 )
	//			( dβ -dα   0   0 )
	//		    (  0   0   0   0 )
	//
	//	Miraculously, m³ = -(dα² + dβ² + dγ²)m,
	//	so the nominal infinite series for
	//
	//		exp(m) = 1 + m + m²/(2!) + m³/(3!) + ...
	//
	//	compresses to a quadratic polynomial in m,
	//
	//		exp(m) = 1 + (...)m + (...)m²
	//
	//	Letting L = sqrt(dα² + dβ² + dγ²),
	//
	//		exp(t m) = 1 + t m + t²m²/(2!) + t³m³/(3!) + ...
	//
	//				 = 1 + (t - t³L²/(3!) + ...) m + (t²/(2!) - ...)m²
	//
	//				 = 1  +  sin(t L) (m/L)  +  (1 - cos(t L)) (m²/L²)
	//
	//	In the present case t = 1.

	//	Normalize (dα, dβ, dγ) to unit length.
	theLength = sqrt(da*da + db*db + dc*dc);
	if (theLength == 0.0)
	{
		MatrixIdentity(aMatrix);
		return;
	}
	theFactor = 1.0 / theLength;
	da *= theFactor;
	db *= theFactor;
	dc *= theFactor;

	//	The derivative matrix
	m.m[0][0] = 0.0;	m.m[0][1] =  dc;	m.m[0][2] = -db;	m.m[0][3] = 0.0;
	m.m[1][0] = -dc;	m.m[1][1] = 0.0;	m.m[1][2] =  da;	m.m[1][3] = 0.0;
	m.m[2][0] =  db;	m.m[2][1] = -da;	m.m[2][2] = 0.0;	m.m[2][3] = 0.0;
	m.m[3][0] = 0.0;	m.m[3][1] = 0.0;	m.m[3][2] = 0.0;	m.m[3][3] = 0.0;

	//	The coefficients
	c1 = sin(theLength);
	c2 = 1.0 - cos(theLength);

	//	Compute 1 + c1 m + c2 m².  The parity is always ImagePositive.
	MatrixProduct(&m, &m, &m2);
	ConstantTimesRawMatrix(c1,  m.m,  m.m);
	ConstantTimesRawMatrix(c2, m2.m, m2.m);
	MatrixIdentity(aMatrix);	//	This call sets aMatrix->itsParity = ImagePositive.
	RawMatrixSum(aMatrix->m,  m.m, aMatrix->m);
	RawMatrixSum(aMatrix->m, m2.m, aMatrix->m);
}


void MatrixGeometricInverse(
	Matrix	*aMatrix,	//	input
	Matrix	*anInverse)	//	output (may coincide with input)
{
	//	Invert a matrix in O(4), Isom(E³) or O(3,1).
	//	Work geometrically for better precision than
	//	row-reduction methods would provide.

	Matrix			theInverse;
	unsigned int	i,
					j;

	//	Spherical case O(4)
	if (aMatrix->m[3][3] <  1.0)
	{
		//	The inverse is the transpose.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse.m[i][j] = aMatrix->m[j][i];
	}

	//	Flat case Isom(E³)
	//	(Would also work for elements of O(4) and O(3,1) that fix the origin,
	//	even though Curved Spaces allows no such elements.)
	if (aMatrix->m[3][3] == 1.0)
	{
		//	The upper-left 3×3 block is the transpose of the original.
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				theInverse.m[i][j] = aMatrix->m[j][i];

		//	The right-most column is mostly zeros.
		for (i = 0; i < 3; i++)
			theInverse.m[i][3] = 0.0;

		//	The bottom row is the negative of a matrix product.
		for (i = 0; i < 3; i++)
		{
			theInverse.m[3][i] = 0.0;
			for (j = 0; j < 3; j++)
				theInverse.m[3][i] -= aMatrix->m[3][j] * aMatrix->m[i][j];
		}

		//	The bottom right entry is 1.
		theInverse.m[3][i] = 1.0;
	}

	//	Hyperbolic case O(3,1)
	if (aMatrix->m[3][3] >  1.0)
	{
		//	The inverse is the transpose,
		//	but with a few minus signs thrown in.
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				theInverse.m[i][j] = ((i == 3) == (j == 3)) ?
										 aMatrix->m[j][i] :
										-aMatrix->m[j][i];
	}

	//	Set the parity.
	theInverse.itsParity = aMatrix->itsParity;

	//	Copy the result to the output variable.
	*anInverse = theInverse;
}


double MatrixDeterminant(Matrix *aMatrix)
{
	//	Here's a quick algorithm to compute the determinant via permutations.
	//	The determinant should always be +1 or -1, so precision isn't an issue.

	double			theDeterminant,
					theProduct;
	unsigned int	a,
					b,
					c,
					d,
					temp,
					theParity,
					i,
					j,
					k;

	//	Initialize theDeterminant to zero.
	theDeterminant = 0.0;

	//	Set up the identity permutation.
	a = 0; b = 1; c = 2; d = 3;
	theParity = 0;

	//	Cycle through all other permutations, keeping track of the parity.
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 2; k++)
			{
				theProduct = aMatrix->m[0][a]
						   * aMatrix->m[1][b]
						   * aMatrix->m[2][c]
						   * aMatrix->m[3][d];

				if (theParity == 0)
					theDeterminant += theProduct;
				else
					theDeterminant -= theProduct;

				//	swap the first two positions
				temp = b; b = a; a = temp;
				theParity = ! theParity;
			}

			//	cycle the first three positions
			temp = c; c = b; b = a; a = temp;
			//	parity doesn't change
		}

		//	cycle the whole permutation
		temp = d; d = c; c = b; b = a; a = temp;
		theParity = ! theParity;
	}

	return theDeterminant;
}


void VectorTernaryCrossProduct(
	Vector	*aFactorA,	//	input
	Vector	*aFactorB,	//	input
	Vector	*aFactorC,	//	input
	Vector	*aProduct)	//	output, which may coincide with an input
{
	//	Compute the determinant
	//
	//		|	I	J	K	L	|
	//		|	a0	a1	a2	a3	|
	//		|	b0	b1	b2	b3	|
	//		|	c0	c1	c2	c3	|
	//
	//	and interpret the result as a vector
	//	d0 I + d1 J + d2 K + d3 L.

	Vector	theProduct;

	//	Imitate the algorithm from MatrixDeterminant().
	//	I still haven't thought about how good the numerical precision is,
	//	but if I eventually switch to exact arithmetic the precision will be perfect!

	double			theTerm;
	unsigned int	a,
					b,
					c,
					d,
					temp,
					theParity,
					i,
					j,
					k;

	//	Initialize theProduct to zero.
	for (i = 0; i < 4; i++)
		theProduct.v[i] = 0.0;

	//	Set up the identity permutation.
	a = 0; b = 1; c = 2; d = 3;
	theParity = 0;

	//	Cycle through all other permutations, keeping track of the parity.
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 2; k++)
			{
				theTerm = aFactorA->v[b]
						* aFactorB->v[c]
						* aFactorC->v[d];

				if (theParity == 0)
					theProduct.v[a] += theTerm;
				else
					theProduct.v[a] -= theTerm;

				//	swap the first two positions
				temp = b; b = a; a = temp;
				theParity = ! theParity;
			}

			//	cycle the first three positions
			temp = c; c = b; b = a; a = temp;
			//	parity doesn't change
		}

		//	cycle the whole permutation
		temp = d; d = c; c = b; b = a; a = temp;
		theParity = ! theParity;
	}

	//	Copy the result.
	for (i = 0; i < 4; i++)
		aProduct->v[i] = theProduct.v[i];
}


bool MatrixEquality(
	Matrix	*aMatrixA,
	Matrix	*aMatrixB,
	double	anEpsilon)
{
	unsigned int	i,
					j;

	if (aMatrixA->itsParity != aMatrixB->itsParity)
		return false;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (fabs(aMatrixA->m[i][j] - aMatrixB->m[i][j]) > anEpsilon)
				return false;

	return true;
}


void MatrixProduct(
	const Matrix	*aMatrixA,
	const Matrix	*aMatrixB,
	      Matrix	*aProduct)	//	output may coincide with one or both inputs
{
	Matrix			theProduct;
	unsigned int	i,
					j,
					k;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			theProduct.m[i][j] = 0.0;

			for (k = 0; k < 4; k++)
				theProduct.m[i][j] += aMatrixA->m[i][k] * aMatrixB->m[k][j];
		}

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			aProduct->m[i][j] = theProduct.m[i][j];

	aProduct->itsParity =
		(aMatrixA->itsParity == aMatrixB->itsParity) ?
		ImagePositive : ImageNegative;
}


static void RawMatrixSum(
	double	a[4][4],
	double	b[4][4],
	double	sum[4][4])	//	output may coincide with one or both inputs
{
	//	Warning:  This function does not determine ImageParity.  Use with caution!

	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			sum[i][j] = a[i][j] + b[i][j];
}


static void ConstantTimesRawMatrix(
	double	c,			//	input
	double	m[4][4],	//	input
	double	cm[4][4])	//	output, which may coincide with input
{
	//	Warning:  This function does not set aResult->itsParity.  Use with caution!

	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			cm[i][j] = c * m[i][j];
}


void VectorNegate(
	Vector	*aVector,		//	input
	Vector	*aNegation)		//	output, which may coincide with input
{
	unsigned int	i;

	for (i = 0; i < 4; i++)
		aNegation->v[i] = - aVector->v[i];
}


void VectorSum(
	Vector	*aVectorA,	//	input
	Vector	*aVectorB,	//	input
	Vector	*aSum)		//	output, which may coincide with an input
{
	unsigned int	i;

	for (i = 0; i < 4; i++)
		aSum->v[i] = aVectorA->v[i] + aVectorB->v[i];
}


void VectorDifference(
	Vector	*aVectorA,		//	input
	Vector	*aVectorB,		//	input
	Vector	*aDifference)	//	output, which may coincide with an input
{
	unsigned int	i;

	for (i = 0; i < 4; i++)
		aDifference->v[i] = aVectorA->v[i] - aVectorB->v[i];
}


void VectorInterpolate(
	Vector	*aVectorA,	//	input
	Vector	*aVectorB,	//	input
	double	t,			//	input, typically 0 ≤ t ≤ 1, but any value is OK
	Vector	*aResult)	//	output, which may coincide with an input
{
	double			s;
	unsigned int	i;

	s = 1.0 - t;

	for (i = 0; i < 4; i++)
		aResult->v[i] = s * aVectorA->v[i]
					  + t * aVectorB->v[i];
}


double VectorDotProduct(
	Vector	*aVectorA,	//	input
	Vector	*aVectorB)	//	input
{
	double			theResult;
	unsigned int	i;

	theResult = 0.0;

	for (i = 0; i < 4; i++)
		theResult += aVectorA->v[i] * aVectorB->v[i];

	return theResult;
}


ErrorText VectorNormalize(
	Vector		*aRawVector,		//	input
	SpaceType	aSpaceType,			//	input
	Vector		*aNormalizedVector)	//	output, which may coincide with aRawVector
{
	double			theLength,
					theFactor;
	unsigned int	i;

	switch (aSpaceType)
	{
		case SpaceSpherical:
			theLength = sqrt(aRawVector->v[0] * aRawVector->v[0]
						   + aRawVector->v[1] * aRawVector->v[1]
						   + aRawVector->v[2] * aRawVector->v[2]
						   + aRawVector->v[3] * aRawVector->v[3]);
			break;

		case SpaceFlat:
			//	If greater control is needed over the metric used
			//	for flat space normalization, reorganized this function
			//	to use MetricHorizontal or MetricVertical as in KaleidoTile.
			theLength = aRawVector->v[3];
			break;

		case SpaceHyperbolic:
			theLength = sqrt( - aRawVector->v[0] * aRawVector->v[0]
							  - aRawVector->v[1] * aRawVector->v[1]
							  - aRawVector->v[2] * aRawVector->v[2]
							  + aRawVector->v[3] * aRawVector->v[3]);
			break;

		default:
			return u"Bad space type passed to VectorNormalize().";
	}

	if (theLength <= 0.0)	//	includes case of points at infinity
	{
		for (i = 0; i < 4; i++)
			aNormalizedVector->v[i] = aRawVector->v[i];
		return u"Bad raw vector passed to VectorNormalize().";
	}

	theFactor = 1.0 / theLength;

	for (i = 0; i < 4; i++)
		aNormalizedVector->v[i] = theFactor * aRawVector->v[i];

	return NULL;
}


double VectorGeometricDistance(
	Vector	*aVector)	//	assumed to be normalized to the SpaceType and not at infinity
{
	if (aVector->v[3] <  1.0)	//	spherical
		return SafeAcos(aVector->v[3]);	//	correct for front hemisphere
	else
	if (aVector->v[3] == 1.0)	//	flat
		return sqrt(aVector->v[0] * aVector->v[0]
				  + aVector->v[1] * aVector->v[1]
				  + aVector->v[2] * aVector->v[2]);
	else
	if (aVector->v[3] >  1.0)	//	hyperbolic
		return SafeAcosh(aVector->v[3]);
	else
		return 0.0;	//	should never occur
}


double VectorGeometricDistance2(
	Vector	*aVectorA,	//	assumed to be normalized to the SpaceType and not at infinity
	Vector	*aVectorB)	//	assumed to be normalized to the SpaceType and not at infinity
{
	Vector	theDifference;

	if (aVectorA->v[3] == 1.0 && aVectorB->v[3] == 1.0)	//	SpaceFlat
	{
		VectorDifference(aVectorA, aVectorB, &theDifference);
		return sqrt(theDifference.v[0] * theDifference.v[0]
				  + theDifference.v[1] * theDifference.v[1]
				  + theDifference.v[2] * theDifference.v[2]);
	}
	else
	if (aVectorA->v[3] <= 1.0 && aVectorB->v[3] <= 1.0)	//	SpaceSpherical
	{
		return SafeAcos(VectorDotProduct(aVectorA, aVectorB));
	}
	else
	if (aVectorA->v[3] >= 1.0 && aVectorB->v[3] >= 1.0)	//	SpaceHyperbolic
	{
		return SafeAcosh(VectorDotProduct(aVectorA, aVectorB));
	}
	else
	{
		//	Do *not* call ErrorMessage() here.
		//	The Win32 UI messes up the stack if we try to put up
		//	a MessageBox while our OpenGL context is active.
		return 0.0;
	}
}


void VectorTimesMatrix(
	Vector	*aVector,	//	input
	Matrix	*aMatrix,	//	input
	Vector	*aProduct)	//	output, which may coincide with aVector
{
	Vector			theProduct;
	unsigned int	i,
					j;

	for (i = 0; i < 4; i++)
	{
		theProduct.v[i] = 0.0;

		for (j = 0; j < 4; j++)
			theProduct.v[i] += aVector->v[j] * aMatrix->m[j][i];
	}

	for (i = 0; i < 4; i++)
		aProduct->v[i] = theProduct.v[i];
}


void ScalarTimesVector(
	double	aScalar,	//	input
	Vector	*aVector,	//	input
	Vector	*aProduct)	//	output, which may coincide with aVector
{
	unsigned int	i;

	for (i = 0; i < 4; i++)
		aProduct->v[i] = aScalar * aVector->v[i];
}


MatrixList *AllocateMatrixList(unsigned int aNumMatrices)
{
	MatrixList		*theMatrixList;
	unsigned int	theNumArrayBytes;

	if (aNumMatrices > 0xFFFFFFFF / sizeof(Matrix))	//	for safety
		return NULL;

	theMatrixList = (MatrixList *) GET_MEMORY(sizeof(MatrixList));
	if (theMatrixList == NULL)
		return NULL;
	
	theNumArrayBytes = aNumMatrices * sizeof(Matrix);
	if (theNumArrayBytes == 0)	//	Allocating 0 bytes should be fine, but why take chances?
		theNumArrayBytes = 1;

	theMatrixList->itsMatrices = (Matrix *) GET_MEMORY(theNumArrayBytes);
	if (theMatrixList->itsMatrices == NULL)
	{
		FREE_MEMORY_SAFELY(theMatrixList);
		return NULL;
	}

	theMatrixList->itsNumMatrices = aNumMatrices;

	return theMatrixList;
}


void FreeMatrixList(MatrixList **aMatrixList)
{
	if (aMatrixList != NULL
	 && *aMatrixList != NULL)
	{
		FREE_MEMORY_SAFELY((*aMatrixList)->itsMatrices);
		FREE_MEMORY_SAFELY(*aMatrixList);
	}
}
