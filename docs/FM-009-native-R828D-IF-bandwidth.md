# FM-009: RX888 MkII native R828D IF bandwidth — implementation checkpoint

## Implementation update — 2026-10-18

The opt-in `-B <Hz>` source changes have now been committed to this **existing branch** (no new branch). Changes span `Interface.h`, FX3 `USBhandler.c`, RX888R2 hardware class, RadioHandler, libsddc and `sddc_gain_capture`. Existing invocation without `-B` retains firmware 8 MHz default; `-B 290000` requests the driver's nominal 290 kHz filter with host IF correction to 1.925 MHz. The capture requests bandwidth before tuning and before streaming.

**Not yet validated:** these commits have not been compiled or used on RX888 hardware. A matching newly built FX3 image is required for explicit `-B`, and receiver firmware loading must be carefully controlled before any on-air test. Never run `-B` against old firmware expecting it to work. Device ACK is checked but there is no independent register readback yet. Analogue filter change does not change IQ sample rate.

**Important:** `r82xx_set_bandwidth()` returns positive IF centre on success; the FX3 argument handler converts this to a zero success result. The synthetic 5 MHz alias was excluded since the driver maps that request to 6 MHz.

## Original source audit


Status: **source audited; device control NOT yet implemented or validated**. This note deliberately does not change default firmware or the working `sddc_gain_capture` behavior.

## Exact current source linkage
- `SDDC_FX3/USBhandler.c:r820_initialize` initializes R828D with `r82xx_set_bandwidth(&tuner, 8000000, 0, &bw, 1)`.
- `SDDC_FX3/driver/tuner_r82xx.c:r82xx_set_bandwidth` sets the real R828D tuner filter registers and updates `priv->int_freq`.
- `Core/radio/RX888R2Radio.cpp:GetTunerFrequency_VHF` always returns `4570000` Hz, so `Core/RadioHandler.cpp:SetCenterFrequency` uses the wrong FFT frequency-offset correction for narrow filters.
- `libsddc/sddc_gain_capture.c` uses ADC `-s` and decimation=0, and assumes complex output rate ADC/2. This is separate from R828D analogue filter bandwidth.
- A nominal 290-kHz table entry in `tuner_r82xx.c` selects IF frequency **1,925,000 Hz** (1950 kHz - 25 kHz), rather than 4,570,000 Hz. This requires corresponding host translation on every selected filter mode.

## Required implementation (before live narrowband operation)
1. Implement an explicit opt-in bandwidth-setting protocol to FX3 tuner firmware, returning the selected analogue BW and tuner IF offset; restrict to R82xx-compatible hardware and reject setting changes while streaming.
2. Expose it in `libsddc` and `sddc_gain_capture` as a NEW option distinct from ADC rate and old redundant KA9Q `-E`. No analogue changes for existing capture calls; 8 MHz remains default.
3. Update RX888R2 host offset to the selected IF, including the VHF fine-frequency correction in `RadioHandler::SetCenterFrequency`; ensure changes occur before tuning and starting stream. Validate filter setting, IF frequency and readback together; fail closed on unsupported firmware/protocol.
4. Compile host + matching FX3 image as a pair and verify against independent tuner frequency or known 92.7-MHz FM signal before AIS.
5. Test 8 MHz versus narrow 290/375/420 kHz IF at 92.7 MHz using 1-second baseline and short FM audio captures. ADC rate remains 20 MS/s; expect ~10 million complex CF32 samples (~80 MB) per 1-second *sample-count-limited* capture at both analogue BWs. A reduction in IQ samples requires a separate ADC/decimation change and does **not** verify analogue selectivity.
6. Compare measured audio quality, pilot lock (if demodulated), RF/interference rejection and actual timing. For AIS at centre 162 MHz preserve both +/-25-kHz channels and alternate repeated paired windows.

Do not interpret an output file with fewer samples as an RF selectivity measurement. Do not alter the SDDC 2.3 firmware auto-loader or the working AIS pipelines without a confirmed rollback plan.

## Hardware validation and overnight handoff — 2026-10-18 (AEST)

**Supersedes the earlier 'not yet compiled/validated' status above.** Host and new FX3 firmware compiled on patio receiver host **devlap200**. A new opt-in runtime firmware image override, `sddc_gain_capture -F <path>`, is supported; without it the original root `SDDC_FX3.img` stays the embedded default. Experimental image is `~/tools/rx888-sddc-fm007/SDDC_FX3/SDDC_FX3.img`. The independent Nooelec AIS-Catcher remains running throughout. Do not replace the original root image merely to test the override.

After USB unplug/replug, RX888 enumerated `Bus 001 Device 014 ID 04b4:00f3`. Granted temporary access with `sudo setfacl -m u:vk4tmz:rw /dev/bus/usb/001/014` (bus/device ID changes at each enumeration; verify first). Explicit `-F` loading logged `normal FW binary executable image with checksum`, `writing image...`, and `transfer execution to Program Entry at 0x40012d88`. Receiver subsequently enumerated as `04b4:00f1` (Bus 002 Device 009). When an already-running 00f1 firmware is present, `-F` cannot reload it; reboot to 00f3 before changing images.

**92.7 MHz MixFM functional A/B:** tuner requested 92,698,498 Hz with +16.2 ppm correction, RF 37.2 dB, IF 26.5 dB, ADC 20 MS/s, each capture 2 seconds. `-B 290000` and `-B 8000000` both returned status=0, 20,000,000 complex CF32 samples and precisely 160,000,000 bytes; wall-time ~2.007 s. User heard **clear, loud MixFM music in BOTH recordings** using FM demodulation over SSH from patio devlap200 to local **devpc001** via `aplay`. Audio and capture functionality validated, **not yet independent passband attenuation, register readback, quantitative SNR or AIS decoding improvement**. 290 kHz is a nominal analogue filter setting, not a digital sample-rate reduction. ADC 20 MS/s real -> nominal 10 MS/s complex -> 80 MB/s CF32.

**Overnight AIS session:** the existing `fm-measure/scripts/ais/ais-session.py` manager gained `--if-bandwidth` to pass `-B` to SDDC; tuner firmware already loaded, so overnight producer did not need `-F`. RX888 centre 162,000,000 Hz, +16.2 ppm, RF 37.2 dB, IF 26.5 dB, native IF bandwidth 290000, `--seconds 0` (continuous). Launched 2026-10-18T09:04:48Z in `~/tools/ais-gain-runs/rx888/20261018T090448Z_rf_37.2_if_26.5`, supervisor PID 50248, dashboard http://devlap200:8100 (HTTP 200). Nooelec reference PID 48616, `~/tools/ais-gain-runs/nooelec/20261018T082955Z_gain_43.9`, dashboard http://devlap200:8101 (HTTP 200). **These gains are working starting values, not a proven AIS optimum**. Both manager sessions reported RUNNING at handoff; HTTP 200 confirms web servers, not message decoding.

**Tomorrow:** inspect producer/decoder logs and AIS message/MMSI counts, preserve paired time windows, quantitatively compare 8 MHz vs 290 kHz filter spectrum/passband and AIS reception, check IF-centre placement, and measure gain sensitivity. Keep KA9Q udev boot rule masked until SDDC work is complete; restore from documented saved rule in `ka9q-radio-misc/docs/rx888-ka9q-sddc-firmware-handoff.md` when returning to KA9Q.
