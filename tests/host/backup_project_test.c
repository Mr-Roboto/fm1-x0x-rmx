/* SPDX-License-Identifier: GPL-3.0-only */
/* Real RMX serializer, NOR staging/recovery, and strict streaming validation. */
#define st_crc32 host_storage_crc32
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#undef st_crc32
#include "../../firmware/src/app/engine.c"
#include "../../firmware/src/app/backup_check.c"
#define BACKUP_CONTENT_CHECK 1
#define main backup_transport_main
#include "../backup_test.c"
#undef main
#include <assert.h>

static int staged_read(uint32_t i, void *dst, uint32_t n)
{
    return st_read(st_sector(rs_rec.e[i].obj, rs_rec.e[i].copy) + ST_PAYLOAD_OFF, dst, n);
}
static int rs_contents(void)
{
    uint32_t len[6];
    for (int i = 0; i < 6; i++) len[i] = rs_rec.e[i].len;
    return project_backup_check(len, staged_read);
}
static void to_nor(void)
{
    for (uint32_t i = 0; i < 6; i++) {
        uint32_t o = bu_obj(i);
        assert(st_save(o, store[o], (uint32_t)store_len[o]) == 0);
    }
}
static void from_nor(void)
{
    for (uint32_t i = 0; i < 6; i++) {
        uint32_t o = bu_obj(i);
        store_len[o] = st_load(o, store[o], PLAT_STORE_MAX);
    }
}
static int current_read(uint32_t i, void *dst, uint32_t n)
{
    memcpy(dst, store[bu_obj(i)], n);
    return 0;
}
static void check_current(void)
{
    uint32_t len[6];
    for (uint32_t i = 0; i < 6; i++) len[i] = (uint32_t)store_len[bu_obj(i)];
    assert(project_backup_check(len, current_read) == 0);
}
int main(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(nor + 0xA0000, 0x39, 0x3C000); /* user sample regions */
    memset(nor + 0xDC000, 0x73, 0x2000); /* calibration */
    project_defaults();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound); drain(); ui_init();
    settings_set_remix_apply(&proj.set, REMIX_LOOP);
    for (int p = 0; p < NPAT; p++) {
        for (int part = 0; part < 4; part++) {
            pattern_set_remix_selection(&proj.pat[p], part, (p + part) & 15, (p * 3 + part) & 15);
            pattern_set_remix_enabled(&proj.pat[p], part, p & 1);
        }
        proj.pat[p].bass[0].dir = DIR_MREV;
        proj.pat[p].bass[1].dir = DIR_RANDOM;
        proj.pat[p].bass[0].gen.root = p % 12;
        proj.pat[p].bass[0].gen.seed += p;
    }
    engine_bass_wave_user(0, BASS303_WAVE_MORPH); drain();
    engine_sync_sound(&proj.sound);
    proj.sound.v[T_303][0][BASS303_MORPH] = 21;
    proj.sound.v[T_BRK][0][2] = 94;
    int lane = motion_alloc(proj.arr.lane, 0, PART_BRK, T_BRK, 0, 2);
    assert(lane >= 0); proj.arr.lane[lane].val[0] = 29;
    proj.arr.song.len = 1;
    assert(project_save() == 0);
    check_current();
    static project_t expected; expected = proj;
    to_nor(); backup(0); memcpy(img, got, got_n); img_n = got_n;
    /* Different active project: cancel must leave it and its Undo unchanged. */
    project_defaults(); assert(project_save() == 0); to_nor();
    static project_t before; before = proj;
    answer = 0; assert(restore() == RS_E_NO); from_nor();
    assert(project_load() == 0 && !memcmp(&proj, &before, sizeof proj));
    answer = 1; assert(restore() == 0); from_nor();
    ui.rec = seq.song_on = seq.song_rec = 1; /* session state before reload */
    app_restored(); drain();
    assert(!ui.rec && !seq.song_on && !seq.song_rec);
    assert(!memcmp(&proj, &expected, sizeof proj));
    assert(b303[0].wave == BASS303_WAVE_MORPH && breaks_get(&brk, BRK_FILT) == 94);
    for (uint32_t i = 0xA0000; i < 0xDC000; i++) assert(nor[i] == 0x39);
    for (uint32_t i = 0xDC000; i < 0xDE000; i++) assert(nor[i] == 0x73);
    assert(undo.n == 0 && !ui_dirty());
    assert(project_save() == 0); /* invalidated stored-hash cache after reload */
    puts("PASS: actual RMX six-object round trip, Remix/APPLY/MREV/TB3PO/Morph/filter/song/motion, cancel, reload/Undo/save and untouched samples/calibration");
    /* Inner project corruption with correct transport/object/file CRCs is refused. */
    static uint8_t keep_image[BU_MAXIMG]; memcpy(keep_image, img, img_n);
    uint32_t off = BU_HEAD;
    for (int i = 0; i < 2; i++) off += BU_REC + bu_rd16(img + off + 2);
    img[off + BU_REC + 8] ^= 1;
    bu_wr32(img + off + 4, bu_crc(0, img + off + BU_REC, bu_rd16(img + off + 2)));
    bu_wr32(img + img_n - 4, bu_crc(0, img, img_n - 4));
    snap(&set_b); asks = 0;
    assert(restore() == RS_E_OBJ && asks == 0); snap(&now); assert(same(&now, &set_b));
    memcpy(img, keep_image, img_n);
    /* Saves cannot overwrite pinned copies during a backup. */
    ui.dirty = 1; ui_hold_saves(1);
    uint32_t writes = store_writes; save_project(); autosave();
    assert(store_writes == writes && save_wait && ui_dirty());
    ui_hold_saves(0); save_project(); assert(!save_wait && !ui_dirty());
    puts("PASS: malformed inner CRC refused before confirmation/commit; saves wait while backup pins copies");
    /* Older supported sound revision, preserving all bytes except migration. */
    proj.set.snd_rev = 4; proj.sound.v[T_BRK][0][2] = 0;
    assert(project_save() == 0); check_current(); to_nor(); backup(0); memcpy(img, got, got_n); img_n = got_n;
    assert(restore() == 0); from_nor(); assert(project_load() == 0);
    assert(proj.set.snd_rev == 5 && proj.sound.v[T_BRK][0][2] == 64);
    puts("PASS: revision-4 RMX backup loads with BREAK FILTER OFF");
    /* Correct transport CRC cannot make malformed project contents valid. */
    uint32_t len[6]; for (int i = 0; i < 6; i++) len[i] = (uint32_t)store_len[bu_obj(i)];
    uint32_t saved_crc = get32(store[OBJ_PAT0] + 8);
    put32(store[OBJ_PAT0] + 8, saved_crc ^ 1u);
    assert(project_backup_check(len, current_read) != 0);
    put32(store[OBJ_PAT0] + 8, saved_crc);
    uint8_t keep = store[OBJ_PAT0][16]; store[OBJ_PAT0][16] = 0xFF;
    assert(project_backup_check(len, current_read) != 0); store[OBJ_PAT0][16] = keep;
    store[OBJ_SET][4] ^= 1; assert(project_backup_check(len, current_read) != 0); store[OBJ_SET][4] ^= 1;
    store[OBJ_PAT0][0] = 1; assert(project_backup_check(len, current_read) != 0); store[OBJ_PAT0][0] = 2;
    check_current();
    puts("PASS: strict inner raw CRC, LZ bounds, settings magic and unsupported project format rejection");
    return 0;
}
