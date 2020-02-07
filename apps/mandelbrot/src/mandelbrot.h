#ifndef MANDELBROT_H
#define MANDELBROT_H


#if defined(USE_APML)
#include <fprecision.h>
#elif defined(USE_MPFR)
#include <mpfr.h>
#else
#include <cmath>
#endif

#include <string>

namespace Mandelbrot {
#if defined(USE_APML)
    using Number = float_precision;
#elif defined (USE_MPFR)
class Number {
public:
    ~Number() {if(m_set) mpfr_clear(m_value);}
    Number() : m_set(true) {mpfr_init2 (m_value, 200);}
    Number(double v) : Number() { mpfr_set_d (m_value, v, MPFR_RNDN);}
    Number(Number&& other ) {mpfr_swap(other.m_value, m_value);
                            std::swap(m_set, other.m_set);
                            }
    Number(const Number& other ) {mpfr_init_set(m_value, other.m_value, MPFR_RNDN);}
    Number& operator=(Number&& other) {mpfr_swap(other.m_value, m_value); return *this;}
    Number& operator=(const Number& other) {mpfr_init_set(m_value, other.m_value, MPFR_RNDN); return *this;}
    Number sqrt() const {
        Number out;
        mpfr_sqrt (out.m_value, m_value, MPFR_RNDN);
        return out;}
    std::string toString() const {
        mpfr_exp_t e;
        auto buf = mpfr_get_str(nullptr, &e, 10, 20, m_value, MPFR_RNDN);
        std::string out(buf);
        mpfr_free_str(buf);
        const auto p = (out.front() == '-') ? 2 : 1;
        out.insert(static_cast<unsigned>(p), ".");
        out.append(" * 10<sup>" + std::to_string(e) + "</sup>");
        return out;
    }
    friend Number operator*(const Number& a, const Number& b);
    friend Number operator+(const Number& a, const Number& b);
    friend Number operator-(const Number& a, const Number& b);
    friend Number operator/(const Number& a, const Number& b);
    friend bool operator<=(const Number& a, const Number& b);
private:
    bool m_set = false;
    mpfr_t m_value;
};

inline Number operator*(const Number& a, const Number& b) {Number out; mpfr_mul(out.m_value, a.m_value, b.m_value, MPFR_RNDU); return out;}
inline Number operator+(const Number& a, const Number& b) {Number out; mpfr_add(out.m_value, a.m_value, b.m_value, MPFR_RNDU); return out;}
inline Number operator-(const Number& a, const Number& b) {Number out; mpfr_sub(out.m_value, a.m_value, b.m_value, MPFR_RNDU); return out;}
inline Number operator/(const Number& a, const Number& b) {Number out; mpfr_div(out.m_value, a.m_value, b.m_value, MPFR_RNDU); return out;}
inline bool operator<=(const Number& a, const Number& b) {return mpfr_lessequal_p(a.m_value, b.m_value);}

#else
    using Number = double;
#endif

    Number sqrt(const Number& number) {
        return
#if defined(USE_APML)
               ::sqrt(number);
#elif defined (USE_MPFR)
                number.sqrt();
#else
               std::sqrt(number);
#endif
    }


    std::string toString(const Number& number) {
        return
#ifdef USE_APML
                number.toString();
#elif defined (USE_MPFR)
                number.toString();
#else
                std::to_string(number);
#endif

    }


    class Complex {
    public:
        Complex(const Number& rr, const Number& ii) : r(rr), i(ii) {}
        Complex(Number&& rr, Number&& ii) : r(std::forward<Number>(rr)), i(std::forward<Number>(ii)) {}
        Complex(const Complex& other) = default;
        Complex(Complex&& other) = default;
        Complex& operator=(const Complex& other) = default;
        Number abs2() const {return r * r + i * i;}
    public:
        Number r;
        Number i;
    };

   Complex operator*(const Complex& a, const Complex& b) {return Complex(a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r);}
   Complex operator+(const Complex& a, const Complex& b) {return Complex(a.r + b.r, a.i + b.i);}

    int calculate(const Complex& c, int iterations) {
        Complex z(0, 0);
        int n = 0;
        while(z.abs2() <= Number(4.) && n < iterations) {
            z = z * z + c; //assign happens here! :-(
            ++n;
        }
        return n;
    }
}


#endif // MANDELBROT_H
