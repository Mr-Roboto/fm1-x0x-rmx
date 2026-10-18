/* SPDX-License-Identifier: GPL-3.0-only */
/* RMX backup profile: validate staged objects without another project buffer.
 * Included after project.c. io_buf caches one object; the compressor's existing
 * lz_head supplies a 512-byte token index and a 3.5 KiB decoded-byte ring. Older references are resolved by walking
 * tokens backwards, never recursion. A work limit rejects pathological streams
 * before commit. Neither scratch area is used by the audio ISR. */
typedef struct {
    int (*read)(uint32_t obj, void *dst, uint32_t n);
    const uint32_t *len;
    uint32_t work;
    int loaded;
} backup_check_t;

static int bc_object(backup_check_t *b, int i)
{
    if (b->loaded != i) {
        if (b->read((uint32_t)i, io_buf, b->len[i]))
            return -1;
        b->loaded = i;
    }
    return 0;
}

/* Offset in the concatenated packed bytes (all object format words omitted). */
static int bc_get(backup_check_t *b, uint32_t off)
{
    int i;
    for (i = 2; i < 6; i++) {
        uint32_t skip = i == 2 ? STREAM_HDR : 4u, n = b->len[i] - skip;
        if (off < n) {
            if (++b->work > 2000000u || bc_object(b, i))
                return -1;
            return io_buf[skip + off];
        }
        off -= n;
    }
    return -1;
}

/* A raw byte older than the ring. Each match maps to an earlier token, so
 * malformed data cannot recurse or create a loop. All reads share the bound. */
static int bc_old(backup_check_t *b, uint32_t want, uint32_t plen)
{
    for (;;) {
        uint32_t slot = want / 256u;
        uint32_t p = lz_head[slot * 2u + 1u], w = lz_head[slot * 2u];
        while (p < plen) {
            int c = bc_get(b, p++), lo, hi;
            uint32_t n, d;
            if (c < 0)
                return -1;
            n = c < 128 ? (uint32_t)c + 1u : ((uint32_t)c & 127u) + LZ_MIN;
            if (c < 128) {
                if (p + n > plen)
                    return -1;
                if (want < w + n)
                    return bc_get(b, p + want - w);
                p += n;
            } else {
                if (p + 2u > plen || (lo = bc_get(b, p++)) < 0 || (hi = bc_get(b, p++)) < 0)
                    return -1;
                d = 1u + (uint32_t)lo + ((uint32_t)hi << 8);
                if (d > w)
                    return -1;
                if (want < w + n) {
                    want = w - d + (want - w) % d;
                    goto earlier;
                }
            }
            w += n;
        }
        return -1;
earlier:;
    }
}

/* Index order is backup.c's six objects. Only RMX format-2 packed projects and
 * absent/default streams are supported. Sound revisions 0..5 use project_load's
 * existing migrations; an older format-1 backup is safely refused. */
static int project_backup_check(const uint32_t len[6], int (*read)(uint32_t, void *, uint32_t))
{
    backup_check_t b = {read, len, 0, -1};
    uint8_t *ring = (uint8_t *)lz_head + 512u;
    const uint32_t ring_n = sizeof lz_head - 512u;
    uint32_t next_index = 0;
    typedef char index_fits[(RAW_LEN <= 32768u) ? 1 : -1];
    (void)sizeof(index_fits);
    uint32_t plen, want_crc, total = 0, p = 0, w = 0, crc = 0xFFFFFFFFu;
    int i;
    uint32_t have = 0;
    for (i = 0; i < 6; i++)
        have += len[i] != 0;
    if (!have)
        return 0; /* a never-saved project restores factory defaults */
    if (have != 6)
        return -1; /* incomplete saved projects would trigger all-default fallback */
    if (len[0]) {
        if (len[0] < 20u || len[0] > sizeof(settings_t) + 4u || bc_object(&b, 0) ||
            get32(io_buf) != PROJ_FORMAT || get32(io_buf + 4) != PROJ_MAGIC ||
            get32(io_buf + 8) != PROJ_FORMAT)
            return -1;
        if (len[0] > 4u + __builtin_offsetof(settings_t, snd_rev) &&
            io_buf[4u + __builtin_offsetof(settings_t, snd_rev)] > PROJ_SND_REV)
            return -1;
    }
    if (len[1] && (len[1] != sizeof(sound_t) + 4u || bc_object(&b, 1) || get32(io_buf) != PROJ_FORMAT))
        return -1;
    if (!len[2])
        return len[3] || len[4] || len[5] ? -1 : 0;
    for (i = 2; i < 6; i++) {
        if (len[i] < (i == 2 ? STREAM_HDR : 4u) || len[i] > PLAT_STORE_MAX ||
            bc_object(&b, i) || get32(io_buf) != PROJ_STREAM)
            return -1;
        total += len[i] - (i == 2 ? STREAM_HDR : 4u);
    }
    if (bc_object(&b, 2) || get32(io_buf + 4) != RAW_LEN)
        return -1;
    want_crc = get32(io_buf + 8);
    plen = get32(io_buf + 12);
    if (!plen || plen != total || plen > STREAM_CAP)
        return -1;
    while (w < RAW_LEN) {
        int c, lo, hi;
        uint32_t n, d = 0, token = p;
        if (p >= plen || (c = bc_get(&b, p++)) < 0)
            return -1;
        n = c < 128 ? (uint32_t)c + 1u : ((uint32_t)c & 127u) + LZ_MIN;
        if (n > RAW_LEN - w)
            return -1;
        if (c >= 128) {
            if (p + 2u > plen || (lo = bc_get(&b, p++)) < 0 || (hi = bc_get(&b, p++)) < 0)
                return -1;
            d = 1u + (uint32_t)lo + ((uint32_t)hi << 8);
            if (d > w)
                return -1;
        } else if (p + n > plen)
            return -1;
        while (next_index * 256u < w + n) {
            lz_head[next_index * 2u] = (uint16_t)w;
            lz_head[next_index * 2u + 1u] = (uint16_t)token;
            next_index++;
        }
        while (n--) {
            int v = d ? (d <= ring_n ? ring[(w - d) % ring_n] : bc_old(&b, w - d, plen)) : bc_get(&b, p++);
            if (v < 0)
                return -1;
            ring[w++ % ring_n] = (uint8_t)v;
            crc ^= (uint32_t)v;
            for (i = 0; i < 8; i++)
                crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return p == plen && ~crc == want_crc ? 0 : -1;
}
