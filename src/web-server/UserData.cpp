#include <sstream>
#include <fmt/ostream.h>

#include "UserData.hpp"
#include "log.hpp"
#include "DB.hpp"

#include "YieldCurve.hpp"
#include "ForwardRateAgreement.hpp"

using json = nlohmann::json;
using namespace date::literals;
using namespace std::chrono_literals;

namespace {
const char
    *payload = "payload",
    *request_id = "request_id";
}

UserData::UserData(void) {
    logger->debug("UserData Ctor");
}

UserData::~UserData(void) {
    logger->debug("UserData Dtor");
}

json UserData::Call(const json &data ) noexcept {
    json rv, pld;
    std::string type;

    try {
        logger->debug("{} {}",__PRETTY_FUNCTION__,data.dump(4));

        type = data.at("type").get<std::string>();
        pld[request_id] = data.at(payload).at(request_id).get<int>();

        if( type == "echo" ) {
            pld = data.at(payload);
        } else if( type == "get_quotes" ) {
            get_quotes(data,pld);
        } else if( type == "build_curve" ) {
            build_curve(data,pld);
        } else {
            throw std::invalid_argument("The request is not implemented by the C++ engine.");
        }

        // Normal return
        rv = {
            {"type",type+"_OK"}
        };

        if( not pld.is_null() )
            rv[payload] = std::move(pld);

    } catch( const std::exception &e ) {
        logger->error("{}",e.what());
        // Error return
        rv = json({
            {"type",type+"_ERROR"},
            {"error",true},
            {payload,{
                {"name",e.what()}
            }}
        });
    }

    return rv;
}

void UserData::get_quotes (const json &data, json &pld) {
    // connect to the DB
    // FIXME:config: constants in the code
    DB db("mongodb://localhost:27017","MarketData","b20190612");

    // get the date, return the date
    const std::string the_date_str = data.at(payload).at("date");
    date::year_month_day the_date;
    std::istringstream(the_date_str) >> date::parse("%F",the_date);
    pld["date"] = the_date_str;

    // get quotes from the db
    auto db_data = db.LoadData(the_date);
    logger->debug("On {} we have {} instruments.\n",to_str(the_date),db_data.size());

    // store quotes
    json &quotes = pld["quotes"];
    for(auto &[name,value]: db_data)
        quotes.push_back(json{name,value});
}

void UserData::build_curve (const json &data, json &pld) {
    YieldCurve curve;

    const std::string the_date_str = data.at(payload).at("date");
    date::year_month_day the_date;
    std::istringstream(the_date_str) >> date::parse("%F",the_date);
    auto instruments = data.at(payload).at("instruments");

    debug("build_curve 1");

    for(auto instr: instruments){
        auto type = instr.at("type").get<std::string>();
        if(type=="FRA") {
            int
                start  = instr.at("start" ).get<int>(),
                length = instr.at("length").get<int>();
            double
                quote = instr.at("quote").get<double>();
            curve.Add(ForwardRateAgreement(start/12.,length/12.,quote));
        }else{
            debug("skipping: {}",instr.dump());
        }
        // debug("{}",instr.dump());
    }
    debug("build_curve 2");

    auto result = curve.Build();
    curve.Print();

    auto points = data.at(payload).at("points").get<int>();

    if(curve.GetX().empty())
        throw std::runtime_error("Failed to build the curve");
    auto tmax = curve.GetX().back();

    json vx,vy;
    for(int i=0; i<points; i++){
        auto t = (tmax*i)/(points-1);
        vx.push_back(t);
        vy.push_back(curve(t));
    }

    pld["plot"] = json {{"x",vx},{"y",vy}};
    pld["resuts"] = nullptr;
}
