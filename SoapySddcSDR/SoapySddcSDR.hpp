#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Types.h>
#include "../Core/config.h"
#include "../Core/RadioHandler.h"
#include "../Core/FX3Class.h"


const double DEFAULT_SAMPLE_RATE = 2e6; 

typedef enum sdrRXFormat
{
    SDR_RX_FORMAT_FLOAT32, SDR_RX_FORMAT_INT16
} sdrRXFormat;

struct FirmwareImage {
    char *imagefile = nullptr;
    unsigned char* res_data = nullptr;
    uint32_t res_size = 0;
};

typedef struct FirmwareImage FirmwareImage_t;

inline 
void freeFirmwareImage(FirmwareImage_t fwImage) {
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

inline const char * BoolToString(bool b)
{
  return b ? "true" : "false";
}

inline bool StringToBool(std::string s)
{
  return (s=="true");
}


class SoapySddcSDR: public SoapySDR::Device
{
public:
    inline static FirmwareImage_t fwImage;
    inline static std::map<std::string, SoapySDR::Kwargs> _cachedResults;

    mutable std::mutex _general_state_mutex;

    SoapySddcSDR(const SoapySDR::Kwargs &args);

    ~SoapySddcSDR(void);

    static bool loadFirmwareImage(const SoapySDR::Kwargs &args);

    /*******************************************************************
     * Identification API
     ******************************************************************/

    std::string getDriverKey(void) const;

    std::string getHardwareKey(void) const;

    SoapySDR::Kwargs getHardwareInfo(void) const;

    /*******************************************************************
     * Channels API
     ******************************************************************/

    size_t getNumChannels(const int dir) const;

    bool getFullDuplex(const int direction, const size_t channel) const;

    /*******************************************************************
     * Antenna API
     ******************************************************************/

    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;

    void setAntenna(const int direction, const size_t channel, const std::string &name);

    std::string getAntenna(const int direction, const size_t channel) const;

    /*******************************************************************
     * Frontend corrections API
     ******************************************************************/

    bool hasDCOffsetMode(const int direction, const size_t channel) const;

    bool hasFrequencyCorrection(const int direction, const size_t channel) const;

    void setFrequencyCorrection(const int direction, const size_t channel, const double value);

    double getFrequencyCorrection(const int direction, const size_t channel) const;


    /*******************************************************************
     * Gain API
     ******************************************************************/

    std::vector<std::string> listGains(const int direction, const size_t channel) const;

    bool hasGainMode(const int direction, const size_t channel) const;

    void setGainMode(const int direction, const size_t channel, const bool automatic);

    bool getGainMode(const int direction, const size_t channel) const;

    void setGain(const int direction, const size_t channel, const double value);

    void setGain(const int direction, const size_t channel, const std::string &name, const double value);

    double getGain(const int direction, const size_t channel, const std::string &name) const;

    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;


    /*******************************************************************
     * Stream API
     ******************************************************************/

    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;

    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;

    // SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;

    SoapySDR::Stream *setupStream(const int direction, 
                                  const std::string &format, 
                                  const std::vector<size_t> &channels = std::vector<size_t>(), 
                                  const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    void closeStream(SoapySDR::Stream *stream);

    size_t getStreamMTU(SoapySDR::Stream *stream) const;

    int activateStream(SoapySDR::Stream *stream,
                       const int flags = 0,
                       const long long timeNs = 0,
                       const size_t numElems = 0);

    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);

    int readStream(SoapySDR::Stream *stream,
                   void * const *buffs,
                   const size_t numElems,
                   int &flags,
                   long long &timeNs,
                   const long timeoutUs = 200000);


    /*******************************************************************
     * Frequency API
     ******************************************************************/

    void setFrequency(
            const int direction,
            const size_t channel,
            const double frequency,
            const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    double getFrequency(const int direction, const size_t channel) const;

    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;

    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;

    SoapySDR::ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const;


    /*******************************************************************
     * Sample Rate API
     ******************************************************************/

    void setSampleRate(const int direction, const size_t channel, const double rate);

    double getSampleRate(const int direction, const size_t channel) const;

    std::vector<double> listSampleRates(const int direction, const size_t channel) const;

    SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;

    void setBandwidth(const int direction, const size_t channel, const double bw);

    double getBandwidth(const int direction, const size_t channel) const;

    std::vector<double> listBandwidths(const int direction, const size_t channel) const;

    SoapySDR::RangeList getBandwidthRange(const int direction, const size_t channel) const;
    

    /*******************************************************************
     * Settings API
     ******************************************************************/

    SoapySDR::ArgInfoList getSettingInfo(void) const;

    void writeSetting(const std::string &key, const std::string &value);

    std::string readSetting(const std::string &key) const;

    

private:

    //device Info
    SoapySDR::Kwargs dev;
    int deviceId = -1;   
    std::string serNo;

    // Settings
    sdrRXFormat rxFormat = SDR_RX_FORMAT_FLOAT32;
    int sampleRateIdx;
    int srateIdxVHF;
    int srateIdxHF;

    double LOfreq;
    double ppmVHF = 0;
    double ppmHF = 0;
    bool vga = false;

    int attIdx = 0;
    int gainHF = 0;
    int attHF = 0;
    int gainVHF = 0;
    int attVHF = 0;
 
    bool iqSwap = false;

    bool useShort = false;
    bool streamActive = false;

    fx3class *Fx3 = CreateUsbHandler();
    RadioHandlerClass RadioHandler;
    bool gbInitHW = false;




    void selectDevice(const std::string idx,
                      const std::string &serial);

    double getFreqCorrectionFactor();
    int getActualSrateIdx(void);
    int setActualSrateIdx(int srate_idx);
    int SetOverclock(uint32_t adcfreq);
    int TuneLO(double freq);
    int convertToSampleRateIdx(double samplerate);

};