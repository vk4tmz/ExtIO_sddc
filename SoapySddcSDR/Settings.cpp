
#include <errno.h>
#include "SoapySddcSDR.hpp"

static void Callback(void* context, const float* data, uint32_t len)
{
    //SoapySDR_logf(SOAPY_SDR_DEBUG, "CB: data len [%d]", len);
}

SoapySddcSDR::SoapySddcSDR(const SoapySDR::Kwargs &args)
{
    if (args.count("serial") == 0) throw std::runtime_error("no available SDDC-SDR devices found");

    if (!SoapySddcSDR::loadFirmwareImage(args)) {
        throw std::runtime_error("Failed to load firmware image.");
    }

    SetOverclock(DEFAULT_ADC_FREQ);

    selectDevice(args.at("idx"),
                 args.at("serial"));    
}

SoapySddcSDR::~SoapySddcSDR(void)
{
    Fx3->Close();
}

int SoapySddcSDR::getSampleRateIdx(void)
{
	if (RadioHandler.GetmodeRF() == VHFMODE)
		return srateIdxVHF;
	else
		return srateIdxHF;
}

int SoapySddcSDR::setSampleRateIdx(int srate_idx)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting sample rate idx: [%d].", srate_idx);
	if (RadioHandler.GetmodeRF() == VHFMODE)
		srateIdxVHF = srate_idx;
	else
		srateIdxHF = srate_idx;

    if (streamActive)
        RadioHandler.Start(srate_idx);

    setFrequency(SOAPY_SDR_RX, 0, LOfreq);

    return 0;
}

int SoapySddcSDR::SetOverclock(uint32_t adcfreq) {

    if (adcfreq > MAX_ADC_FREQ) {
        SoapySDR_logf(SOAPY_SDR_WARNING, "Requested ADC sample rate: [%ld] above MAX_ADC_FREQ: [%ld], using the default maximum.", adcfreq, MAX_ADC_FREQ);
        adcfreq = MAX_ADC_FREQ;
    }
    if (adcfreq < MIN_ADC_FREQ) {
        SoapySDR_logf(SOAPY_SDR_WARNING, "Requested ADC sample rate: [%ld] below MIN_ADC_FREQ: [%ld], using the default minimum.", adcfreq, MIN_ADC_FREQ);
        adcfreq = MIN_ADC_FREQ;
    }
    adcnominalfreq = adcfreq;

    RadioHandler.UpdateSampleRate(adcfreq);
    uint32_t actSampleRate = RadioHandler.getSampleRate();

    if (actSampleRate != adcfreq) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Failed to change ADC Sample Rate - Requested: [%d] - Current: [%d]", adcfreq, (uint32_t) actSampleRate);
    }

    int index = getSampleRateIdx();
	double rate;
	while (getSrates(index, &rate) == -1)
	{
		index--;
	}
    
	setSampleRateIdx(index);

    return 0;
}

int SoapySddcSDR::convertToSampleRateIdx(uint32_t samplerate) {
    uint64_t rate = 0;

    if      (samplerate <= 2000000) rate = 0;
    else if (samplerate <= 4000000) rate = 1;
    else if (samplerate <= 8000000) rate = 2;
    else if (samplerate <= 16000000) rate = 3;
    else if (samplerate <= 32000000) rate = 4;
    else {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Invalid sample rate: [%ld], defaulting to 2MHz", samplerate);
    }

    return rate;
}

int SoapySddcSDR::getSrates(int srate_idx, double *samplerate) const
{
    double div = pow(2.0, srate_idx);
	double srateM = div * 2.0;
	double bwmin = adcnominalfreq / 64.0;
	if (adcnominalfreq > N2_BANDSWITCH) bwmin /= 2.0;
	double srate = bwmin * srateM;

	if (srate / adcnominalfreq * 2.0 > 1.1)
		return -1;

    *samplerate = srate * getFrequencyCorrectionFactor();
	SoapySDR_logf(SOAPY_SDR_DEBUG, "getSrate idx [%d]  rate: [%ld]", srate_idx, (uint32_t) *samplerate);
	return 0;
}

//-----------

int SoapySddcSDR::GetAttenuators(int atten_idx, float* attenuation) const
{
	// fill in attenuation
	// use positive attenuation levels if signal is amplified (LNA)
	// use negative attenuation levels if signal is attenuated
	// sort by attenuation: use idx 0 for highest attenuation / most damping
	// this functions is called with incrementing idx
	//    - until this functions return != 0 for no more attenuator setting

	const float *steps;
	int max_step = RadioHandler.GetRFAttSteps(&steps);
	if (atten_idx < max_step) {
		*attenuation = steps[atten_idx];
		return 0;
	}

	return 1;
}

int SoapySddcSDR::GetActualAttIdx(void) const
{
	int AttIdx;
	if (RadioHandler.GetmodeRF() == VHFMODE)
		AttIdx = attIdxVHF;
	else
		AttIdx = attIdxHF;

	const float *steps;
	int max_step = RadioHandler.GetRFAttSteps(&steps);
	if (AttIdx >= max_step)
	{
		AttIdx = max_step - 1;
	}

	return AttIdx;
}

int SoapySddcSDR::SetAttenuator(int atten_idx)
{
    EnterFunction1(atten_idx);
	RadioHandler.UpdateattRF(atten_idx);
	if (RadioHandler.GetmodeRF() == VHFMODE)
		attIdxVHF = atten_idx;
	else
		attIdxHF = atten_idx;

	return 0;
}

//
// MGC
// sort by ascending gain: use idx 0 for lowest gain
// this functions is called with incrementing idx
//    - until this functions returns != 0, which means that all gains are already delivered
int SoapySddcSDR::getMGCs(int mgc_idx, float * gain) const
{
	const float *steps;
	int max_step = RadioHandler.GetIFGainSteps(&steps);
	if (mgc_idx < max_step) {
		*gain = steps[mgc_idx];
		return 0;
	}

	return 1;
}

int SoapySddcSDR::getActualMgcIdx(void) const
{
	int MgcIdx;
	if (RadioHandler.GetmodeRF() == VHFMODE)
		MgcIdx = mgcIdxVHF;
	else
		MgcIdx = mgcIdxHF;

	const float *steps;
	int max_step = RadioHandler.GetIFGainSteps(&steps);
	if (MgcIdx >= max_step)
	{
		MgcIdx = max_step - 1;
	}

	return MgcIdx;
}

int SoapySddcSDR::setMGC(int mgc_idx)
{
	RadioHandler.UpdateIFGain(mgc_idx);
	if (RadioHandler.GetmodeRF() == VHFMODE)
		mgcIdxVHF = mgc_idx;
	else
		mgcIdxHF = mgc_idx;

	return 0;
}


//-----------



bool SoapySddcSDR::loadFirmwareImage(const SoapySDR::Kwargs &args) {
    
    if (fwImage.imagefile != nullptr) {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Firmware Image: [%s] already cached.", fwImage.imagefile);
        return true;
    }

    char *imagefile = getenv("FX3_FW_IMG");

    if (imagefile == nullptr) {
        throw std::runtime_error("Please set environment variable FX3_FW_IMG to location of firmware image.");
    }

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

    //r2iqControl = new r2iqBasicControlClass();
    r2iqControl = nullptr;

    SoapySDR_logf(SOAPY_SDR_DEBUG, "serNo: [%s]", serNo.c_str());
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Selected Device ID: [%d]", deviceId);
    gbInitHW = Fx3->Open(deviceId, fwImage.res_data, fwImage.res_size) &&
				RadioHandler.Init(Fx3, Callback, r2iqControl); // Check if it there hardware
  
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
        throw std::runtime_error("setAntena failed: SDDC-SDR only supports RX");
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

bool SoapySddcSDR::hasFrequencyCorrection(const int direction, const size_t channel) const 
{
    return true;
}

double SoapySddcSDR::getFrequencyCorrectionFactor() const
{
    return 1.0 + (getFrequencyCorrection(SOAPY_SDR_RX, 0) / 1e6); 
}

double SoapySddcSDR::getFrequencyCorrection(const int direction, const size_t channel) const
{
    if (RadioHandler.GetmodeRF() == VHFMODE) {
	    return ppmVHF;
    }
    else {
        return ppmHF;
    } 
}

void SoapySddcSDR::setFrequencyCorrection(const int direction, const size_t channel, const double value)
{
    if (RadioHandler.GetmodeRF() == VHFMODE) {
	    ppmVHF = value;
    }
    else {
        ppmHF = value;
    } 

    // Update Frequency with new PPM
    setFrequency(direction, channel, LOfreq);
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
        //SoapySDR_log(SOAPY_SDR_WARNING, "GET HF GAIN  - TODO");
    }
    else if (name == "VHF")
    {
        //SoapySDR_log(SOAPY_SDR_WARNING, "GET VHF GAIN  - TODO");
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
        const double frequency,
        const SoapySDR::Kwargs &args)
{
    setFrequency(direction, channel, "RF", frequency, args);
}

void SoapySddcSDR::setFrequency(
        const int direction,
        const size_t channel,
        const std::string &name,
        const double frequency,
        const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting center freq: [%d] for name: [%s]", (uint32_t)frequency, name.c_str());
    if (name == "RF") {

        // ..... set here the LO frequency in the controlled hardware
        // Set here the frequency of the controlled hardware to LOfreq
        const double wishedLO = frequency;
        double ret = 0;
        rf_mode rfmode = RadioHandler.GetmodeRF();
        rf_mode newmode = RadioHandler.PrepareLo(frequency);

        if (newmode == NOMODE) // this freq is not supported
            throw std::runtime_error("Invalid RF Mode set. Double check requested frequency.");

        LOfreq = frequency;
        if (streamActive) {
            if ((newmode == VHFMODE) && (rfmode != VHFMODE))
            {
                    RadioHandler.UpdatemodeRF(VHFMODE);
                    setMGC(mgcIdxVHF);
                    SetAttenuator(attIdxVHF);
                    //SetSrateInternal(giExtSrateIdxVHF, false);
            }
            else if ((newmode == HFMODE) && (rfmode != HFMODE))
            {
                    RadioHandler.UpdatemodeRF(HFMODE);
                    setMGC(mgcIdxHF);
                    SetAttenuator(attIdxHF);
                    //SetSrateInternal(giExtSrateIdxHF, false)
            }
        
            double internal_LOfreq = LOfreq / getFrequencyCorrectionFactor();
            internal_LOfreq = RadioHandler.TuneLO(internal_LOfreq);
           	LOfreq = internal_LOfreq * getFrequencyCorrectionFactor();
            if (frequency != LOfreq)
            {
                SoapySDR_logf(SOAPY_SDR_WARNING, "requested Freq: [%.3f] != LOFreq: [%.3f].", frequency, LOfreq);
            }    

        }
    } else if (name == "CORR") {
        setFrequencyCorrection(direction, channel, frequency);
    }

}

double SoapySddcSDR::getFrequency(const int direction, const size_t channel) const
{
    return getFrequency(direction, channel, "RF");
}

double SoapySddcSDR::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    double freq = 0.0;

    if (name == "RF")
    {
        return LOfreq;
    }
    else if (name == "CORR")
    {
        return getFrequencyCorrection(direction, channel);
    }

    SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency(): Freq: [%d] for name: [%s]", freq, name.c_str());

    return freq;
}

std::vector<std::string> SoapySddcSDR::listFrequencies(const int direction, const size_t channel) const
{
    std::vector<std::string> names;;
    names.push_back("RF");
    names.push_back("CORR");
    return names;
}

SoapySDR::RangeList SoapySddcSDR::getFrequencyRange(
        const int direction,
        const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyRange()...");
    SoapySDR::RangeList results;
    results.push_back(SoapySDR::Range(10000, 1764000000));
    return results;
}

SoapySDR::ArgInfoList SoapySddcSDR::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList freqArgs;

    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyArgsInfo()...");

    // TODO: frequency arguments

    return freqArgs;
}


/*******************************************************************
 * Sample Rate API
 ******************************************************************/

void SoapySddcSDR::setSampleRate(const int direction, const size_t channel, const double rate)
{
    setSampleRateIdx(convertToSampleRateIdx(rate));
}

double SoapySddcSDR::getSampleRate(const int direction, const size_t channel) const
{
    return RadioHandler.getSampleRate();
}

std::vector<double> SoapySddcSDR::listSampleRates(const int direction, const size_t channel) const
{
    std::vector<double> results;

    double rate;
    for(int i=0; ; i++) 
    {
		if (getSrates(i, &rate) == -1)
			break;

		results.push_back(rate);
	}

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
    return RadioHandler.getSampleRate();
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

    SoapySDR::ArgInfo overclockArg;

    overclockArg.key = "overclock";
    overclockArg.value = "false";
    overclockArg.name = "OVERCLOCK";
    overclockArg.description = "ADC Overclock (True = 128Msps, else 64Msps)";
    overclockArg.type = SoapySDR::ArgInfo::BOOL;
   
    setArgs.push_back(overclockArg);

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

    //--------------------------
    // VHF Band Specific
    //--------------------------

    SoapySDR::ArgInfo attVHFArg;

    attVHFArg.key = "att_vhf";
    attVHFArg.value = "0";
    attVHFArg.name = "Att VHF";
    attVHFArg.description = "VHF Attenuation";
    attVHFArg.type = SoapySDR::ArgInfo::STRING;
    
    float attVal = 0.0;
    for(int i=0; ; i++) 
    {
		if (GetAttenuators(i, &attVal) != 0)
			break;

	    attVHFArg.options.push_back(std::to_string(i));
        attVHFArg.optionNames.push_back(std::to_string(attVal));
	}
    
    setArgs.push_back(attVHFArg);

    SoapySDR::ArgInfo gainVHFArg;

    gainVHFArg.key = "gain_vhf";
    gainVHFArg.value = "0";
    gainVHFArg.name = "Gain VHF";
    gainVHFArg.description = "VHF Gain";
    gainVHFArg.type = SoapySDR::ArgInfo::STRING;
    
    float gainVal = 0.0;
    for(int i=0; ; i++) 
    {
		if (getMGCs(i, &gainVal) != 0)
			break;

	    gainVHFArg.options.push_back(std::to_string(i));
        gainVHFArg.optionNames.push_back(std::to_string(gainVal));
	}
    
    setArgs.push_back(gainVHFArg);


    //--------------------------
    // HF Band Specific
    //--------------------------

    SoapySDR::ArgInfo attHFArg;

    attHFArg.key = "att_hf";
    attHFArg.value = "0";
    attHFArg.name = "Att HF";
    attHFArg.description = "HF Attenuation";
    attHFArg.type = SoapySDR::ArgInfo::STRING;
    
    attVal = 0.0;
    for(int i=0; ; i++) 
    {
		if (GetAttenuators(i, &attVal) != 0)
			break;

	    attHFArg.options.push_back(std::to_string(i));
        attHFArg.optionNames.push_back(std::to_string(attVal));
	}
    
    setArgs.push_back(attHFArg);

    SoapySDR::ArgInfo gainHFArg;

    gainHFArg.key = "gain_hf";
    gainHFArg.value = "0";
    gainHFArg.name = "Gain HF";
    gainHFArg.description = "HF Gain";
    gainHFArg.type = SoapySDR::ArgInfo::STRING;
    
    gainVal = 0.0;
    for(int i=0; ; i++) 
    {
		if (getMGCs(i, &gainVal) != 0)
			break;

	    gainHFArg.options.push_back(std::to_string(i));
        gainHFArg.optionNames.push_back(std::to_string(gainVal));
	}
    
    setArgs.push_back(gainHFArg);

    return setArgs;
}

void SoapySddcSDR::writeSetting(const std::string &key, const std::string &value)
{
    if (key == "overclock")
    {
        overclock = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "ADC OVERCLOCK: %s", vga ? "true" : "false");
        SetOverclock(overclock ? DEFAULT_ADC_FREQ * 2 : DEFAULT_ADC_FREQ); 
    }
    else if (key == "vga")
    {
        vga = ((value=="true") ? true : false);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "VGA: %s", vga ? "true" : "false");
        // TODO: Update RadioHandler 
    }
    else if (key == "pga")
    {
        bool pga = StringToBool(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "PGA: %s", pga ? "true" : "false");
        RadioHandler.UptPga(pga);
    }
    else if (key == "dither")
    {
        bool dither = StringToBool(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "DITHER: %s", dither ? "true" : "false");
        RadioHandler.UptDither(dither);
    }
    else if (key == "rand")
    {
        bool rand = StringToBool(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "RAND: %s", rand ? "true" : "false");
        RadioHandler.UptRand(rand);
    }
    else if (key == "bias_tee_vhf")
    {
        bool biasTeeVHF = StringToBool(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "BiasT VHF: %s", biasTeeVHF ? "true" : "false");
        RadioHandler.UpdBiasT_VHF(biasTeeVHF);
    }
    else if (key == "bias_tee_hf")
    {
        bool biasTeeHF = StringToBool(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "BiasT HF: %s", biasTeeHF ? "true" : "false");
        RadioHandler.UpdBiasT_HF(biasTeeHF);
    }
    else if (key == "att_vhf")
    {
        attIdxVHF = std::stod(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "VHF RF Att Idx: %d", attIdxVHF);
        if (RadioHandler.GetmodeRF() == VHFMODE)
            RadioHandler.UpdateattRF(attIdxVHF);
    }
    else if (key == "gain_vhf")
    {
        int gainIdxVHF = std::stod(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "VHF IFGain Idx: %d", gainIdxVHF);
        if (RadioHandler.GetmodeRF() == VHFMODE)
            RadioHandler.UpdateIFGain(gainIdxVHF);
    }
    else if (key == "att_hf")
    {
        attIdxHF = std::stod(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "HF RF Att Idx: %d", attIdxVHF);
        if (RadioHandler.GetmodeRF() == HFMODE)
            RadioHandler.UpdateattRF(attIdxHF);
    }
    else if (key == "gain_hf")
    {
        int gainIdxHF = std::stod(value);
        SoapySDR_logf(SOAPY_SDR_DEBUG, "HF IFGain Idx: %d", gainIdxHF);
        if (RadioHandler.GetmodeRF() == HFMODE)
            RadioHandler.UpdateIFGain(gainIdxHF);
    }
}

std::string SoapySddcSDR::readSetting(const std::string &key) const
{
    if (key == "overclock")
    {
        return BoolToString(overclock);
    }
    else if (key == "vga")
    {
        return BoolToString(vga);
    }
    else if (key == "pga")
    {
        return BoolToString(RadioHandler.GetPga());
    }
    else if (key == "dither")
    {
        return BoolToString(RadioHandler.GetDither());
    }
    else if (key == "rand")
    {
        return BoolToString(RadioHandler.GetRand());
    }
    else if (key == "bias_tee_vhf")
    {
        return BoolToString(RadioHandler.GetBiasT_VHF());
    }
    else if (key == "bias_tee_hf")
    {
        return BoolToString(RadioHandler.GetBiasT_HF());
    }
    else if (key == "att_vhf")
    {
        return std::to_string(attIdxVHF);
    }
    else if (key == "gain_vhf")
    {
        return std::to_string(mgcIdxVHF);
    }
    else if (key == "att_hf")
    {
        return std::to_string(attIdxHF);
    }
    else if (key == "gain_hf")
    {
        return std::to_string(mgcIdxHF);
    }

    SoapySDR_logf(SOAPY_SDR_WARNING, "Unknown setting '%s'", key.c_str());
    return "";
}
