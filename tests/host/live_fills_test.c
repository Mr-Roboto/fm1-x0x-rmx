/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static seq_t S;
static pattern_t P[NPAT];
static unsigned now;
static int reject_stored;
typedef struct { unsigned at; int kit, voice, velocity, live; } hit_t;
static hit_t hits[4096];
static int nhit, midi_on_count;
static int hit(void *ctx, int kit, int voice, float velocity)
{
    (void)ctx;
    if (reject_stored && !S.live_firing) return 0;
    assert(nhit < 4096);
    hits[nhit++] = (hit_t){now, kit, voice, (int)(velocity * 127.0f + 0.5f), S.live_firing};
    return 1;
}
static void on(void *ctx, int b, int n, int a, int sl) { (void)ctx;(void)b;(void)n;(void)a;(void)sl; }
static void off(void *ctx, int b) { (void)ctx;(void)b; }
static void midi_out(void *ctx, uint8_t st, uint8_t note, uint8_t vel)
{ (void)ctx;(void)note;if ((st & 0xf0) == 0x90 && vel) midi_on_count++; }
static const seq_sink_t sink = {hit,on,off,0,0,midi_out,0,0};

static void fixture(float bpm)
{
    for (int i = 0; i < NPAT; i++) pattern_init(&P[i], 123, 456);
    seq_init(&S, P); S.bpm = bpm; S.send_clock = 0; S.send_notes = 1;
    /* The transport grid must not follow the 909's polymetric loop/rate. */
    P[0].drum[0].len = 7; P[0].drum[0].rate = RATE_32;
    now = 0; nhit = midi_on_count = reject_stored = 0;
}
static void advance(unsigned samples, unsigned block)
{
    seq_advance(&S, 0, &sink);
    while (samples) {
        unsigned n = seq_until_event(&S, samples < block ? samples : block);
        now += n; seq_advance(&S, n, &sink); samples -= n;
    }
}
/* Independent score oracle, indexed by the musical 1/32 slot. */
static int expected(unsigned mask, int grid, int kit, int voice)
{
    const int slow[] = {95,70,85,75,105,75,90,80};
    const int medium[] = {95,70,80,65,100,75,85,70,105,75,90,70,110,80,95,85};
    const int fast[] = {110,65,85,70,100,65,90,75};
    int step = grid / 2 + 1, value = 0;
    if (kit == 0 && voice == DR_SD) {
        if ((mask & 1) && grid % 4 == 0) value = slow[grid / 4];
        if ((mask & 2) && grid % 2 == 0 && medium[grid / 2] > value) value = medium[grid / 2];
        if ((mask & 4) && fast[grid % 8] > value) value = fast[grid % 8];
    }
    if (grid % 2) return value;
    if ((mask & 8) && kit == 1 && voice == D8_CP)
        value = step == 3 ? 95 : step == 7 ? 90 : step == 11 ? 105 : step == 15 ? 110 : 0;
    if ((mask & 8) && kit == 1 && voice == D8_SD)
        value = step == 4 ? 65 : step == 12 ? 70 : step == 16 ? 85 : 0;
    if (mask & 16) {
        if (kit == 0 && voice == DR_HT)
            value = step == 1 ? 100 : step == 3 ? 80 : step == 4 ? 70 : step == 7 ? 95 : step == 8 ? 75 : step == 10 ? 85 : step == 11 ? 100 : step == 14 ? 80 : step == 15 ? 110 : step == 16 ? 100 : 0;
        if (kit == 1 && voice == D8_CP) {
            int clap = step == 5 ? 85 : step == 13 ? 95 : 0;
            if (clap > value) value = clap;
        }
    }
    return value;
}
static void scores_test(void)
{
    const float tempos[] = {135,150,155};
    const unsigned blocks[] = {1,17,256};
    const int kits[] = {0,1,0,1}, voices[] = {DR_SD,D8_CP,DR_HT,D8_SD};
    for (int tempo = 0; tempo < 3; tempo++) for (int block = 0; block < 3; block++)
        for (unsigned mask = 1; mask < 32; mask++) {
            fixture(tempos[tempo]); pattern_t saved[NPAT]; memcpy(saved,P,sizeof P);
            S.mute = S.vmute = ~0u; /* sequenced mutes must not mute performance hits */
            seq_start(&S); seq_live_fills(&S,mask);
            double slot = 44100.0 * 60.0 / tempos[tempo] / 8.0;
            /* Q8 musical boundaries are dispatched at their containing sample;
             * stay two samples clear of the next bar's rounded downbeat. */
            advance((unsigned)(slot * 32) - 2u, blocks[block]);
            int checked = 0;
            for (int grid = 0; grid < 32; grid++) for (int i = 0; i < LIVE_FILL_VOICES; i++) {
                int velocity = expected(mask,grid,kits[i],voices[i]);
                if (!velocity) continue;
                int found = 0;
                for (int h = 0; h < nhit; h++) if (hits[h].kit == kits[i] && hits[h].voice == voices[i] &&
                    fabs((double)hits[h].at - grid * slot) <= 2.0) {
                    assert(hits[h].velocity == velocity && hits[h].live); found++;
                }
                assert(found == 1); checked++;
            }
            if (nhit != checked || midi_on_count != checked || memcmp(saved,P,sizeof P))
                fprintf(stderr,"score bpm=%g block=%u mask=%u hits=%d midi=%d expected=%d changed=%d\n",tempos[tempo],blocks[block],mask,nhit,midi_on_count,checked,memcmp(saved,P,sizeof P));
            assert(nhit == checked && midi_on_count == checked && !memcmp(saved,P,sizeof P));
        }
    puts("LIVE fills: all 31 key combinations at 135/150/155 BPM, 1/17/256-frame blocks, exact scores/maximum velocities, no duplicates, mutes bypassed, MIDI and unchanged patterns passed");
}
static void phase_test(void)
{
    fixture(150); seq_start(&S); advance(36382,256); seq_live_fills(&S,2);
    advance(3307,17); assert(!nhit); advance(1,1);
    assert(nhit == 1 && hits[0].at == 39690 && hits[0].velocity == 75);
    seq_live_fills(&S,0); int count = nhit; advance(20000,256); assert(nhit == count);
    fixture(150); seq_start(&S); seq_live_fills(&S,1); advance(36382,256);
    count = nhit; seq_live_fills(&S,2); advance(3308,256);
    assert(nhit == count + 1 && hits[nhit-1].at == 39690);
    /* Tempo change: the next musical tick uses the new tempo, never a key timer. */
    fixture(150); seq_start(&S); seq_live_fills(&S,4); advance(2205,1);
    S.bpm = 135; count = nhit; advance(2204,1); assert(nhit == count);
    advance(1000,1); assert(nhit == count+1);
    assert(abs((int)hits[nhit-1].at-4573)<=1 && hits[nhit-1].velocity==85);
    /* External MIDI clock: one 1/32 hit every three 24-ppqn ticks. */
    fixture(150); seq_start(&S); seq_live_fills(&S,4);
    for (int tick = 0; tick < 96; tick++) {
        if (tick) advance(735,256);
        seq_ext_clock(&S); advance(0,256);
        assert(nhit == tick / 3 + 1);
    }
    assert(nhit == 32);
    seq_stop(&S); advance(0,256); assert(!S.live_fills);
    count = nhit; advance(44100,256); seq_live_fills(&S,31); advance(44100,256); assert(nhit == count);
    seq_start(&S); advance(5000,256); assert(nhit == count); /* stop clears held state */
    puts("LIVE fills: mid-bar entry, release, roll switching without reset, tempo change, external clock and stop/restart cleanup passed");
}
static void collision_test(void)
{
    fixture(150); P[0].drum[0].hit[DR_SD][0] = 1;
    seq_start(&S); seq_live_fills(&S,31); advance(0,256);
    int snare = 0;
    for (int i = 0; i < nhit; i++) if (!hits[i].kit && hits[i].voice == DR_SD) {
        assert(hits[i].velocity == 88 && !hits[i].live); snare++;
    }
    assert(snare == 1);
    fixture(150); P[0].drum[0].hit[DR_SD][0] = 1; reject_stored = 1;
    seq_start(&S); seq_live_fills(&S,4); advance(0,256);
    assert(nhit == 1 && hits[0].velocity == 110 && hits[0].live && midi_on_count == 1);
    puts("LIVE fills: stored-hit velocity priority and rejected stored hit falling back to one live trigger passed");
}

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(void)
{
    held_btn = held_keys = 0; memset(enc_acc,0,sizeof enc_acc); memset(&ui,0,sizeof ui);
    project_defaults(); engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
}
static void tap(int button)
{held_btn=1u<<button;ui_input_only();held_btn=0;ui_input_only();flush();}
static void ui_test(void)
{
    setup(); tap(B_HOME);tap(B_HOME);tap(B_HOME);assert(ui.view==V_LIVE);
    ui.live[0]=80;ui.live[1]=45;ui.live[2]=100;ui.live[3]=90;
    for(int i=0;i<4;i++)engine_live_set(i,ui.live[i]);flush();
    project_t reset_saved=proj;
    tap(B_ENV);
    assert(ui.live[0]==80 && ui.live[1]==45 && ui.live[2]==64 && ui.live[3]==64);
    assert(live_acid[0]==16 && live_acid[1]==-19 && !live_feedback && !live_drums_enabled);
    assert(!memcmp(&reset_saved,&proj,sizeof proj));
    const int physical[] = {13,15,17,20,22};
    project_t saved = proj;
    ui.rec = 1; /* performance hits must never write the pattern, even while REC is on */
    for (int i=0;i<5;i++) { assert(BLACK_KEY[5+i]==physical[i]); key_event(physical[i],1);flush(); }
    assert(!ui.live_fills && !seq.live_fills); /* keys pressed while stopped do not arm */
    seq_start(&seq);int32_t audio[512];engine_render(audio,256);
    for (int i=0;i<5;i++) {
        key_event(physical[i],1);flush();uint32_t queued=cq_w;
        key_event(physical[i],1);assert(cq_w==queued); /* ignore repeated down */
        assert(ui.live_fills==(1u<<(i+1))-1u && seq.live_fills==ui.live_fills);
    }
    ui.frame=8;leds();for(int i=0;i<5;i++)assert(lit_keys&(1u<<physical[i]));
    key_event(physical[1],0);flush();assert(ui.live_fills==29 && seq.live_fills==29);
    uint32_t mute=seq.mute;key_event(BLACK_KEY[0],1);flush();assert(seq.mute==(mute^seq_part_mask(0)));
    key_event(BLACK_KEY[10],1);flush();assert(seq.live_quantize);
    key_event(WHITE_KEY[12],1);flush();assert(seq.live_pending==5 && ui.live_fills==29);
    assert(!memcmp(&saved,&proj,sizeof proj));
    ui.help=0;ui.msg_until=0;ui.touched=-1;any_button=1;
    draw_header();draw_main();draw_knobs();png_write("build/host/live-fills-held.png");
    tap(B_EDIT);assert(!ui.live_fills && !seq.live_fills);
    tap(B_HOME);tap(B_HOME);tap(B_HOME);key_event(physical[0],1);flush();assert(seq.live_fills==1);
    seq_stop(&seq);engine_render(audio,256);ui_input_only();flush();assert(!ui.live_fills && !seq.live_fills);
    assert(!memcmp(&saved,&proj,sizeof proj));
    puts("LIVE fill UI: verified spare physical keys, independent holds/releases, repeat suppression, LEDs, existing mutes/transpose/bar key, exit/stop and unchanged project passed");
}
static void dsp_test(void)
{
    /* P2's +9 dB is a per-hit gain, never a stored voice or mixer change. */
    const int percussion[] = {DR_HT};
    for(int i=0;i<1;i++) {
        setup();float ordinary[512]={0},boosted[512]={0},rev[512]={0},dly[512]={0};
        sound_t saved=proj.sound;
        s_drum(0,0,percussion[i],100.0f/127.0f);
        drum909_render(&d909,ordinary,rev,dly,256);
        setup();seq.live_firing=1;s_drum(0,0,percussion[i],100.0f/127.0f);seq.live_firing=0;
        memset(rev,0,sizeof rev);memset(dly,0,sizeof dly);drum909_render(&d909,boosted,rev,dly,256);
        double normal_power=0,boost_power=0;
        for(int j=0;j<512;j++){normal_power+=(double)ordinary[j]*ordinary[j];boost_power+=(double)boosted[j]*boosted[j];}
        assert(normal_power>0 && fabs(boost_power/normal_power-7.943282)<0.001);
        assert(!memcmp(&saved,&proj.sound,sizeof saved));
    }
    /* A coincident stored hit keeps its own velocity and already-applied lock. */
    setup(); seq.bpm=150;
    int level=-1;
    for(int i=0;i<engine_nparams(T_909,DR_SD);i++)
        if(!strcmp(engine_param(T_909,DR_SD,i)->name,"Level"))level=i;
    assert(level>=0);
    int lane=motion_alloc(proj.arr.lane,0,PART_909,T_909,DR_SD,level);
    assert(lane>=0);proj.arr.lane[lane].val[0]=35;
    proj.pat[0].drum[0].hit[DR_SD][0]=1;
    seq_start(&seq);engine_live_fills(4);int32_t audio[512];engine_render(audio,256);
    float wanted=d909.bt[DR_SD].level*d909.accent*(1.0f-d909.vel_depth*(1.0f-88.0f/127.0f));
    assert(drum909_get(&d909,DR_SD,level)==35 && fabsf(d909.bt[DR_SD].out_gain-wanted)<1e-6f);
    /* Sequence chance never suppresses a live fill, nor raises its velocity. */
    setup();prob[0][DR_SD]=0;proj.pat[0].drum[0].hit[DR_SD][0]=1;
    seq_start(&seq);engine_live_fills(4);engine_render(audio,256);
    wanted=d909.bt[DR_SD].level*d909.accent*(1.0f-d909.vel_depth*(1.0f-110.0f/127.0f));
    assert(fabsf(d909.bt[DR_SD].out_gain-wanted)<1e-6f);
    setup();seq.bpm=150;seq.mute=seq.vmute=~0u;
    seq_start(&seq);engine_live_fills(31);double power=0;
    for(int block=0;block<1400;block++) {
        engine_render(audio,256);double block_power=0;
        for(int i=0;i<512;i++)block_power+=(double)audio[i]*audio[i];
        assert(block_power>0);power+=block_power;
        assert(isfinite(d909.bt[DR_SD].out_gain) && isfinite(d909.bt[DR_SD].amp.v));
    }
    assert(power>0 && seq.live_fills==31);
    float tail=d909.bt[DR_SD].amp.v;engine_live_fills(0);flush();engine_render(audio,256);
    assert(d909.bt[DR_SD].amp.v<=tail); /* release leaves the current tail; no new retrigger */
    assert(sizeof(sound_t)==1344 && sizeof(pattern_t)==528 && RAW_LEN==29572);
    puts("LIVE fill DSP: eight seconds of all fills through real 909/808/mixer/effects, rapid snare retrigger, finite state, release tail and unchanged persistent sizes passed");
}
int main(void){scores_test();phase_test();collision_test();ui_test();dsp_test();return 0;}
