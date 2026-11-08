/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/dsp/alpha_juno_wave.h"
#include <stdio.h>

int main(void)
{
    const float phases[]={.03f,.09f,.16f,.22f,.28f,.34f,.41f,.47f,
                          .53f,.59f,.66f,.72f,.78f,.84f,.91f,.97f};
    const float widths[]={.2f,.5f,.8f}, pulses[]={.75f,.5f,.3f};
    for(int cycle=4;cycle<8;cycle++)for(int p=0;p<16;p++) {
        int frame=cycle*2400+(int)(phases[p]*2400.0f+0.5f);
        float phase=(float)(frame%2400)/2400.0f;
        float quarter=(float)(frame%9600)/9600.0f;
        printf("%d",frame);
        for(int w=0;w<3;w++)for(int mode=1;mode<=5;mode++)
            printf(",%.9g",(double)juno_saw_wave(phase,widths[w],mode));
        for(int w=0;w<3;w++)printf(",%.9g",(double)juno_pulse_wave(phase,pulses[w]));
        for(int mode=0;mode<6;mode++)printf(",%.9g",(double)juno_sub_wave(quarter,phase,mode));
        puts("");
    }
    return 0;
}
