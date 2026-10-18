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
