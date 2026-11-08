# Juno raw DCO correction

Checkpoint before these changes: **b88ee1c**, the working six-voice engine with
64 presets. The rollback package is `build/x0x-harder-juno-64-presets.fwsc`.

The raw oscillator now follows the gate algebra in
[June-21's DCO block](https://github.com/mikerodd/june-21/blob/b46c09a0548cdbe1d23089c9406f3c67d4a83fb0/src/cabbage-module/june-21.csd).
Csound rendering confirmed its saw falls from +1 to -1, its square starts
positive, and the 8x gate has a half-cycle offset.

- Saw 1 keeps the existing rising bipolar saw.
- Saw 2 gates the offset saw with the negative half of the 2x square, at the
  original relative level. It no longer flips the saw's polarity.
- Saw 3 uses the same offset-saw product with a 2x PWM gate.
- Saw 4 uses the reference's phase-offset 8x gate and relative amplitude.
- Saw 5 uses the intersection of the 2x and phase-offset 8x gates. The old XOR
  combination selected the wrong windows.
- Companion pulse/sub polarity, fixed 75% pulse duty, and complex sub gate
  products are aligned with the reference so their mixtures reinforce/cancel
  the intended harmonics.

These gate products have DC components. June-21 removes DC using its pre-VCF
HPF; our oscillator subtracts each shape's analytical mean before drive/filter,
including the width-dependent mean of PWM. This avoids pushing the nonlinear
drive and filter with a waveform-dependent constant offset, with no new state.

This reproduces the raw gate relationships, not Csound's band-limited output or
a measured original Roland oscillator. No oversampling or band limiting is added.
Envelope curves, preset values, filter and mono chorus remain unchanged.
All six voices remain enabled and synth state remains **4,644 bytes**.

## Validation

`tests/host/juno_saw_reference.py` renders the actual June-21 expressions with
Csound, then compares the C oscillator at 1,536 samples away from waveform edges.
It covers all saw modes at three PWM widths, pulse polarity and all six sub modes.
The maximum difference was **0.00809**; residual differences are expected because
the Csound oscillators are band-limited while this engine intentionally is raw.
Run with Csound and a C compiler installed:

```
python3 tests/host/juno_saw_reference.py
```

The native DSP test also checks exact gate windows and zero cycle means, all 64
presets for audible finite output, oscillator tuning, and extreme control/pitch
stress. The hardware-profile test checks held six-note chords, all presets,
note releases, project migration and save/reload.

Hardware build: image **380,760 B**; static RAM **96,932 / 98,304 B**;
pool **333,300 / 344,064 B**. No voice reduction was required for memory fit.
Actual listening and full-project CPU load remain hardware assessments.
