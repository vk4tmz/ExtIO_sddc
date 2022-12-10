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

char imagefile[] = "/home/drifter/projects/ExtIO_sddc_vk4tmz/SDDC_FX3.img";
unsigned char* res_data = nullptr;
uint32_t res_size = 0;

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
uint32_t loadFirmwareImage(char *imagefile, unsigned char **res_data, uint32_t *res_size) {
   // open the firmware
    FILE *fp = fopen(imagefile, "rb");
    if (fp == nullptr)
    {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    *res_size = ftell(fp);
    *res_data = (unsigned char*)malloc(*res_size);
    fseek(fp, 0, SEEK_SET);
    if (fread(*res_data, 1, *res_size, fp) != *res_size)
        return -1;

    return *res_size;
}

bool isBootloaderFirmwareInuse(char * dev_desc) {
    return (strstr(dev_desc, "WestBridge") != NULL);
}  

SoapySDR::KwargsList findSddcSDR(const SoapySDR::Kwargs &args)
{
    SoapySDR::KwargsList results;
    char lblstr[128];

    auto Fx3 = CreateUsbHandler();

    if (loadFirmwareImage(imagefile, &res_data, &res_size) <= 0)
        return results;    

    unsigned char idx = 0;
//	int selected = 0;
    device_info_t dev_info;
    fprintf(stderr, "Enumerating Devices...\n");
    while (Fx3->Enumerate(idx, &dev_info, res_data, res_size)) {
        // https://en.wikipedia.org/wiki/West_Bridge
        int retry = 2;
        while (isBootloaderFirmwareInuse(dev_info.product) && retry-- > 0) {
            if (Fx3->Open(idx, res_data, res_size))
                Fx3->Close();
            Fx3->Enumerate(idx, &dev_info, res_data, res_size); // if it enumerates as BootLoader retry
        }

        snprintf(lblstr, sizeof(lblstr), "%s:%s:%s", dev_info.manufacturer, dev_info.product, dev_info.serial_number);

        if (isBootloaderFirmwareInuse(dev_info.product)) {
            fprintf(stderr, "Skipped DEV IDX: [%d] Label: [%s] as Boot Loader Firmware still acitve.\n", idx, lblstr);
            continue;
        }
        
        fprintf(stderr, "Found DEV IDX: [%d] Label: [%s]\n", idx, lblstr);

        SoapySDR::Kwargs dev;
        dev["serial"] = dev_info.serial_number;
        const bool serialMatch = args.count("serial") == 0 or args.at("serial") == dev["serial"];
        if (serialMatch) {
            fprintf(stderr, "Found Match... Dev Idx: [%d]\n", idx);

            dev["label"] = lblstr;
            results.push_back(dev);
            _cachedResults[dev["serial"]] = dev;
        }

        // Attempt Next Dev Idx
        idx++;
    }
    devicelist.numdev = idx;

    fprintf(stderr, "Num Devices Found: [%d]\n", devicelist.numdev);
    
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
