
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

	RadioHandler.Start(getSampleRateIdx());
	
    setFrequency(direction, 0, LOfreq, args);

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
    streamActive = false;

    // TODO - Close / Release
    RadioHandler.Stop();
    RadioHandler.Close();
    Fx3->Close();
}

size_t SoapySddcSDR::getStreamMTU(SoapySDR::Stream *stream) const
{
    return EXT_BLOCKLEN * TRANSFER_SAMPLES_MULTIPLIER;
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

    SoapySDR_logf(SOAPY_SDR_INFO, "activateStream(): timeNs: [%ld], numElems: [%d]", timeNs, numElems);

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

    SoapySDR_logf(SOAPY_SDR_INFO, "deactivateStream(): timeNs: [%ld]", timeNs);

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
    int bs = EXT_BLOCKLEN * 2 * sizeof(float);
    //SoapySDR_logf(SOAPY_SDR_DEBUG, "readStream(): timeNs: [%ld], timeoutUs: [%ld] numElems: [%d]  BlockSize: [%d] Flags: ]%d]  useShort: [%d]", timeNs, timeoutUs, numElems, bs, flags, useShort);
   
    int returnedElems = 0;
    if (useShort)
    {
        // TODO: Access RadioHandler.inputbuffer
        //std::memcpy(buffs[0], sdrplay_stream->currentBuff, returnedElems * 2 * sizeof(short));

    }
    else
    {
        char *buf0 = (char *)buffs[0];
        // while ((returnedElems + transferSamples) <= numElems)
        {
            auto buf = SoapySddcSDR::cbbuffer.getReadPtr();
            std::memcpy(buf0, buf, bs);
            SoapySddcSDR::cbbuffer.ReadDone();
            buf0 += bs;
            returnedElems += transferSamples;
        }
    }

    return returnedElems;
}


/*******************************************************************
 * Direct buffer access API
 ******************************************************************/

size_t SoapySddcSDR::getNumDirectAccessBuffers(SoapySDR::Stream *stream)
{
    return 0;
}

int SoapySddcSDR::getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs)
{
    return SOAPY_SDR_NOT_SUPPORTED;
}

int SoapySddcSDR::acquireReadBuffer( SoapySDR::Stream *stream, size_t &handle, const void **buffs, int &flags, long long &timeNs, const long timeoutUs)
{
	return SOAPY_SDR_NOT_SUPPORTED;
}

void SoapySddcSDR::releaseReadBuffer( SoapySDR::Stream *stream, const size_t handle)
{
}