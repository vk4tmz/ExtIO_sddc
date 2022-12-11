
#include <errno.h>
#include "SoapySddcSDR.hpp"

static void Callback(void* context, const float* data, uint32_t len)
{
    // TODO
}

SoapySddcSDR::SoapySddcSDR(const SoapySDR::Kwargs &args)
{
    if (args.count("serial") == 0) throw std::runtime_error("no available SDDC SDR devices found");

    if (!SoapySddcSDR::loadFirmwareImage(args)) {
        throw std::runtime_error("Failed to load firmware image.");
    }

    SoapySDR_logf(SOAPY_SDR_DEBUG, "SoapySddcSDR() - Initialization");
    selectDevice(args.at("serial"),
                 args.count("mode") ? args.at("mode") : "");    
}

SoapySddcSDR::~SoapySddcSDR(void)
{
}

bool SoapySddcSDR::loadFirmwareImage(const SoapySDR::Kwargs &args) {
    
    if (fwImage.imagefile != nullptr) {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Firmware Image: [%s] already cached.", fwImage.imagefile);
        return true;
    }

    if (args.count("fwimage") == 0) {
        throw std::runtime_error("FW Image File not provided. Please supply 'fwimage' argument.");
    }

    const char *imagefile = args.at("fwimage").c_str();
    SoapySDR_logf(SOAPY_SDR_INFO, "Using Firmware Image: [%s]", imagefile);

    // open the firmware
    FILE *fp = fopen(imagefile, "rb");
    if (fp == nullptr)
    {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Unable to load firmware - errorno: [%d]", errno);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    fwImage.res_size = ftell(fp);
    fwImage.res_data = (unsigned char*)malloc(fwImage.res_size);
    fwImage.imagefile = strdup(imagefile);
    fseek(fp, 0, SEEK_SET);
    size_t res = fread(fwImage.res_data, 1, fwImage.res_size, fp);
    fclose(fp);

    if (res != fwImage.res_size) {
        freeFirmwareImage(fwImage);
        return false;
    }

    return true;
}

void SoapySddcSDR::selectDevice(const std::string &serial,
                                const std::string &mode)
{
    serNo = serial;
    dev = _cachedResults[serial];
    devIdx = stoi(dev["idx"]);

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
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Selected Device IDX: [%d]", devIdx);
    gbInitHW = Fx3->Open(devIdx, fwImage.res_data, fwImage.res_size) &&
				RadioHandler.Init(Fx3, Callback); // Check if it there hardware
	
#ifdef _DEBUG
			RadioHandler.EnableDebug( printf_USB_cb , GetConsoleInput);
#endif 
  
    if (!gbInitHW)
    {
        throw std::runtime_error("Failed to load Open and Initialize Device.");
    }

    // device = rspDevs[devIdx];
    // hwVer = device.hwVer;

    // SoapySDR_logf(SOAPY_SDR_INFO, "devIdx: %d", devIdx);
    // SoapySDR_logf(SOAPY_SDR_INFO, "SerNo: %s", device.SerNo);
    // SoapySDR_logf(SOAPY_SDR_INFO, "hwVer: %d", device.hwVer);

    
    // TODO - Close / Release
    RadioHandler.Stop();
    RadioHandler.Close();
    Fx3->Close();
}

