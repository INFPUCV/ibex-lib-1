/* ============================================================================
 * I B E X - Implementation of the Interval class based on Profil/BIAS
 * ============================================================================
 * Copyright   : Ecole des Mines de Nantes (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Dec 23, 2011
 * Modified by : Gilles Trombettoni, Bertrand Neveu
 * ---------------------------------------------------------------------------- */

#ifndef _IBEX_BIAS_INTERVAL_H_
#define _IBEX_BIAS_INTERVAL_H_

#include "Functions.h"
#include "ibex_NonRecoverableException.h"
#include <float.h>
#include <BIAS/BiasF.h>

namespace ibex {

inline Interval::Interval(const INTERVAL& x) : itv(x) {

}

inline Interval& Interval::operator=(const INTERVAL& x) {
	this->itv = x;
	return *this;
}

inline Interval::Interval() : itv(NEG_INFINITY, POS_INFINITY) {

}

inline Interval::Interval(double a, double b) : itv(a,b) {
	if (a==POS_INFINITY || b==NEG_INFINITY || a>b) *this=EMPTY_SET;
}

inline Interval::Interval(double a) : itv(a,a) {
	if (a==NEG_INFINITY || a==POS_INFINITY) *this=EMPTY_SET;
}

inline bool Interval::operator==(const Interval& x) const {
	return lb()==x.lb() && ub()==x.ub();
}

inline bool Interval::operator!=(const Interval& x) const {
	return !(*this==x);
}

inline Interval& Interval::operator+=(double d) {
	if (!is_empty()) {
		if (d==NEG_INFINITY || d==POS_INFINITY) *this=EMPTY_SET;
		else itv+=d;
	}
	return *this;
}

inline Interval& Interval::operator-=(double d) {
	if (!is_empty()) {
		if (d==NEG_INFINITY || d==POS_INFINITY) *this=EMPTY_SET;
		else itv-=d;
	}
	return *this;
}

inline Interval& Interval::operator*=(double d) {
	return ((*this)*=Interval(d));
}

inline Interval& Interval::operator/=(double d) {
	return ((*this)/=Interval(d));
}

inline Interval& Interval::operator+=(const Interval& x) {
	if (is_empty()) return *this;
	else if (x.is_empty()) { *this=Interval::EMPTY_SET; return *this; }
	else { itv+=x.itv; return *this; }
}

inline Interval& Interval::operator-=(const Interval& x) {
	if (is_empty()) return *this;
	else if (x.is_empty()) { *this=Interval::EMPTY_SET; return *this; }
	else { itv-=x.itv; return *this; }
}

/*
 *  Multiplication
 *
 * \note Some situations where Bias/Profil may fail (underscore stands for any value): <ul>
 *  <li> <tt>[0,0] * [-inf,_]</tt>
 *  <li> <tt> [0,0] * [_,inf] </tt>
 *  <li> <tt> [-inf,_] * [_,0] </tt>
 *  <li> <tt> [_,inf] * [_,0] </tt>
 *  <li> <tt> [-0,_] * [_,inf] </tt>
 *  <li> <tt> [inf([0,0])*-1,1] * [1,inf] </tt>

 *  <li> <tt> [inf([0,0])*-1,1] * [-inf,1] </tt>
 *  </ul> */
inline Interval& Interval::operator*=(const Interval& y) {

	INTERVAL r;

	if (is_empty()) return *this;
	if (y.is_empty()) { *this=Interval::EMPTY_SET; return *this; }

	const REAL& a(lb());
	const REAL& b(ub());
	const REAL& c(y.lb());
	const REAL& d(y.ub());

	if ((a==0 && b==0) || (c==0 && d==0)) { *this=Interval(0.0,0.0); return *this; }

	if (((a<0) && (b>0)) && (c==NEG_INFINITY || d==POS_INFINITY)) { *this=Interval(NEG_INFINITY, POS_INFINITY); return *this; }

	if (((c<0) && (d>0)) && (a==NEG_INFINITY || b==POS_INFINITY)) { *this=Interval(NEG_INFINITY, POS_INFINITY); return *this; }

	// [-inf, _] x [_ 0] ou [0,_] x [_, +inf]
	if (((a==NEG_INFINITY) && (d==0)) || ((d==POS_INFINITY) && (a==0))) {
		if ((b<=0) || (c>=0)) { *this=Interval(0.0, POS_INFINITY); return *this; }
		else {
			BiasMulII (Bias(r), Bias(INTERVAL(b)), Bias(INTERVAL(c)));
			*this=Interval(Inf(r), POS_INFINITY);
			return *this;
		}
	}

	// [-inf, _] x [0, _] ou [0, _] x [-inf, _]
	if (((a==NEG_INFINITY) && (c==0)) || ((c==NEG_INFINITY) && (a==0))) {
		if ((b<=0) || (d<=0)) { *this=Interval(NEG_INFINITY, 0.0); return *this; }
		else {
			BiasMulII (Bias(r), Bias(INTERVAL(b)), Bias(INTERVAL(d)));
			*this=Interval(NEG_INFINITY, Sup(r));
			return *this;
		}
	}

	// [_,0] x [-inf, _] ou [_, +inf] x [0,_]
	if (((c==NEG_INFINITY) && (b==0)) || ((b==POS_INFINITY) && (c==0))) {
		if ((d<=0) || (a>=0)) { *this=Interval(0.0, POS_INFINITY); return *this; }
		else {
			BiasMulII (Bias(r), Bias(INTERVAL(a)), Bias(INTERVAL(d)));
			*this=Interval(Inf(r), POS_INFINITY);
			return *this;
		}
	}

	// [_, +inf] x [_,0] ou [_,0] x [_, +inf]
	if (((b==POS_INFINITY) && (d==0)) || ((d==POS_INFINITY) && (b==0))) {
		if ((a>=0) || (c>=0)) { *this=Interval(NEG_INFINITY, 0.0); return *this; }
		else {
			BiasMulII (Bias(r), Bias(INTERVAL(a)), Bias(INTERVAL(c)));
			*this=Interval(NEG_INFINITY, Sup(r));
			return *this;
		}
	}

	BiasMulII (Bias(r), Bias(itv), Bias(y.itv));
	*this=r;
	return *this;
}

inline Interval& Interval::operator/=(const Interval& y) {

	if (is_empty()) return *this;
	if (y.is_empty()) { *this=Interval::EMPTY_SET; return *this; }

	const REAL& a(lb());
	const REAL& b(ub());
	const REAL& c(y.lb());
	const REAL& d(y.ub());

	INTERVAL r;

	if (c==0 && d==0) {
		*this=Interval::EMPTY_SET;
		return *this;
	}

	if (a==0 && b==0) {
		return *this;
	}

	if (c>0 || d<0) {
		BiasDivII (Bias(r), Bias(itv), Bias(y.itv));
		*this=r;
		return *this;
	}

	if ((b<=0) && d==0) {
		BiasDivII (Bias(r), Bias(INTERVAL(b)), Bias(INTERVAL(c)));
		*this=Interval(Inf(r), POS_INFINITY);
		return *this;
	}

	if (b<=0 && c<0 && d<0) {
		*this=Interval(NEG_INFINITY, POS_INFINITY);
		return *this;
	}

	if (b<=0 && c==0) {
		BiasDivII (Bias(r), Bias(INTERVAL(b)), Bias(INTERVAL(d)));
		*this=Interval(NEG_INFINITY, Sup(r));
		return *this;
	}

	if (a>=0 && d==0) {
		BiasDivII (Bias(r), Bias(INTERVAL(a)), Bias(INTERVAL(c)));
		*this=Interval(NEG_INFINITY, Sup(r));
		return *this;
	}

	if (a>=0 && c<0 && d>0) {
		*this=Interval(NEG_INFINITY, POS_INFINITY);
		return *this;
	}

	if (a>=0 && c==0) {
		BiasDivII (Bias(r), Bias(INTERVAL(a)), Bias(INTERVAL(d)));
		*this=Interval(Inf(r), POS_INFINITY);
		return *this;
	}

	*this=Interval(NEG_INFINITY, POS_INFINITY); // a<0<b et c<=0<=d
	return *this;
}

inline Interval Interval:: operator-() const {
	if (is_empty()) return *this;
	return -itv;
}

inline Interval& Interval::div2_inter(const Interval& x, const Interval& y) {
	Interval out2;
	div2_inter(x,y,out2);
	*this |= out2;
	return *this;
}

inline void Interval::set_empty() {
	*this=EMPTY_SET;
}

inline Interval& Interval::operator&=(const Interval& x) {
	if (is_empty()) return *this;
	if (x.is_empty()) { set_empty(); return *this; }

	INTERVAL tmp;
	if (Intersection(tmp, itv, x.itv)) itv = tmp;
	else set_empty();
	return *this;
}

inline Interval& Interval::operator|=(const Interval& x) {
	if (!x.is_empty()) {
		if (is_empty()) itv = x.itv;
		else itv = Hull(itv, x.itv);
	}
	return *this;
}

inline double Interval::lb() const {
	return Inf(itv);
}

inline double Interval::ub() const {
	return Sup(itv);
}

inline double Interval::mid() const {
	if (Inf(itv)==NEG_INFINITY)
		if (Sup(itv)==POS_INFINITY) return 0;
		else return -DBL_MAX;
	else if (Sup(itv)==POS_INFINITY) return DBL_MAX;
	else {
		double m=Mid(itv);
		if (m<Inf(itv)) m=Inf(itv); // watch dog
		else if (m>Sup(itv)) m=Sup(itv);
		return m;
	}
}

inline bool Interval::is_subset(const Interval& x) const {
	return is_empty() || (!x.is_empty() && x.lb()<=lb() && x.ub()>=ub());
}

inline bool Interval::is_strict_subset(const Interval& x) const {
	return (is_empty() && !x.is_empty()) ||
			(!x.is_empty() && (x.lb()==NEG_INFINITY || x.lb()<lb()) &&
							  (x.ub()==POS_INFINITY || x.ub()>ub()));
}

inline bool Interval::is_superset(const Interval& x) const {
	return x.is_subset(*this);
}

inline bool Interval::is_strict_superset(const Interval& x) const {
	return x.is_strict_subset(*this);
}

inline bool Interval::contains(double d) const {
	if (is_empty()) return false;
	else return d>=lb() && d<=ub();
}

inline bool Interval::strictly_contains(double d) const {
	if (is_empty()) return false;
	else return d>lb() && d<ub();
}

inline bool Interval::is_disjoint(const Interval &x) const {
	if (is_empty() || x.is_empty()) return true;
	return lb()>x.ub() || ub()<x.lb();
}

inline bool Interval::is_empty() const {
	return ub()==NEG_INFINITY; //*this==EMPTY_SET;
}

inline bool Interval::is_degenerated() const {
	return is_empty() || lb()==ub();
}

inline bool Interval::is_unbounded() const {
	if (is_empty()) return false;
	return lb()==NEG_INFINITY || ub()==POS_INFINITY;
}

inline double Interval::rad() const {
	return 0.5*Diam(itv);
}

inline double Interval::diam() const {
	return Diam(itv);
}

inline double Interval::mig() const {
	return Mig(itv);
}

inline double Interval::mag() const {
	return Abs(itv);
}

inline Interval operator&(const Interval& x1, const Interval& x2) {
	INTERVAL res;
	if (x1.is_empty() || x2.is_empty() || !Intersection(res, x1.itv, x2.itv))
		return Interval::EMPTY_SET;
	else
		return res;
}

inline Interval operator|(const Interval& x1, const Interval& x2) {
	if (x1.is_empty()) return x2;
	if (x2.is_empty()) return x1;
	return Hull(x1.itv,x2.itv);
}

inline Interval operator+(const Interval& x, double d) {
	if (x.is_empty()) return x;
		else if (d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return x.itv+d;
}

inline Interval operator-(const Interval& x, double d) {
	if (x.is_empty()) return x;
	else if (d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return x.itv-d;
}

inline Interval operator*(const Interval& x, double d) {
	if (x.is_empty()) return x;
	else if (d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return x.itv*d;
}

inline Interval operator/(const Interval& x, double d) {
	if (x.is_empty()) return x;
	else if (d==0 || d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return x.itv/INTERVAL(d);
}

inline Interval operator+(double d,const Interval& x) {
	return x+d;
}

inline Interval operator-(double d, const Interval& x) {
	if (x.is_empty()) return x;
	else if (d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return d-x.itv;
}

inline Interval operator*(double d, const Interval& x) {
	return x*d;
}

inline Interval operator/(double d, const Interval& x) {
	if (x.is_empty()) return x;
	else if (d==0 || d==NEG_INFINITY || d==POS_INFINITY) return Interval::EMPTY_SET;
	else return INTERVAL(d)/x.itv;
}

inline Interval operator+(const Interval& x1, const Interval& x2) {
	if (x1.is_empty() || x2.is_empty())
		return Interval::EMPTY_SET;
	else
		return x1.itv+x2.itv;
}

inline Interval operator-(const Interval& x1, const Interval& x2) {
	if (x1.is_empty() || x2.is_empty())
		return Interval::EMPTY_SET;
	else
		return x1.itv-x2.itv;
}

/*
 *  Multiplication
 *
 * \note Some situations where Bias/Profil may fail (underscore stands for any value): <ul>
 *  <li> <tt>[0,0] * [-inf,_]</tt>
 *  <li> <tt> [0,0] * [_,inf] </tt>
 *  <li> <tt> [-inf,_] * [_,0] </tt>
 *  <li> <tt> [_,inf] * [_,0] </tt>
 *  <li> <tt> [-0,_] * [_,inf] </tt>
 *  <li> <tt> [inf([0,0])*-1,1] * [1,inf] </tt>

 *  <li> <tt> [inf([0,0])*-1,1] * [-inf,1] </tt>
 *  </ul> */
inline Interval operator*(const Interval& x, const Interval& y) {
	return (Interval(x)*=y);
}

inline Interval operator/(const Interval& x, const Interval& y) {
	return (Interval(x)/=y);
}

inline double distance(const Interval &x1, const Interval &x2) {

    if (x1.is_empty()) return x2.rad();

    if (x2.is_empty()) return x1.rad();

    if (x1.lb()==NEG_INFINITY) {
    	if (x2.lb()!=NEG_INFINITY)
    		return POS_INFINITY;
    	else if (x1.ub()==POS_INFINITY) {
    		if (x2.ub()==POS_INFINITY) return 0.0;
    		else return POS_INFINITY;
    	}
    	else if (x2.ub()==POS_INFINITY) return POS_INFINITY;
    	else return fabs(x1.ub()-x2.ub());
    }
    else if (x1.ub()==POS_INFINITY) {
    	if (x2.ub()!=POS_INFINITY)
    		return POS_INFINITY;
    	else if (x2.lb()==NEG_INFINITY)
    		return POS_INFINITY;
    	else return fabs(x1.lb()-x2.lb());
    }
    else if (x2.is_unbounded())
    	return POS_INFINITY;
    else
    	return Distance(x1.itv,x2.itv);
}

inline double Interval::rel_distance(const Interval& x) const {
	  double d=distance(*this,x);
	  if (d==POS_INFINITY) return 1;
	  double D=diam();
	  return (D==0 || D==POS_INFINITY) ? 0.0 : (d/D); // if diam(this)=infinity here, necessarily d=0
}

inline Interval sqr(const Interval& x) {
	return Sqr(x.itv);
}

inline Interval sqrt(const Interval& x) {
	INTERVAL r;
	BiasInit();

	if (x.ub()<0) return Interval::EMPTY_SET;
	else {
		if (x.lb()<0) {
			BiasSqrt (Bias(r), Bias(INTERVAL(0.0, x.ub())));
			return r;
		}
		else {
			BiasSqrt (Bias(r), Bias(x.itv));
			return r;
		}
	}
}

inline Interval pow(const Interval& x, int n) {
	return Power(x.itv,n);
}

inline Interval pow(const Interval& x, double d) {
	return Power(x.itv,d);
}

inline Interval pow(const Interval &x, const Interval &y) {
	return Power(x.itv,y.itv);
}

inline Interval root(const Interval& x, int n) {
	INTERVAL r;

	if (x.lb()==0 && x.ub()==0) return Interval::ZERO;

	// odd root
	if (n%2==0) {
		if (x.ub()<0)
			return Interval::EMPTY_SET;
		else
			if (x.lb()<0) {
				BiasRoot(Bias(r), Bias(INTERVAL(0.0, x.ub())), n);
				return r;
			} else {
				BiasRoot(Bias(r), Bias(x.itv), n);
				return r;
			}
	} else {
		// even root
		if (x.lb()>=0) {
			BiasRoot(Bias(r), Bias(x.itv), n);
			return r;
		}
		if (x.ub()<=0) {
			BiasRoot(Bias(r), Bias(-x.itv), n);
			return -r;
		}
		INTERVAL r2;
		BiasRoot(Bias(r), Bias(INTERVAL(0,-x.lb())), n);
		BiasRoot(Bias(r2), Bias(INTERVAL(0,+x.ub())), n);

		return Interval(-Sup(r), Sup(r2));
	}
}

inline Interval exp(const Interval& x) {
	return Exp(x.itv);
}

inline Interval log(const Interval& x) {
	INTERVAL r;

	if (x.ub()<=0)
		return Interval::EMPTY_SET; // REM: maybe should compute [-inf, -inf] with [_,0]
	else {
		if (x.lb()<=0) {
			BiasLog (Bias(r), Bias(INTERVAL(x.ub())));
			return Interval(NEG_INFINITY, Sup(r));
		}
		else {
			BiasLog (Bias(r), Bias(x.itv));
			return r;
		}
	}
}

inline Interval cos(const Interval& x) {
	return Cos(x.itv);
}

inline Interval sin(const Interval& x) {
	return Sin(x.itv);
}

inline Interval tan(const Interval& x) {
	INTERVAL r;

	Interval nb_period = (x+INTERVAL((x.lb()<0?-1:1)*Interval::PI.lb()/2.0,(x.ub()<0?-1:1)*Interval::PI.ub()/2.0))/Interval::PI;
	if (((int) nb_period.lb())< ((int) nb_period.ub())) return Interval(NEG_INFINITY, POS_INFINITY);

	BiasTan (Bias(r), Bias(x.itv));
	return r;
}

inline Interval acos(const Interval& x) {
	INTERVAL r;

	if (x.ub()<-1.0 || x.lb()>1.0)
		return Interval::EMPTY_SET;
	else {
		bool minus1=x.lb()<=-1;
		bool plus1=x.ub()>=1;
		BiasArcCos(Bias(r), Bias(INTERVAL(minus1?-1:x.lb(), plus1?1:x.ub())));
		return Interval(plus1?0.0:Inf(r), minus1?Interval::PI.ub():Sup(r));
	}

}

inline Interval asin(const Interval& x) {
	INTERVAL r;

	if (x.ub()<-1.0 || x.lb()>1.0)
		return Interval::EMPTY_SET;
	else {
		bool minus1=x.lb()<-1;
		bool plus1=x.ub()>1;
		BiasArcSin(Bias(r), Bias(INTERVAL(minus1?-1:x.lb(), plus1?1:x.ub())));
		return Interval(minus1?(-Interval::HALF_PI).lb():Inf(r), plus1?Interval::HALF_PI.ub():Sup(r));
	}
}

inline Interval atan(const Interval& x) {
	return ArcTan(x.itv);
}

// ArcTan2 by Firas Khemane (feb 2010)
inline Interval atan2(const Interval& a, const Interval& b) {

	if (( a.lb() > 0 && b.lb() > 0) || (  a.lb() > 0 &&   b.ub() < 0 )  || ( b.lb() < 0 && b.ub()> 0  && a.lb() > 0 ) )

		return atan(b/a);

	else if  ( a.lb()<=0  &&  a.ub()>0 )

		return 2*atan(b/a);

	else if  (a.ub()< 0 && b.lb()> 0)

		return atan(b/a)+Interval::PI;

	else if (a.ub() < 0 && b.ub()< 0)

		return atan(b/a)-Interval::PI;

	else if (b.lb() < 0 && b.ub() > 0 && a.ub()< 0 ) {

		if (  (atan(b/a)+Interval::PI).lb() <= (atan(b/a)-Interval::PI).ub() )

			return 2*INTERVAL((atan(b/a)+Interval::PI).lb(), (atan(b/a)-Interval::PI).ub());

		else
			return 2*INTERVAL((atan(b/a)-Interval::PI).lb() , (atan(b/a)+Interval::PI).ub() );
	}
}

inline Interval cosh(const Interval& x) {
	return Cosh(x.itv);
}

inline Interval sinh(const Interval& x) {
	return Sinh(x.itv);
}

inline Interval tanh(const Interval& x) {

	INTERVAL r;

	if (x.lb()==NEG_INFINITY)
		if (x.ub()==POS_INFINITY) return Interval(-1, 1);
		else {
			BiasTanh (Bias(r), Bias(INTERVAL(x.ub())));
			return Interval(-1,Sup(r));
		}
	else {
		if (x.ub()==POS_INFINITY) {
			BiasTanh (Bias(r), Bias(INTERVAL(x.lb())));
			return Interval(Inf(r),1);
		}
		else {
			BiasTanh (Bias(r), Bias(x.itv));
			return r;
		}
	}
}

inline Interval acosh(const Interval& x) {
	if (x.ub()<1.0) return Interval::EMPTY_SET;

	INTERVAL r;

	BiasArCosh (Bias(r), Bias(INTERVAL(x.lb()<1?1:x.lb(),x.ub())));
	return r;

}

inline Interval asinh(const Interval& x) {
	return ArSinh(x.itv);
}

inline Interval atanh(const Interval& x) {
	INTERVAL r;

	if (x.ub()<-1.0 || x.lb()>1.0)
		return Interval::EMPTY_SET;
	else
		if (x.lb()<=-1)
			if (x.ub()>=1)
				return Interval(NEG_INFINITY, POS_INFINITY);
			else {
				BiasArTanh(Bias(r), Bias(INTERVAL(x.ub())));
				return Interval(NEG_INFINITY,Sup(r));
			}
		else
			if (x.ub()>=1) {
				BiasArTanh(Bias(r), Bias(INTERVAL(x.lb())));
				return Interval(Inf(r), POS_INFINITY);
			}

	BiasArTanh (Bias(r), Bias(x.itv));
	return r;
}

inline Interval abs(const Interval &x) {
	return IAbs(x.itv);
}

inline Interval max(const Interval& x, const Interval& y) {
	return Interval(x.lb()>y.lb()? x.lb() : y.lb(), x.ub()>y.ub()? x.ub() : y.ub());
}

inline Interval min(const Interval& x, const Interval& y) {
	return Interval(x.lb()<y.lb()? x.lb() : y.lb(), x.ub()<y.ub()? x.ub() : y.ub());
}

inline bool proj_mul(const Interval& y, Interval& x1, Interval& x2) {
	if (y.contains(0)) {
		if (!x2.contains(0))                           // if y and x2 contains 0, x1 can be any real number.
			if (x1.div2_inter(y,x2).is_empty()) return false;   // otherwise y=x1*x2 => x1=y/x2
		return x1.contains(0) || (!x2.div2_inter(y,x1).is_empty());
	} else {
		if (x1.div2_inter(y,x2).is_empty()) return false;
		return !x2.div2_inter(y,x1).is_empty();
	}
}

inline bool proj_sqr(const Interval& y, Interval& x) {

	Interval proj=sqrt(y);
	Interval pos_proj= proj & x;
	Interval neg_proj = (-proj) & x;

	x = pos_proj | neg_proj;

	return !x.is_empty();
}

inline bool proj_pow(const Interval& y, int expon, Interval& x) {

	if (expon % 2 ==0) {
		Interval proj=root(y,expon);
		Interval pos_proj= proj & x;
		Interval neg_proj = (-proj) & x;

		x = pos_proj | neg_proj;

		return !x.is_empty();

	} else {

		x &= root(y, expon);
		return !x.is_empty();

	}
}

inline bool proj_pow(const Interval& y, Interval& x1, Interval& x2) {
	throw NonRecoverableException("proj_power(y,x1,x2) (with x1 and x2 intervals) not implemented yet with BIAS");
}

/**
 * ftype:
 *   COS = 0
 *   SIN = 1
 *   TAN = 2
 */
inline bool proj_trigo(const Interval& y, Interval& x, int ftype) {

	const int COS=0;
	const int SIN=1;
	const int TAN=2;

	if (x.lb()==NEG_INFINITY || x.ub()==POS_INFINITY) return true; // infinity of periods

	Interval period_0, nb_period;

	switch (ftype) {
	case COS :
		period_0 = acos(y);
		nb_period = x / Interval::PI;
		break;
	case SIN :
		period_0 = asin(y);
		nb_period = (x+Interval::HALF_PI) / Interval::PI;
		break;
	case TAN :
	default :
		period_0 = atan(y);
		nb_period = (x+Interval::HALF_PI) / Interval::PI;
		break;
	}
	if (period_0.is_empty()) return false;

	int p1 = ((int) nb_period.lb())-1;
	int p2 = ((int) nb_period.ub());
	Interval tmp1, tmp2;

	bool found = false;
	int i = p1-1;

	switch(ftype) {
	case COS :
		// should find in at most 2 turns.. but consider rounding !
		while (++i<=p2 && !found) found = !(tmp1 = (x & (i%2==0? period_0 + i*Interval::PI : (i+1)*Interval::PI - period_0))).is_empty();
		break;
	case SIN :
		while (++i<=p2 && !found) found = !(tmp1 = (x & (i%2==0? period_0 + i*Interval::PI : i*Interval::PI - period_0))).is_empty();
		break;
	case TAN :
		while (++i<=p2 && !found) found = !(tmp1 = (x & (period_0 + i*Interval::PI))).is_empty();
		break;
	}

	if (!found) return false;
	found = false;
	i=p2+1;

	switch(ftype) {
	case COS :
		while (--i>=p1 && !found) found = !(tmp2 = (x & (i%2==0? period_0 + i*Interval::PI : (i+1)*Interval::PI - period_0))).is_empty();
		break;
	case SIN :
		while (--i>=p1 && !found) found = !(tmp2 = (x & (i%2==0? period_0 + i*Interval::PI : i*Interval::PI - period_0))).is_empty();
		break;
	case TAN :
		while (--i>=p1 && !found) found = !(tmp2 = (x & (period_0 + i*Interval::PI))).is_empty();
		break;
	}

	if (!found) return false;

	x = tmp1 | tmp2;

	return true;
}

inline bool proj_cos(const Interval& y,  Interval& x) {
	return proj_trigo(y,x,0);
}

inline bool proj_sin(const Interval& y,  Interval& x) {
	return proj_trigo(y,x,1);
}

inline bool proj_tan(const Interval& y,  Interval& x) {
	return proj_trigo(y,x,2);
}

inline bool proj_atan2(const Interval& y, Interval& x1, Interval& x2) {
	throw NonRecoverableException("proj_atan2 non implemented yet with BIAS");
}

inline bool proj_cosh(const Interval& y,  Interval& x) {

	Interval proj=acosh(y);
	if (proj.is_empty()) return false;
	Interval pos_proj= proj & x;
	Interval neg_proj = (-proj) & x;

	x = pos_proj | neg_proj;

	return !x.is_empty();
}

inline bool proj_sinh(const Interval& y,  Interval& x) {
	x &= asinh(y);
	return !x.is_empty();
}

inline bool proj_tanh(const Interval& y,  Interval& x) {
	x &= atanh(y);
	return !x.is_empty();
}



inline bool proj_abs(const Interval& y,  Interval& x) {
	Interval x1 = x & y;
	Interval x2 = x & (-y);
	x &= x1 | x2;
	return !x.is_empty();
}

} // end namespace

#endif /* _IBEX_BIAS_INTERVAL_H_ */
