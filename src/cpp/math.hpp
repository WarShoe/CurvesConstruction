#pragma once

#include <functional>
#include <optional>
#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_integration.h>

#include "gsl_init.hpp"

namespace math {

struct Parameter {
    double value, error, min, max;
    Parameter (double value=NAN, double error=NAN, double min=NAN, double max=NAN)
        : value (value)
        , error (error)
        , min   (min)
        , max   (max)
    {}

    operator double (void) const {return value;}
};

struct Options {
    Options (void) : eps_abs(1e-6), eps_rel(1e-5), limit(10000), iters(100) {}
    std::optional <double> eps_abs, eps_rel;
    std::optional <unsigned> limit;
    std::optional <unsigned> iters;
};

inline
std::ostream & operator << (std::ostream &os, const Options &v) {
    os << "(math options: eps_abs="
       << v.eps_abs.value_or(NAN)
       << " eps_rel="
       << v.eps_rel.value_or(NAN)
       << ")";
    return os;
}

struct Result {
    std::optional <double> value, error;
    std::optional <unsigned> calls;
    std::optional <int> code;
    std::vector <Parameter> x;
    std::optional <std::string> error_text;

    bool IsGood (void) const {return !error_text;}
    void SetError (std::string error) {error_text=error;}
    std::string GetError (void) const {return error_text.value_or("");}

    double ValueOr (const std::string &msg) {
        if(value) return value.value();
        throw std::runtime_error(code?gsl_strerror(code.value()):"");
    }

    double ValueOr (double v) {
        return value.value_or(v);
    }
};

inline
std::ostream & operator << (std::ostream &os, const Result &r) {
    if(r.value) os << "value=" << r.value.value() << " ";
    if(r.error) os << "error=" << r.error.value() << " ";
    if(r.code) os << "code=" << r.code.value() << " ";
    if(r.error_text) os << "error=\"" << r.error_text.value() << "\" ";
    if(!r.x.empty()){
        os << "x=[";
        for(auto x: r.x) os << x << ",";
        os << "]";
    }

    return os;
}

template <typename F>
struct Wrapper {
    F cpp_f;
    gsl_function gsl_f;
    Wrapper (F f) : cpp_f(f) {
        gsl_f.function = Call;
        gsl_f.params = this;
    }
    static double Call (double x, void *pars) {
        return reinterpret_cast <Wrapper*> (pars) -> cpp_f(x);
    }
};

Result integral (
    double (*f) (double, void*),
    void *pars,
    std::vector <double> points,
    Options opts = {}
);

template <typename F>
Result integral (
    F f,
    std::vector <double> points,
    Options opts = {}
){
    Wrapper w {f};
    return integral (&w.Call,&w,points,opts);
}

inline auto
make_normal_PDF (
    double mean=0,
    double sigma=1
){
    return [=] (double x) -> double {
        return 1/(sigma*std::sqrt(2*M_PI)) * std::exp (-std::pow((x-mean)/sigma,2)/2);
    };
}

inline double
normal_PDF (double x) {
    return 1/std::sqrt(2*M_PI) * std::exp (-x*x/2);
}

inline auto
normal_CDF (
    double x
){
    return (std::erf(x/std::sqrt(2))+1) / 2;
}

inline
std::vector <double>
linspace (unsigned n,double min,double max) {
    switch(n) {
        case 0: return {};
        case 1: if(min==max) return {min}; else throw std::invalid_argument("linspace: n=1 and min!=max");
        default: {
            std::vector<double> v(n,NAN);
            v[0] = min;
            for( unsigned i=1; i<n-1; i++)
                v[i] = min + (max-min)* i/double(n-1);
            v[n-1] = max;
            return v;
        }
    }
}

template <typename F>
auto derivative1 (F f,double h) {
    return [=] (double x) {return (f(x+h/2) - f(x-h/2)) / h;};
}

template <typename F>
auto derivative2 (F f,double h) {
    return [=] (double x) {
        const double
            m = f (x-h),
            z = f (x  ),
            p = f (x+h),
            d = (m - 2*z + p) / std::pow(h,2);
        return d;
    };
}

Result solver (
    double (*f) (double, void*),
    void *data,
    const Parameter &p,
    Options opts = Options {}
);

template <typename F>
Result solver (
    F f,
    const Parameter &p,
    const Options &opts = Options {}
){
    Wrapper w {f};
    return solver (&w.Call, &w, p, opts);
}

// axx + bx + c = 0
inline
std::pair <double, double>
quadratic_equation_roots (double a, double b, double c) {
    const double
        D  = std::sqrt (b*b-4*a*c),
        x1 = (-b+D) / (2*a),
        x2 = (-b-D) / (2*a);
    return std::make_pair (x1, x2);
}

}
