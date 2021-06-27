#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdio>

#include "YieldCurve.hpp"
#include "Instrument.hpp"
#include "ZeroCouponBond.hpp"
#include "ForwardRateAgreement.hpp"
#include "Swap.hpp"

// extrapolation: constant from the last point
//
// the curve internal time is defined as a real number,
// because of the integral(r*dt)
// the curves starts at t=0 (by definition)
//
// on the curve construction, we create a new curve
// from the previous one

TEST_CASE("zcb"){
    YieldCurve curve;

    curve
        .Add (ZeroCouponBond(2,0.8))
        .Add (ZeroCouponBond(1,0.9))
        .Add (ZeroCouponBond(5,0.6))
        .Build ()
    ;
    curve.Print();
}

TEST_CASE("fra"){
    YieldCurve curve;

    curve
        .Add (ForwardRateAgreement(0,1,0.01))
        .Add (ForwardRateAgreement(0.5,2,0.02))
        .Add (ForwardRateAgreement(0,3,0.03))
        .Build ()
    ;
    curve.Print();
}

TEST_CASE("swap"){

    YieldCurve
        curve_for_float_leg,
        curve_discount;

    curve_for_float_leg
        .Add (ForwardRateAgreement(0,1,0.01))
        .Add (ForwardRateAgreement(0.5,2,0.02))
        .Add (ForwardRateAgreement(0,3,0.03))
        .Build ()
    ;
    curve_for_float_leg.Print();


    Swap swap1;
    if(1) {
        // Finalyze the swap

        LegFixed &lfix = swap1.lfix;
        lfix.t0 = 0;
        lfix.dt = 1;
        lfix.n = 2;
        lfix.rate = 0.05;

        LegFloat &lflt = swap1.lflt;
        lflt.t0 = 0;
        lflt.dt = 0.5;
        lflt.n = 2;
        lflt.curve = &curve_for_float_leg;
    }

    curve_discount
        .Add (swap1)
        .Build ()
    ;
    curve_discount.Print();
}