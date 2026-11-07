/* SPDX-License-Identifier: GPL-3.0-only */
/* Native build of the hardware profile: no X0X_WEB migration bypass. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void tap(int button)
{
    held_btn=1u<<button;ui_input_only();held_btn=0;ui_input_only();flush();
}

int main(int argc, char **argv)
{
    project_defaults();
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    /* Model the installed pre-Juno sound, with real data in unrelated parts. */
    memset(proj.sound.v[T_BRK],0,sizeof proj.sound.v[T_BRK]);
    proj.sound.v[T_BRK][0][0]=127;proj.sound.v[T_BRK][0][1]=64;
    proj.sound.v[T_BRK][0][2]=64;proj.set.snd_rev=8;
    proj.sound.v[T_909][DR_SD][3]=37;
    proj.sound.v[T_303][0][B303_HPF]=55;
    proj.pat[3].drum[0].hit[DR_BD][0]=0x1234;
    proj.pat[5].bass[1].step[3].note=71;
    proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_BRK;
    proj.arr.lane[0].part=PART_BRK;proj.arr.lane[0].val[0]=80;
    proj.arr.lane[1].used=1;proj.arr.lane[1].t=T_303;
    proj.arr.lane[1].part=PART_303A;proj.arr.lane[1].val[0]=70;
    seq.bpm=125;project_apply_io();assert(project_save()==0);
    project_t expected=proj;
    expected.set.snd_rev=9;
    for(int v=0;v<engine_nvoices(T_BRK);v++)
        for(int j=0;j<engine_nparams(T_BRK,v);j++)
            expected.sound.v[T_BRK][v][j]=engine_param(T_BRK,v,j)->def;
    expected.arr.lane[0].used=0;
    assert(project_load()==0);
    assert(!memcmp(&expected,&proj,sizeof proj));
    engine_apply_sound(&proj.sound);flush();
    assert(project_save()==0 && project_load()==0);
    assert(!memcmp(&expected,&proj,sizeof proj));

    /* Also exercise the user's actual backup, if supplied, without writing it. */
    if(argc>1) {
        for(int o=0;o<OBJ_NOBJ;o++)store_len[o]=-1;
        const int objects[]={0,1,2,3,5,6};
        for(int k=0;k<6;k++) {
            int o=objects[k];char path[512];
            snprintf(path,sizeof path,"%s%d.bin",argv[1],o);
            FILE *f=fopen(path,"rb");assert(f);
            store_len[o]=(int)fread(store[o],1,PLAT_STORE_MAX,f);fclose(f);
        }
        int rev_offset=4+__builtin_offsetof(settings_t,snd_rev);
        int old_revision=store[0][rev_offset];assert(old_revision==8);
        store[0][rev_offset]=9;assert(project_load()==0);expected=proj;
        store[0][rev_offset]=(uint8_t)old_revision;
        for(int v=0;v<engine_nvoices(T_BRK);v++)
            for(int j=0;j<engine_nparams(T_BRK,v);j++)
                expected.sound.v[T_BRK][v][j]=engine_param(T_BRK,v,j)->def;
        for(int k=0;k<NLANE;k++)if(expected.arr.lane[k].used&&expected.arr.lane[k].t==T_BRK)expected.arr.lane[k].used=0;
        assert(project_load()==0 && !memcmp(&expected,&proj,sizeof proj));
        engine_apply_sound(&proj.sound);flush();
        puts("User backup: only Juno patch/revision and former BREAK automation change; drums, 303s, mixer, patterns and song preserved");
    }

    held_btn=held_keys=0;ui_init();ui.part=PART_BRK;build_pages();
    for(int i=0;i<6;i++)key_event(i,1);
    int32_t block[512];double power=0;
    for(int b=0;b<100;b++) {
        engine_render(block,256);
        for(int i=0;i<512;i++)power+=(double)block[i]*block[i];
    }
    int gates=0;for(int i=0;i<6;i++)gates+=part_juno.voice[i].gate;
    assert(gates==6 && power>1e8);
    tap(B_OCTUP);tap(B_EDIT);
    for(int i=0;i<6;i++)key_event(i,0);
    flush();gates=0;for(int i=0;i<6;i++)gates+=part_juno.voice[i].gate;assert(!gates);
    for(int b=0;b<300;b++)engine_render(block,256);
    assert(!alpha_juno_active(&part_juno));
    project_t untouched=proj;
    int original_page=cur_page();
    uint8_t original_patterns[NPARTS];
    for(int p=0;p<NPARTS;p++)original_patterns[p]=seq.ppat[p];
    for(int k=0;k<6;k++)key_event(k,1);
    flush();
    for(int preset=0;preset<JUNO_PRESET_COUNT;preset++) {
        uint32_t queue_before=cq_w;
        enc_acc[EN_PRESET]=1;ui_input_only();
        assert(cq_w-queue_before==1); /* one atomic audio command, not 44 partial edits */
        flush();
        assert(juno_preset_index()==preset && !juno_preset_edited(preset));
        const uint8_t *values=alpha_juno_preset_values(preset);
        for(int j=0;j<J_NPARAMS;j++) {
            assert(proj.sound.v[T_BRK][j/16][j%16]==values[j]);
            assert(part_juno.value[j]==values[j]);
            untouched.sound.v[T_BRK][j/16][j%16]=values[j];
        }
        untouched.sound.v[T_BRK][11][14]=(uint8_t)preset;
        untouched.sound.v[T_BRK][11][15]=0xA6;
        assert(!memcmp(&untouched,&proj,sizeof proj));
        assert(cur_page()==original_page);
        for(int p=0;p<NPARTS;p++)assert(seq.ppat[p]==original_patterns[p]);
        engine_render(block,256);
        gates=0;for(int k=0;k<6;k++)gates+=part_juno.voice[k].gate;assert(gates==6);
    }
    enc_acc[EN_PRESET]=1;ui_input_only();flush();assert(juno_preset_index()==0);
    enc_acc[EN_PRESET]=-1;ui_input_only();flush();assert(juno_preset_index()==63);
    enc_acc[EN_PRESET]=8;ui_input_only();flush();assert(juno_preset_index()==7);
    pref_set(PR(R_ENG,T_BRK,0,J_NOISE),1);flush();assert(juno_preset_edited(7));
    for(int k=0;k<6;k++)key_event(k,0);flush();
    assert(project_save()==0 && project_load()==0);
    engine_apply_sound(&proj.sound);flush();ui_init();
    assert(juno_preset_index()==7 && juno_preset_edited(7));
    assert(proj.sound.v[T_BRK][0][J_NOISE]==1);
    puts("64-preset UI: atomic recall, names/edited identity, held chords, wrap/fast turning, fixed page/pattern/mixer and edited SAVE/reload passed");
    puts("Juno hardware profile: one-time/idempotent migration, all six keys through real engine/mixer, octave/page-safe release and save/reload passed");
    return 0;
}
