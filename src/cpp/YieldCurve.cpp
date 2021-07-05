#include "YieldCurve.hpp"
#include "Instrument.hpp"
#include "minimizer.hpp"

YieldCurve & YieldCurve::Add (const Instrument &x) {
    instruments[x.GetMaturity()].reset(x.Clone());
    return *this;
}

YieldCurve &
YieldCurve::BuildPiecewiseConstant (void) {
    *this = math::Interpolator1D();
    // for(auto [t,instr]: GetInstruments())
    //     instr->AddToCurve(*this);
    for(std::map <float,std::shared_ptr<Instrument>>::const_iterator it=instruments.cbegin(); it!=instruments.cend(); it++)
        it->second->AddToCurve(*this);
    return *this;
}

YieldCurve &
YieldCurve::Build (
    math::Interpolator1D::Type itype
){
    math::Options opts;
    opts.eps_abs = 1e-5;
    opts.eps_rel = 1e-5;
    opts.iters = 1000 + instruments.size()*1000;
    return Build(itype,opts);
}

YieldCurve &
YieldCurve::Build (
    math::Interpolator1D::Type itype,
    const math::Options &opts
) {
    if(itype==math::Interpolator1D::Type::PiecewiseConstant)
        return BuildPiecewiseConstant();

    // grid on time
    std::vector<double> vx {0}; // always start with t=0 point
    // for(auto [t,instr]: GetInstruments()) {
    for(std::map <float,std::shared_ptr<Instrument>>::const_iterator it=instruments.cbegin(); it!=instruments.cend(); it++){
        const float &t = it->first;
        if(t>0)
            vx.push_back(t);
        else
            throw std::invalid_argument("YieldCurve::Build: instrument maturity must be >1");
    }

    std::vector<math::Parameter> pars (vx.size(),math::Parameter(0,1e-2));

    // unsigned pars_max = pars.size();

    // printf("-1-\n");
    //
    // printf("x=[");
    // for(auto x: vx)
    //     printf("%g,",x);
    // printf("]\n");

    // const float yield_to_infinity = 0; // FIXME

    auto func = [&] (const std::vector<double> &_vy) -> double {

        // for(unsigned i=pars_max;i<_vy.size(); i++)
        //     if(std::fabs(_vy[i]-pars[i])>pars[i].error) return NAN;

        std::vector<double> vy(_vy.begin(),_vy.end());
        // vy.push_back(yield_to_infinity);

        // printf("y=[");
        // for(auto x: vy)
        //     printf("%g,",x);
        // printf("]\n");

        // YieldCurve curve(std::span<double>(vx.data(),vx.size()),pars,_type);
        // printf("vx,vy sizes: %d %d    itype=%d\n",(int)vx.size(),(int)vy.size(),(int)(itype));

        *this = Interpolator1D (vx,vy,itype);
        // printf("ok!\n");

        // ft();
        // printf("C %g %g %g\n",curve.GetDiscountFactor(0),curve.GetDiscountFactor(3),curve.GetDiscountFactor(5));

        // printf("Calculating result... %d %d\n",(int)curve.GetX().size(),(int)curve.GetY().size());
        double result {0};
        // for(auto &[t,instr]: instruments) {
        for(std::map <float,std::shared_ptr<Instrument>>::const_iterator it=instruments.cbegin(); it!=instruments.cend(); it++)
            result += std::pow( it->second->Value()-it->second->Eval(*this), 2 );
        // printf("result=%g\n",result);
        return result;
    };
    math::Result r = math::minimize( func, pars, opts );

    if(!r) throw std::runtime_error(r.GetError());

    std::vector<double> vy;
    // for( auto y: r.x)
    for(std::vector<math::Parameter>::const_iterator it=r.x.cbegin(); it!=r.x.cend(); it++)
        vy.push_back(it->value);
    // vy.push_back(yield_to_infinity);

    if(vx.size()!=vy.size())
        throw std::logic_error("YieldCurve::Build: internal error");

    *this = math::Interpolator1D (vx,vy,itype);

    return *this;
}

void YieldCurve::Print(void) const {
    printf("YieldCurve interpolation type: %s size=%d\n",Name().c_str(),GetSize());


    std::cout << static_cast<math::Interpolator1D>(*this) << "\n";

    // for( auto [t,y]: yields) {
    //     printf("  t={}  yield={}\n",t,y);
    // }

    // if(iconst){
    //     printf("PiecewiseConstant (time,yield)=[ ");
    //     for(std::map<double,double>::const_iterator it=iconst->GetXY().cbegin(); it!=iconst->GetXY().cend(); it++ ){
    //         printf("(%g,%g) ",it->first,it->second);
    //     }
    //     printf("]\n");
    // }

    // for(auto [t,instr]: GetInstruments()) {
    for(std::map <float,std::shared_ptr<Instrument>>::const_iterator it=instruments.cbegin(); it!=instruments.cend(); it++){
        const Instrument *instr = it->second.get();
        if(!instr) continue;
        printf("%s\n",instr->About().c_str());
        printf("    market quote ... %g\n",instr->Value());
        printf("    caluclated ..... %g\n",instr->Eval(*this));
        printf("    difference ..... %g\n",instr->Value()-instr->Eval(*this));
        printf("\n");
    }
}
