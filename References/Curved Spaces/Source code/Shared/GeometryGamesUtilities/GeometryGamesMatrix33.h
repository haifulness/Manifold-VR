//	GeometryGamesMatrix33.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

extern void		Matrix33Identity(double m[3][3]);
extern double	Matrix33Determinant(double m[3][3]);
extern void		Matrix33CramersRule(double a[3][3], double x[3][3], double b[3][3]);

extern void		Matrix33fIdentity(float m[3][3]);
extern void		Matrix33fCopy(float dst[3][3], float src[3][3]);
extern void		Matrix33fRowVectorTimesMatrix(float v[3], float m[3][3], float vm[3]);
