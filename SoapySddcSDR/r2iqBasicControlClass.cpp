
#include "r2iqBasicControlClass.hpp"
#include <SoapySDR/ConverterPrimitives.hpp>

r2iqControlClass::r2iqControlClass()
{
	r2iqOn = false;
	randADC = false;
	sideband = false;
	mdecimation = 0;
	mratio[0] = 1;  // 1,2,4,8,16
	for (int i = 1; i < NDECIDX; i++)
	{
		mratio[i] = mratio[i - 1] * 2;
	}
}

r2iqBasicControlClass::r2iqBasicControlClass() {
}

r2iqBasicControlClass::~r2iqBasicControlClass(void) {

}

void r2iqBasicControlClass::TurnOn() {
	this->r2iqOn = true;
	this->bufIdx = 0;
	this->lastThread = threadArgs[0];

	for (unsigned t = 0; t < processor_count; t++) {
		r2iq_thread[t] = std::thread(
			[this] (void* arg)
				{ return this->r2iqThreadf((r2iqThreadArg*)arg); }, (void*)threadArgs[t]);
	}
}

void r2iqBasicControlClass::TurnOff(void) {
	this->r2iqOn = false;

	inputbuffer->Stop();
	outputbuffer->Stop();
	for (unsigned t = 0; t < processor_count; t++) {
		r2iq_thread[t].join();
	}
}

bool r2iqBasicControlClass::IsOn(void) { return(this->r2iqOn); }

void r2iqBasicControlClass::Init(float gain, ringbuffer<int16_t> *input, ringbuffer<float>* obuffers)
{
	this->inputbuffer = input;    // set to the global exported by main_loop
	this->outputbuffer = obuffers;  // set to the global exported by main_loop

	this->GainScale = gain;

    {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Generating SDR lookup tables");
        // create lookup tables
        for (unsigned int i = 0; i <= 0xffff; i++)
        {
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
            float re = ((i & 0xff) - 127.4f) * (1.0f / 128.0f);
            float im = ((i >> 8) - 127.4f) * (1.0f / 128.0f);
#else
            float re = ((i >> 8) - 127.4f) * (1.0f / 128.0f);
            float im = ((i & 0xff) - 127.4f) * (1.0f / 128.0f);
#endif

            std::complex<float> v32f, vs32f;

            v32f.real(re);
            v32f.imag(im);
            _lut_32f.push_back(v32f);

            vs32f.real(v32f.imag());
            vs32f.imag(v32f.real());
            _lut_swap_32f.push_back(vs32f);

            std::complex<int16_t> v16i, vs16i;

            v16i.real(int16_t((float(SHRT_MAX) * re)));
            v16i.imag(int16_t((float(SHRT_MAX) * im)));
            _lut_16i.push_back(v16i);

            vs16i.real(vs16i.imag());
            vs16i.imag(vs16i.real());
            _lut_swap_16i.push_back(vs16i);
        }
    }

	// Get the processor count
	processor_count = std::thread::hardware_concurrency() - 1;
	if (processor_count == 0)
		processor_count = 1;
	if (processor_count > N_MAX_R2IQ_THREADS)
		processor_count = N_MAX_R2IQ_THREADS;

	{
		DbgPrintf((char *) "r2iqCntrl initialization\n");

		for (unsigned t = 0; t < processor_count; t++) {
			r2iqThreadArg *th = new r2iqThreadArg();
			threadArgs[t] = th;

            // xx
		}
	}
}

void r2iqBasicControlClass::convertSamples( void *out_buff, void *in_buff, size_t st, size_t n)
{
	float *ftarget = reinterpret_cast<float*>(out_buff);
	int16_t *itarget = reinterpret_cast<int16_t*>(out_buff);

	uint16_t *ptr=reinterpret_cast<uint16_t*>(in_buff);

	for (size_t i = 0; i < n; i++)
	{
		int16_t val = ptr[i];
		if (this->getRand() && (ptr[i] & 1))
		{
			val = val ^ (-2);
		}

		if (dataFormat == SOAPY_SDR_CF32)
		{
			//ftarget[(st + (i * 2))] = float(val);
			//ftarget[(st + (i * 2))] = float(val)/65536lu;
			ftarget[(st + (i * 2))] = SoapySDR::S16toF32(val);
			ftarget[(st + (i * 2) + 2)] = 0.0;
			
		}
		else if (dataFormat == SOAPY_SDR_S16)
		{
			itarget[(st + i)] = ptr[i];			
		}
	}
}

void * r2iqBasicControlClass::r2iqThreadf(r2iqThreadArg *th)
{
	const int decimate = this->mdecimation;
	const bool lsb = this->getSideband();

	int decimate_count = 0;

	while (r2iqOn) {
		const int16_t *dataADC;  // pointer to input data

		{
			std::unique_lock<std::mutex> lk(mutexR2iqControl);
			dataADC = inputbuffer->getReadPtr();

			if (!r2iqOn)
				return 0;
		}

		{
			std::unique_lock<std::mutex> lk(mutexR2iqControl);
			auto buf = outputbuffer->getWritePtr();
			convertSamples(buf, (void *)dataADC, 0, transferSamples);
		}
		inputbuffer->ReadDone();
		outputbuffer->WriteDone();

    }

	return 0;

};