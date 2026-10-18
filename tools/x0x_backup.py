#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Back up X0X's project to a file, and put it back, over USB-MIDI (after Polly's tools/polly_backup.py).

    python3 tools/x0x_backup.py backup FILE [--port NAME]
    python3 tools/x0x_backup.py restore FILE [--yes] [--port NAME]
    python3 tools/x0x_backup.py info FILE

A backup (.x0b) holds the project: the sounds, the 32 patterns, the song, the knob motion, the tempo and the
settings. Not the break loops in the sample slots (load them again from their WAV files: tools/upload_breaks.py or
the break loops page) and not the panel's calibration. X0X can play on while it is backed up (changes not saved yet
are left out then: stop first to have them).

A restore replaces the project. X0X must be stopped ("STOP FIRST" otherwise). It receives the whole backup and
checks it (every part's CRC-32, the whole file's) before anything changes, then asks on its screen: RESTORE FROM
USB? SEL yes, HOME no. Before confirmation, a cut cable, a damaged file or HOME changes nothing (firmware/src/backup.c). Only RMX-profile backups are accepted. Standard X0X backups are incompatible.
Supported RMX sound revisions migrate as on normal project loading.

Needs mido with python-rtmidi (as tools/fm1_install.py). Exit codes: 0 done, 1 cancelled or failed, 2 bad arguments
or file, 3 FM-1 not found, 4 connection lost or the FM-1 stopped answering, 5 playing (stop it first), 6 not X0X.
"""
import argparse
import os
import re
import struct
import sys
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import fm1_install as I  # noqa: E402

RS_REQ, BU_REQ, BU_END = 0x41, 0x42, 0x43
OK_ADDR, ASK_ADDR, FAIL_ADDR = 0xC0000000, 0xC1000000, 0xCF000000
MAGIC, VERSION, NOBJ, HEAD, REC, PAYLOAD_MAX = b"X0XB", 0x524D, 6, 16, 8, 3840
OBJ_IDS = [0, 1, 2, 3, 5, 6]                      # firmware/src/app/plat.h: OBJ_SET, OBJ_SOUND, OBJ_PAT0, OBJ_PAT1,
                                                  # OBJ_SONG, OBJ_MOTION (not OBJ_PANEL, 4)
PLAYING, DECLINED = 10, 11
ERRORS = {1: "the file's size", 2: "the transfer (a read request unanswered)", 3: "the file's head",
          4: "project format, contents or validation limit", 5: "a part's CRC-32 (a byte changed on the way)", 6: "the whole file's CRC-32",
          7: "a flash erase", 8: "a flash write", 9: "the read-back (the flash does not hold what was written)",
          10: "X0X is playing: STOP FIRST, then restore", 11: "answered HOME (no) on the FM-1",
          12: "writing the headers (the FM-1 finishes it when switched on again)"}
X0X = "FM-1_987"                    # X0X's package identity (tools/build.py PRODUCT)
DELAY = {"poll": 0.5, "idle": 10.0, "ask": 75.0, "first": 3.0}
EXIT = dict(I.EXIT, playing=5)


class BackupError(Exception):
    pass


def name_of(obj):
    return {0: "settings", 1: "sound", 2: "patterns 1", 3: "patterns 2", 5: "song", 6: "motion"}.get(obj, f"object {obj}")


def frame(cmd, body):
    """a host message in the update's framing (ota.c ota_take)"""
    n = len(body)
    msg = bytes([0x00, 0x59, cmd, n & 0xFF, (n >> 8) & 0xFF, 0]) + body + bytes([~sum(body) & 0xFF])
    return b"\xF0" + I.pack7(msg) + b"\xF7"


def parse_frame(pkt):
    """a device message -> (cmd, body) or None"""
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    u = I.unpack7(pkt[1:-1])
    if len(u) < 7 or u[:2] != b"\x00\x59":
        return None
    n = u[3] | u[4] << 8 | u[5] << 16
    if n + 7 != len(u) or ~sum(u[6:6 + n]) & 0xFF != u[6 + n]:
        return None
    return u[2], u[6:6 + n]


def parse(image):
    """check a backup file whole -> (flags, [(obj, payload)]); BackupError if it is not one or is damaged"""
    if len(image) >= 4 and image[:4] == b"PLYB":
        raise BackupError("a Polly backup, not X0X's")
    if len(image) < HEAD + NOBJ * REC + 4:
        raise BackupError("too short for an X0X backup")
    magic, ver, n, flags, hcrc = struct.unpack_from("<4sHHII", image)
    if magic != MAGIC:
        raise BackupError("not an X0X backup")
    if ver != VERSION or n != NOBJ:
        raise BackupError(f"a backup from another version of X0X (version {ver}, {n} parts): "
                          f"this X0X cannot restore it" + (" (update X0X)" if ver > VERSION else ""))
    if flags & ~1:
        raise BackupError("unsupported backup flags")
    if hcrc != zlib.crc32(image[:12]):
        raise BackupError("its head is damaged")
    objs, pos = [], HEAD
    for want in OBJ_IDS:
        if pos + REC > len(image):
            raise BackupError("cut short")
        obj, ln, crc = struct.unpack_from("<HHI", image, pos)
        pos += REC
        if obj != want or ln > PAYLOAD_MAX or pos + ln + 4 > len(image):
            raise BackupError(f"part {name_of(want)}: a wrong header")
        data = image[pos:pos + ln]
        if zlib.crc32(data) != crc:
            raise BackupError(f"part {name_of(want)}: damaged (CRC-32)")
        objs.append((obj, bytes(data)))
        pos += ln
    if pos + 4 != len(image) or struct.unpack_from("<I", image, pos)[0] != zlib.crc32(image[:pos]):
        raise BackupError("damaged (the whole file's CRC-32)")
    return flags, objs


def build(objs, flags=0):
    """a backup file from [(obj, payload)] in OBJ_IDS order (the tests' simulated X0X)"""
    head = MAGIC + struct.pack("<HHI", VERSION, NOBJ, flags)
    out = bytearray(head + struct.pack("<I", zlib.crc32(head)))
    for obj, data in objs:
        out += struct.pack("<HHI", obj, len(data), zlib.crc32(data)) + data
    out += struct.pack("<I", zlib.crc32(out))
    return bytes(out)


def summary(objs):
    have = {o for o, d in objs if d}
    parts = [w for w, o in (("settings", 0), ("sound", 1), ("patterns, song and motion", 2)) if o in have]
    return f"{sum(len(d) for _, d in objs)} bytes: " + (", ".join(parts) if parts else "an empty project")


def find(backend, port):
    up = I.Updater(backend, port or "X0X")
    dev = up.find(only_port=bool(port))           # ports named "X0X" first, then any FM-1
    if not dev:
        raise I.not_found(up)
    if not re.fullmatch(r"FM-1_987[0-9]{6}", dev.id.text):
        dev.link.close()
        raise I.InstallError("model", f"the FM-1 runs {dev.id.text}, not X0X ({X0X}xx)")
    return dev


def receive(link, out):
    """the device's backup frames until its end frame -> the image (checked)"""
    got, last, flags = bytearray(), time.monotonic(), 0
    first = True
    while True:
        pkt = link.read(DELAY["poll"])
        if pkt is None:
            if link.lost:
                raise I.InstallError("lost", "the FM-1 was disconnected: run this again")
            if time.monotonic() - last > (DELAY["first"] if first else DELAY["idle"]):
                raise I.InstallError("stopped", "the FM-1 does not answer (an older X0X, without backup?)"
                                     if first else "the FM-1 stopped sending")
            continue
        f = parse_frame(pkt)
        if not f:
            continue
        cmd, body = f
        last = time.monotonic()
        if cmd == BU_REQ and len(body) >= 4:
            first = False
            off = struct.unpack_from("<I", body)[0]
            if off != len(got):
                raise I.InstallError("lost", f"a piece of the backup went missing at {len(got)}: run this again")
            got += body[4:]
            print(f"\rreceiving {len(got)} bytes\033[K", end="", file=out, flush=True)
        elif cmd == BU_END and len(body) >= 10:
            n, crc, status, flags = struct.unpack_from("<IIBB", body)
            print(file=out)
            if status:
                raise I.InstallError("badreq", f"the FM-1 could not read its flash (status {status})")
            if n != len(got) or crc != zlib.crc32(got):
                raise I.InstallError("lost", "the backup arrived damaged: run this again")
            return bytes(got)


def backup(backend, path, port=None, out=sys.stdout):
    dev = find(backend, port)
    print(f"device: {dev.id.text}  ({dev.name})", file=out)
    try:
        dev.link.drain()
        dev.link.send(frame(BU_REQ, MAGIC))
        image = receive(dev.link, out)
    finally:
        dev.link.close()
    try:
        flags, objs = parse(image)
    except BackupError as e:
        raise I.InstallError("lost", f"the backup arrived damaged ({e}): run this again")
    with open(path, "wb") as f:
        f.write(image)
    print(f"done: {path}  ({summary(objs)})", file=out)
    if flags & 1:
        print("note: X0X had changes it had not saved yet (it was playing), so they are not in it (stop, then back "
              "up again to have them)", file=out)
    return 0


def serve(link, image, out):
    """answer the FM-1's read requests until it reports; -> 0 restored, else its error code"""
    last, idle, asked = time.monotonic(), DELAY["first"], 0
    while True:
        pkt = link.read(DELAY["poll"])
        if pkt is None:
            if link.lost:
                raise I.InstallError("lost", "the FM-1 was disconnected: nothing was changed unless it said so")
            if time.monotonic() - last > idle:
                raise I.InstallError("stopped", "the FM-1 stopped asking: nothing changed" if asked else
                                     "the FM-1 does not answer (an older X0X, without restore?)")
            continue
        r = I.parse_request(pkt)
        if not r:
            continue
        fl, addr, n = r
        last, idle, asked = time.monotonic(), DELAY["idle"], asked + 1
        if addr & 0xFF000000 in (OK_ADDR, FAIL_ADDR, ASK_ADDR):
            link.send(I.response(addr, b"success\0", fl))
            if addr == ASK_ADDR:
                print("\rreceived and checked. On the FM-1: RESTORE FROM USB? SEL yes, HOME no\033[K", file=out,
                      flush=True)
                idle = DELAY["ask"]
                continue
            return 0 if addr == OK_ADDR else (addr & 0xFFFF) or 99
        if not n or n > I.MAXDATA or addr >= len(image) or addr + n > len(image):
            raise I.InstallError("badreq", f"the FM-1 asked for {addr:#x}+{n}, outside the backup")
        data = image[addr:addr + n]
        if not link.send(I.response(addr, data + bytes(n - len(data)), fl)):
            raise I.InstallError("lost", "the FM-1 was disconnected")
        print(f"\rsending {min(addr + n, len(image)) * 100 // len(image):3d}%\033[K", end="", file=out, flush=True)


def restore(backend, path, port=None, yes=False, ask=I.ask_tty, out=sys.stdout):
    try:
        with open(path, "rb") as f:
            image = f.read()
    except OSError as e:
        raise I.InstallError("usage", f"cannot read {path}: {e.strerror}")
    try:
        flags, objs = parse(image)
    except BackupError as e:
        raise I.InstallError("badpkg", f"{path}: {e}")
    print(f"backup: {summary(objs)}", file=out)
    dev = find(backend, port)
    print(f"device: {dev.id.text}  ({dev.name})", file=out)
    if not yes and not ask("Replace the project on X0X with this backup? (X0X asks too) [y/N] "):
        dev.link.close()
        print("cancelled", file=out)
        return 1
    try:
        dev.link.drain()
        dev.link.send(frame(RS_REQ, struct.pack("<II", len(image), zlib.crc32(image))))
        code = serve(dev.link, image, out)
    finally:
        print(file=out)
        dev.link.close()
    if code == PLAYING:
        print("STOP FIRST: X0X is playing. Stop it, then restore again.", file=sys.stderr)
        return EXIT["playing"]
    if code == DECLINED:
        print("not restored: answered no on the FM-1. Nothing changed.", file=out)
        return 1
    if code:
        print(f"error: not restored: {ERRORS.get(code, 'code %d' % code)}." +
              ("" if code == 12 else " Nothing changed."), file=sys.stderr)
        return 1
    print("done: restored (X0X loaded the project again)", file=out)
    return 0


def main(argv=None, backend=None, out=sys.stdout, ask=I.ask_tty):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["backup", "restore", "info"])
    ap.add_argument("file", help="the backup file")
    ap.add_argument("--port", metavar="NAME", help="MIDI port to use (part of its name; default: X0X's)")
    ap.add_argument("--yes", action="store_true", help="restore: do not ask here (X0X still asks)")
    a = ap.parse_args(argv)
    try:
        if a.command == "info":
            with open(a.file, "rb") as f:
                flags, objs = parse(f.read())
            print(f"{a.file}: an X0X-RMX backup (RMX profile), {summary(objs)}" + (" (changes not saved left out)" if flags & 1 else ""),
                  file=out)
            for o, d in objs:
                print(f"  {name_of(o):10s} {len(d):5d} bytes" if d else f"  {name_of(o):10s}     -", file=out)
            return 0
        if backend is None:
            try:
                backend = I.MidoBackend()
            except ImportError:
                raise I.InstallError("usage", "needs mido and python-rtmidi (pip install mido python-rtmidi)")
        if a.command == "backup":
            return backup(backend, a.file, a.port, out)
        return restore(backend, a.file, a.port, a.yes, ask, out)
    except (OSError, BackupError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    except I.InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT.get(e.code, 1)
    except KeyboardInterrupt:
        print("\ninterrupted (a restore not confirmed on the FM-1 changed nothing)", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
