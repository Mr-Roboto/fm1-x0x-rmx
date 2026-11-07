/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(void)
{
    held_btn = held_keys = 0; memset(enc_acc,0,sizeof enc_acc); memset(&ui,0,sizeof ui);
    project_defaults(); engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
}
static void tap(int button)
{ held_btn=1u<<button;ui_input_only();held_btn=0;ui_input_only();flush(); }
static void key(int physical)
{ key_event(physical,1);key_event(physical,0);flush(); }
static void knob(int i,int by)
{ accel_off=1;enc_acc[EN_K1+i]=by;ui_input_only();accel_off=0;flush(); }
static void macro_test(void)
{
    setup(); tap(B_HOME);assert(ui.view==V_HOME);tap(B_HOME);assert(ui.view==V_SONG);tap(B_HOME);assert(ui.view==V_LIVE);
    project_t saved=proj;
    for(int i=0;i<4;i++) assert(pg.r[0][i].kind==R_LIVE && pref_get(pg.r[0][i])==64);
    ui.help=0;ui.msg_until=0;ui.touched=-1;any_button=1;
    draw_header();draw_main();draw_knobs();png_write("build/host/live-neutral.png");
    int cutoff=proj.sound.v[T_303][0][BASS303_CUTOFF],reso=proj.sound.v[T_303][0][BASS303_RESO];
    knob(0,20);assert(bass303_get(&b303[0],BASS303_CUTOFF)==cutoff+40);
    assert(bass303_get(&b303[0],BASS303_RESO)==reso+5);
    assert(bass303_get(&b303[1],BASS303_CUTOFF)==proj.sound.v[T_303][1][BASS303_CUTOFF]);
    apply_param(T_303,0,BASS303_CUTOFF,20);assert(bass303_get(&b303[0],BASS303_CUTOFF)==60);
    knob(0,-20);assert(bass303_get(&b303[0],BASS303_CUTOFF)==20);
    apply_param(T_303,0,BASS303_CUTOFF,cutoff);
    knob(1,-64);assert(bass303_get(&b303[1],BASS303_CUTOFF)==0);
    knob(1,64);assert(bass303_get(&b303[1],BASS303_CUTOFF)==proj.sound.v[T_303][1][BASS303_CUTOFF]);
    int base=fxbus_get(&fx,FX_DL_FDBK);knob(2,10);assert(fxbus_get(&fx,FX_DL_FDBK)==base+20);
    apply_param(T_FX,FX_DL_FDBK/16,FX_DL_FDBK%16,30);assert(fxbus_get(&fx,FX_DL_FDBK)==50);
    knob(2,-10);assert(fxbus_get(&fx,FX_DL_FDBK)==30);
    key(WHITE_KEY[8]);assert(seq.live_transpose==1);
    key(WHITE_KEY[7]);assert(seq.live_transpose==0);
    assert(!memcmp(&saved,&proj,sizeof proj));
    ui.frame=0;leds();assert(lit_keys&(1u<<WHITE_KEY[7]));assert(!(lit_keys&(1u<<BLACK_KEY[10])));
    key(BLACK_KEY[10]);assert(ui.live_quantize && seq.live_quantize);
    seq.playing=1;key(WHITE_KEY[11]);assert(seq.live_pending==4 && seq.live_transpose==0);
    ui.frame=0;leds();uint32_t dark=lit_keys;ui.frame=8;leds();uint32_t bright=lit_keys;
    assert((dark&(1u<<WHITE_KEY[7])) && (bright&(1u<<WHITE_KEY[7])));
    assert(!(dark&(1u<<WHITE_KEY[11])) && (bright&(1u<<WHITE_KEY[11])) && (bright&(1u<<BLACK_KEY[10])));
    key(WHITE_KEY[13]);assert(seq.live_pending==6);key(WHITE_KEY[7]);assert(seq.live_pending==-128);
    key(WHITE_KEY[9]);assert(seq.live_pending==2);key(BLACK_KEY[10]);assert(seq.live_transpose==2 && seq.live_pending==-128);
    seq.playing=0;
    uint32_t mute=seq.mute;for(int i=0;i<5;i++)key(BLACK_KEY[i]);assert(seq.mute==(mute^((1u<<MUTE_BRK|seq_part_mask(0)|seq_part_mask(1)|seq_part_mask(2)|seq_part_mask(3)))));
    assert(!memcmp(&saved,&proj,sizeof proj));
    tap(B_HOME);assert(ui.view==V_HOME);tap(B_HOME);tap(B_HOME);assert(ui.view==V_LIVE);
    ui.frame=8;ui.help=0;ui.msg_until=0;ui.touched=-1;any_button=1;
    draw_header();draw_main();draw_knobs();png_write("build/host/live-immediate.png");
    seq.playing=1;key(BLACK_KEY[10]);key(WHITE_KEY[12]);
    draw_header();draw_main();draw_knobs();png_write("build/host/live-queued.png");
    seq.playing=0;run_action(ACT_RESET,0);flush();assert(!seq.live_quantize && seq.live_transpose==0);
    assert(!live_acid[0] && !live_acid[1] && !live_feedback && !live_drums_enabled);
    assert(help_fits());
    puts("LIVE UI: HOME cycle, four bipolar macros, automation-relative offsets, unchanged project, transpose keys/LEDs, latest queue/cancel, toggle, mutes and reset passed");
}

static seq_t S;static pattern_t P[NPAT];static unsigned sample;
static uint8_t midi_notes[2][128];
typedef struct{unsigned at;int b,n,retune;}event_t;static event_t events[2048];static int count;
static void on(void*x,int b,int n,int a,int sl){(void)x;(void)a;(void)sl;assert(count<2048);events[count++]=(event_t){sample,b,n,S.live_retuning};}
static void off(void*x,int b){(void)x;(void)b;}
static int drum(void*x,int k,int v,float a){(void)x;(void)k;(void)v;(void)a;return 1;}
static void midi_note(void*x,uint8_t st,uint8_t note,uint8_t vel)
{(void)x;int b=(st&15)-1;if(b<0||b>1)return;if((st&0xf0)==0x90)midi_notes[b][note]=vel!=0;else if((st&0xf0)==0x80)midi_notes[b][note]=0;}
static const seq_sink_t sink={drum,on,off,0,0,midi_note,0,0};
static void advance(unsigned n)
{seq_advance(&S,0,&sink);while(n){unsigned k=seq_until_event(&S,n>256?256:n);sample+=k;seq_advance(&S,k,&sink);n-=k;}}
static void clock_setup(void)
{
    for(int i=0;i<NPAT;i++)pattern_init(&P[i],123,456);seq_init(&S,P);S.send_clock=0;S.bpm=120;sample=count=0;
    memset(midi_notes,0,sizeof midi_notes);S.send_notes=1;
    P[0].drum[0].len=7;P[0].bass[0].len=64;P[0].bass[0].rate=RATE_32;P[0].bass[1].len=3;
    for(int b=0;b<2;b++)for(int i=0;i<NSTEPS;i++){P[0].bass[b].step[i].note=(uint8_t)(60+12*b);P[0].bass[b].step[i].flags=BS_SLIDE|(i?G_TIE:G_NOTE);}
}
static void timing_test(void)
{
    clock_setup();seq_start(&S);advance(1000);
    seq_live_request(&S,2,&sink);assert(S.live_transpose==2 && events[count-2].n==62 && events[count-1].n==74);
    assert(events[count-1].retune && events[count-1].at==1000);
    seq_live_quantize(&S,1,&sink);seq_live_request(&S,5,&sink);advance(87199);
    assert(S.live_pending==5 && S.live_transpose==2);
    advance(2);assert(S.live_transpose==5 && S.live_pending==-128);
    assert(events[count-1].retune && abs((int)events[count-1].at-88200)<=1);
    seq_live_request(&S,-3,&sink);seq_stop(&S);advance(0);assert(S.live_transpose==-3 && S.live_pending==-128);
    clock_setup();seq_start(&S);seq_ext_clock(&S);advance(0);
    seq_live_quantize(&S,1,&sink);seq_live_request(&S,7,&sink);
    for(int i=1;i<96;i++){advance(919);seq_ext_clock(&S);advance(0);assert(S.live_transpose==0);}
    advance(919);seq_ext_clock(&S);advance(0);assert(S.live_transpose==7 && S.live_pending==-128);
    clock_setup();S.bass_mirror=1;S.bass_mirror_transpose=36;seq_start(&S);advance(1000);
    seq_live_request(&S,-7,&sink);assert(events[count-2].n==53 && events[count-1].n==65);
    seq_live_request(&S,8,&sink);assert(events[count-2].n==68 && events[count-1].n==80);
    assert(P[0].bass[0].transpose==24 && P[0].bass[1].transpose==24);
    clock_setup();P[0].bass[0].step[0].note=0;P[0].bass[1].step[0].note=127;
    seq_start(&S);advance(1000);seq_live_request(&S,-24,&sink);seq_live_request(&S,24,&sink);
    assert(midi_notes[0][24] && midi_notes[1][127] && !midi_notes[0][0]);
    seq_stop(&S);advance(0);for(int b=0;b<2;b++)for(int n=0;n<128;n++)assert(!midi_notes[b][n]);
    puts("LIVE clock: immediate held-note retune, true four-beat bar independent of pattern length/rate, 96 external ticks, stop, mirror and no cumulative transpose passed");
}

static double filter_power(int value,float hz)
{
    setup();live_command(3,(uint32_t)value);float wave[256];double power=0;int sample=0;
    for(int b=0;b<120;b++){
        for(int i=0;i<256;i++,sample++)wave[i]=sinf(2*FM_PI*hz*(float)sample/44100.0f);
        live_filter_render(&live_drums[0],wave,256);
        if(b>60)for(int i=0;i<256;i++){assert(isfinite(wave[i]));power+=wave[i]*wave[i];}
    }return power;
}
static void acid_audio(int filter,int32_t*out)
{
    setup();engine_live_set(3,filter);engine_bass_on(0,48,0,0);flush();
    for(int i=0;i<16;i++)engine_render(out+i*512,256);
}
static double drum_audio(int part,int filter,int32_t*out)
{
    setup();engine_live_set(3,filter);
    if(part<2)engine_drum(part,0,1.0f);
    else {engine_brk_loops();engine_brk_live(0,1);}
    flush();double power=0;
    for(int i=0;i<16;i++)engine_render(out+i*512,256);
    for(int i=0;i<8192;i++)power+=(double)out[i]*out[i];
    return power;
}
static void audio_test(void)
{
    double lp_low=filter_power(8,40),lp_high=filter_power(8,8000);
    double hp_low=filter_power(118,40),hp_high=filter_power(118,8000);
    assert(lp_high<lp_low*.01 && hp_low<hp_high*.01);
    static int32_t reference[8192],filtered[8192];acid_audio(64,reference);acid_audio(8,filtered);
    assert(!memcmp(reference,filtered,sizeof reference));acid_audio(118,filtered);assert(!memcmp(reference,filtered,sizeof reference));
    const int parts[]={PART_909,PART_808,PART_BRK};
    for(int i=0;i<3;i++){
        double original=drum_audio(parts[i],64,reference),highpassed=drum_audio(parts[i],118,filtered);
        assert(original>0 && highpassed<original*.9 && memcmp(reference,filtered,sizeof reference));
    }
    setup();float a[256],copy[256];for(int i=0;i<256;i++)a[i]=copy[i]=(float)i/256.0f;
    live_filter_render(&live_drums[0],a,256);assert(!memcmp(a,copy,sizeof a));
    assert(sizeof(sound_t)==1344 && sizeof(pattern_t)==528 && RAW_LEN==29572);
    puts("LIVE audio: LP/HP response, finite output, exact neutral bypass and both drum filter directions leave 303 audio bit-exact passed");
}
int main(void){macro_test();timing_test();audio_test();return 0;}
