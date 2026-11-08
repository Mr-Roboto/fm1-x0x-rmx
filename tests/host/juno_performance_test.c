/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>
static void flush(void)
{
    drain();
    engine_sync_sound(&proj.sound);
}
static void setup(void)
{
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    memset(&ui, 0, sizeof ui);
    project_defaults();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound);
    flush();
    ui_init();
    ui.part = PART_BRK;
    build_pages();
}
static void set(int i, int v)
{
    pref_set(PR(R_ENG, T_BRK, JUNO_EXTRA_BANK, i), v);
    flush();
}
static void render(int n)
{
    int32_t a[512];
    while (n) {
        int k = n < 256 ? n : 256;
        engine_render(a, k);
        n -= k;
    }
}
static void tap(int b)
{
    held_btn = 1u << b;
    ui_input_only();
    held_btn = 0;
    ui_input_only();
    flush();
}
static int newest(void)
{
    int pick = -1;
    for (int i = 0; i < 6; i++)
        if (part_juno.voice[i].active &&
            (pick < 0 || part_juno.voice[i].serial > part_juno.voice[pick].serial))
            pick = i;
    return pick < 0 ? -1 : part_juno.voice[pick].note;
}
static int gates(void)
{
    int n = 0;
    for (int i = 0; i < 6; i++)
        n += part_juno.voice[i].gate;
    return n;
}
int main(void)
{
    setup();
    set(JX_MODE, 1);
    juno_part_key(60, 100);
    render(512);
    uint32_t serial = part_juno.serial;
    juno_part_key(64, 100);
    assert(gates() == 1 && newest() == 64 && part_juno.serial == serial + 1);
    juno_part_key(64, 0);
    assert(newest() == 60);
    juno_part_key(60, 0);
    assert(!gates());
    setup();
    set(JX_MODE, 2);
    juno_part_key(60, 100);
    render(1000);
    serial = part_juno.serial;
    float env = part_juno.voice[0].env;
    juno_part_key(64, 100);
    assert(part_juno.serial == serial && part_juno.voice[0].env == env && newest() == 64);
    juno_part_key(64, 0);
    assert(newest() == 60 && part_juno.serial == serial);
    set(JX_GLIDE, 80);
    juno_part_key(72, 100);
    assert(part_juno.voice[0].pitch < 61);
    render(2205);
    assert(part_juno.voice[0].pitch > 60 && part_juno.voice[0].pitch < 72);
    juno_part_key(72, 0);
    juno_part_key(60, 0);
    assert(!gates());
    puts("Juno voice modes: mono retrigger, legato no retrigger, last-note fallback, glide and "
         "release passed");
    const int expect[5][8] = {{60, 64, 67, 60, 64, 67, 60, 64},
                              {67, 64, 60, 67, 64, 60, 67, 64},
                              {60, 64, 67, 64, 60, 64, 67, 64},
                              {67, 64, 60, 64, 67, 64, 60, 64},
                              {67, 60, 64, 67, 60, 64, 67, 60}};
    for (int pattern = 0; pattern < 6; pattern++) {
        setup();
        set(JX_ARP, 1);
        set(JX_ARP_PATTERN, pattern);
        juno_part_transport(120, 1);
        juno_part_key(67, 100);
        juno_part_key(60, 100);
        juno_part_key(64, 100);
        for (int n = 0; n < 8; n++) {
            juno_part_tick(0, (unsigned)n * 6);
            int note = newest();
            if (pattern < 5)
                assert(note == expect[pattern][n]);
            else
                assert(note == 60 || note == 64 || note == 67);
        }
        juno_part_key(67, 0);
        juno_part_key(60, 0);
        juno_part_key(64, 0);
        assert(!gates());
    }
    setup();
    set(JX_ARP, 1);
    set(JX_ARP_OCT, 2);
    set(JX_LATCH, 1);
    juno_part_transport(120, 1);
    juno_part_key(60, 100);
    juno_part_key(60, 0);
    for (int n = 0; n < 6; n++) {
        juno_part_tick(0, (unsigned)n * 6);
        assert(newest() == 60 + (n % 3) * 12);
    }
    set(JX_LATCH, 0);
    assert(!gates());
    setup();
    set(JX_ARP, 1);
    juno_part_key(60, 100);
    juno_part_midi(0xb0, 64, 127);
    juno_part_key(60, 0);
    juno_part_tick(0, 0);
    assert(gates());
    juno_part_midi(0xb0, 64, 0);
    assert(!gates());
    puts("Juno ARP: six standard orders, octave cycling, latch, pedal hold and cleanup passed");
    for (int playing = 0; playing < 2; playing++) {
        setup();
        seq.bpm = 150;
        set(JX_ARP, 1);
        juno_part_key(60, 100);
        juno_part_key(64, 100);
        juno_part_key(67, 100);
        if (playing)
            seq_start(&seq);
        int count = 0, last = -1;
        serial = part_juno.serial;
        for (int sample = 0; sample < 14000; sample++) {
            render(1);
            if (part_juno.serial != serial) {
                if (last >= 0)
                    assert(abs(sample - last - 4410) <= 1);
                last = sample;
                serial = part_juno.serial;
                assert(newest() == expect[0][count]);
                count++;
            }
        }
        assert(count == 4);
        if (playing) {
            seq_stop(&seq);
            render(1);
            assert(!gates());
        }
    }
    setup();
    set(JX_TRANCE, 1);
    seq.bpm = 150;
    seq_start(&seq);
    render(1);
    for (int n = 0; n < 100; n++) {
        render(17);
        assert(juno_part_gate_step() == ((seq.live_phase + 95u) % 96u) / 6u);
    }
    seq.ext = 1;
    for (int n = 0; n < 96; n++) {
        seq_ext_clock(&seq);
        render(16);
        assert(juno_part_gate_step() == ((seq.live_phase + 95u) % 96u) / 6u);
    }
    puts("Juno clock: exact 1/16 timing stopped/playing, internal clock, external MIDI clock and "
         "stop cleanup passed");
    setup();
    juno_part_transport(125, 1);
    juno_part_key(60, 127);
    float a[256];
    breaks_t b;
    for (int i = 0; i < 10; i++)
        breaks_render(&b, a, 256);
    set(JX_TRANCE, 1);
    set(JX_GATE0, 1);
    set(JX_GATE1, 0);
    set(JX_GATE2, 0);
    set(JX_GATE3, 0);
    juno_part_tick(0, 0);
    breaks_render(&b, a, 256);
    double power = 0;
    for (int i = 0; i < 256; i++)
        power += a[i] * a[i];
    assert(power > 0);
    juno_part_tick(0, 6);
    breaks_render(&b, a, 256);
    breaks_render(&b, a, 256);
    for (int i = 0; i < 256; i++)
        assert(a[i] == 0);
    set(JX_TRANCE, 0);
    set(JX_FILTER, 1);
    for (int i = 0; i < 20; i++)
        breaks_render(&b, a, 256);
    for (int i = 0; i < 256; i++)
        assert(isfinite(a[i]));
    set(JX_FILTER, 127);
    for (int i = 0; i < 20; i++)
        breaks_render(&b, a, 256);
    for (int i = 0; i < 256; i++)
        assert(isfinite(a[i]));
    puts("Juno trance gate and DJ filter: audible on steps, silent off steps, smooth switching and "
         "finite LP/HP passed");
    setup();
    assert(juno_gate_mask() == JUNO_GATE_DEFAULT);
    tap(B_SEL);
    assert(ui.overlay == O_LIST && ui.list_n == 5);
    ui.list_sel = 3;
    assert(juno_gate_editing());
    pattern_t patterns_before[NPAT];
    memcpy(patterns_before, proj.pat, sizeof patterns_before);
    serial = part_juno.serial;
    for (int w = 0; w < 16; w++) {
        key_event(WHITE_KEY[w], 1);
        key_event(WHITE_KEY[w], 0);
    }
    flush();
    assert(juno_gate_mask() == (JUNO_GATE_DEFAULT ^ 0xffffu) && part_juno.serial == serial);
    assert(!memcmp(patterns_before, proj.pat, sizeof patterns_before));
    leds();
    for (int w = 0; w < 16; w++)
        assert(!!(lit_keys & (1u << WHITE_KEY[w])) == !!(juno_gate_mask() & (1u << w)));
    tap(B_HOME);
    ui.page[V_PART][PART_BRK] = 11;
    build_pages();
    assert(pg.r[11][1].a == T_BRK && pg.r[11][1].b == JUNO_EXTRA_BANK &&
           pg.r[11][1].c == JX_FILTER);
    tap(B_ARP);
    assert(ui.view == V_GEN && proj.sound.v[T_BRK][JUNO_EXTRA_BANK][JX_ARP]);
    assert(pg.n == 1 && pg.r[0][0].c == JX_ARP_PATTERN);
    tap(B_EDIT);
    assert(ui.view == V_PART && proj.sound.v[T_BRK][JUNO_EXTRA_BANK][JX_ARP]);
    tap(B_ARP);
    assert(!proj.sound.v[T_BRK][JUNO_EXTRA_BANK][JX_ARP]);
    set(JX_MODE, 2);
    set(JX_GLIDE, 75);
    set(JX_FILTER, 90);
    assert(project_save() == 0);
    project_t saved = proj;
    assert(project_load() == 0);
    assert(!memcmp(&saved, &proj, sizeof proj));
    engine_apply_sound(&proj.sound);
    flush();
    assert(alpha_juno_preset_values(32)[J_T1] == 70);
    puts("Juno UI/storage: direct EXTRA section, all 16 gate keys/LEDs, no sequence edits, ARP "
         "button/page, MIX DJ filter, persistent extras and preset 33 T1=70 passed");

    setup();
    tap(B_SEQ);
    assert(ui.view == V_SEQ && pg.n == 1 && pg.r[0][0].kind == R_JSEQ);
    brkpart_t *bp = &cur_pat()->brk;
    assert(!bp->steps && bp->set[2] == 3 && bp->set[3] == 75);
    pref_set(PR(R_JSEQ,1,0,0),5); /* major */
    key_event(WHITE_KEY[4],1);
    key_event(BLACK_KEY[2],1); flush();
    assert(bp->slice[4] == 2 && pattern_juno_note(bp,4) == 52 && gates() == 1);
    key_event(BLACK_KEY[2],0); flush();
    key_event(WHITE_KEY[4],0);
    assert(bp->steps == (1u<<4));
    /* A tap on an occupied step removes it; assigning a note restores it. */
    key_event(WHITE_KEY[4],1); key_event(WHITE_KEY[4],0);
    assert(!bp->steps);
    key_event(BLACK_KEY[10],1); key_event(BLACK_KEY[10],0); flush();
    assert(bp->steps == (1u<<4) && pattern_juno_note(bp,4) == 65);
    pref_set(PR(R_JSEQ,0,0,0),2); pref_set(PR(R_JSEQ,2,0,0),4);
    assert(pattern_juno_note(bp,4)==79);
    /* Note timer expires exactly, including with arp and trance gate disabled. */
    breaks_stop(&brk); juno_part_sequence(60,100);
    render(99); assert(gates()==1); render(2); assert(!gates());
    /* A sequencer release must preserve a physically held note of the same pitch. */
    juno_part_key(60,100); juno_part_sequence(60,100);
    render(102); assert(gates()==1); juno_part_key(60,0); assert(!gates());
    /* Actual transport: per-step scale notes and locks precede their note trigger. */
    bp->steps=3; bp->slice[0]=0; bp->slice[1]=2;
    bp->set[0]=0; bp->set[1]=5; bp->set[2]=3; bp->set[3]=50;
    tap(B_PLAY); render(1); build_pages();
    assert(seq.playing && pg.n==12 && newest()==48);
    key_event(WHITE_KEY[1],1);
    lock_turn(PR(R_ENG,T_BRK,0,J_CUTOFF),-10,EN_K1);
    key_event(WHITE_KEY[1],0); flush();
    int lane=motion_find(proj.arr.lane,seq.ppat[PART_BRK],PART_BRK,T_BRK,0,J_CUTOFF);
    assert(lane>=0 && proj.arr.lane[lane].val[1]!=MOT_NONE && (bp->steps&2));
    render((int)(44100.0f*60.0f/seq_tempo(&seq)/4.0f)+10);
    assert(eng_step[TRK_BRK]==1 && newest()==52);
    assert(part_juno.value[J_CUTOFF]==proj.arr.lane[lane].val[1]);
    tap(B_PLAY); render(256); build_pages();
    assert(!seq.playing && pg.n==1 && !gates());
    assert(project_save()==0); project_t sequence_saved=proj;
    assert(project_load()==0 && !memcmp(&sequence_saved,&proj,sizeof proj));
    brkpart_t legacy={0}; legacy.steps=65535; legacy.slice[0]=127;
    pattern_juno_prepare(&legacy); assert(!legacy.steps && legacy.slice[0]==0);
    assert(sizeof(pattern_t)==528);
    puts("Juno SEQ: stopped knobs, scale degrees, step gestures, exact note lengths, held-note ownership, transport, p-locks, migration and persistence passed");
    setup();
    juno_part_key(60,100);
    alpha_juno_t gain_start=part_juno;
    float raw_gain[256], boosted_gain[256];
    alpha_juno_render(&part_juno,raw_gain,256);
    part_juno=gain_start;
    breaks_render(&brk,boosted_gain,256);
    for(int i=0;i<256;i++) assert(fabsf(boosted_gain[i]-raw_gain[i]*5.62341325f)<0.000001f);
    puts("Juno gain: +15 dB total (+12 dB additional) before mixer verified sample by sample");

    /* ENV is a four-state step flag control; OCT flags leave the base octave alone. */
    setup(); tap(B_SEQ);
    bp = &cur_pat()->brk;
    key_event(WHITE_KEY[2],1); tap(B_ENV); key_event(WHITE_KEY[2],0);
    assert(bp->steps==(1u<<2) && (bp->slice[2] & JSEQ_TIE));
    tap(B_ENV); assert((bp->slice[2] & (JSEQ_TIE|JSEQ_SLIDE))==JSEQ_SLIDE);
    tap(B_ENV); assert((bp->slice[2] & (JSEQ_TIE|JSEQ_SLIDE))==(JSEQ_TIE|JSEQ_SLIDE));
    tap(B_ENV); assert(!(bp->slice[2] & (JSEQ_TIE|JSEQ_SLIDE)));
    int base_oct=bp->set[2], base_note=pattern_juno_note(bp,2);
    tap(B_OCTUP); assert(bp->set[2]==base_oct && pattern_juno_note(bp,2)==base_note+12);
    tap(B_OCTDN); assert((bp->slice[2] & JSEQ_DOWN) && !(bp->slice[2] & JSEQ_UP));
    assert(pattern_juno_note(bp,2)==base_note-12);
    tap(B_OCTDN); assert(pattern_juno_note(bp,2)==base_note);
    tap(B_OCTUP); tap(B_ENV); tap(B_ENV); tap(B_ENV);
    key_event(BLACK_KEY[3],1); key_event(BLACK_KEY[3],0); flush();
    assert((bp->slice[2] & (JSEQ_TIE|JSEQ_SLIDE|JSEQ_UP|JSEQ_DEGREE))==(JSEQ_SLIDE|JSEQ_UP|3));
    assert(project_save()==0); sequence_saved=proj;
    assert(project_load()==0 && !memcmp(&sequence_saved,&proj,sizeof proj));
    pattern_juno_prepare(bp); assert(bp->slice[2]==(JSEQ_SLIDE|JSEQ_UP|3));
    puts("Juno flags: ENV cycles, held-step retention, independent octave flags, note editing and SAVE/reload passed");

    /* Exact semitone interpolation and legato envelope continuity, independent of GLIDE. */
    setup(); set(JX_GLIDE,127);
    juno_part_sequence_step(48,750,JSEQ_SLIDE,1,60,3000);
    juno_voice_t *sv=0;
    for(int i=0;i<6;i++) if(part_juno.voice[i].gate) sv=&part_juno.voice[i];
    assert(sv && sv->pitch==48 && sv->seq_left==3000);
    serial=sv->serial; uint32_t start_serial=part_juno.serial;
    render(1000); assert(fabsf(sv->pitch-52)<0.0001f && sv->gate);
    float age=sv->age;
    juno_part_sequence_step(48,750,JSEQ_TIE,1,60,2000);
    assert(sv->serial==serial && sv->age==age);
    render(1000); assert(fabsf(sv->pitch-56)<0.0001f && sv->gate);
    juno_part_sequence_step(48,750,JSEQ_TIE|JSEQ_SLIDE,0,60,1000);
    render(1000); assert(sv->pitch==60 && sv->gate);
    age=sv->age;
    juno_part_sequence_step(60,750,0,0,-1,0);
    assert(sv->pitch==60 && sv->age==age && sv->serial==serial && part_juno.serial==start_serial);
    render(751); assert(!sv->gate);
    /* Orphan ties stay silent; a rest cuts a sweep and freezes its release pitch. */
    breaks_stop(&brk); juno_part_sequence_step(60,750,JSEQ_TIE,1,72,2000);
    assert(!gates());
    juno_part_sequence_step(48,750,JSEQ_SLIDE,1,60,3000); render(500);
    for(int i=0;i<6;i++) if(part_juno.voice[i].gate) sv=&part_juno.voice[i];
    float cut_pitch=sv->pitch;
    juno_part_sequence_step(-1,750,0,0,-1,0); render(100);
    assert(!gates() && sv->pitch==cut_pitch);
    /* Ordinary sequence notes ignore keyboard GLIDE and retain envelope retriggering. */
    juno_part_sequence(72,1000);
    for(int i=0;i<6;i++) if(part_juno.voice[i].gate) sv=&part_juno.voice[i];
    assert(sv->pitch==72); serial=part_juno.serial;
    juno_part_sequence(60,1000); assert(part_juno.serial==serial+1);
    puts("Juno slides: sample-exact interpolation over tied steps, no envelope retrigger at destination, orphan ties/rests, independent keyboard glide passed");

    /* Real clock: C3 -> two ties -> C4; midpoint, destination, loop wrap and tempo. */
    for(int tempo=100;tempo<=200;tempo+=100) {
        setup(); seq.bpm=(float)tempo; bp=&cur_pat()->brk;
        bp->set[1]=5; bp->set[3]=25;
        bp->steps=15; bp->slice[0]=JSEQ_SLIDE;
        bp->slice[1]=JSEQ_TIE; bp->slice[2]=JSEQ_TIE|JSEQ_SLIDE; bp->slice[3]=7;
        int distance=0; assert(pattern_juno_target(bp,0,&distance)==60 && distance==3);
        tap(B_PLAY); render(1);
        for(int i=0;i<6;i++) if(part_juno.voice[i].gate) sv=&part_juno.voice[i];
        serial=sv->serial;
        int step_samples=(int)(44100.0f*60.0f/(float)tempo/4.0f);
        render(step_samples+step_samples/2);
        assert(fabsf(sv->pitch-54)<0.02f && sv->gate && sv->serial==serial);
        render(step_samples*3-(step_samples+step_samples/2)+3);
        assert(eng_step[TRK_BRK]==3 && sv->pitch==60 && sv->gate && sv->serial==serial);
        tap(B_PLAY); render(256); assert(!gates());
    }
    setup(); bp=&cur_pat()->brk; bp->set[1]=5;
    bp->steps=(1u<<14)|(1u<<15)|1u;bp->slice[14]=JSEQ_SLIDE;bp->slice[15]=JSEQ_TIE;bp->slice[0]=7;
    int distance=0;assert(pattern_juno_target(bp,14,&distance)==60 && distance==2);
    bp->steps &= ~(1u<<15);assert(pattern_juno_target(bp,14,&distance)==-1 && distance==0);
    bp->steps=0xffff;for(int i=0;i<16;i++)bp->slice[i]=JSEQ_TIE;
    assert(pattern_juno_target(bp,0,&distance)==-1 && distance==0);
    puts("Juno transport slides: 100/200 BPM sweeps reach target on time, tied envelope preserved, loop-wrap lookahead and rest/all-tie boundaries passed");

    setup(); bp=&cur_pat()->brk; bp->set[1]=5;
    bp->steps=15;bp->slice[0]=JSEQ_SLIDE;bp->slice[1]=bp->slice[2]=JSEQ_TIE;bp->slice[3]=7;
    proj.pat[seq.ppat[TRK_DRUM]].swing=80; seq.bpm=150;
    float swung_span=seq_sixteenth_span(&seq,0,3);
    assert(fabsf(swung_span-4410.0f*3.4f)<1.0f);
    seq_start(&seq);render(1);
    for(int i=0;i<6;i++)if(part_juno.voice[i].gate)sv=&part_juno.voice[i];
    serial=sv->serial;
    render((int)swung_span/2);
    assert(fabsf(sv->pitch-54)<0.02f);
    render((int)swung_span-(int)swung_span/2+3);
    assert(eng_step[TRK_BRK]==3 && sv->pitch==60 && sv->serial==serial);
    seq_stop(&seq);render(1);
    /* External 24-PPQN at 150 BPM: 735 samples per tick, 4410 per sixteenth. */
    setup();bp=&cur_pat()->brk;bp->set[1]=5;
    bp->steps=15;bp->slice[0]=JSEQ_SLIDE;bp->slice[1]=bp->slice[2]=JSEQ_TIE;bp->slice[3]=7;
    seq.ext=1;seq.ext_bpm=150;seq.ext_period=735u*256u;seq.ext_since=734;
    seq_start(&seq);render(1);
    for(int tick=0;tick<19;tick++) {
        seq_ext_clock(&seq); render(1);
        if(tick==0) {
            for(int i=0;i<6;i++)if(part_juno.voice[i].gate)sv=&part_juno.voice[i];
            serial=sv->serial;
        }
        if(tick==9) assert(fabsf(sv->pitch-54)<0.03f);
        if(tick==18) assert(eng_step[TRK_BRK]==3 && sv->pitch==60 && sv->serial==serial);
        render(734);
    }
    seq_stop(&seq);render(1);assert(!gates());
    puts("Juno slide timing: swing-adjusted midpoint/arrival and external 24-PPQN endpoint/envelope continuity passed");
    setup();
    juno_part_sequence(60,1000);juno_part_key(60,100);juno_part_key(60,0);
    assert(!gates());render(1001);assert(!gates());
    juno_part_key(67,100);juno_part_sequence(60,1000);
    juno_part_key(60,100);render(1001);
    assert(gates()==2);juno_part_key(60,0);juno_part_key(67,0);assert(!gates());
    puts("Juno sequence voice ownership: keyboard steals/retriggers and unrelated held notes survive stale sequencer note-offs");
    /* A held BRIGHT HOOVER and the same note continued by ties must be bit-identical,
     * through the full mixer/FX/master, despite extra sequencer render boundaries. */
    static int32_t held_bright[60000 * 2];
    int32_t tied_bright[512];
    for (int tied = 0; tied < 2; tied++) {
        setup();
        const uint8_t *patch = alpha_juno_preset_values(35);
        for (int i = 0; i < J_NPARAMS; i++) proj.sound.v[T_BRK][i / 16][i % 16] = patch[i];
        engine_apply_sound(&proj.sound); flush();
        seq.bpm = 126;
        seq.mute = seq_part_mask(PART_909) | seq_part_mask(PART_808) |
                   seq_part_mask(PART_303A) | seq_part_mask(PART_303B);
        bp = &proj.pat[0].brk;
        bp->set[0] = 0; bp->set[1] = 1; bp->set[2] = 2; bp->set[3] = 100;
        bp->steps = 0xffff;
        for (int i = 0; i < 16; i++) bp->slice[i] = JSEQ_TIE;
        if (tied) { bp->slice[0] = 0; seq_start(&seq); }
        else juno_part_key(36,100);
        for (int done = 0; done < 60000;) {
            int n = 60000 - done; if (n > 256) n = 256;
            if (tied) {
                engine_render(tied_bright,n);
                assert(!memcmp(tied_bright,held_bright+2*done,(unsigned)n*2*sizeof(int32_t)));
                assert(part_juno.serial == 1 && gates() == 1);
            } else engine_render(held_bright+2*done,n);
            done += n;
        }
    }
    /* A fresh voice must calculate pitch/filter immediately between control ticks. */
    setup(); juno_part_key(60,100); render(7); juno_part_key(64,100); render(1);
    for (int i=0;i<JUNO_VOICES;i++) if(part_juno.voice[i].gate) assert(part_juno.voice[i].inc>0);
    puts("BRIGHT HOOVER ties: sample-identical to a stopped held note through mixer/FX/master; no voice/envelope/LFO restart; new voices update immediately");
    return 0;
}
