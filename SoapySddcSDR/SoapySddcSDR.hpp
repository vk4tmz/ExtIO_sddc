#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Types.h>
#include "../Core/config.h"
#include "../Core/RadioHandler.h"
#include "../Core/FX3Class.h"


struct FirmwareImage {
    char *imagefile = nullptr;
    unsigned char* res_data = nullptr;
    uint32_t res_size = 0;
};

typedef struct FirmwareImage FirmwareImage_t;

inline void freeFirmwareImage(FirmwareImage_t fwImage) {
    if (fwImage.imagefile != nullptr) {
        free(fwImage.imagefile);
        fwImage.imagefile = nullptr;
    }
    if (fwImage.res_data != nullptr) {
        free(fwImage.res_data);
        fwImage.res_data = nullptr;
    }
    fwImage.res_size = 0;
}

class SoapySddcSDR: public SoapySDR::Device
{
public:
    inline static FirmwareImage_t fwImage;
    inline static std::map<std::string, SoapySDR::Kwargs> _cachedResults;

    explicit SoapySddcSDR(const SoapySDR::Kwargs &args);

    ~SoapySddcSDR(void);
    static bool loadFirmwareImage(const SoapySDR::Kwargs &args);

private:

    std::string serNo;
    rf_mode rfMode;

    fx3class *Fx3 = CreateUsbHandler();
    RadioHandlerClass RadioHandler;
    bool gbInitHW = false;
    SoapySDR::Kwargs dev;
    int devIdx = -1;

    void selectDevice(const std::string &serial,
                      const std::string &mode);

};