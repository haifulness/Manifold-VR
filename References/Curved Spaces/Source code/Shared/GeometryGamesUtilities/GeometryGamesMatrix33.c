//	GeometryGamesMatrix33.c
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesMatrix33.h"


void Matrix33Identity(double m[3][3])
{
	m[0][0] = 1.0;  m[0][1] = 0.0;  m[0][2] = 0.0;
	m[1][0] = 0.0;  m[1][1] = 1.0;  m[1][2] = 0.0;
	m[2][0] = 0.0;  m[2][1] = 0.0;  m[2][2] = 1.0;
}

double Matrix33Determinant(double m[3][3])
{
	return m[0][0] * m[1][1] * m[2][2]
		 + m[0][1] * m[1][2] * m[2][0]
		 + m[0][2] * m[1][0] * m[2][1]
		 - m[0][0] * m[1][2] * m[2][1]
		 - m[0][1] * m[1][0] * m[2][2]
		 - m[0][2] * m[1][1] * m[2][0];
}

void Matrix33CramersRule(
	//	Uses Cramer's Rule to solve 
	//	the matrix equation a x = b.
	double	a[3][3],	//	input
	double	x[3][3],	//	output
	double	b[3][3])	//	input
{
	double			theDeterminant,
					theModifiedMatrix[3][3],
					theModifiedMatrixDeterminant;
	unsigned int	i,
					j,
					k,
					l;
	
	//	Pre-compute the determinant.
	theDeterminant = Matrix33Determinant(a);

	//	The caller should have already tested for zero determinant.
	//	Nevertheless, just to be safe...
	if (theDeterminant == 0.0)	//	Could use an epsilon tolerance in other contexts.
	{
		//	This code should never get called,
		//	but just to be safe set the output to the identity matrix.
		Matrix33Identity(x);

		return;
	}

	//	Use Cramer's Rule to solve ax=b for x.
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			//	Make a copy of 'a'.
			for (k = 0; k < 3; k++)
				for (l = 0; l < 3; l++)
					theModifiedMatrix[k][l] = a[k][l];
		
			//	Replace the i-th column of theModifiedMatrix
			//	with j-th column of 'b'.
			for (k = 0; k < 3; k++)
				theModifiedMatrix[k][i] = b[k][j];
			
			//	Compute the determinant of theModifiedMatrix.
			theModifiedMatrixDeterminant = Matrix33Determinant(theModifiedMatrix);
			
			//	Apply Cramer's Rule.
			x[i][j] = theModifiedMatrixDeterminant / theDeterminant;
		}
	}
}


void Matrix33fIdentity(float m[3][3])
{
	m[0][0] = 1.0;  m[0][1] = 0.0;  m[0][2] = 0.0;
	m[1][0] = 0.0;  m[1][1] = 1.0;  m[1][2] = 0.0;
	m[2][0] = 0.0;  m[2][1] = 0.0;  m[2][2] = 1.0;
}

void Matrix33fCopy(
	float	dst[3][3],	//	output destination matrix
	float	src[3][3])	//	input  source matrix
{
	unsigned int	i,
					j;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			dst[i][j] = src[i][j];
}

void Matrix33fRowVectorTimesMatrix(
	float	v[3],		//	input
	float	m[3][3],	//	input
	float	vm[3])		//	output, OK to pass same array as v
{
	unsigned int	i,
					j;
	double			temp[3];	//	double precision is good here
	
	for (i = 0; i < 3; i++)
	{
		temp[i] = 0.0;
		for (j = 0; j < 3; j++)
			temp[i] += v[j] * m[j][i];
	}

	for (i = 0; i < 3; i++)
		vm[i] = temp[i];
}
