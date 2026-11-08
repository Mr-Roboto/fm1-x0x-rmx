#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare raw DCO relationships with June-21's actual Csound vco2 expressions.

Requires csound + a native C compiler. Compare samples away from discontinuities:
Csound is band-limited, our raw waveforms intentionally are not. DC is removed
analytically in C, before drive/filter; the Csound reference is centered equally.
"""
import os
import struct
import subprocess
from pathlib import Path

out=Path('build/host');out.mkdir(parents=True,exist_ok=True)
units=['asaw vco2 1,20,0','asq2 vco2 1,40,10',
       'asq4 vco2 1,80,10','asq8 vco2 1,160,10,0.5,0.5',
       'asub2 vco2 1,10,10','asub4 vco2 1,5,10',
       'asubpw2 vco2 1,10,2,0.75','asubpw4 vco2 1,5,2,0.75']
expressions=[];means=[]
for i,width in enumerate([.2,.5,.8]):
    units.append(f'apw{i} vco2 1,40,2,{width}')
    expressions += ['-asaw','(-asaw+1)*(-asq2+1)/4',
                    f'(-asaw+1)*(-apw{i}+1)/2',
                    '(-asaw+1)*(-asq8+1)',
                    '(-asaw+1)*(asq8+1)/2*(-asq2+1)/2']
    means += [0,.3125,(1-width)*(1+width/2),.9375,.328125]
for i,width in enumerate([.75,.5,.3]):
    units.append(f'apulse{i} vco2 1,20,2,{width}')
    expressions.append(f'-apulse{i}');means.append(1-2*width)
expressions += ['-asub2','-asubpw2','(-asub2+1)*(-asq2+1)/2',
                '(-asub2+1)*(-asq4+1)/2','-asub4','-asubpw4']
means += [0,-.5,.5,.5,0,-.5]
units += [f'aout{i}={expr}' for i,expr in enumerate(expressions)]
units.append('outch '+','.join(f'{i+1},aout{i}' for i in range(len(expressions))))
csd='''<CsoundSynthesizer>
<CsOptions>
-d -m0 -W -f -o build/host/juno-saw-reference.wav
</CsOptions>
<CsInstruments>
sr=48000
ksmps=1
nchnls=24
0dbfs=1
instr 1
'''+ '\n'.join(units)+'''
endin
</CsInstruments>
<CsScore>
i1 0 0.4
e
</CsScore>
</CsoundSynthesizer>
'''
path=out/'juno-saw-reference.csd';path.write_text(csd)
subprocess.run(['csound',str(path)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
exe=out/'juno_wave_probe'
subprocess.run([os.getenv('CC','cc'),'-O2','-ffp-contract=off','-o',str(exe),
                'tests/host/juno_wave_probe.c'],check=True)
rows=subprocess.check_output([str(exe)],text=True).splitlines()
wav=(out/'juno-saw-reference.wav').read_bytes();offset=12;data=None
while offset<len(wav):
    tag,size=struct.unpack_from('<4sI',wav,offset);offset+=8
    if tag==b'data':data=wav[offset:offset+size];break
    offset+=size+(size&1)
assert data is not None
worst=0
for row in rows:
    items=row.split(',');frame=int(items[0]);actual=list(map(float,items[1:]))
    reference=struct.unpack_from('<24f',data,frame*24*4)
    for channel,(value,ref,dc) in enumerate(zip(actual,reference,means)):
        error=abs(value-(ref-dc));worst=max(worst,error)
        assert error<.035,(frame,channel,value,ref-dc,error)
print(f'June-21 Csound reference: {len(rows)*24} phase samples, all saw modes at three PWM widths, pulse polarity and six sub modes passed; max edge-free error {worst:.5f}')
