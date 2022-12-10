
#include "SoapySddcSDR.hpp"

SoapySddcSDR::SoapySddcSDR(const SoapySDR::Kwargs &args)
{
    if (args.count("serial") == 0) throw std::runtime_error("no available SDDC SDR devices found");

    SoapySDR_logf(SOAPY_SDR_DEBUG, "SoapySddcSDR() - Initialization");
    selectDevice(args.at("serial"),
                 args.count("mode") ? args.at("mode") : "");    
}

SoapySddcSDR::~SoapySddcSDR(void)
{
}

void SoapySddcSDR::selectDevice(const std::string &serial,
                                const std::string &mode)
{
    serNo = serial;

    if (mode.empty() || (mode == "HF")) {
        rfMode = HFMODE;
    }
    else if (mode == "VHF") {
        rfMode = VHFMODE;
    } else {
        throw std::runtime_error("RF Mode is invalid: [" + mode + "]");
    }

    SoapySDR_logf(SOAPY_SDR_DEBUG, "serNo: [%s]", serNo.c_str());
    SoapySDR_logf(SOAPY_SDR_DEBUG, "rfMode: [%d]", rfMode);
}