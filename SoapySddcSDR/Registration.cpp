#include <iostream>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include "../Core/config.h"
#include "../Core/RadioHandler.h"
#include "../Core/FX3Class.h"
#include "SoapySddcSDR.hpp"


struct DevContext 
{
	unsigned char numdev;
	char dev[MAXNDEV][MAXDEVSTRLEN];
};

static std::map<std::string, SoapySDR::Kwargs> _cachedResults;

DevContext  devicelist; // list of FX3 devices

/***********************************************************************
 * Device interface
 **********************************************************************/
class SddcSDR : public SoapySDR::Device
{
    //Implement constructor with device specific arguments...

    //Implement all applicable virtual methods from SoapySDR::Device
};

/***********************************************************************
 * Find available devices
 **********************************************************************/


bool isBootloaderFirmwareInuse(char * dev_desc) {
    return (strstr(dev_desc, "WestBridge") != NULL);
}  

SoapySDR::KwargsList findSddcSDR(const SoapySDR::Kwargs &args)
{
    SoapySDR::KwargsList results;
    char lblstr[128];

    // TODO: Remove once done 
    SoapySDR_setLogLevel(SOAPY_SDR_DEBUG);

    auto Fx3 = CreateUsbHandler();

    if (!SoapySddcSDR::loadFirmwareImage(args))
        return results;    

    unsigned char idx = 0;
//	int selected = 0;
    device_info_t dev_info;
    SoapySDR_logf(SOAPY_SDR_INFO, "Enumerating Devices...");
    while (Fx3->Enumerate(idx, &dev_info, SoapySddcSDR::fwImage.res_data, SoapySddcSDR::fwImage.res_size)) {
        // https://en.wikipedia.org/wiki/West_Bridge
        int retry = 2;
        while (isBootloaderFirmwareInuse(dev_info.product) && retry-- > 0) {
            if (Fx3->Open(idx, SoapySddcSDR::fwImage.res_data, SoapySddcSDR::fwImage.res_size))
                Fx3->Close();
            Fx3->Enumerate(idx, &dev_info, SoapySddcSDR::fwImage.res_data, SoapySddcSDR::fwImage.res_size); // if it enumerates as BootLoader retry
        }

        snprintf(lblstr, sizeof(lblstr), "%s:%s:%s", dev_info.manufacturer, dev_info.product, dev_info.serial_number);

        if (isBootloaderFirmwareInuse(dev_info.product)) {
            SoapySDR_logf(SOAPY_SDR_WARNING, "  - Skipped DEV IDX: [%d] Label: [%s] as Boot Loader Firmware still acitve.", idx, lblstr);
            continue;
        }
        
        SoapySDR_logf(SOAPY_SDR_INFO, "  - Found DEV IDX: [%d] Label: [%s]", idx, lblstr);

        SoapySDR::Kwargs dev;
        dev["serial"] = dev_info.serial_number;
        const bool serialMatch = args.count("serial") == 0 or args.at("serial") == dev["serial"];
        if (serialMatch) {
            if (args.count("serial") > 0) {
                SoapySDR_logf(SOAPY_SDR_DEBUG, "  - Matched Dev IDX: [%d] to requested serial: [%s] ", idx, dev["serial"].c_str());
            } else {
                SoapySDR_logf(SOAPY_SDR_DEBUG, "  - Matched Dev IDX: [%d]", idx);
            }

            dev["label"] = lblstr;
            results.push_back(dev);
            _cachedResults[dev["serial"]] = dev;
        }

        // Attempt Next Dev Idx
        idx++;
    }
    devicelist.numdev = idx;

    SoapySDR_logf(SOAPY_SDR_INFO, "Num Devices Found: [%d]", devicelist.numdev);
    
    return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeSddcSDR(const SoapySDR::Kwargs &args)
{
     return new SoapySddcSDR(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerSddcSDR("sddcsdr", &findSddcSDR, &makeSddcSDR, SOAPY_SDR_ABI_VERSION);
