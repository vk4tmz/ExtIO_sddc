/* Independent RX888 VHF CF32 capture, intended for fm-measure.
 * GPL-3.0-or-later. Raw interleaved native-endian float32 I,Q.
 * No hardware auto-gain setting is implied by these manual gain controls.
 */
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "libsddc.h"

static volatile sig_atomic_t interrupted = 0;
static FILE *out = NULL;
static uint64_t received = 0, goal = 0;
static int write_failed = 0;
static void on_signal(int sig) { (void)sig; interrupted = 1; }

static void callback(uint32_t samples, const sddc_complex_t *data, void *context) {
    (void)context;
    if (!out || interrupted || write_failed) return;
    /* libsddc forwards RadioHandler::AttachIQ length as COMPLEX SAMPLE COUNT,
       not as bytes; see RadioHandler::OnDataPacket callbackIQ(..., len_iq/2). */
    const size_t n = samples;
    size_t wanted = (uint64_t)n > goal - received ? (size_t)(goal - received) : n;
    if (wanted && fwrite(data, sizeof(sddc_complex_t), wanted, out) != wanted)
        write_failed = 1;
    received += wanted;
}
static void print_steps(const char *name, libsddc_handler_t dev, int rf) {
    const float *steps = NULL;
    int count = rf ? sddc_get_rf_gain_steps(dev, &steps) : sddc_get_if_gain_steps(dev, &steps);
    fprintf(stderr, "%s: %d steps", name, count);
    for (int i = 0; i < count; ++i) fprintf(stderr, " %g", steps[i]);
    fprintf(stderr, "\n");
    free((void *)steps);
}
static void usage(const char *name) {
    fprintf(stderr, "Usage: %s -f Hz -o file.cf32 [-p ppm] [-r RF_dB] [-i IF_dB] [-s ADC_Hz] [-t seconds] [-P RF_precondition_dB] [-L list gain steps]\n", name);
    fprintf(stderr, "PPM convention: corrected tuner request = round(nominal_Hz / (1 + ppm/1e6)); positive ppm means oscillator runs fast.\n");
    fprintf(stderr, "Output is interleaved native-endian float32 I,Q; ADC rate is NOT necessarily complex IQ rate.\n");
}
static int parse_double(const char *arg, double *result) {
    char *end = NULL;
    errno = 0;
    *result = strtod(arg, &end);
    return errno == 0 && end != arg && *end == '\0' && isfinite(*result);
}
int main(int argc, char **argv) {
    double frequency = 0, ppm = 0, rf_gain = 0, if_gain = 0, seconds = 2, rf_precondition = 0;
    int use_rf = 0, use_if = 0, list_only = 0, use_precondition = 0;
    uint32_t adc_rate = 20000000;
    const char *filename = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "hLf:o:p:r:i:s:t:P:")) != -1) {
        double value;
        switch (opt) {
        case 'h': usage(argv[0]); return 0;
        case 'L': list_only = 1; break;
        case 'o': filename = optarg; break;
        case 'f': if (!parse_double(optarg, &frequency)) return 2; break;
        case 'p': if (!parse_double(optarg, &ppm)) return 2; break;
        case 'r': if (!parse_double(optarg, &rf_gain)) return 2; use_rf = 1; break;
        case 'P': if (!parse_double(optarg, &rf_precondition)) return 2; use_precondition = 1; break;
        case 'i': if (!parse_double(optarg, &if_gain)) return 2; use_if = 1; break;
        case 's':
            if (!parse_double(optarg, &value) || value < 1 || value > UINT32_MAX || floor(value) != value) return 2;
            adc_rate = (uint32_t)value;
            break;
        case 't': if (!parse_double(optarg, &seconds)) return 2; break;
        default: usage(argv[0]); return 2;
        }
    }
    if ((!list_only && (!filename || frequency < 30000000 || frequency > 1800000000)) ||
        fabs(ppm) > 500 || seconds <= 0 || seconds > 60 ||
        (use_precondition && (!use_rf || rf_precondition == rf_gain)) || optind != argc) {
        usage(argv[0]); return 2;
    }
    if (list_only && frequency == 0) frequency = 104500000;
    double adjusted = frequency / (1.0 + ppm * 1e-6);
    if (adjusted < 1 || adjusted > UINT32_MAX) return 2;
    uint32_t command_hz = (uint32_t)llround(adjusted);
    fprintf(stderr, "Nominal %.0f Hz; ppm %+.4f; tuner command %" PRIu32 " Hz; ADC request %" PRIu32 " Hz\n",
            frequency, ppm, command_hz, adc_rate);
    libsddc_handler_t dev = sddc_create();
    if (!dev) return 1;
    int rc = 1, running = 0;
    if (sddc_init(dev, 0) != ERR_SUCCESS) { fprintf(stderr, "SDDC init failed\n"); goto done; }
    if (sddc_set_rf_mode(dev, VHFMODE) != ERR_SUCCESS ||
        sddc_set_adc_sample_rate(dev, adc_rate) != ERR_SUCCESS ||
        sddc_set_decimation(dev, 0) != ERR_SUCCESS) {
        fprintf(stderr, "VHF/ADC/decimation configuration failed\n"); goto done;
    }
    fprintf(stderr, "Reported ADC rate: %" PRIu32 " Hz; nominal r2iq rate: %" PRIu32 " complex samples/s\n",
            sddc_get_adc_sample_rate(dev), sddc_get_adc_sample_rate(dev) / 2);
    print_steps("RF steps (driver units)", dev, 1);
    print_steps("IF steps (driver units)", dev, 0);
    if (list_only) { rc = 0; goto done; }
    uint32_t actual = sddc_set_center_frequency(dev, command_hz);
    /* API returns uint32_t but implementation forwards an error code;
       do not treat this return as an actual tuned frequency. */
    (void)actual;
    /* The same device instance is retained across both setters.
     * This probes whether an RF=0 request after a nonzero state actually
     * reprograms hardware; separate capture processes cannot test that.
     */
    if (use_precondition) {
        fprintf(stderr, "RF precondition: %.1f -> final %.1f (same SDDC session)\\n",
                rf_precondition, rf_gain);
        if (sddc_set_rf_gain(dev, (float)rf_precondition) != ERR_SUCCESS) {
            fprintf(stderr, "RF precondition failed\\n");
            goto done;
        }
    }
    if ((use_rf && sddc_set_rf_gain(dev, (float)rf_gain) != ERR_SUCCESS) ||
        (use_if && sddc_set_if_gain(dev, (float)if_gain) != ERR_SUCCESS)) {
        fprintf(stderr, "Gain configuration failed\n"); goto done;
    }
    out = fopen(filename, "wb");
    if (!out) { perror("open capture"); goto done; }
    /* Nominal complex-rate estimate: verify by callback rate at runtime.
       r2iq converts a real ADC stream, and this factor depends on its implementation. */
    double complex_rate = (double)adc_rate / 2;
    goal = (uint64_t)llround(seconds * complex_rate);
    if (!goal) goto done;
    if (sddc_set_stream_callback(dev, callback, NULL) != ERR_SUCCESS ||
        sddc_start_streaming(dev) != ERR_SUCCESS) {
        fprintf(stderr, "Streaming start failed\n"); goto done;
    }
    running = 1;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    struct timespec started, ended;
    clock_gettime(CLOCK_MONOTONIC, &started);
    struct timespec nap = {0, 10000000};
    while (received < goal && !interrupted && !write_failed) nanosleep(&nap, NULL);
    clock_gettime(CLOCK_MONOTONIC, &ended);
    double elapsed = (ended.tv_sec - started.tv_sec) + (ended.tv_nsec - started.tv_nsec) * 1e-9;
    fprintf(stderr, "Wall-time %.3f s; callback throughput %.3f Mcomplex/s (not an independent ADC clock calibration)\n", elapsed, received / elapsed / 1e6);
    rc = (!interrupted && !write_failed && received == goal) ? 0 : 1;
done:
    if (running) sddc_stop_streaming(dev);
    if (out && fclose(out) != 0) rc = 1;
    out = NULL;
    sddc_destroy(dev);
    fprintf(stderr, "Saved %" PRIu64 " complex samples (%" PRIu64 " bytes), requested %" PRIu64 ", status %d\n",
            received, received * (uint64_t)sizeof(sddc_complex_t), goal, rc);
    return rc;
}
