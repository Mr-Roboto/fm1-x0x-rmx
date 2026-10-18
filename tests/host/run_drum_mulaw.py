#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Codec/A-B regression and v1.0.1 decoded-PCM renderer equivalence.

The PCM oracle is the released renderer, not a reimplementation of its DSP.
Optional --benchmark measures host process CPU time; it is not FM-1 profiling.
"""
import argparse
import ast
import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import statistics
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = "a3cca394f942d66b7b75a47af3e944ed03728765"
# Fingerprints from Charles Vestal's f56a395bb3448a4ea6001a32fecc123514dd8841
# and the independently prepared v1.0.1 A/B listening set.
EXPECTED = {
    "hh": (
        20250,
        "8ae963e80b8604bf23733545117d3b115767aa48ecd3013ad5d91506337d289a",
        "6214224e68f0c6e71e964f59f0a93a52a03a4ff5b99c5424b180e936ec3e349f",
        "3feb25ffab5b9c7c32d3575172e0125d681d91c7618764c342c44185916e7b7b",
    ),
    "ride": (
        46999,
        "f1d1cee24a7c91bb2ca9c6eb97badad3841226897ad0daabfc35801371d78182",
        "95ad3cb52f8dc103e329167c4afe52b740da216d149082eefa5d8c73942f860c",
        "a893f0f5dfe301250928f44f8c24f08d1030295bdef737862e25c0445f2ec1bf",
    ),
    "crash": (
        43300,
        "4a1b8d8828f9e530e9ae7f1a3cdffbcb1466e633a2b0607b3aac914699bc8400",
        "0b9b03a6fa791e3819940f2646f31eb6dce849598634131f75ca2f6070e4f6ce",
        "cac8c3d91bfc459b4ce5511f33a3e3f7317cfff1535adbfe2e5370819a6dbbd6",
    ),
}
ENC_SHA = "5ee7cf5f273f842d2234121e4cb0c98d6b20a99ac29026f94e05b36955b195be"
DEC_SHA = "3dab54339e520bb2c924826e3b72a917a2b612e9fd12fc867500f1d983a75827"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def released(path):
    return subprocess.check_output(["git", "show", f"{BASE}:{path}"], cwd=ROOT).decode()


def function(source, name):
    return ast.dump(next(n for n in ast.parse(source).body
                         if isinstance(n, ast.FunctionDef) and n.name == name))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--benchmark", action="store_true")
    args = ap.parse_args()
    os.chdir(ROOT)
    spec = importlib.util.spec_from_file_location("drum_samples", ROOT / "tools/gen_drum_samples.py")
    g = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(g)
    assert function(Path(g.__file__).read_text(), "read_wav") == function(
        released("tools/gen_drum_samples.py"), "read_wav"), "PCM/24-bit rounding changed"
    assert sha(bytes(g.mulaw_enc(v) for v in range(-32768, 32768))) == ENC_SHA
    table = [g.mulaw_dec(v) for v in range(256)]
    assert sha(struct.pack("<256h", *table)) == DEC_SHA
    out = ROOT / "build/host/drum_mulaw"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        subprocess.run(["python3", "tools/gen_drum_samples.py", str(tmp / "x0x_drum_samples.h"),
                        "--ref-wavs", str(tmp / "ref")], check=True)
        header = (tmp / "x0x_drum_samples.h").read_text()
        decode_array = re.search(r"static const int16_t x0x_mulaw\[256\] = \{(.*?)\};", header, re.S)
        assert decode_array and list(map(int, re.findall(r"-?\d+", decode_array[1]))) == table
        pcm_header = []
        ab = ROOT / "build/audio-comparison-pcm16-vs-mulaw-v1.0.1-2026-10-18/comparison"
        ab_count = 0
        for name, (count, source_sha, encoded_sha, wav_sha) in EXPECTED.items():
            source = ROOT / "assets/909" / (name + ".wav")
            assert sha(source.read_bytes()) == source_sha
            pcm, _ = g.read_wav(source)
            mu = bytes(g.mulaw_enc(v) for v in pcm)
            assert len(pcm) == count and sha(mu) == encoded_sha
            c_name = "x0x_smp_" + name
            array = re.search(r"static const uint8_t " + c_name + r"\[\d+\] = \{(.*?)\};", header, re.S)
            assert array and bytes(map(int, re.findall(r"\d+", array[1]))) == mu
            wav = (tmp / "ref" / (name + ".wav")).read_bytes()
            assert sha(wav) == wav_sha
            decoded = [table[v] for v in mu]
            assert wav[44:] == struct.pack(f"<{count}h", *decoded)
            pcm_header += [f"#define {c_name.upper()}_LEN {count}u",
                           g.arr("int16_t", c_name, decoded)]
            if ab.exists():
                assert wav == (ab / (name + "_mulaw_decoded16.wav")).read_bytes()
                original = (ab / (name + "_pcm16.wav")).read_bytes()
                assert original[44:] == struct.pack(f"<{count}h", *pcm)
                ab_count += 1
        print("PASS: exhaustive 65536 encodes, 256 decodes, unchanged PCM rounding, 3 source/array/reference hashes")
        print(f"Saved A/B pair comparisons: {ab_count}/3" + (" (fixtures absent; frozen hashes checked)" if not ab_count else ""))
        libs = []
        for variant in ("pcm", "mulaw"):
            d = tmp / variant
            d.mkdir()
            for file in ("drum909.c", "drum909.h"):
                path = "firmware/src/dsp/" + file
                (d / file).write_text(released(path) if variant == "pcm" else Path(path).read_text())
            (d / "x0x_drum_samples.h").write_text("\n".join(pcm_header) if variant == "pcm" else header)
            (d / "wrapper.c").write_text(Path("tests/host/drum_mulaw_test.c").read_text())
            so = d / "renderer.so"
            subprocess.run([os.environ.get("CC", "cc"), "-O2", "-std=c99", "-ffp-contract=off",
                            "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared", "-Wl,-Bsymbolic",
                            "-Ifirmware/src/dsp", "-Ifirmware/src", "-I" + str(tmp),
                            str(d / "wrapper.c"), "-lm", "-o", str(so)], check=True)
            lib = ctypes.CDLL(str(so))
            lib.render_case.argtypes = [ctypes.c_int] * 5 + [ctypes.c_void_p, ctypes.c_void_p]
            lib.benchmark.argtypes = [ctypes.c_int]
            lib.benchmark.restype = ctypes.c_double
            libs.append(lib)
        assert libs[0].engine_bytes() == libs[1].engine_bytes(), "engine RAM layout changed"
        arrays = [(ctypes.c_float * (60000 * 4))() for _ in libs]
        states = [(ctypes.c_uint32 * 16)() for _ in libs]
        cases = 0
        for voice in range(4):
            for pitch in (0, 64, 127):
                for dist in range(7):
                    for block in (1, 17, 256):
                        for lib, audio, state in zip(libs, arrays, states):
                            lib.render_case(voice, pitch, dist, block, 0, audio, state)
                        assert bytes(arrays[0]) == bytes(arrays[1]), (voice, pitch, dist, block)
                        assert bytes(states[0]) == bytes(states[1])
                        cases += 1
                # Fractional interpolation into the zero guard at the sample end.
                for lib, audio, state in zip(libs, arrays, states):
                    lib.render_case(voice, pitch, 0, 17, 1, audio, state)
                assert bytes(arrays[0]) == bytes(arrays[1]) and bytes(states[0]) == bytes(states[1])
                cases += 1
        print(f"PASS: {cases} bit-exact renderer cases (pitch, 7 distortions, 3 block sizes, choke/retrigger, stereo/sends, end guard/state)")
        if args.benchmark:
            timings = [[], []]
            for _ in range(9):
                for i in (_ % 2, 1 - _ % 2):
                    timings[i].append(libs[i].benchmark(512))
            med = [statistics.median(t) for t in timings]
            result = dict(pcm_seconds=med[0], mulaw_seconds=med[1],
                          overhead_percent=100 * (med[1] / med[0] - 1),
                          frames_per_trial=512 * 128 * 256, trials=timings,
                          note="Host process CPU time, 3 simultaneous sample voices; not FM-1 cycles")
            (out / "benchmark.json").write_text(json.dumps(result, indent=2) + "\n")
            print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
