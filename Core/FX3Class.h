#ifndef FX3CLASS_H
#define FX3CLASS_H

//
// FX3handler.cpp 
// 2020 10 12  Oscar Steila ik1xpv
// loading arm code.img from resource by Howard Su and Hayati Ayguen
// This module was previous named:openFX3.cpp
// MIT License Copyright (c) 2016 Booya Corp.
// booyasdr@gmail.com, http://booyasdr.sf.net
// modified 2017 11 30 ik1xpv@gmail.com, http://www.steila.com/blog
// 

#include <stdint.h>
#include <functional>
#include "../Interface.h"
#include "dsp/ringbuffer.h"

struct device_info {
  char *manufacturer;
  char *product;
  char *serial_number;
};

typedef struct device_info device_info_t;

inline void free_device_info(device_info_t dev_info) {
	free(dev_info.manufacturer);
	free(dev_info.product);
	free(dev_info.serial_number);
}

class fx3class
{
public:
	virtual ~fx3class(void) {}
	virtual bool Open(uint8_t dev_idx, const uint8_t* fw_data, uint32_t fw_size) = 0;
	virtual bool Close() = 0;
	virtual bool Control(FX3Command command, uint8_t data = 0) = 0;
	virtual bool Control(FX3Command command, uint32_t data) = 0;
	virtual bool Control(FX3Command command, uint64_t data) = 0;
	virtual bool SetArgument(uint16_t index, uint16_t value) = 0;
	virtual bool GetHardwareInfo(uint32_t* data) = 0;
	virtual bool ReadDebugTrace(uint8_t* pdata, uint8_t len) = 0;
	virtual void StartStream(ringbuffer<int16_t>& input, int numofblock) = 0;
	virtual void StopStream() = 0;
	virtual bool Enumerate(unsigned char& idx, char* lbuf, const uint8_t* fw_data, uint32_t fw_size) = 0;
	virtual bool Enumerate(unsigned char &idx, device_info_t *dev_info, const uint8_t* fw_data, uint32_t fw_size) = 0;
};

extern "C" fx3class* CreateUsbHandler();

#endif // FX3CLASS_H
