in mat4			atrModelViewMatrix;
uniform mat4	uniProjectionMatrix;

uniform float	uniFogFactor;			//	0.0 = off;  1.0 = on
#ifdef SPHERICAL_FOG
uniform float	uniFogParameterNear,	//	fog saturation at observer (or at distance  π when drawing back hemisphere)
				uniFogParameterFar;		//	fog saturation at antipode (or at distance 2π when drawing back hemisphere)
#endif
#ifdef EUCLIDEAN_FOG
uniform float	uniInverseSquareFogSaturationDistance;	//	1 / max_r²
#endif
#ifdef HYPERBOLIC_FOG
uniform float	uniInverseLogCoshFogSaturationDistance;	//	1 / log(cosh(max_r))
#endif

in vec4			atrPosition;
in vec2			atrTextureCoordinates;
in vec4			atrColor;				//	premultiplied alpha

out vec2		varTextureCoordinates;
out vec4		varColor;				//	premultiplied alpha


void main()
{
	vec4	tmpPositionEC;	//	vertex's position in eye coordinates
	float	tmpFraction,	//	0.0 = at observer;  1.0 = at observer's antipode
			tmpFogValue,	//	0.0 = bright;       1.0 = dark
			tmpFogCoef;		//	0.0 = dark;         1.0 = bright
	
	tmpPositionEC			= atrModelViewMatrix * atrPosition;
	gl_Position				= uniProjectionMatrix * tmpPositionEC;
	varTextureCoordinates	= atrTextureCoordinates;

#ifdef SPHERICAL_FOG
	//	Whether we use the true distance d or the coordinate w = cos(d)
	//	to compute the fog, the results come out nearly identical.
	//	So let's stick with w, to reduce the computational load.
	//
	//	Just for the record, here's the "true distance" version
	//	that we are *not* using:
	//
	//		#define PI_INVERSE	0.31830988618379067154
	//
	//		tmpFogValue	= acos(clamp(w, -1.0, 1.0)) * PI_INVERSE;
	//
	tmpFraction	= 0.5 - 0.5*tmpPositionEC[3];
	tmpFogValue	= uniFogParameterNear * (1.0 - tmpFraction)
				+ uniFogParameterFar  * (   tmpFraction   );
#endif
#ifdef EUCLIDEAN_FOG
	tmpFogValue = min(
					uniInverseSquareFogSaturationDistance
						* dot(vec3(tmpPositionEC), vec3(tmpPositionEC)),
					1.0);
#endif
#ifdef HYPERBOLIC_FOG
	//	Let the fog be proportional to
	//
	//		log(w) = log(cosh(d)) ≈ log(exp(d)/2) = d - log(2)
	//
	tmpFogValue	= uniInverseLogCoshFogSaturationDistance * log(tmpPositionEC[3]);
#endif

	tmpFogCoef	= 1.0 - uniFogFactor*tmpFogValue;
	varColor	= vec4(tmpFogCoef, tmpFogCoef, tmpFogCoef, 1.0) * atrColor;
}
