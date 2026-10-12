# SPDX-License-Identifier: GPL-3.0-only
"""Keep the German reference, firmware and actual C pitch names in agreement."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
firmware = (root / 'firmware/src/seq/tb3po.c').read_text()
doc = (root / 'docs/TB3PO_SCALES.md').read_text()
scales = [(list(map(int, offsets.split(',')))[:int(length)], int(length))
          for offsets, length in re.findall(r'\{\{([^}]+)\}, (\d+)\}', firmware)]
name_block = firmware.split('TB3PO_SCALE_NAMES[TB3PO_NSCALES] = {', 1)[1].split('};', 1)[0]
abbreviations = re.findall(r'"([A-Z]+)"', name_block)
full_names = [
    'Minor Pentatonic', 'Major Pentatonic', 'Egyptian Pentatonic', 'Hirajoshi',
    'Natural Minor', 'Major', 'Dorian', 'Phrygian', 'Lydian', 'Mixolydian', 'Locrian',
    'Harmonic Minor', 'Melodic Minor', 'Harmonic Major', 'Altered', 'Hungarian Minor',
    'Romanian Minor', 'Bhairav Thaat', 'Marva Thaat', 'Purvi Thaat', 'Todi Thaat', 'Arabic',
]
entries = re.findall(r'^\| (\d+) \| ([^|]+) \| ([A-Z]+) \| (\d+) \| ([\d ]+) \|$', doc, re.M)
assert len(entries) == len(scales) == len(abbreviations) == 22
pitch = {'C': 0, 'Des': 1, 'D': 2, 'Es': 3, 'E': 4, 'F': 5,
         'Fis': 6, 'Ges': 6, 'G': 7, 'As': 8, 'A': 9, 'B': 10, 'H': 11}
for i, (index, name, abbr, length, offsets) in enumerate(entries):
    expected, count = scales[i]
    assert int(index) == i and name == full_names[i] and abbr == abbreviations[i]
    assert int(length) == count and list(map(int, offsets.split())) == expected
    section = doc.split(f'### {i}. {name} ({abbr})\n', 1)[1].split('\n##', 1)[0]
    assert f'Vollständiger Name: **{name}**' in section
    assert f'Display: **`{abbr}`**' in section
    assert f'Anzahl der Töne: **{count}**' in section
    detail = re.search(r'Halbtonabstände vom Grundton: \*\*([\d ]+)\*\*', section)
    assert list(map(int, detail[1].split())) == expected
    c_notes = re.search(r'Töne bei ROOT C \(deutsche Namen\): \*\*([^*]+)\*\*', section)
    assert [pitch[note] for note in c_notes[1].split()] == expected
    for label in ['Charakteristische Intervalle', 'Musikalischer Eindruck', 'Einsatzideen']:
        assert f'**{label}:**' in section
print('TB-3PO reference: all 22 full names, abbreviations, counts, intervals and German C notes match firmware')
