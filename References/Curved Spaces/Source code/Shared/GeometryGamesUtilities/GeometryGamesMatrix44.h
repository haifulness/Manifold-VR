//	GeometryGamesMatrix44.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

extern void	Matrix44Identity(double m[4][4]);
extern void	Matrix44Copy(double dst[4][4], /*const*/ double src[4][4]);
extern void	Matrix44Product(/*const*/ double m1[4][4], /*const*/ double m2[4][4], double product[4][4]);
extern void	Matrix44GeometricInverse(/*const*/ double m[4][4], double mInverse[4][4]);
extern void	Matrix44DoubleToFloat(float dst[4][4], /*const*/ double src[4][4]);
extern void	Matrix44RowVectorTimesMatrix(const double v[4], /*const*/ double m[4][4], double vm[4]);
extern void	Matrix44TimesColumnVector(/*const*/ double m[4][4], const double v[4], double mv[4]);

extern void	Matrix44fIdentity(float m[4][4]);
extern void	Matrix44fCopy(float dst[4][4], /*const*/ float src[4][4]);
extern void	Matrix44fProduct(/*const*/ float m1[4][4], /*const*/ float m2[4][4], float product[4][4]);
extern void	Matrix44fGeometricInverse(/*const*/ float m[4][4], float mInverse[4][4]);
extern void	Matrix44fRowVectorTimesMatrix(const float v[4], /*const*/ float m[4][4], float vm[4]);
extern void	Matrix44fTimesColumnVector(/*const*/ float m[4][4], const float v[4], float mv[4]);
