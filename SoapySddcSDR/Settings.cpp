
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

    sampleRate = (args.count("rate") == 0) ? DEFAULT_SAMPLE_RATE : stoi(args.at("rate"));
    sampleRateIdx = convertToSampleRateIdx(sampleRate);

    attIdx = (args.count("rate") == 0) ? DEFAULT_SAMPLE_RATE : stoi(args.at("attidx"));

    selectDevice(args.at("idx"),
                 args.at("serial"));    
}

SoapySddcSDR::~SoapySddcSDR(void)
{
}

double SoapySddcSDR::getFreqCorrectionFactor()
{
    if (RadioHandler.GetmodeRF() == VHFMODE) {
	    return 1.0 + (ppmVHF / 1e6);
    }
    else {
        return 1.0 + (ppmHF / 1e6);
    } 
}

int SoapySddcSDR::convertToSampleRateIdx(double samplerate) {
    if      (samplerate <= 2000000) return 4;
    else if (samplerate <= 4000000) return 3;
    else if (samplerate <= 8000000) return 2;
    else if (samplerate <= 16000000) return 1;
    else if (samplerate <= 32000000) return 0;
    else
        return -1;
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

void SoapySddcSDR::selectDevice(const std::string idx,
                                const std::string &serial)
{
    serNo = serial;
    deviceId = stoi(idx);
    dev = _cachedResults[serial];

    SoapySDR_logf(SOAPY_SDR_DEBUG, "serNo: [%s]", serNo.c_str());
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Selected Device ID: [%d]", deviceId);
    gbInitHW = Fx3->Open(deviceId, fwImage.res_data, fwImage.res_size) &&
				RadioHandler.Init(Fx3, Callback); // Check if it there hardware
  
    if (!gbInitHW)
    {
        throw std::runtime_error("Failed to load Open and Initialize Device.");
    }

}

/*******************************************************************
 * Identification API
 ******************************************************************/

std::string SoapySddcSDR::getDriverKey(void) const
{
    return "SddcSDR";
}

std::string SoapySddcSDR::getHardwareKey(void) const
{
    return dev.at("label");
}

SoapySDR::Kwargs SoapySddcSDR::getHardwareInfo(void) const 
{
    return dev;
}

/*******************************************************************
 * Channels API
 ******************************************************************/

size_t SoapySddcSDR::getNumChannels(const int dir) const
{
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

bool SoapySddcSDR::getFullDuplex(const int direction, const size_t channel) const
{
    return false;
}

/*******************************************************************
 * Antenna API
 ******************************************************************/

std::vector<std::string> SoapySddcSDR::listAntennas(const int direction, const size_t channel) const
{
    std::vector<std::string> antennas;
    antennas.push_back("RX");
    return antennas;
}

void SoapySddcSDR::setAntenna(const int direction, const size_t channel, const std::string &name)
{
    if (direction != SOAPY_SDR_RX)
    {
        throw std::runtime_error("setAntena failed: SDDC SDR only supports RX");
    }
}

std::string SoapySddcSDR::getAntenna(const int direction, const size_t channel) const
{
    return "RX";
}

/*******************************************************************
 * Frontend corrections API
 ******************************************************************/

bool SoapySddcSDR::hasDCOffsetMode(const int direction, const size_t channel) const
{
    return false;
}

bool SoapySddcSDR::hasFrequencyCorrection(const int direction, const size_t channel) const {
    return true;
}

void SoapySddcSDR::setFrequencyCorrection(const int direction, const size_t channel, const double value) {
    std::lock_guard <std::mutex> lock(_general_state_mutex);
}

double SoapySddcSDR::getFrequencyCorrection(const int direction, const size_t channel) const {
    // TODO: need to handle const !!!
    // if (RadioHandler.GetmodeRF() == VHFMODE) {
    //     return ppmVHF;
    // }
    
    return ppmVHF;
}

/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapySddcSDR::listGains(const int direction, const size_t channel) const
{
    //list available gain elements,
    //the functions below have a "name" parameter
    std::vector<std::string> results;

    results.push_back("HF");
    results.push_back("VHF");

    return results;
}

bool SoapySddcSDR::hasGainMode(const int direction, const size_t channel) const
{
    return true;
}

void SoapySddcSDR::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    std::lock_guard <std::mutex> lock(_general_state_mutex);
}

bool SoapySddcSDR::getGainMode(const int direction, const size_t channel) const
{
    return false;
}

void SoapySddcSDR::setGain(const int direction, const size_t channel, const double value)
{
    //set the overall gain by distributing it across available gain elements
    //OR delete this function to use SoapySDR's default gain distribution algorithm...
    SoapySDR::Device::setGain(direction, channel, value);
}

void SoapySddcSDR::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    if (name == "HF")
    {
        SoapySDR_log(SOAPY_SDR_WARNING, "SET HF GAIN  - TODO");
    }
    else if (name == "VHF")
    {
        SoapySDR_log(SOAPY_SDR_WARNING, "SET VHF GAIN  - TODO");
    }
}

double SoapySddcSDR::getGain(const int direction, const size_t channel, const std::string &name) const
{
    if (name == "HF")
    {
        SoapySDR_log(SOAPY_SDR_WARNING, "GET HF GAIN  - TODO");
    }
    else if (name == "VHF")
    {
        SoapySDR_log(SOAPY_SDR_WARNING, "GET VHF GAIN  - TODO");
    }

   return 0.0;
}

SoapySDR::Range SoapySddcSDR::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
   if (name == "HF") {
      return SoapySDR::Range(20, 59);
   }
   else if (name == "VHF") {
      return SoapySDR::Range(20, 59);
   } 
   
   return SoapySDR::Range(0, 0);
}

/*******************************************************************
 * Frequency API
 ******************************************************************/

void SoapySddcSDR::setFrequency(
        const int direction,
        const size_t channel,
        const std::string &name,
        const double frequency,
        const SoapySDR::Kwargs &args)
{
    if (name == "RF")
    {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting center freq: %d", (uint32_t)frequency);
        int r = RadioHandler.TuneLO(frequency);
        if (r != 0)
        {
            throw std::runtime_error("setFrequency failed");
        }
        LOfreq = frequency;
    }

    if (name == "CORR")
    {
        throw std::runtime_error("TODO: need to handle this setFrequencyCorrection failed");
    }
}

double SoapySddcSDR::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    if (name == "RF")
    {
        return (double) LOfreq;
    }

    if (name == "CORR")
    {
        // TODO: need to handle const and both
        return ((double) ppmVHF);
    }

    return 0;
}

std::vector<std::string> SoapySddcSDR::listFrequencies(const int direction, const size_t channel) const
{
    std::vector<std::string> names;
    names.push_back("RF");
    names.push_back("CORR");
    return names;
}

SoapySDR::RangeList SoapySddcSDR::getFrequencyRange(
        const int direction,
        const size_t channel,
        const std::string &name) const
{
    SoapySDR::RangeList results;
    if (name == "RF")
    {
        results.push_back(SoapySDR::Range(10000, 1764000000));
    }
    if (name == "CORR")
    {
        results.push_back(SoapySDR::Range(-1000, 1000));
    }
    return results;
}

SoapySDR::ArgInfoList SoapySddcSDR::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList freqArgs;

    // TODO: frequency arguments

    return freqArgs;
}