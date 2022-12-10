
#include "SoapySddcSDR.hpp"

SoapySddcSDR::SoapySddcSDR(const SoapySDR::Kwargs &args)
{
    if (args.count("serial") == 0) throw std::runtime_error("no available SDDC SDR devices found");

    fprintf(stderr, "SoapySddcSDR() - Initialization,");
    // selectDevice(args.at("serial"),
    //              args.count("mode") ? args.at("mode") : "",
    //              args.count("antenna") ? args.at("antenna") : "");    
}

SoapySddcSDR::~SoapySddcSDR(void)
{
}