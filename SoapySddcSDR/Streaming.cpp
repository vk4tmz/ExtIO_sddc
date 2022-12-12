
#include "SoapySddcSDR.hpp"
#include <SoapySDR/Formats.hpp>

/*******************************************************************
 * Stream API
 ******************************************************************/

std::vector<std::string> SoapySddcSDR::getStreamFormats(const int direction, const size_t channel) const
{
    std::vector<std::string> formats;
    formats.push_back(SOAPY_SDR_CS16);
    formats.push_back(SOAPY_SDR_CF32);

    return formats;
}

std::string SoapySddcSDR::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    if (direction != SOAPY_SDR_RX) {
        throw std::runtime_error("SDDC-SDR is RX only, use SOAPY_SDR_RX");
    }

    fullScale = 32767;
    return SOAPY_SDR_CS16;
}

SoapySDR::Stream *SoapySddcSDR::setupStream(const int direction,
                                            const std::string &format,
                                            const std::vector<size_t> &channels,
                                            const SoapySDR::Kwargs &args)
{
    if (direction != SOAPY_SDR_RX)
    {
        throw std::runtime_error("SSDC-SDR is RX only, use SOAPY_SDR_RX");
    }

    //check the channel configuration
    if (channels.size() > 1 or (channels.size() > 0 and channels.at(0) != 0))
    {
        throw std::runtime_error("setupStream invalid channel selection");
    }

    // check the format
    if (format == SOAPY_SDR_CS16)
    {
        useShort = true;
        // shortsPerWord = 1;
        // bufferLength = bufferElems * elementsPerSample * shortsPerWord;
        SoapySDR_log(SOAPY_SDR_INFO, "Using format CS16.");
    }
    else if (format == SOAPY_SDR_CF32)
    {
        useShort = false;
        // shortsPerWord = sizeof(float) / sizeof(short);
        // bufferLength = bufferElems * elementsPerSample * shortsPerWord;  // allocate enough space for floats instead of shorts
        SoapySDR_log(SOAPY_SDR_INFO, "Using format CF32.");
    }
    else
    {
        throw std::runtime_error( "setupStream invalid format '" + format +
                                  "' -- Only CS16 or CF32 are supported by the SDDC-SDR module.");
    }

	if (!gbInitHW)
		return 0;

	RadioHandler.Start(sampleRateIdx);
	
    // ..... set here the LO frequency in the controlled hardware
	// Set here the frequency of the controlled hardware to LOfreq
	const double wishedLO = LOfreq;
	double ret = 0;
	rf_mode rfmode = RadioHandler.GetmodeRF();
	rf_mode newmode = RadioHandler.PrepareLo(LOfreq);

	if (newmode == NOMODE) // this freq is not supported
		throw std::runtime_error("Invalid RF Mode set. Double check requested frequency.");

	if ((newmode == VHFMODE) && (rfmode != VHFMODE))
	{
			RadioHandler.UpdatemodeRF(VHFMODE);
	}
	else if ((newmode == HFMODE) && (rfmode != HFMODE))
	{
			RadioHandler.UpdatemodeRF(HFMODE);
	}

	double internal_LOfreq = LOfreq / getFreqCorrectionFactor();
	internal_LOfreq = RadioHandler.TuneLO(internal_LOfreq);
	LOfreq = internal_LOfreq * getFreqCorrectionFactor();
	if (wishedLO != LOfreq)
	{
		 SoapySDR_logf(SOAPY_SDR_WARNING, "wishedLO: [%.3f] != LOFreq: [%.3f].", wishedLO, LOfreq);
	}    

	if (RadioHandler.IsReady()) //  HF103 connected
	{
		SoapySDR_log(SOAPY_SDR_INFO, "SDR successfully initialised streaming.");
	}
	else
	{
		throw std::runtime_error("SDR failed to initialise streaming.");
	}

    return (SoapySDR::Stream *) this;
}

void SoapySddcSDR::closeStream(SoapySDR::Stream *stream)
{
    // TODO - Close / Release
    RadioHandler.Stop();
    RadioHandler.Close();
    Fx3->Close();
}

size_t SoapySddcSDR::getStreamMTU(SoapySDR::Stream *stream) const
{
    // TODO - depends on CS16 or CF32 ???
    return transferSamples;
}

int SoapySddcSDR::activateStream(SoapySDR::Stream *stream,
                                 const int flags,
                                 const long long timeNs,
                                 const size_t numElems)
{
    if (flags != 0)
    {
        SoapySDR_log(SOAPY_SDR_ERROR, "error in activateStream() - flags != 0");
        return SOAPY_SDR_NOT_SUPPORTED;
    }

    streamActive = true;

    return 0;
}

int SoapySddcSDR::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    if (flags != 0)
    {
        SoapySDR_log(SOAPY_SDR_ERROR, "error in deactivateStream() - flags != 0");
        return SOAPY_SDR_NOT_SUPPORTED;
    }

    // do nothing because deactivateStream() can be called multiple times
    return 0;
}

int SoapySddcSDR::readStream(SoapySDR::Stream *stream,
                             void * const *buffs,
                             const size_t numElems,
                             int &flags,
                             long long &timeNs,
                             const long timeoutUs)
{

    // copy into user's buff - always write to buffs[0] since each stream
    // can have only one rx/channel
    if (useShort)
    {
        // TODO: Access RadioHandler.inputbuffer
        //std::memcpy(buffs[0], sdrplay_stream->currentBuff, returnedElems * 2 * sizeof(short));

    }
    else
    {
        auto buf = RadioHandler.outputbuffer.getReadPtr();
        std::memcpy(buffs[0], buf, transferSize);
    }

    return (int)transferSize;
}
