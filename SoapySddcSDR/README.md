# SoapySddcSDR

## Install libusb v1.0.23

When using libusb-1.0.24+ you will encounter a couple of issues:

- The following error upon calling "**hardware->FX3producerOff();**"

```
usbi_mutex_lock: Assertion `pthread_mutex_lock(mutex) == 0' failed
```

Temp Fix uninstall the default installed version and install v1.0.23:

### Uninstall Current libusb
```
dpkg -l libusb-1.0*
apt-cache policy libusb-1.0*

sudo apt remove libusb-1.0-0-dev
sudo apt remove libusb-1.0-0
```

### Install LibUSB v1.0.23:

```
wget "https://launchpad.net/ubuntu/+archive/primary/+files/libusb-1.0-0_1.0.23-3_amd64.deb"
wget "https://launchpad.net/ubuntu/+archive/primary/+files/libusb-1.0-0-dev_1.0.23-3_amd64.deb"
wget "https://launchpad.net/ubuntu/+archive/primary/+files/libusb-1.0-doc_1.0.23-3_all.deb"
sudo dpkg --install libusb-1.0-*_1.0.23-3_*.deb

```


## Build Instructions

Following te main project ExtIO_sddc build instructs and once built run
```
sudo make install
sudo ldconfig
```

This will install the libsddc.so and SoapySddcSDR module under system folders available for use with SoapySDRUtil, CubicSDR etc. 

You will need to set the FX3_FW_IMG environment variable:

```Bash
export FX3_FW_IMG="~/projects/ExtIO_sddc/SDDC_FX3.img"
```

Create following alias:

```Bash
alias grant_usb='lsusb | grep "Cypress" | while read a BUS_ID c DEV_ID DEV_DESC; do  BUSDEV_PATH=/dev/bus/usb/$BUS_ID/${DEV_ID::-1}; echo "Granting Permission to : [$BUSDEV_PATH] - DESC: [$DEV_DESC]"; sudo chmod o+rw $BUSDEV_PATH ;  done'
```

Run "grant_usb; SoapySDRUtil --probe=...." at least twice. First time, grants permission for the current user to access the usb device. Then running SoapySDRUtil probe will uploads firmware.  This causes the device number to change, so you then need to "grant_usb" once gain.

Once done you can run SoapySDRUtil, CubicSDR etc and access the device as the current user.

```
grant_usb; SoapySDRUtil --probe="driver=sddcsdr"
grant_usb; SoapySDRUtil --probe="driver=sddcsdr"
```
