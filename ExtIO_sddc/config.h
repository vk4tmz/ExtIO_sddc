#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "license.txt" 

#include <windows.h>   // LARGE_INTEGER
#include <math.h>      // atan => PI
#include <thread>
#include <mutex>
#include <condition_variable>


#ifdef __cplusplus
inline void null_func(const char *format, ...) { }
#define DbgEmpty null_func
#else
#define DbgEmpty { }
#endif

// macro to call callback function with just status extHWstatusT
#define EXTIO_STATUS_CHANGE( CB, STATUS )   \
	do { \
	  /*SendMessage(h_dialog, WM_USER + 1, STATUS, 0);*/ \
	  if (CB) { \
		  DbgPrintf("<==CALLBACK: %s\n", #STATUS); \
		  CB( -1, STATUS, 0, NULL );\
	  }\
	}while(0)

#define EnterFunction() \
  DbgPrintf("==>%s\n", __FUNCDNAME__)

#define EnterFunction1(v1) \
  DbgPrintf("==>%s(%d)\n", __FUNCDNAME__, (v1))

#ifdef TRACE
#define DbgPrintf (printf)
#else
#define DbgPrintf DbgEmpty
#endif

#define VERSION             (1.01)	//	Dll version number x.xx

#define HWNAME				"BBRF103"			
#define HWMODEL				"HF103"
#define SETTINGS_IDENTIFIER	"sddc_1.01"
#define SWNAME				"ExtIO_sddc.dll"

#define	QUEUE_SIZE 64

#ifndef ADC_FREQ
#define ADC_FREQ  (64000000)	// ADC sampling frequency
#endif

#define HF_UPPER  (ADC_FREQ/2)	   
#define FREQCORRECTION (0.0)   // Default xtal frequency correction in ppm
#define GAIN_ADJ (0.0)          // default gain factor in DB

#define R820T_FREQ (32000000)	// R820T reference frequency
#define R820T_ZERO (0)          // freq 0
#define R820T2_IF_CARRIER (5000000)

#define FFTN_R_ADC (2048)       // FFTN used for ADC real stream DDC

// GAINFACTORS to be adjusted with lab reference source measured with HDSDR Smeter rms mode  
#define BBRF103_GAINFACTOR (0.000000078F)       // BBRF103
#define HF103_GAINFACTOR   (0.0000000114F)      // HF103
#define RX888_GAINFACTOR   (0.00000000695F)     // RX888

enum rf_mode { HFMODE = 0x1, VLFMODE = 0x2, VHFMODE = 0x3, NOMODE = 4 };

#define HF_HIGH (ADC_FREQ/2)    // 32M
#define MW_HIGH (2000000)

#ifdef OFFSET_BINARY
#define ADCSAMPLE UINT16
#define DC 32768
#else
#define ADCSAMPLE INT16
#define DC 0
#endif

#define EXT_BLOCKLEN		512	* 64	/* 32768 only multiples of 512 */

#define RFDDCNAME ("NVIA L768M256")
#define RFDDCVER ("v 1.0")

// URL definitions
#define URL1B               "16bit SDR Receiver"
#define URL1               "<a>http://www.hdsdr.de/</a>"
#define URL_HDSR            "http://www.hdsdr.de/"
#define URL_HDSDRA          "<a>http://www.hdsdr.de/</a>"

#define TIMEOUT (2000)

extern double pi;
extern int Xfreq;
extern char strversion[];
extern double gdFreqCorr_ppm;
extern double adcfixedfreq;
extern double gdGainCorr_dB;
extern bool saveADCsamplesflag;

enum radiotype { NORADIO = 0, BBRF103 = 1, HF103 = 2, RX888 = 3 };
extern const char* radioname[4];

extern bool run;
extern int transferSize;
extern radiotype radio;

#endif // _CONFIG_H_

