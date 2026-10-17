# FM-007: experimental RX888 MkII CF32 capture

Work in this branch only; upstream `master` remains at RenardSpark.

`sddc_gain_capture` is an independent VHF research utility. It uses libsddc callbacks (interleaved native-endian float32 I/Q), not the old `sddc_vhf_stream_test` WAV-writing path.

Example, **after compilation and help check**:

```sh
./build/libsddc/sddc_gain_capture -h
./build/libsddc/sddc_gain_capture -f 104500000 -p 0 -r 20 -i 26.5 -s 20000000 -t 2 -o triplem-rx888-test.cf32
```

Options: `-f` requested nominal Hz, `-p` oscillator frequency correction ppm, `-r` RF-gain argument, `-i` IF gain, `-s` requested **real ADC** sample rate in Hz, `-t` seconds (maximum 60), `-o` CF32 output.

PPM sign: commanded LO = round(nominal / (1+ppm/1e6)). Positive ppm means a *fast* RF frequency reference; verify with known broadcast frequency before relying on sign. **PPM corrects LO tuning only; not sample clock**.

Important caveats:
- libsddc `sddc_set_rf_gain` semantics/range must be verified on RX888 MkII before treating input value as equivalent to RTL tuner gain dB; this code does not enable tuner hardware AUTO.
- The code assumes 20 MS/s real ADC gives ~10 MS/s complex IQ after r2iq; verify actual callback sample rate and capture duration empirically. A nominal two-second CF32 file can approach 160 MB. No embedded file header or sample-rate metadata is written.
- Cannot feed CF32 directly to fm_measure's CU8 analysis path.
- Experimental: compilation and actual hardware capture not yet verified. Stop and inspect output if warnings, gain mismatch, anomalous file size or sample-rate evidence appear.
- Do not modify KA9Q udev rules while trying this build.
