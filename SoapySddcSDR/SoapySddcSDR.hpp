#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Types.h>
#include "../Core/config.h"


class SoapySddcSDR: public SoapySDR::Device
{
public:
    explicit SoapySddcSDR(const SoapySDR::Kwargs &args);

    ~SoapySddcSDR(void);

private:

    std::string serNo;
    rf_mode rfMode;

    void selectDevice(const std::string &serial,
                      const std::string &mode);

};