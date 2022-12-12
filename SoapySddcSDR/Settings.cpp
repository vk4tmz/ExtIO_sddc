
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


/*******************************************************************
 * Sample Rate API
 ******************************************************************/

void SoapySddcSDR::setSampleRate(const int direction, const size_t channel, const double rate)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting sample rate: %d", sampleRate);
    RadioHandler.UpdateSampleRate(rate);
    double actSampleRate = RadioHandler.getSampleRate();

    if (actSampleRate != rate) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Failed to change Sample Rate - Requested: [%d] - Current: [%d]", (uint32_t)rate, (uint32_t) actSampleRate);
    }
}

double SoapySddcSDR::getSampleRate(const int direction, const size_t channel) const
{
    return sampleRate;
}

std::vector<double> SoapySddcSDR::listSampleRates(const int direction, const size_t channel) const
{
    std::vector<double> results;

    results.push_back( 2000000);
    results.push_back( 4000000);
    results.push_back( 8000000);
    results.push_back(16000000);
    results.push_back(32000000);

    return results;
}

SoapySDR::RangeList SoapySddcSDR::getSampleRateRange(const int direction, const size_t channel) const
{
    SoapySDR::RangeList results;

    results.push_back(SoapySDR::Range(2000000, 32000000));
 
    return results;
}

void SoapySddcSDR::setBandwidth(const int direction, const size_t channel, const double bw)
{
    // TODO: Review
}

double SoapySddcSDR::getBandwidth(const int direction, const size_t channel) const
{
    return sampleRate;
}

std::vector<double> SoapySddcSDR::listBandwidths(const int direction, const size_t channel) const
{
    std::vector<double> results;

    return results;
}

SoapySDR::RangeList SoapySddcSDR::getBandwidthRange(const int direction, const size_t channel) const
{
    SoapySDR::RangeList results;

    return results;
}


/*******************************************************************
 * Settings API
 ******************************************************************/

SoapySDR::ArgInfoList SoapySddcSDR::getSettingInfo(void) const
{
    SoapySDR::ArgInfoList setArgs;

    SoapySDR::ArgInfo vgaArg;

    vgaArg.key = "vga";
    vgaArg.value = "true";
    vgaArg.name = "VGA";
    vgaArg.description = "Variable Gain Amplifier";
    vgaArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(vgaArg);

    SoapySDR::ArgInfo pgaArg;

    pgaArg.key = "pga";
    pgaArg.value = "true";
    pgaArg.name = "PGA";
    pgaArg.description = "PGA Front End Amplifier";
    pgaArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(pgaArg);

    SoapySDR::ArgInfo ditherArg;

    ditherArg.key = "dither";
    ditherArg.value = "true";
    ditherArg.name = "Dither";
    ditherArg.description = "Dither";
    ditherArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(ditherArg);

    SoapySDR::ArgInfo randArg;

    randArg.key = "rand";
    randArg.value = "true";
    randArg.name = "Rand";
    randArg.description = "Rand";
    randArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(randArg);

    SoapySDR::ArgInfo biasTVHFArg;

    biasTVHFArg.key = "bias_tee_vhf";
    biasTVHFArg.value = "true";
    biasTVHFArg.name = "BiasT (VHF)";
    biasTVHFArg.description = "BiasT (VHF)";
    biasTVHFArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(biasTVHFArg);

    SoapySDR::ArgInfo biasTHFArg;

    biasTHFArg.key = "bias_tee_hf";
    biasTHFArg.value = "true";
    biasTHFArg.name = "BiasT (HF)";
    biasTHFArg.description = "BiasT (HF)";
    biasTHFArg.type = SoapySDR::ArgInfo::BOOL;
 
   
    setArgs.push_back(biasTHFArg);

    SoapySDR::ArgInfo iqSwapArg;

    iqSwapArg.key = "iq_swap";
    iqSwapArg.value = "false";
    iqSwapArg.name = "I/Q Swap";
    iqSwapArg.description = "I/Q Swap Mode";
    iqSwapArg.type = SoapySDR::ArgInfo::BOOL;

    setArgs.push_back(iqSwapArg);

    SoapySDR_logf(SOAPY_SDR_DEBUG, "SETARGS?");

    return setArgs;
}

void SoapySddcSDR::writeSetting(const std::string &key, const std::string &value)
{
    if (key == "vga_vhf")
    {
        vgaVHF = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "VGA VHF: %s", vgaVHF ? "true" : "false");
    }
    else if (key == "vga_hf")
    {
        vgaHF = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "VGA HF: %s", vgaHF ? "true" : "false");
    }
    else if (key == "pga")
    {
        pga = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "PGA: %s", pga ? "true" : "false");
    }
    else if (key == "dither")
    {
        dither = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "DITHER: %s", dither ? "true" : "false");
    }
    else if (key == "rand")
    {
        rand = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "RAND: %s", rand ? "true" : "false");
    }
    else if (key == "bias_tee_vhf")
    {
        biasTeeVHF = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "BiasT VHF: %s", biasTeeVHF ? "true" : "false");
    }
    else if (key == "bias_tee_hf")
    {
        biasTeeHF = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "BiasT HF: %s", biasTeeHF ? "true" : "false");
    }
    else if (key == "iq_swap")
    {
        iqSwap = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "RTL-SDR I/Q swap: %s", iqSwap ? "true" : "false");
    }
}

std::string SoapySddcSDR::readSetting(const std::string &key) const
{
    if (key == "vga_vhf")
    {
        return BoolToString(vgaVHF);
    }
    else if (key == "vga_hf")
    {
        return BoolToString(vgaHF);
    }
    else if (key == "pga")
    {
        return BoolToString(pga);
    }
    else if (key == "dither")
    {
        return BoolToString(dither);
    }
    else if (key == "rand")
    {
        return BoolToString(rand);
    }
    else if (key == "bias_tee_vhf")
    {
        return BoolToString(biasTeeVHF);
    }
    else if (key == "bias_tee_hf")
    {
        return BoolToString(biasTeeHF);
    }
    else if (key == "iq_swap")
    {
        return BoolToString(iqSwap);
    }

    SoapySDR_logf(SOAPY_SDR_WARNING, "Unknown setting '%s'", key.c_str());
    return "";
}
