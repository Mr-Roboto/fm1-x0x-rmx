/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WAVEFORM_PROTOTYPE_H
#define WAVEFORM_PROTOTYPE_H

/* Host experiment only. phase in [0,1), 0 < increment < .5. No hidden state.
 * Prepare once per stationary morph/control update, evaluate at 44100*OS.
 * Decimation is deliberately the analysis harness's responsibility. */
typedef struct {
    float shark, trap, notch, dc, jump, slope[5];
    int endpoint;
} waveform_mix_t;
enum { WAVEFORM_RAW = 0, WAVEFORM_TRIANGULAR = 1, WAVEFORM_CUBIC = 2 };
/* Default selection only for the host experiment; the original kernel remains
 * selectable. Cubic uses four-box convolution, not the approved two-box kernel. */
#ifndef WAVEFORM_CORRECTION
#define WAVEFORM_CORRECTION WAVEFORM_CUBIC
#endif
void waveform_prepare(waveform_mix_t *mix, float morph);
float waveform_eval(const waveform_mix_t *mix, float phase, float increment, int corrected);
float waveform_endpoint(int shape, float phase, float increment, int corrected);
void waveform_samples(float *out, const float *phase, int count, float increment,
                      float morph, int corrected);
float waveform_legacy(int square, float phase, float increment);
/* Benchmark-only copied square initialization; call before legacy evaluation. */
void waveform_legacy_init(void);
/* Copied production decimator architecture. State belongs to the caller. */
typedef struct { float x1, y1; } waveform_ap_t;
typedef struct { waveform_ap_t hb1[6], hb2[3]; } waveform_decimator_t;
void waveform_decimate(waveform_decimator_t *state, float *out, const float *in,
                      int output_count, int oversampling);
#endif
