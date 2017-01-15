//	GeometryGamesSpinGroup.h
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt


//	Overview
//
//	The isometry groups Isom(S²) ≈ SO(3), Isom(E²) and Isom(H²) ≈ SO(1,2)
//	are inherently 3-dimensional.  In each case, the group's double cover
//	sits naturally in R⁴.  In the spherical case the double cover
//	is called Spin(3).  In the euclidean and hyperbolic cases I don't know
//	what it's called.  In all cases it provides a wonderfully simple
//	way to keep track of motions of the sphere, the euclidean plane,
//	and the hyperbolic plane.  In particular, the computations are faster
//	than using 3×3 matrices from SO(3), Isom(E²) or SO(1,2), and
//	the numerical errors accumulate far more slowly (see "Achilles' heel" article).
//
//	The file "Spin3 geometrical intuition.txt" explains
//	the purely geometrical intuition behind Clifford parallels,
//	Clifford transformations, and the group Spin(3) ≈ SU(2).
//	While the geometrical intuition is vitally important,
//	in this .h file I'm going to cut to the chase
//	and start with coordinates.
//
//
//	Coordinates
//
//	Complex coordinates (α, β) ∈ ℂ² and real coordinates (a, b, c, d) ∈ ℝ⁴
//	often work equally well, and are equivalent via α = a + bi and β = c + di.
//	However, the complex language will be simpler when we project (α, β) ↦ β:α ,
//	and the real language will be simpler when specify an isometry
//	as a translation or rotation about a given axis.
//	For writing the computer code, the real coordinates are simpler.
//	For the most part I'll write things in complex variables here because
//	they're more succinct, but will occasionally include real equivalents as well.
//
//
//	Surfaces
//
//	The double covers of Isom(S²), Isom(E²) and Isom(H²) are, respectively,
//	the 3-sphere, the "hypercylinder" and the "hyperhyperboloid"
//
//		spherical  case:  { (α,β) ∈ ℂ² | |α|² + |β|² = 1 }
//		euclidean  case:  { (α,β) ∈ ℂ² | |α|²        = 1 }
//		hyperbolic case:  { (α,β) ∈ ℂ² | |α|² - |β|² = 1 }
//
//		unified:          { (α,β) ∈ ℂ² | |α|² ± |β|² = 1 }
//
//	or equivalently, in real terms,
//
//		spherical  case:  { (a,b,c,d) ∈ ℝ⁴ | a² + b² + c² + d² = 1 }
//		euclidean  case:  { (a,b,c,d) ∈ ℝ⁴ | a² + b²           = 1 }
//		hyperbolic case:  { (a,b,c,d) ∈ ℝ⁴ | a² + b² - c² - d² = 1 }
//
//		unified:          { (a,b,c,d) ∈ ℝ⁴ | a² + b² ± c² ± d² = 1 }
//
//	Here and henceforth symbols ‘±’ and ‘∓’ include the euclidean-space value of 0 as well.
//
//
//	Points as points
//
//	The projection (α, β) ↦ β:α takes each point of ℂ² to a point of ℂP¹ ≈ ℂ ∪ {∞}.
//	We write the image point as a ratio β:α rather than a quotient β/α,
//	so (0, 1) ↦ 1:0 creates no problems.  Note:  We put β (not α) in the numerator, 
//	because (α,0) is the natural origin in the flat and hyperbolic cases.
//
//	But what does one do with a point β:α in ℂ ∪ {∞} ?
//	In all three geometries we identify ℂ with the plane x = 0 in Euclidean xyz-space
//	(don't worry about the point ∞ = 1:0 for now) and project stereographically 
//	from the "south pole" (x,y,z) = (-1,0,0) onto
//
//		(spherical  case)  the unit sphere       x² + y² + z² = 1
//		(euclidean  case)  the plane             x²           = 1   (x > 0)
//		(hyperbolic case)  the unit hyperboloid  x² - y² - z² = 1   (x > 0)
//
//		(unified)								 x² ± y² ± z² = 1
//
//	with results as follows.  For reasons that will become clear later,
//	it's convenient to put the south pole at (-1,0,0) and the north pole
//	at (+1,0,0), rather than at (0,0,-1) and (0,0,+1) as you might expect.
//
//		Spherical case
//
//			For a given point β/α, we need to choose an "expansion factor" c 
//			(from the south pole) that will carry the point β/α 
//			from the equatorial plane onto the surface of the sphere:
//
//				(c - 1)² + c²|β/α|² = 1
//		
//				        2
//				c = ----------
//				    1 + |β/α|²
//
//			This gives final coordinates on the unit sphere
//
//				          2            -2 im(β/α)    2 re(β/α) 
//				    ( ---------- - 1 , -----------, ----------- )
//				      1 + |β/α|²        1 + |β/α|²   1 + |β/α|²
//
//				      1 - |β/α|²   i(β/α - β̅/α̅)    β/α + β̅/α̅ 
//				  = ( ----------, --------------, ----------- )
//				      1 + |β/α|²    1 + |β/α|²     1 + |β/α|²
//
//				       αα̅ - ββ̅    i(βα̅ - β̅α)    βα̅ + β̅α 
//				  = ( ---------, ------------, --------- )
//				       αα̅ + ββ̅      αα̅ + ββ̅     αα̅ + ββ̅ 
//
//				  = ( αα̅ - ββ̅, i(βα̅ - β̅α), βα̅ + β̅α )
//
//			One couldn't ask for a simpler formula than that!
//			Best of all, the division has disappeared.
//			This is a very good thing, because while computers
//			do additions and multiplications at lightning speed,
//			floating-point divisions typically take an order of magnitude longer.
//
//			Note that the special case (α,β) = (0,1) works out fine.
//
//		Flat case
//
//			The complex plane ℂ is already a Euclidean plane,
//			so in a naive sense, we're done.  However, the fact that 
//			we need to do floating-point divisions to compute
//
//				( re(β/α), im(β/α) )
//
//			is a little unsettling, and suggests that there might be
//			more to this story...
//
//			The desired computation is
//
//				  ( 1, -2 im(β/α), 2 re(β/α) )
//
//				= ( 1, i(β/α - β̅/α̅), β/α + β̅/α̅ )
//
//				        i(βα̅ - β̅α)    βα̅ + β̅α 
//				= ( 1, ------------, --------- )
//				            αα̅          αα̅    
//
//				= ( 1, i(βα̅ - β̅α), βα̅ + β̅α )
//
//			The geometrical justification for the initial factor of 2 is that, 
//			in strict analogy with the spherical and hyperbolic cases, if we place 
//			the complex plane ℂ at x = 0 and project it onto a parallel plane at x = 1,
//			projecting from the south pole (x,y,z) = (-1,0,0),
//			we expand the plane by a factor of 2.
//
//			Note too that ( 1, i(βα̅ - β̅α), βα̅ + β̅α ) = ( αα̅, i(βα̅ - β̅α), βα̅ + β̅α )
//			agrees with the spherical formula except for the missing ββ̅
//			in the first coordinate.
//
//			Again, we're happy to see that the division has disappeared.
//
//		Hyperbolic case
//
//			Because |α|² - |β|² = 1, we know |β/α| < 1.
//			More precisely, β/α represents a point in the Poincaré disk model
//			of the hyperbolic plane.  So, if that's what we want, we may write
//
//				( re(β/α), im(β/α) )
//
//			and we're done.  However, that solution has two flaws.
//			First, it requires a floating-point division that will
//			turn out to be unnecessary.  Second, in the Geometry Games software,
//			what looks like the Poincaré disk model in monoscopic 3D
//			should really look like a sheet of a hyperboloid
//			extending away from the viewer in stereoscopic 3D.
//
//			So place the Poincaré disk in the "equatorial plane" of a unit hyperboloid,
//			and project it from the hyperboloid's "south pole" onto
//			the "northern sheet" of the hyperboloid.
//			As in the spherical and flat cases, put the south pole at (-1,0,0) not (0,0,-1).
//			For a given point β/α, we need to choose an "expansion factor" c 
//			(from the south pole) that will carry the point β/α 
//			from the equatorial plane onto the "northern sheet" of the hyperboloid:
//
//				(c - 1)² - c²|β/α|² = 1
//		
//				        2
//				c = ----------
//				    1 - |β/α|²
//
//			This gives final coordinates on the unit hyperboloid
//
//				          2           -2 im(β/α)    2 re(β/α) 
//				    ( ---------- - 1, -----------, ----------- ) 
//				      1 - |β/α|²      1 - |β/α|²   1 - |β/α|² 
//
//				      1 + |β/α|²   i(β/α - β̅/α̅)    β/α + β̅/α̅ 
//				  = ( ----------, --------------, ----------- ) 
//				      1 - |β/α|²    1 - |β/α|²     1 - |β/α|²
//
//				       αα̅ + ββ̅   i(βα̅ - β̅α)   βα̅ + β̅α 
//				  = ( ---------, ----------, --------- ) 
//				       αα̅ - ββ̅    αα̅ - ββ̅     αα̅ - ββ̅ 
//
//				  = ( αα̅ + ββ̅, i(βα̅ - β̅α), βα̅ + β̅α ) 
//
//			Again the division drops out, and the final formula
//			agrees with the formulas for the spherical and flat cases,
//			except for the ββ̅ term in the first coordinate.
//
//			Strictly speaking, the division hasn't completely disappeared,
//			but has been absorbed into OpenGL final "perspective division".
//			But that perspective division is going to happen anyhow,
//			and is well supported by the GPU, so in effect it's "free".
//
//		Unified formula
//
//			In all three geometries, the mapping from (α, β)
//			to the sphere, Euclidean plane or hyperbolic plane
//			may be written as
//
//				( αα̅ ∓ ββ̅, i(βα̅ - β̅α), βα̅ + β̅α )
//
//	If we ever needed to evaluate the formula ( αα̅ ∓ ββ̅, i(βα̅ - β̅α), βα̅ + β̅α )
//	directly, it would be very efficient on a GPU, where
//	the (real-valued) expressions αα̅ ∓ ββ̅, i(βα̅ - β̅α) and βα̅ + β̅α would, 
//	I think, be evaluated in one cycle each.  For example, 
//	αα̅ + ββ̅ = dot4(vec4(α.re, α.im, β.re, β.im), vec4(α.re, α.im, β.re, β.im)).
//
//	Technical Note:  In the hyperbolic case, the slice b = 0 is 
//	a valid topological cross section, but geometrically it's distorted.
//	In particular, the projection (α, β) ↦ β:α = β/a acts like central projection
//	and therefore takes the cross section's would-be geodesics
//	to straight lines in the unit disk, not to arcs of circles
//	as an exact geometrical cross section would require.
//	This isn't too surprising:  the fact that in the spherical case
//	not even a local geometrically correct cross section is possible
//	(proof: the cross section must act like a sphere of radius 1/2, not 1)
//	suggests that it's unlikley to be possible in the hyperbolic case either.
//
//
//	Points as transformations
//
//	The file "Spin3 geometrical intuition.txt" explains the geometrical reason
//	why each point (α, β) corresponds to a transformation of the whole
//	hypersurface { (α,β) ∈ ℂ² | |α|² ± |β|² = 1 }.  (Well, it explains it
//	in the spherical case at least.)  Here we explain the mapping in algebraic terms.
//
//	Each point (α, β) ∈ ℂ² defines a matrix in SL(2,ℂ), as follows.
//
//		spherical  case:  (ζ η)	( α  β )
//								(-β̅  α̅ )
//							for all α and β satisfying |α|² + |β|² = 1
//
//		flat       case:  (ζ η)	( α  β )
//								( 0  α̅ )
//							for all α and β satisfying |α|²        = 1
//
//		hyperbolic case:  (ζ η)	( α  β )
//								( β̅  α̅ )
//							for all α and β satisfying |α|² - |β|² = 1
//
//		unified:		  (ζ η)	( α  β )
//								(∓β̅  α̅ )
//							for all α and β satisfying |α|² ± |β|² = 1
//
//	I personally prefer to let matrices act as
//
//		(row vector)(first transformation)(second transformation)
//
//	because I like to read the transformations left-to-right,
//	but if you prefer the opposite convention, namely
//
//		(second transformation)(first transformation)(column vector)
//
//	you should take the transpose of all matrices herein.
//	Note that in all cases, the matrices act as (row vector)(matrix),
//	not (matrix)(column vector).  
//
//	In the flat case, think of the matrix as acting on (1 η), not (ζ 1).
//
//	TO-DO:  HOW TO PROVE THAT THESE ARE ISOM(S²), ISOM(E²) AND ISOM(H²)?
//
//	The composition of two such motions gives a product of the same form:
//	when |α|² ± |β|² = 1,
//
//		( α  β ) ( γ  δ ) = (  αγ ∓ βδ̅    αδ + βγ̅ )
//		(∓β̅  α̅ ) (∓δ̅  γ̅ )   ( ∓β̅γ ∓ α̅δ̅   ∓β̅δ + α̅γ̅ )
//	
//	Offhand it seems we don't need to check the norms of the entries --
//	the fact that the transformation respects the metric should be enough
//	to guarantee that the norms come out right -- but just for the record
//	
//		(αγ ∓ βδ̅)(α̅γ̅ ∓ β̅δ) ± (αδ + βγ̅)(α̅δ̅ + β̅γ)
//		= (αγα̅γ̅ ∓ αγβ̅δ ∓ βδ̅α̅γ̅ +₀ βδ̅β̅δ) ± (αδα̅δ̅ + αδβ̅γ + βγ̅α̅δ̅ + βγ̅β̅γ)
//		= (αγα̅γ̅ +      +      +₀ βδ̅β̅δ) ± (αδα̅δ̅ +      +      + βγ̅β̅γ)
//		= ( |α|² ± |β|² )( |γ|² ± |δ|² )
//		= 1
//	
//	
//	Real coordinates
//
//	The 2×2 complex matrix
//
//			( α  β )
//			(∓β̅  α̅ )
//
//	with α = a + bi and β = c + di is equivalent to the 4×4 real matrix
//
//			(  a  b  c  d )
//			( -b  a -d  c )
//			( ∓c ±d  a -b )
//			( ∓d ∓c  b  a )
//
//	A computer program need record only the top row (a,b,c,d),
//	because the rest of the matrix may be deduced from it.
//	Similarly, when computing a product
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
//
//	In all three cases (spherical, euclidean, hyperbolic),
//	the inverse of an element (a b c d) is (a -b -c -d),
//	as can be checked by direct computation:
//
//			  ( aa + bb ± cc ± dd,
//			   -ab + ba ∓ cd ± dc,
//			   -ac + bd + ca - db,
//			   -ad - bc + cb + da )
//
//			= ( a² + b² ± c² ± d²  0  0  0 )
//			= ( 1  0  0  0 )
//
//
//	3×3 matrices
//
//	Consider a motion taking the surface (sphere, plane or hyperbolic plane)
//	from its identity position (1,0) ∈ ℂ² to some final position (α,β) ∈ ℂ².
//	The 2×2 complex matrix
//
//		( α  β )
//		(∓β̅  α̅ )
//
//	takes (1,0) to (α,β) in the state space, but what 3×3 real matrix
//	realizes this transformation on the surface itself?
//
//		Coordinate conventions
//
//			This is a little tricky.  On the one hand, in the spherical case
//			we'd like to end up with the intuitive natural convention that
//
//				(1 0 0 0)	is the identity
//				(0 1 0 0)	is a half turn about the X axis
//				(0 0 1 0)	is a half turn about the Y axis
//				(0 0 0 1)	is a half turn about the Z axis
//			and
//				(cosθ, X sinθ, Y sinθ, Z sinθ), with X² + Y² + Z² = 1, 
//							is a rotation about the axis (X,Y,Z) through an angle 2θ. 
//		
//			On the other hand, in the hyperbolic case, only the timelike axis
//			(not either of the two spacelike axes) admits a pure rotation.
//			And the pure rotation seems pretty well tied to the second coordinate,
//			i.e. (0 1 0 0) is the only obvious candidate for a half turn about the origin.
//			In other words, the "identity coordinate" (the first one) and
//			the "timelike coordinate" (the second one) both need to sit
//			in a '+' slot of the ++-- metric.
//
//			So... it seems like we need to make the x-axis the timelike one,
//			at least for now.  Afterwards the computer code can permute the variables
//			as it wishes.
//
//		Spherical case
//
//			Take three points on S², stereographically project them to ℂ ⊂ ℂP¹,
//			and lift each one to a Clifford parallel in the state space.  
//			Use the formula 
//			( ζζ̅ - ηη̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ ) to confirm that the sign choices
//			came out right.
//
//				(1,0,0) ↦ (  1,   0 )⋅exp(iθ₀)
//				(0,1,0) ↦ ( √½i, √½ )⋅exp(iθ₁)
//				(0,0,1) ↦ ( √½,  √½ )⋅exp(iθ₂)
//
//			Use the matrix
//
//				( α  β )
//				(-β̅  α̅ )
//
//			to transform the Clifford parallels in ℂ²,
//
//				( 1,   0 )⋅exp(iθ₀) ↦ (     α,          β    )⋅exp(iθ₀)
//				(√½i, √½ )⋅exp(iθ₁) ↦ (√½αi - √½β̅, √½βi + √½α̅)⋅exp(iθ₁)
//				(√½,  √½ )⋅exp(iθ₂) ↦ (√½α  - √½β̅, √½β  + √½α̅)⋅exp(iθ₂)
//
//			Then use the formula ( ζζ̅ - ηη̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ )
//			to project back down to S²,
//
//				(     α,          β    )⋅exp(iθ₀) ↦ (  αα̅ - ββ̅,    2im(αβ̅),     2re(αβ̅)   )
//				(√½αi - √½β̅, √½βi + √½α̅)⋅exp(iθ₁) ↦ (  2im(αβ),  re(α̅² + β²), im(α̅² + β²) )
//				(√½α  - √½β̅, √½β  + √½α̅)⋅exp(iθ₂) ↦ ( -2re(αβ), -im(α̅² - β²), re(α̅² - β²) )
//
//			Pulling it all together,
//
//				(1,0,0) ↦ (  αα̅ - ββ̅,   i(α̅β - αβ̅),    α̅β + αβ̅  )
//				(0,1,0) ↦ (  2im(αβ),  re(α̅² + β²), im(α̅² + β²) )
//				(0,0,1) ↦ ( -2re(αβ), -im(α̅² - β²), re(α̅² - β²) )
//
//			To put that in real terms, substitute α=a+ib and β=c+id to get
//
//				(1,0,0) ↦ ( a² + b² - c² - d²,    2(bc - ad),        2(db + ac)     )
//				(0,1,0) ↦ (    2(bc + ad),     a² - b² + c² - d²,    2(cd - ab)     )
//				(0,0,1) ↦ (    2(db - ac),        2(cd + ab),     a² - b² - c² + d² )
//
//			Sample values
//
//				(cos(θ/2), x sin(θ/2), y sin(θ/2), z sin(θ/2)), with x² + y² + z² = 1, 
//					is a rotation about the axis (x,y,z) through an angle θ.
//				Proof:  Compose appropriate rotations about the coordinate axes.
//
//				For comparison with the hyperbolic case, 
//				(cos(θ/2), x sin(θ/2), y sin(θ/2), z sin(θ/2)) may be written as
//				(cos(θ/2), sin(θ/2)cos(ρ), sin(θ/2)sin(ρ)cos(φ), sin(θ/2)sin(ρ)sin(φ)) 
//				which describes a rotation through angle θ about an axis that 
//				sits a distance ρ from the origin (1,0,0) in azimuthal direction φ.
//
//		Flat case
//
//			Take three points on E², stereographically project them to ℂ ⊂ ℂP¹,
//			and write each as a Clifford parallel.  Use the formula 
//			( ζζ̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ ) to confirm that the sign choices
//			came out right.
//
//				(1,0,0) ↦ ( 1, 0 )⋅exp(iθ₀)
//				(1,1,0) ↦ ( i, ½ )⋅exp(iθ₁)
//				(1,0,1) ↦ ( 1, ½ )⋅exp(iθ₂)
//
//			Use the matrix
//
//				( α  β )
//				( 0  α̅ )
//
//			to transform the Clifford parallels in ℂ²,
//
//				( 1, 0 )⋅exp(iθ₀) ↦ ( α,     β   )⋅exp(iθ₀)
//				( i, ½ )⋅exp(iθ₁) ↦ ( αi, βi + ½α̅)⋅exp(iθ₁)
//				( 1, ½ )⋅exp(iθ₂) ↦ ( α,  β  + ½α̅)⋅exp(iθ₂)
//
//			Then use the formula ( ζζ̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ )
//			to project back down to E²,
//
//				( α,     β   )⋅exp(iθ₀) ↦ ( αα̅, 2im(αβ̅),          2re(αβ̅)          )
//				( αi, βi + ½α̅)⋅exp(iθ₁) ↦ ( αα̅, 2im(αβ̅) + re(α̅²), 2re(αβ̅) + im(α̅²) )
//				( α,  β  + ½α̅)⋅exp(iθ₂) ↦ ( αα̅, 2im(αβ̅) - im(α̅²), 2re(αβ̅) + re(α̅²) )
//
//			Pulling it all together,
//
//				(1,0,0) ↦ ( αα̅, 2im(αβ̅),          2re(αβ̅)          )
//				(1,1,0) ↦ ( αα̅, 2im(αβ̅) + re(α̅²), 2re(αβ̅) + im(α̅²) )
//				(1,0,1) ↦ ( αα̅, 2im(αβ̅) - im(α̅²), 2re(αβ̅) + re(α̅²) )
//
//			Subtracting the first row from each of the second and third rows,
//
//				(1,0,0) ↦ ( αα̅, 2im(αβ̅), 2re(αβ̅) )
//				(0,1,0) ↦ ( 0,   re(α̅²),  im(α̅²) )
//				(0,0,1) ↦ ( 0,  -im(α̅²),  re(α̅²) )
//
//			To put that in real terms, substitute α=a+ib and β=c+id to get
//
//				(1,0,0) ↦ ( 1, 2(bc - ad), 2(db + ac) )
//				(0,1,0) ↦ ( 0,   a² - b²,     -2ab    )
//				(0,0,1) ↦ ( 0,     2ab,      a² - b²  )
//
//			Sample values
//
//				( cos(θ/2)  sin(θ/2)  0  0 ) gives a rotation 
//					about the origin (1,0,0) through an angle θ.
//
//					( 1   0     0   )
//					( 0  cosθ -sinθ )
//					( 0  sinθ  cosθ )
//
//				( 1  0  (ρ/2)sin(φ) -(ρ/2)cos(φ) ) gives a translation
//					through a distance ρ in the direction with azimuth φ.
//					The azimuth φ equals 0 along the y-axis and π/2 along the z-axis.
//
//					( 1  ρ cos(φ)  ρ sin(φ) )
//					( 0     1         0     )
//					( 0     0         1     )
//
//				( cos(θ/2), sin(θ/2), sin(θ/2) ρ cos(φ), sin(θ/2) ρ sin(φ) ) 
//					gives a rotation through angle θ about an axis that 
//					sits a distance ρ from the origin (1,0,0) in azimuthal direction φ,
//					measured as before.
//
//				Interpreting that last example geometrically,
//				( cos(θ/2), x sin(θ/2), y sin(θ/2), z sin(θ/2) ), 
//				with x = 1, is a rotation about the axis (1,y,z) 
//				through an angle θ.
//
//		Hyperbolic case
//
//			Take three points on H², stereographically project them to ℂ ⊂ ℂP¹,
//			and write each as a Clifford parallel.  Use the formula 
//			( ζζ̅ + ηη̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ ) to confirm that the sign choices
//			came out right.
//
//				( 1,   0,  0 ) ↦ (    1,       0   )⋅exp(iθ₀)
//				(5/4, 3/4, 0 ) ↦ ( 3/4 √2i, 1/4 √2 )⋅exp(iθ₁)
//				(5/4,  0, 3/4) ↦ ( 3/4 √2,  1/4 √2 )⋅exp(iθ₂)
//
//			Use the matrix
//
//				( α  β )
//				( β̅  α̅ )
//
//			to transform the Clifford parallels in ℂ²,
//
//				(    1,       0   )⋅exp(iθ₀) ↦ (         α,                β        )⋅exp(iθ₀)
//				( 3/4 √2i, 1/4 √2 )⋅exp(iθ₁) ↦ ( 3/4√2αi + 1/4√2β̅, 3/4√2βi + 1/4√2α̅ )⋅exp(iθ₁)
//				( 3/4 √2,  1/4 √2 )⋅exp(iθ₂) ↦ ( 3/4√2α  + 1/4√2β̅, 3/4√2β  + 1/4√2α̅ )⋅exp(iθ₂)
//
//			Then use the formula ( ζζ̅ + ηη̅, i(ηζ̅ - η̅ζ), ηζ̅ + η̅ζ )
//			to project back down to H²,
//
//				(         α,                β        )⋅exp(iθ₀) ↦ (     αα̅ + ββ̅,                2im(αβ̅),                    2re(αβ̅)                  )
//				( 3/4√2αi + 1/4√2β̅, 3/4√2βi + 1/4√2α̅ )⋅exp(iθ₁) ↦ ( 5/4(αα̅ + ββ̅) - 3/2im(αβ), 5/2im(αβ̅) + 3/4re(α̅² - β²), 5/2re(αβ̅) + 3/4im(α̅² - β²) )
//				( 3/4√2α  + 1/4√2β̅, 3/4√2β  + 1/4√2α̅ )⋅exp(iθ₂) ↦ ( 5/4(αα̅ + ββ̅) + 3/2re(αβ), 5/2im(αβ̅) - 3/4im(α̅² + β²), 5/2re(αβ̅) + 3/4re(α̅² + β²) )
//
//			Pulling it all together,
//
//				( 1,   0,  0 ) ↦ (     αα̅ + ββ̅,                2im(αβ̅),                    2re(αβ̅)                  )
//				(5/4, 3/4, 0 ) ↦ ( 5/4(αα̅ + ββ̅) - 3/2im(αβ), 5/2im(αβ̅) + 3/4re(α̅² - β²), 5/2re(αβ̅) + 3/4im(α̅² - β²) )
//				(5/4,  0, 3/4) ↦ ( 5/4(αα̅ + ββ̅) + 3/2re(αβ), 5/2im(αβ̅) - 3/4im(α̅² + β²), 5/2re(αβ̅) + 3/4re(α̅² + β²) )
//
//			For each of the second and third rows, subtract 5/4 times the first row,
//
//				( 1,   0,   0 ) ↦ (   αα̅ + ββ̅,     2im(αβ̅),        2re(αβ̅)      )
//				( 0,  3/4,  0 ) ↦ ( -3/2im(αβ),  3/4re(α̅² - β²), 3/4im(α̅² - β²) )
//				( 0,   0,  3/4) ↦ (  3/2re(αβ), -3/4im(α̅² + β²), 3/4re(α̅² + β²) )
//
//			and multiply through by 4/3,
//
//				( 1,   0,   0 ) ↦ (  αα̅ + ββ̅,    2im(αβ̅),     2re(αβ̅)   )
//				( 0,   1,   0 ) ↦ ( -2im(αβ),  re(α̅² - β²), im(α̅² - β²) )
//				( 0,   0,   1 ) ↦ (  2re(αβ), -im(α̅² + β²), re(α̅² + β²) )
//
//			To put that in real terms, substitute α=a+ib and β=c+id to get
//
//				(1,0,0) ↦ ( a² + b² + c² + d²,    2(bc - ad),        2(db + ac)     )
//				(0,1,0) ↦ (   -2(bc + ad),     a² - b² - c² + d²,   -2(cd + ab)     )
//				(0,0,1) ↦ (   -2(db - ac),       -2(cd - ab),     a² - b² + c² - d² )
//
//			Sample values
//
//				( cos(θ/2)  sin(θ/2)  0  0 ) gives a rotation 
//					about the origin (1,0,0) through an angle θ.
//
//					( 1   0     0   )
//					( 0  cosθ -sinθ )
//					( 0  sinθ  cosθ )
//
//				( cosh(ρ/2)  0  sinh(ρ/2)  0 ) gives a translation
//					through a distance ρ in the z direction
//
//					(  cosh(ρ)     0     sinh(ρ) )
//					(     0        1        0    )
//					(  sinh(ρ)     0     cosh(ρ) )
//
//				( cosh(ρ/2)  0  0  -sinh(ρ/2) ) gives a translation
//					through a distance ρ in the y direction
//
//					(  cosh(ρ)  sinh(ρ)     0    )
//					(  sinh(ρ)  cosh(ρ)     0    )
//					(     0        0        1    )
//
//				( cosh(ρ/2)  0  sinh(ρ/2)sin(φ)  -sinh(ρ/2)cos(φ) ) gives a translation
//					through a distance ρ in the direction with azimuth φ.
//					The azimuth φ equals 0 along the y-axis and π/2 along the z-axis.
//
//				( cosh(ρ/2)  sinh(σ)sinh(ρ/2)  cosh(σ)sinh(ρ/2)sin(φ)  -cosh(σ)sinh(ρ/2)cos(φ) ) 
//					gives a translation similar to the one in the preceding example,
//					except that now the geodesic (the centerline of the translation)
//					is offset laterally by a distance σ.  So σ=0 recovers the previous
//					example of a geodesic through the origin.
//				Interpreting that last example geometrically,
//					( cosh(ρ/2)  x sinh(ρ/2)  y sinh(ρ/2)  z sinh(ρ/2) ), 
//					with x² - y² - z² = -1, is a translation about the "axis" (x,y,z) 
//					through a distance ρ.
//	
//				(  1    c    c sin(φ)   -c cos(φ)  )
//					In the example
//						( cosh(ρ/2)  sinh(σ)sinh(ρ/2)  cosh(σ)sinh(ρ/2)sin(φ)  -cosh(σ)sinh(ρ/2)cos(φ) )
//					if σ gets large and ρ gets small in such a way that 
//					sinh(σ)sinh(ρ/2) approaches a constant c, the point approaches
//					the limit just shown, which gives a parabolic transformation.
//					This same expression comes from the limit of a rotation 
//					(see immediately below) as θ gets small and ρ gets large,
//					with sin(θ/2)cosh(ρ) approaching c.
//				Interpreting that last example geometrically,
//					( 1  x  y  z ), with x² - y² - z² = 0, is a parabolic transformation 
//					about the lightlike "axis" (x,y,z).
//			
//
//				( cos(θ/2), sin(θ/2)cosh(ρ), sin(θ/2)sinh(ρ)cos(φ), sin(θ/2)sinh(ρ)sin(φ) ) 
//					gives a rotation through angle θ about an axis that sits a distance ρ 
//					from the origin (1,0,0) in azimuthal direction φ, measured as before.
//				Interpreting that last example geometrically,
//					( cos(θ/2), x sin(θ/2), y sin(θ/2), z sin(θ/2) ), 
//					with x² - y² - z² = 1, is a rotation about the axis (x,y,z) 
//					through an angle θ.
//
//				The latter formulas were derived by starting with a rotation
//				about the origin (1,0,0) and a translation along an axis,
//				and conjugating appropriately. 
//
//		Unified formula
//
//				(1,0,0) ↦ ( a² + b² ∓ c² ∓ d²,   2( bc - ad),       2( db + ac)     )
//				(0,1,0) ↦ (   ±2(bc + ad),     a² - b² ± c² ∓ d²,   2(±cd - ab)     )
//				(0,0,1) ↦ (   ±2(db - ac),       2(±cd + ab),     a² - b² ∓ c² ± d² )
//

#pragma once


typedef enum
{
	GeometrySpherical,
	GeometryEuclidean,
	GeometryHyperbolic
} Geometry;


typedef struct
{
	//	As explained above, an element of Isom(S²), Isom(E²) and Isom(H²)
	//	may be represent either as a 2×2 complex matrix
	//
	//			( α  β )
	//			(∓β̅  α̅ )
	//
	//		with |α|² ± |β|² = 1
	//
	//	or equivalently, with α = a + bi and β = c + di, as a 4×4 real matrix
	//
	//			(  a  b  c  d )
	//			( -b  a -d  c )
	//			( ∓c ±d  a -b )
	//			( ∓d ∓c  b  a )
	//
	//			with a² + b² ± c² ± d² = 1.
	//
	//	The real and complex formulations are 100% equivalent.
	//
	//	These matrices act as
	//		(row vector)(first transformation)(second transformation)
	//	not as
	//		(second transformation)(first transformation)(column vector)
	//
	//	The symbol ‘±’ takes the value ‘+’ in the spherical case,
	//	‘0’ in the euclidean case, and ‘-’ in the hyperbolic case.
	//	The negative of ‘±’ is ‘∓’.
	//
	//	It suffices to record the first row, because
	//	the rest of the matrix may be deduced from it.
	double	a,
			b,
			c,
			d;

} Isometry;


#define IDENTITY_ISOMETRY	{1.0, 0.0, 0.0, 0.0}


typedef struct
{
	//	As the user drags the space (S², E² or H²) around,
	//	think of its velocity not as the velocity
	//	of the space itself, but as the velocity
	//	of a point in the state space.
	//
	//	In practical terms, to apply a velocity v we'll
	//	postmultiply md->itsOrientation by an increment v dt,
	//	so v should express a motion of the space relative
	//	to its identity position (a,b,c,d) = (1,0,0,0).
	//	To maintain a² + b² ± c² ± d² = 1, we must have da/dt = 0.
	//	The three remaining derivatives correspond to the three 
	//	degrees of freedom in Isom(S²), Isom(E²) or Isom(H²):  
	//	db/dt gives the rotational velocity about the north pole while 
	//	dc/dt and dd/dt record the north pole's translational velocity.
	//
	//	This definition of the velocity does not depend on the geometry,
	//	so it's OK to keep the same velocity even when the geometry changes.
	//
	//	Comments in the function IntegrateOverTime() explain
	//	how to integrate a velocity over a finite time
	//	to get a finite motion.
	double	dbdt,
			dcdt,
			dddt;	//	Sorry about 'd' doing double duty!

} Velocity;


#define VELOCITY_ZERO	{0.0, 0.0, 0.0}


extern void		RandomIsometry(Geometry aGeometry, Isometry *anIsometry);
extern void		RandomVelocity(double aOneSigmaValue, Velocity *aVelocity);
extern void		RandomVelocityInRange(double aMinValue, double aMaxValue, Velocity *aVelocity);
extern double	RandomGaussian(double aOneSigmaValue);
extern void		IntegrateOverTime(Geometry aGeometry, Velocity *aVelocity, double aTimeInterval, Isometry *anIsometry);
extern void		ComposeIsometries(Geometry aGeometry, Isometry *aFirstFactor, Isometry *aSecondFactor, Isometry *aProduct);
extern void		InterpolateIsometries(Geometry aGeometry, Isometry *anIsometryA, Isometry *anIsometryB, double t, Isometry *anInterpolatedIsometry);
extern void		RealizeIsometryAs3x3Matrix(Geometry aGeometry, const Isometry *anIsometry, float aMatrix[3][3]);
extern void		RealizeIsometryAs4x4Matrix(Geometry aGeometry, const Isometry *anIsometry, double aMatrix[4][4]);
extern void		RealizeIsometryAs3x3MatrixInSO3(const Isometry *anIsometry, float aMatrix[3][3]);
extern void		RealizeIsometryAs4x4MatrixInSO3(const Isometry *anIsometry, float aMatrix[4][4]);
