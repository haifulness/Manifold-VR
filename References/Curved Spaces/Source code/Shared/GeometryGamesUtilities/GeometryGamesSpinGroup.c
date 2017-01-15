//	GeometryGamesSpinGroup.c
//
//	Comments at top of GeometryGamesSpinGroup.h explain what's going on here.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesSpinGroup.h"
#include "GeometryGamesUtilities-Common.h"
#include <math.h>							//	for sqrt()

//	π
#define PI	3.14159265358979323846

#define TINY_VELOCITY	(1e-6)

#define INITIAL_EUCLIDEAN_TRANSLATION_1_SIGMA	0.5
#define INITIAL_HYPERBOLIC_1_SIGMA				0.5


static void		NormalizeIsometry(Geometry aGeometry, Isometry *anIsometry, bool aSmallCorrection);


void RandomIsometry(
	Geometry	aGeometry,
	Isometry	*anIsometry)
{
	double	x,
			y,
			z,
			w,
			theLengthSquared,
			theFactor,
			theAngle,
			theXYRadius,
			theZWRadius,
			theXYAngle,
			theZWAngle;

	switch (aGeometry)
	{
		case GeometrySpherical:

			//	Choose a point on the 3-sphere, with uniform distribution.
			//	The simplest way to do this is to choose points
			//	in the circumscribed hypercube, take the first one
			//	that happens to fall within the 3-ball, and
			//	project it onto the 3-sphere.
			
			do
			{
				x = -1.0  +  2.0 * RandomFloat();
				y = -1.0  +  2.0 * RandomFloat();
				z = -1.0  +  2.0 * RandomFloat();
				w = -1.0  +  2.0 * RandomFloat();
				
				theLengthSquared = x*x + y*y + z*z + w*w;

			} while (theLengthSquared > 1.0 || theLengthSquared < 0.01);
			
			theFactor = 1.0 / sqrt(theLengthSquared);

			anIsometry->a = x * theFactor;
			anIsometry->b = y * theFactor;
			anIsometry->c = z * theFactor;
			anIsometry->d = w * theFactor;
			
			break;
		
		case GeometryEuclidean:

			//	Choose (a,b) with uniform distribution on the unit circle.
			theAngle = 2.0 * PI * RandomFloat();
			anIsometry->a = cos(theAngle);
			anIsometry->b = sin(theAngle);
			
			//	Choose c and d with independent Gaussian distributions.
			//
			//	The factor of 0.5 corresponds to the factor of 2 
			//	in the formula ( 1  0  (ρ/2)sin(φ) -(ρ/2)cos(φ) )
			//	for a translation, so we may interpret
			//	INITIAL_EUCLIDEAN_TRANSLATION_1_SIGMA as a physical distance.
			//
			anIsometry->c = RandomGaussian(0.5 * INITIAL_EUCLIDEAN_TRANSLATION_1_SIGMA);
			anIsometry->d = RandomGaussian(0.5 * INITIAL_EUCLIDEAN_TRANSLATION_1_SIGMA);
		
			break;

		case GeometryHyperbolic:

			//	Choosing a point with a uniform distribution
			//	on an infinite-volume space is clearly out of the question,
			//	so let's use a distribution that
			//
			//		1.	is rotationally symmetric in the xy plane,
			//		2.	is rotationally symmetric in the zw plane, and
			//		3.	the action on H² doesn't translate the origin too far.
			//
			
			theZWRadius	= RandomGaussian(0.5 * INITIAL_HYPERBOLIC_1_SIGMA);
			theXYRadius	= sqrt(1.0 + theZWRadius*theZWRadius);
			theZWAngle	= 2.0 * PI * RandomFloat();
			theXYAngle	= 2.0 * PI * RandomFloat();
			
			anIsometry->a = theXYRadius * cos(theXYAngle);
			anIsometry->b = theXYRadius * sin(theXYAngle);
			anIsometry->c = theZWRadius * cos(theZWAngle);
			anIsometry->d = theZWRadius * sin(theZWAngle);
		
			break;

		default:	//	should never occur
			*anIsometry = (Isometry) IDENTITY_ISOMETRY;
			break;
	}
}

void RandomVelocity(
	double		aOneSigmaValue,
	Velocity	*aVelocity)
{
	aVelocity->dbdt = RandomGaussian(aOneSigmaValue);
	aVelocity->dcdt = RandomGaussian(aOneSigmaValue);
	aVelocity->dddt = RandomGaussian(aOneSigmaValue);
}

void RandomVelocityInRange(
	double		aMinValue,
	double		aMaxValue,
	Velocity	*aVelocity)
{
	aVelocity->dbdt = aMinValue  +  RandomFloat() * (aMaxValue - aMinValue);
	aVelocity->dcdt = aMinValue  +  RandomFloat() * (aMaxValue - aMinValue);
	aVelocity->dddt = aMinValue  +  RandomFloat() * (aMaxValue - aMinValue);
}

double RandomGaussian(
	double	aOneSigmaValue)
{
	double	u,
			v;

	//	Use the Box-Muller transform to generate
	//	a normally distributed random variable.

	u = RandomFloat();
	do
	{
		v = RandomFloat();
	} while (v < 0.0001);	//	avoid ln(0)
	
	return aOneSigmaValue * sqrt(-2.0 * log(v)) * cos(2.0*PI*u);
}


void IntegrateOverTime(
	Geometry	aGeometry,		//	input
	Velocity	*aVelocity,		//	input
	double		aTimeInterval,	//	input, in seconds
	Isometry	*anIsometry)	//	output
{
	double	v,
			n1,
			n2,
			n3,
			theHalfAngle,
			theCosine,
			theSine,
			theHalfDistance,
			v2,
			theCosh,
			theSinh;

	//	The following code could probably be simplified
	//	by first computing v² according to aGeometry,
	//	and then splitting into cases according to whether v²
	//	is positive, negative or zero.
	//	This would eliminate the nested cases and shorten the code.
	//	For now I'm leaving the code as it is because 
	//	its geometrical meaning is more transparent.

	switch (aGeometry)
	{
		case GeometrySpherical:
		
			//	As explained in SpinGroup.h, the element (a,b,c,d) =
			//
			//		(cos(θ/2), n₁ sin(θ/2), n₂ sin(θ/2), n₃ sin(θ/2)),
			//
			//	with n₁² + n₂² + n₃² = 1, is a rotation about 
			//	the axis (n₁, n₂, n₃) through an angle θ.
			//	The important thing here is not what the rotation looks like,
			//	but that every isometry of S² has that form.
			
			//	Write the velocity as a scalar times a unit vector,
			//
			//		(db/dt, dc/dt, dd/dt) = v (n₁, n₂, n₃)
			//
			v = sqrt( aVelocity->dbdt * aVelocity->dbdt
					+ aVelocity->dcdt * aVelocity->dcdt
					+ aVelocity->dddt * aVelocity->dddt );
			if (v > TINY_VELOCITY)
			{
				n1 = aVelocity->dbdt / v;
				n2 = aVelocity->dcdt / v;
				n3 = aVelocity->dddt / v;
				
				//	The flow
				//
				//		(cos(vt), n₁ sin(vt), n₂ sin(vt), n₃ sin(vt))
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		(   0,       n₁v,        n₂v,        n₃v    )
				//
				//	and moreover correctly integrates the given velocity.

				theHalfAngle	= v * aTimeInterval;
				theCosine		= cos(theHalfAngle);
				theSine			= sin(theHalfAngle);

				anIsometry->a = theCosine;
				anIsometry->b = n1 * theSine;
				anIsometry->c = n2 * theSine;
				anIsometry->d = n3 * theSine;
			}
			else
			{
				//	The velocity is zero,
				//	which integrates to the identity isometry.
				*anIsometry = (Isometry) IDENTITY_ISOMETRY;
			}

			break;
		
		case GeometryEuclidean:

			//	As explained in SpinGroup.h, the element (a,b,c,d) =
			//
			//		( cos(θ/2), sin(θ/2), n₂ sin(θ/2), n₃ sin(θ/2) ),
			//
			//	is a rotation about the point (1, n₂, n₃) 
			//	through an angle θ, while
			//
			//		( 1,  0,  (ρ/2)sin(φ), -(ρ/2)cos(φ) )
			//
			//	is a translation through a distance ρ 
			//	in the direction with azimuth φ.
			//
			//	For θ ~ 0 the former expression reduces to the latter.
			//	To see how, substitute n₂ = n sin(φ) and n₃ = - n cos(φ), to get
			//
			//		( cos(θ/2), sin(θ/2),  n sin(φ) sin(θ/2),  - n cos(φ) sin(θ/2)   )
			//	  = ( cos(θ/2), sin(θ/2), [n sin(θ/2)] sin(φ), - [n sin(θ/2)] cos(φ) )
			//
			//	If ρ = nθ stays finite as n gets large and θ gets small,
			//	then that last expression approaches
			//
			//		( cos(θ/2), 0, (ρ/2)sin(φ), -(ρ/2)cos(φ) )
			//
			//	The important thing here is not what the transformation looks like,
			//	but that every isometry of E² has that form.
			
			//	If db/dt is close to zero, model the flow as a translation.
			//	Otherwise model it as a rotation.
			v = fabs(aVelocity->dbdt);
			if (v > TINY_VELOCITY)
			{
				//	Model the flow as a rotation.

				//	If we set
				n1 = aVelocity->dbdt / v;	//	= ±1
				n2 = aVelocity->dcdt / v;
				n3 = aVelocity->dddt / v;

				//	then the flow
				//
				//		( cos(vt), n₁ sin(vt), n₂ sin(vt), n₃ sin(vt) ),
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		(   0,       n₁v,        n₂v,        n₃v    )
				//	  = (   0,      db/dt,      dc/dt,      dd/dt   )
				//
				//	and moreover correctly integrates the given velocity.

				theHalfAngle	= v * aTimeInterval;
				theCosine		= cos(theHalfAngle);
				theSine			= sin(theHalfAngle);

				anIsometry->a = theCosine;
				anIsometry->b = n1 * theSine;
				anIsometry->c = n2 * theSine;
				anIsometry->d = n3 * theSine;
			}
			else
			{
				//	Model the flow as a translation.

				//	The flow
				//
				//		( 1, 0, dc/dt t, dd/dt t ),
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		( 0,   0,   dc/dt, dd/dt )
				//	  = ( 0, db/dt, dc/dt, dd/dt )
				//
				//	and moreover correctly integrates the given velocity.

				anIsometry->a = 1.0;
				anIsometry->b = 0.0;
				anIsometry->c = aVelocity->dcdt * aTimeInterval;
				anIsometry->d = aVelocity->dddt * aTimeInterval;
			}

			break;
		
		case GeometryHyperbolic:
		
			v2 = aVelocity->dbdt * aVelocity->dbdt
			   - aVelocity->dcdt * aVelocity->dcdt
			   - aVelocity->dddt * aVelocity->dddt;

			//	Split into sub-cases, according to whether
			//
			//		(db/dt)² > (dc/dt)² + (dd/dt)²
			//		(db/dt)² = (dc/dt)² + (dd/dt)²
			//		(db/dt)² < (dc/dt)² + (dd/dt)²
			//
			if (v2 > TINY_VELOCITY*TINY_VELOCITY)
			{
				//	elliptic transformation (rotation)

				//	As explained in SpinGroup.h,
				//
				//		(cos(θ/2), n₁ sin(θ/2), n₂ sin(θ/2), n₃ sin(θ/2)),
				//
				//	with n₁² - n₂² - n₃² = 1, is a rotation about 
				//	the axis (n₁, n₂, n₃) through an angle θ.

				//	Write the velocity as a scalar times a unit vector,
				//
				//		(db/dt, dc/dt, dd/dt) = v (n₁, n₂, n₃)
				//
				v = sqrt(v2);
				n1 = aVelocity->dbdt / v;
				n2 = aVelocity->dcdt / v;
				n3 = aVelocity->dddt / v;
				
				//	The flow
				//
				//		(cos(vt), n₁ sin(vt), n₂ sin(vt), n₃ sin(vt))
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		(   0,       n₁v,        n₂v,        n₃v    )
				//
				//	and moreover correctly integrates the given velocity.

				theHalfAngle	= v * aTimeInterval;
				theCosine		= cos(theHalfAngle);
				theSine			= sin(theHalfAngle);

				anIsometry->a = theCosine;
				anIsometry->b = n1 * theSine;
				anIsometry->c = n2 * theSine;
				anIsometry->d = n3 * theSine;
			}
			else
			if (v2 < -TINY_VELOCITY*TINY_VELOCITY)
			{
				//	hyperbolic transformation (translation)
				
				//	As explained in SpinGroup.h, a generic translation looks like
				//
				//		( cosh(ρ/2), sinh(σ)sinh(ρ/2), cosh(σ)sinh(ρ/2)sin(φ), -cosh(σ)sinh(ρ/2)cos(φ) )
				//
				//	which we may think of as
				//
				//		( cosh(ρ/2), n₁ sinh(ρ/2), n₂ sinh(ρ/2), n₃ sinh(ρ/2)),
				//
				//	with  n₁² - n₂² - n₃² = -1.
				
				//	Write the velocity as a scalar times a unit vector,
				//
				//		(db/dt, dc/dt, dd/dt) = v (n₁, n₂, n₃)
				//
				v = sqrt(-v2);
				n1 = aVelocity->dbdt / v;
				n2 = aVelocity->dcdt / v;
				n3 = aVelocity->dddt / v;
				
				//	The flow
				//
				//		(cosh(vt), n₁ sinh(vt), n₂ sinh(vt), n₃ sinh(vt))
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		(   0,        n₁v,         n₂v,         n₃v     )
				//
				//	and moreover correctly integrates the given velocity.

				theHalfDistance	= v * aTimeInterval;
				theCosh			= cosh(theHalfDistance);
				theSinh			= sinh(theHalfDistance);

				anIsometry->a = theCosh;
				anIsometry->b = n1 * theSinh;
				anIsometry->c = n2 * theSinh;
				anIsometry->d = n3 * theSinh;
			}
			else
			{
				//	parabolic transformation (sliding along horocycles)

				//	As explained in SpinGroup.h, a generic parabolic motion looks like
				//
				//		(  1    c    c sin(φ)   -c cos(φ)  )
				//
				//	which we may think of as
				//
				//		( 1  n₁  n₂  n₃ ),
				//
				//	with  n₁² - n₂² - n₃² = 0.  Note that here,
				//	unlike in the previous cases, the (n₁, n₂, n₃) are
				//	defined only up to a scalar multiple.
				//	There's no way to normalize them.

				//	The flow
				//
				//		( 1, (db/dt) t, (dc/dt) t, (dd/dt) t )
				//
				//	has the desired derivative at time t = 0, namely
				//
				//		( 0, db/dt, dc/dt, dd/dt )
				//
				//	and moreover correctly integrates the given velocity.
				anIsometry->a = 1.0;
				anIsometry->b = aVelocity->dbdt * aTimeInterval;
				anIsometry->c = aVelocity->dcdt * aTimeInterval;
				anIsometry->d = aVelocity->dddt * aTimeInterval;
			}

			break;
		
		default:	//	should never occur
			*anIsometry = (Isometry) IDENTITY_ISOMETRY;
			break;
	}
}


void ComposeIsometries(
	Geometry	aGeometry,		//	input
	Isometry	*aFirstFactor,	//	input
	Isometry	*aSecondFactor,	//	input
	Isometry	*aProduct)		//	output, may coincide with either or both inputs
{
	double		theGeometryFactor = 0.0;	//	initialize to suppress compiler warnings
	Isometry	theProduct;

	//	As explained in SpinGroup.h, when computing a product
	//
	//			(  a  b  c  d ) (  a'  b'  c'  d' )
	//			( -b  a -d  c ) ( -b'  a' -d'  c' )
	//			( ∓c ±d  a -b ) ( ∓c' ±d'  a' -b' )
	//			( ∓d ∓c  b  a ) ( ∓d' ∓c'  b'  a' )
	//
	//	it suffices to compute only the top row of the result,
	//	because the rest of the product matrix may be deduced from it.
	//	The numerical formula for the top row of that product is
	//
	//			( aa' - bb' ∓ cc' ∓ dd',
	//			  ab' + ba' ± cd' ∓ dc',
	//			  ac' - bd' + ca' + db',
	//			  ad' + bc' - cb' + da' )

	switch (aGeometry)
	{
		case GeometrySpherical:   theGeometryFactor = +1.0;  break;
		case GeometryEuclidean:   theGeometryFactor =  0.0;  break;
		case GeometryHyperbolic:  theGeometryFactor = -1.0;  break;
	}
	
	//	Write the product to a local variable first, in case 
	//	the output variable (aProduct) coincides with one or both 
	//	of the input variables (aFirstFactor or aSecondFactor).

	theProduct.a	= aFirstFactor->a * aSecondFactor->a
					- aFirstFactor->b * aSecondFactor->b
					- aFirstFactor->c * aSecondFactor->c * theGeometryFactor
					- aFirstFactor->d * aSecondFactor->d * theGeometryFactor;

	theProduct.b	= aFirstFactor->a * aSecondFactor->b
					+ aFirstFactor->b * aSecondFactor->a
					+ aFirstFactor->c * aSecondFactor->d * theGeometryFactor
					- aFirstFactor->d * aSecondFactor->c * theGeometryFactor;

	theProduct.c	= aFirstFactor->a * aSecondFactor->c
					- aFirstFactor->b * aSecondFactor->d
					+ aFirstFactor->c * aSecondFactor->a
					+ aFirstFactor->d * aSecondFactor->b;

	theProduct.d	= aFirstFactor->a * aSecondFactor->d
					+ aFirstFactor->b * aSecondFactor->c
					- aFirstFactor->c * aSecondFactor->b
					+ aFirstFactor->d * aSecondFactor->a;

	NormalizeIsometry(aGeometry, &theProduct, true);

	*aProduct = theProduct;
}

void InterpolateIsometries(
	Geometry	aGeometry,		//	input
	Isometry	*anIsometryA,
	Isometry	*anIsometryB,
	double		t,							//	∈ [0,1]
	Isometry	*anInterpolatedIsometry)	//	(1-t)A + tB
{
	double	s;
	
	s = 1.0 - t;
	
	anInterpolatedIsometry->a = s * anIsometryA->a  +  t * anIsometryB->a;
	anInterpolatedIsometry->b = s * anIsometryA->b  +  t * anIsometryB->b;
	anInterpolatedIsometry->c = s * anIsometryA->c  +  t * anIsometryB->c;
	anInterpolatedIsometry->d = s * anIsometryA->d  +  t * anIsometryB->d;
	
	NormalizeIsometry(aGeometry, anInterpolatedIsometry, false);
}

static void NormalizeIsometry(
	Geometry	aGeometry,
	Isometry	*anIsometry,
	bool		aSmallCorrection)	//	Does the caller expect anIsometry
									//	to be almost unit length already?
{
	double	theGeometryFactor = 0.0,	//	initialize to suppress compiler warnings
			theLengthSquared,
			theFactor;

	//	Divide the vector (a,b,c,d) by its length.
	
	switch (aGeometry)
	{
		case GeometrySpherical:   theGeometryFactor = +1.0;  break;
		case GeometryEuclidean:   theGeometryFactor =  0.0;  break;
		case GeometryHyperbolic:  theGeometryFactor = -1.0;  break;
	}
	
	theLengthSquared = anIsometry->a * anIsometry->a
					 + anIsometry->b * anIsometry->b
					 + anIsometry->c * anIsometry->c * theGeometryFactor
					 + anIsometry->d * anIsometry->d * theGeometryFactor;
	
	if (aSmallCorrection)
	{
		//	Computational note:  In practice the length will be 
		//	very close to 1 already -- the typical error is
		//	similar to the machine precision -- so...
		if (theLengthSquared > 0.99 && theLengthSquared < 1.01)
		{
			//	for
			//		          theLengthSquared = 1 + ε       (1)
			//
			//	we may use the linear approximations
			//
			//		    sqrt(theLengthSquared) ≈ 1 + ε/2     (2)
			//	and
			//		1 / sqrt(theLengthSquared) ≈ 1 - ε/2     (3)
			//
			//	Solving (1) for ε = theLengthSquared - 1
			//	and substituting into (3) gives
			//
			//		1 / sqrt(theLengthSquared)
			//			≈ 1 - ε/2
			//			= 1 - (theLengthSquared - 1)/2
			//			= (3 - theLengthSquared)/2
			//

			theFactor = 1.5 - 0.5*theLengthSquared;

			anIsometry->a *= theFactor;
			anIsometry->b *= theFactor;
			anIsometry->c *= theFactor;
			anIsometry->d *= theFactor;
		}
		else
		{
			//	We should never arrive at this point.
			//	Flag the error only in DEBUG mode.
#ifdef DEBUG
			GEOMETRY_GAMES_ASSERT(0, "NormalizeIsometry() received anIsometry of length significantly different from 1.");
#endif
			anIsometry->a = 1.0;
			anIsometry->b = 0.0;
			anIsometry->c = 0.0;
			anIsometry->d = 0.0;
		}
	}
	else	//	The required correction may be large.
	{
		if (theLengthSquared > 1e-6)
		{
			theFactor = 1.0 / sqrt(theLengthSquared);

			anIsometry->a *= theFactor;
			anIsometry->b *= theFactor;
			anIsometry->c *= theFactor;
			anIsometry->d *= theFactor;
		}
		else	//	An interpolation from (a,b,c,d) to (-a,-b,-c,-d)
				//	may be passing through (0,0,0,0).
		{
			anIsometry->a = 1.0;
			anIsometry->b = 0.0;
			anIsometry->c = 0.0;
			anIsometry->d = 0.0;
		}
	}
}


void RealizeIsometryAs3x3Matrix(
	Geometry		aGeometry,		//	input
	const Isometry	*anIsometry,	//	input
	float			aMatrix[3][3])	//	output, last coordinate is trivial
{
	double	a,
			b,
			c,
			d,
			theGeometryFactor = 0.0;	//	initialize to suppress compiler warnings

	//	We're given anIsometry as an element (a,b,c,d) in the state space,
	//	and must convert it to a traditional 3×3 matrix.
	//	From the mathematical development, the most natural 3×3 matrix is
	//
	//		( a² + b² ∓ c² ∓ d²,   2( bc - ad),       2( db + ac)     )
	//		(   ±2(bc + ad),     a² - b² ± c² ∓ d²,   2(±cd - ab)     )
	//		(   ±2(db - ac),       2(±cd + ab),     a² - b² ∓ c² ± d² )
	//
	//	For consistency with the needs of the euclidean and hyperbolic cases,
	//	we must rotate the coordinates so that the basepoint moves 
	//	from (x,y,z) = (1,0,0) to (x,y,z) = (0,0,1), giving
	//
	//		( a² - b² ± c² ∓ d²,   2(±cd - ab),       ±2(bc + ad)     )
	//		(   2(±cd + ab),     a² - b² ∓ c² ± d²,   ±2(db - ac)     )
	//		(   2( bc - ad),       2( db + ac),     a² + b² ∓ c² ∓ d² )
	//
	
	//	For conciseness...
	a = anIsometry->a;
	b = anIsometry->b;
	c = anIsometry->c;
	d = anIsometry->d;
	
	switch (aGeometry)
	{
		case GeometrySpherical:   theGeometryFactor = +1.0;  break;
		case GeometryEuclidean:   theGeometryFactor =  0.0;  break;
		case GeometryHyperbolic:  theGeometryFactor = -1.0;  break;
	}
	
	aMatrix[0][0] = a*a - b*b + theGeometryFactor*(c*c - d*d);
	aMatrix[0][1] = 2.0 * (theGeometryFactor*c*d - a*b);
	aMatrix[0][2] = theGeometryFactor * 2.0 * (b*c + a*d);
	
	aMatrix[1][0] = 2.0 * (theGeometryFactor*c*d + a*b);
	aMatrix[1][1] = a*a - b*b + theGeometryFactor*(-c*c + d*d);
	aMatrix[1][2] = theGeometryFactor * 2.0 * (d*b - a*c);
	
	aMatrix[2][0] = 2.0 * (b*c - a*d);
	aMatrix[2][1] = 2.0 * (d*b + a*c);
	aMatrix[2][2] = a*a + b*b - theGeometryFactor*(c*c + d*d);
}

void RealizeIsometryAs4x4Matrix(
	Geometry		aGeometry,		//	input
	const Isometry	*anIsometry,	//	input
	double			aMatrix[4][4])	//	output, last coordinate is trivial
{
	double	a,
			b,
			c,
			d,
			theGeometryFactor = 0.0;	//	initialize to suppress compiler warnings

	//	We're given anIsometry as an element (a,b,c,d) in the state space,
	//	and must convert it to a traditional 3×3 matrix.
	//	From the mathematical development, the most natural 3×3 matrix is
	//
	//		( a² + b² ∓ c² ∓ d²,   2( bc - ad),       2( db + ac)     )
	//		(   ±2(bc + ad),     a² - b² ± c² ∓ d²,   2(±cd - ab)     )
	//		(   ±2(db - ac),       2(±cd + ab),     a² - b² ∓ c² ± d² )
	//
	//	For consistency with the needs of the euclidean and hyperbolic cases,
	//	we must rotate the coordinates so that the basepoint moves 
	//	from (x,y,z) = (1,0,0) to (x,y,z) = (0,0,1), giving
	//
	//		( a² - b² ± c² ∓ d²,   2(±cd - ab),       ±2(bc + ad)     )
	//		(   2(±cd + ab),     a² - b² ∓ c² ± d²,   ±2(db - ac)     )
	//		(   2( bc - ad),       2( db + ac),     a² + b² ∓ c² ∓ d² )
	//
	
	//	For conciseness...
	a = anIsometry->a;
	b = anIsometry->b;
	c = anIsometry->c;
	d = anIsometry->d;
	
	switch (aGeometry)
	{
		case GeometrySpherical:   theGeometryFactor = +1.0;  break;
		case GeometryEuclidean:   theGeometryFactor =  0.0;  break;
		case GeometryHyperbolic:  theGeometryFactor = -1.0;  break;
	}
	
	aMatrix[0][0] = a*a - b*b + theGeometryFactor*(c*c - d*d);
	aMatrix[0][1] = 2.0 * (theGeometryFactor*c*d - a*b);
	aMatrix[0][2] = theGeometryFactor * 2.0 * (b*c + a*d);
	aMatrix[0][3] = 0.0;
	
	aMatrix[1][0] = 2.0 * (theGeometryFactor*c*d + a*b);
	aMatrix[1][1] = a*a - b*b + theGeometryFactor*(-c*c + d*d);
	aMatrix[1][2] = theGeometryFactor * 2.0 * (d*b - a*c);
	aMatrix[1][3] = 0.0;
	
	aMatrix[2][0] = 2.0 * (b*c - a*d);
	aMatrix[2][1] = 2.0 * (d*b + a*c);
	aMatrix[2][2] = a*a + b*b - theGeometryFactor*(c*c + d*d);
	aMatrix[2][3] = 0.0;
	
	aMatrix[3][0] = 0.0;
	aMatrix[3][1] = 0.0;
	aMatrix[3][2] = 0.0;
	aMatrix[3][3] = 1.0;
}

void RealizeIsometryAs3x3MatrixInSO3(
	const Isometry	*anIsometry,	//	input
	float			aMatrix[3][3])	//	output
{
	double	a,
			b,
			c,
			d;
	
	//	Same as RealizeIsometryAsMatrix(), but with
	//		GeometrySpherical hard-coded,
	//		a matrix of floats instead of doubles,
	//		and "natural" coordinate conventions.
	//	
	//		( a² + b² - c² - d²,   2( bc - ad),       2( db + ac)     )
	//		(    2(bc + ad),     a² - b² + c² - d²,   2( cd - ab)     )
	//		(    2(db - ac),       2( cd + ab),     a² - b² - c² + d² )
	
	a = anIsometry->a;
	b = anIsometry->b;
	c = anIsometry->c;
	d = anIsometry->d;

	aMatrix[0][0] = a*a + b*b - c*c - d*d;
	aMatrix[0][1] = 2.0 * (b*c - a*d);
	aMatrix[0][2] = 2.0 * (d*b + a*c);
	
	aMatrix[1][0] = 2.0 * (b*c + a*d);
	aMatrix[1][1] = a*a - b*b + c*c - d*d;
	aMatrix[1][2] = 2.0 * (c*d - a*b);
	
	aMatrix[2][0] = 2.0 * (d*b - a*c);
	aMatrix[2][1] = 2.0 * (c*d + a*b);
	aMatrix[2][2] = a*a - b*b - c*c + d*d;
}

void RealizeIsometryAs4x4MatrixInSO3(
	const Isometry	*anIsometry,	//	input
	float			aMatrix[4][4])	//	output, last coordinate is trivial
{
	double	a,
			b,
			c,
			d;
	
	//	Same as RealizeIsometryAsMatrix(), but with
	//		GeometrySpherical hard-coded,
	//		a matrix of floats instead of doubles,
	//		and "natural" coordinate conventions.
	//	
	//		( a² + b² - c² - d²,   2( bc - ad),       2( db + ac)     )
	//		(    2(bc + ad),     a² - b² + c² - d²,   2( cd - ab)     )
	//		(    2(db - ac),       2( cd + ab),     a² - b² - c² + d² )
	
	a = anIsometry->a;
	b = anIsometry->b;
	c = anIsometry->c;
	d = anIsometry->d;

	aMatrix[0][0] = a*a + b*b - c*c - d*d;
	aMatrix[0][1] = 2.0 * (b*c - a*d);
	aMatrix[0][2] = 2.0 * (d*b + a*c);
	aMatrix[0][3] = 0.0;
	
	aMatrix[1][0] = 2.0 * (b*c + a*d);
	aMatrix[1][1] = a*a - b*b + c*c - d*d;
	aMatrix[1][2] = 2.0 * (c*d - a*b);
	aMatrix[1][3] = 0.0;
	
	aMatrix[2][0] = 2.0 * (d*b - a*c);
	aMatrix[2][1] = 2.0 * (c*d + a*b);
	aMatrix[2][2] = a*a - b*b - c*c + d*d;
	aMatrix[2][3] = 0.0;
	
	aMatrix[3][0] = 0.0;
	aMatrix[3][1] = 0.0;
	aMatrix[3][2] = 0.0;
	aMatrix[3][3] = 1.0;
}
