#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Types.h>


class SoapySddcSDR: public SoapySDR::Device
{
public:
    explicit SoapySddcSDR(const SoapySDR::Kwargs &args);

    ~SoapySddcSDR(void);
};