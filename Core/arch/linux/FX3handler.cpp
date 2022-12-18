#include <string.h>

#include "FX3handler.h"
#include "usb_device.h"

fx3class* CreateUsbHandler()
{
	return new fx3handler();
}

fx3handler::fx3handler()
{
}

fx3handler::~fx3handler()
{
}

bool fx3handler::Open(uint8_t dev_idx, const uint8_t* fw_data, uint32_t fw_size)
{
    dev = usb_device_open(dev_idx, (const char*)fw_data, fw_size);

    return dev != nullptr;
}

bool fx3handler::Close()
{
    if (dev != nullptr) {
        usb_device_close(dev);
        dev=nullptr;
        return true;
    }
    return false;
}

bool fx3handler::Control(FX3Command command, uint8_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *) &data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint32_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *) &data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint64_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *) &data, sizeof(data), 0) == 0;
}

bool fx3handler::SetArgument(uint16_t index, uint16_t value)
{
    uint8_t data = 0;
    return usb_device_control(this->dev, SETARGFX3, value, index, (uint8_t *) &data, sizeof(data), 0) == 0;
}

bool fx3handler::GetHardwareInfo(uint32_t* data)
{
    return usb_device_control(this->dev, TESTFX3, 0, 0, (uint8_t *) data, sizeof(*data), 1) == 0;
}

void fx3handler::StartStream(ringbuffer<int16_t>& input, int numofblock)
{
    inputbuffer = &input;
    auto readsize = input.getWriteCount() * sizeof(uint16_t);
    stream = streaming_open_async(this->dev, readsize, numofblock, PacketRead, this);

    // Start background thread to poll the events
    run = true;
    poll_thread = std::thread(
        [this]() {
            while(run)
            {
                usb_device_handle_events(this->dev);
            }
        });

    if (stream)
    {
        streaming_start(stream);
    }
}

void fx3handler::StopStream()
{
    run = false;
    poll_thread.join();

    streaming_stop(stream);
    streaming_close(stream);
}

void fx3handler::PacketRead(uint32_t data_size, uint8_t *data, void *context)
{
    fx3handler *handler = (fx3handler*)context;

    auto *ptr = handler->inputbuffer->getWritePtr();
    memcpy(ptr, data, data_size);
    handler->inputbuffer->WriteDone();
}

bool fx3handler::ReadDebugTrace(uint8_t* pdata, uint8_t len)
{
	return true;
}

bool fx3handler::Enumerate(unsigned char &idx, char *lbuf, const uint8_t* fw_data, uint32_t fw_size)
{
    device_info_t dev_info;

    strcpy(lbuf, "");
    if (this->Enumerate(idx, &dev_info, fw_data, fw_size)) {
        strcpy(lbuf, (char *)dev_info.manufacturer);
	    strcat(lbuf, ":");
        strcat(lbuf, (char *)dev_info.product);
	    strcat(lbuf, ":");
	    strcat(lbuf, (char *)dev_info.serial_number);
    }

    free_device_info(dev_info);

    return false;
}

bool fx3handler::Enumerate(unsigned char &idx, device_info_t *dev_info, const uint8_t* fw_data, uint32_t fw_size)
{
    bool ret_val = false;

    struct usb_device_info *usb_device_infos;
    int dev_count = usb_device_get_device_list(&usb_device_infos);
    if (dev_count < 0)
        return false;

    if (idx < dev_count) {
        dev_info->manufacturer =  strdup((char *)usb_device_infos[idx].manufacturer);
        dev_info->product =  strdup((char *)usb_device_infos[idx].product);
        dev_info->serial_number =  strdup((char *)usb_device_infos[idx].serial_number);
        ret_val = true;
    } 

    // Release resources
    usb_device_free_device_list(usb_device_infos);
    usb_device_infos = NULL;

	return ret_val;
}
