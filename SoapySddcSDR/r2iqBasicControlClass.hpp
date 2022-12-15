#pragma once

#include "r2iq.h"
#include "config.h"
#include <SoapySDR/ConverterPrimitives.hpp>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Types.h>
#include <complex>
#include <vector>
#include <limits.h>

// use up to this many threads
#define N_MAX_R2IQ_THREADS 1

// assure, that ADC is not oversteered?
struct r2iqThreadArg {

	r2iqThreadArg()
	{
	}

	int data1;
};


class r2iqBasicControlClass : public r2iqControlClass
{
public:
    r2iqBasicControlClass();
    virtual ~r2iqBasicControlClass();

    void Init(float gain, ringbuffer<int16_t>* buffers, ringbuffer<float>* obuffers);
    void TurnOn();
    void TurnOff(void);
    bool IsOn(void);

private:
    ringbuffer<int16_t>* inputbuffer;    // pointer to input buffers
    ringbuffer<float>* outputbuffer;    // pointer to ouput buffers
    int bufIdx;         // index to next buffer to be processed
    unsigned int processor_count = 1;
    float GainScale;
    std::string dataFormat = SOAPY_SDR_CF32;
    //std::string dataFormat = SOAPY_SDR_CS16;

    r2iqThreadArg* threadArgs[N_MAX_R2IQ_THREADS];
    std::mutex mutexR2iqControl;
    std::thread r2iq_thread[N_MAX_R2IQ_THREADS];
    r2iqThreadArg* lastThread;

    std::vector<std::complex<float> > _lut_32f;
    std::vector<std::complex<float> > _lut_swap_32f;
    std::vector<std::complex<int16_t> > _lut_16i;
    std::vector<std::complex<int16_t> > _lut_swap_16i;

    void *r2iqThreadf(r2iqThreadArg *th);   // thread function
    void convertSamples( void *out_buff, void *in_buff, size_t st, size_t n);

};