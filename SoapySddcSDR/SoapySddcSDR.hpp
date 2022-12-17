#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Types.h>
#include "../Core/config.h"
#include "../Core/RadioHandler.h"
#include "../Core/FX3Class.h"
#include "r2iqBasicControlClass.hpp"


const double DEFAULT_SAMPLE_RATE = 2e6; 

const int TRANSFER_SAMPLES_MULTIPLIER = 1;

const int MAX_CB_BUFFER_BLOCK_SIZE = EXT_BLOCKLEN * 2 * sizeof(float);

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
    inline static ringbuffer<float> cbbuffer;

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
\
    double getFrequencyCorrectionFactor() const;

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
     * Direct buffer access API
     ******************************************************************/

    size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream);

    int getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs);

    int acquireReadBuffer( SoapySDR::Stream *stream, size_t &handle, const void **buffs, int &flags, long long &timeNs, const long timeoutUs);

    void releaseReadBuffer( SoapySDR::Stream *stream, const size_t handle);

    /*******************************************************************
     * Frequency API
     ******************************************************************/

    void setFrequency(
            const int direction,
            const size_t channel,
            const double frequency,
            const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    void setFrequency(
            const int direction,
            const size_t channel,
            const std::string &name,
            const double frequency,
            const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    double getFrequency(const int direction, const size_t channel) const;

    double getFrequency(const int direction, const size_t channel, const std::string &name) const;

    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;

    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel) const;

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
    bool overclock = false;
    int srateIdxVHF = 0;
    int srateIdxHF = 0;

    double LOfreq = 2000000;
    double ppmVHF = 0;
    double ppmHF = 0;
    bool vga = false;

    int mgcIdxHF = 0;
    int attIdxHF = 0;
    int mgcIdxVHF = 0;
    int attIdxVHF = 0;

    bool useShort = false;
    bool streamActive = false;

    fx3class *Fx3 = CreateUsbHandler();
    RadioHandlerClass RadioHandler;
    r2iqBasicControlClass *r2iqControl;
    bool gbInitHW = false;



    int GetAttenuators(int atten_idx, float* attenuation) const;
    int GetActualAttIdx(void) const;
    int SetAttenuator(int atten_idx);
    int getMGCs(int mgc_idx, float * gain) const;
    int getActualMgcIdx(void) const;
    int setMGC(int mgc_idx);

    void selectDevice(const std::string idx,
                      const std::string &serial);

    int getSampleRateIdx(void) const;
    int getSrates(int srate_idx, double *samplerate) const;
    int setSampleRateIdx(int srate_idx);
    int SetOverclock(uint32_t adcfreq);
    int TuneLO(double freq);
    int convertToSampleRateIdx(uint32_t samplerate);
    
};