#ifndef INC_FRACTIONPRECISION
#define INC_FRACTIONRECISION

/*
 *******************************************************************************
 *
 *
 *                       Copyright (c) 2019
 *                       Henrik Vestermark
 *                       Denmark
 *
 *                       All Rights Reserved
 *
 *   This source file is subject to the terms and conditions of the
 *   Future Team Software License Agreement which restricts the manner
 *   in which it may be used.
 *   Mail: hve@hvks.com
 *
 *******************************************************************************
*/

/*
 *******************************************************************************
 *
 *
 * Module name     :   fractionprecision.h
 * Module ID Nbr   :   
 * Description     :   Arbitrary fraction precision class
 *                     Actually it a general fraction class that works with both
 *                     standard types like int, or int_precision 
 * --------------------------------------------------------------------------
 * Change Record   :   
 *
 * Version	Author/Date		Description of changes
 * -------  -----------		----------------------
 * 01.01	HVE/15-JUL-2019	Initial release
 *
 * End of Change Record
 * --------------------------------------------------------------------------
*/

/* define version string */
static char _VC_[] = "@(#)fractionprecision.h 01.01 -- Copyright (C) Henrik Vestermark";

#include <iostream>

// Complex Precision template class for fraction arithmetic
// Notice construction,assignments always guarantees that normalized fraction sign is always in the numerator. As a consequence the sign method just return the sign of the numerator
template<class _Ty> class fraction_precision {
   _Ty n, d;
   public:
      typedef _Ty value_type;

      // constructor
	  fraction_precision(const _Ty& a = (_Ty)0, const _Ty& b = (_Ty)1) : n(a), d(b) { normalize(); }		// fraction constructions
	  fraction_precision(const _Ty& whole, const _Ty& a, const _Ty& b) : n(a + w*b), d(b) { normalize(); }  // mixed number constructions

      // constructor for any other type to _Ty
      template<class _X> fraction_precision( const fraction_precision<_X>& a ) : n((_Ty)a.numerator()), d((_Ty)a.denominator()) {}
      
      // Coordinate functions
      _Ty numerator() const { return n; }						// return numerator
      _Ty denominator() const { return d; }						// return denominator
      _Ty numerator( const _Ty& a )   { return ( n = a ); }		// Set mumerator
	  _Ty numerator( const fraction_precision<_Ty>& a )   { return ( n = a.numerator() ); }  // Set numerator from another fraction
      _Ty denominator( const _Ty& b )   { return ( d = b ); }	// Set denominator
	  _Ty denominator( const fraction_precision<_Ty>& b )   { return ( d = b.denominator() ); } // Set denominator from another fraction
	  _Ty whole() const { return n / d; }						// return the whole number 
	  _Ty reduce() const { _Ty w = n / d; n %= d; return w; }	// Reduce the fraction by removing and returning the whole number from the fraction

	  // Methods
	  fraction_precision<_Ty>& abs(const fraction_precision<_Ty>& a) { n = abs(a.numerator()); d = abs(a.denominator()); return *this; }
	  fraction_precision<_Ty>& normalize()	{ _Ty z = gcd(n, d); if (z == (_Ty)0) throw divide_by_zero(); n/= z; d /= z; if (!(d >= 0)) { n *= (_Ty)-1; d *= (_Ty)-1; } return *this; }
	  fraction_precision<_Ty>& inverse()	{ _Ty z; z = n; n = d; d = z; if (!(d >= 0)) { n *= (_Ty)-1; d *= (_Ty)-1; } return *this; }
	  // Conversion methods. Safer and less ambiguous than overloading implicit/explicit conversion operators
	 // std::string fraction_precision<int_precision>& toString() { return n.toString() + "/" + d.toString(); }
																			

	  // Implicit/explicit conversion operators
	  operator long() const				{ return (long)(n) / long(d); }
	  operator int() const				{ return (int)n / (int)d; };
	  operator short() const			{ return (short)n / (short)d; };
	  operator char() const				{ return (char)n / (char)d; };
	  operator unsigned long() const	{ return (unsigned long)( n / d ); }
	  operator unsigned int() const		{ return (unsigned int)( n / d ); }
	  operator unsigned short() const	{ return (unsigned short)( n / d ); }
	  operator unsigned char() const	{ return (unsigned char)(n / d );}
	  operator double() const			{ return (double)n / (double)d; }
	  operator float() const			{ return (float)n / (float)d; }
	  operator int_precision() const	{ return (int_precision)n / (int_precision)d; }

      // Essential operators
	  fraction_precision<_Ty>& operator= (const fraction_precision<_Ty>& x)		{ n = x.numerator(); d = x.denominator(); normalize();  return *this; }
	  fraction_precision<_Ty>& operator+=( const fraction_precision<_Ty>& x)	{ n *= x.denominator(); n += d*x.numerator(); d *= x.denominator(); return *this; }
      fraction_precision<_Ty>& operator-=( const fraction_precision<_Ty>& x )   { n *= x.denominator(); n -= d*x.numerator(); d *= x.denominator(); return *this; }
      fraction_precision<_Ty>& operator*=( const fraction_precision<_Ty>& x )   { n *= x.numerator(); d *= x.denominator(); return *this; }
	  fraction_precision<_Ty>& operator/=( const fraction_precision<_Ty>& x )	{ n *= x.denominator(); d *= x.numerator(); return *this; } 
		 
	  class divide_by_zero {};
   };

template<class _Ty> std::ostream& operator<<( std::ostream& strm, const fraction_precision<_Ty>& a )
	{ return strm << a.numerator() << "/" << a.denominator(); }

template<class _Ty> std::istream& operator>>( std::istream& strm, fraction_precision<_Ty>& f ) 
   {
   _Ty n, d; char ch;
   strm >> n;
   strm >> std::noskipws >> ch;  
   if (ch == '/') 
	   strm >> d;
   else 
	   strm.putback(ch), d = (_Ty)0;
   if(!strm.fail())
		f = fraction_precision<_Ty>( n, d );
   return strm;
   }


// Arithmetic
template<class _Ty> fraction_precision<_Ty> operator+( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );	// Binary
template<class _Ty> fraction_precision<_Ty> operator+( const fraction_precision<_Ty>& );									// Unary
template<class _Ty> fraction_precision<_Ty> operator-( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );	// Binary
template<class _Ty> fraction_precision<_Ty> operator-( const fraction_precision<_Ty>& );									// Unary
template<class _Ty> fraction_precision<_Ty> operator*( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );	// Binary
template<class _Ty> fraction_precision<_Ty> operator/( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );	// Binary
template<class _Ty> fraction_precision<_Ty> operator++(fraction_precision<_Ty>&);		// Prefix
template<class _Ty> fraction_precision<_Ty> operator++(fraction_precision<_Ty>&, int);	// Postfix
template<class _Ty> fraction_precision<_Ty> operator--(fraction_precision<_Ty>&);		// Prefix
template<class _Ty> fraction_precision<_Ty> operator--(fraction_precision<_Ty>&, int);	// Postfix
                                                                                                                       
// Boolean Comparision Operators
template<class _Ty> inline bool operator==( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );
template<class _Ty> inline bool operator!=( const fraction_precision<_Ty>&, const fraction_precision<_Ty>& );
template<class _Ty> inline bool operator>=( const fraction_precision<_Ty>&, const fraction_precision<_Ty>&);
template<class _Ty> inline bool operator> ( const fraction_precision<_Ty>&, const fraction_precision<_Ty>&);
template<class _Ty> inline bool operator<=( const fraction_precision<_Ty>&, const fraction_precision<_Ty>&);
template<class _Ty> inline bool operator< ( const fraction_precision<_Ty>&, const fraction_precision<_Ty>&);


// Arithmetic operators
//

// lhs + rhs
template<class _Ty> fraction_precision<_Ty> operator+( const fraction_precision<_Ty>& x, const fraction_precision<_Ty>& y )
   {
   fraction_precision<_Ty> c(x);
   c += y;
   return c;
   }

// +x
template<class _Ty> fraction_precision<_Ty> operator+( const fraction_precision<_Ty>& x )
   {
   return x;
   }

// lhs - rhs
template<class _Ty> fraction_precision<_Ty> operator-( const fraction_precision<_Ty>& x, const fraction_precision<_Ty>& y )
   {
   fraction_precision<_Ty> c(x);
   c -= y;
   return c;
   }

// -x
template<class _Ty> fraction_precision<_Ty> operator-( const fraction_precision<_Ty>& x )
   {
   fraction_precision<_Ty> c(x);
   c.numerator( -c.numerator() );
   return c;
   }

// lhs * rhs
template<class _Ty> fraction_precision<_Ty> operator*( const fraction_precision<_Ty>& x, const fraction_precision<_Ty>& y )
   {
   fraction_precision<_Ty> c(x);
   c *= y;
   return c;
   }

// lhs / rhs
template<class _Ty> fraction_precision<_Ty> operator/( const fraction_precision<_Ty>& x, const fraction_precision<_Ty>& y )
   {
   fraction_precision<_Ty> c(x);
   c /= y;
   return c;
   }

// Prefix ++
template<class _Ty> fraction_precision<_Ty> operator++(fraction_precision<_Ty>& a)
	{
	a.numerator(a.numerator()+a.denominator());
	return a;
	}

// Postfix ++
template<class _Ty> fraction_precision<_Ty> operator++(fraction_precision<_Ty>& a, int)
	{
	fraction_precision<_Ty> postfix_a(a);

	a.numerator(a.numerator()+a.denominator());
	return postfix_a;
	}

// Prefix --
template<class _Ty> fraction_precision<_Ty> operator--(fraction_precision<_Ty>& a)
	{
	a.numerator(a.numerator()-a.denominator());
	return a;
	}

// Postfix --
template<class _Ty> fraction_precision<_Ty> operator--(fraction_precision<_Ty>& a, int)
	{
	fraction_precision<_Ty> postfix_a(a);

	a.numerator(a.numerator()-a.denominator());
	return postfix_a;
	}


// lhs == rhs
template<class _Ty> bool operator==( const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs )
	{fraction_precision<_Ty> a, b;
	a = lhs; 
	b = rhs; 
	return a.numerator() == b.numerator() && a.denominator() == b.denominator();
	}

// lhs != rhs
template<class _Ty> bool operator!=( const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs )
	{
	return lhs == rhs ? false : true;
	}

// lhs >= rhs
// a/b>=c/d => a*d>=c*b
template<class _Ty> bool operator>=(const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs)
{
	bool bb;
	fraction_precision<_Ty> a, b;
	a = lhs;
	b = rhs; 
	bb= a.numerator()*b.denominator()>=b.numerator()*a.denominator();
	return bb;
	}

// lhs > rhs 
// a/b>c/d => a*d>c*b
template<class _Ty> bool operator>(const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs)
{
	bool bb;
	fraction_precision<_Ty> a, b;
	a = lhs;  
	b = rhs;  
	bb = a.numerator()*b.denominator() > b.numerator()*a.denominator();
	return bb;
}

// lhs <= rhs
template<class _Ty> bool operator<=(const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs)
	{
	return lhs > rhs ? false : true;
	}

// lhs < rhs
template<class _Ty> bool operator<(const fraction_precision<_Ty>& lhs, const fraction_precision<_Ty>& rhs)
	{
	return lhs >= rhs ? false : true;
	}

///	@author Henrik Vestermark (hve@hvks.com)
///	@date  3/Feb/2017, revised 20/JUL/2019
///	@brief 			gcd - fraction_precision for Greatest Common Divisor
///	@return 		The greates common divisor of fraction precision a
///	@param "a"	-	First operand number 
///
///	@todo
///
/// Description:
///   gcd of fraction_precision. 
///   Call normalize that do a gcd()
//
template<class _TY> inline fraction_precision<_TY> gcd(const fraction_precision<_TY>& a )
	{
	return a.normalize();
	}
#endif
