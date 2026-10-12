# Remix lookup data

`remixes.txt` is the canonical import of the complete `%remixes[16*16*16]`
declaration at lines 1055–1157 of the historical `Luminary.ksp` (compiled
2023-10-24). Only conversion from source steps 1–16 to 0–15 was applied.
Its 256 rows each contain 16 decimal values, in bank-major order A1–P16;
trailing comments label each selection. Repeats retain their historical order.

SHA-256 of the 4096 zero-based bytes:
`88034625d620ce4a1a87f954edf81850adf261155a7891fabfea68b01b6d60cb`.
Focused tests pin this digest so normal tests do not need the external KSP.
For an additional direct historical comparison, run:

```sh
sh tests/host/run_remix.sh --historical-source ~/Luminary.ksp
```

`tools/gen_remix.py` validates the canonical rows and generates
`build/gen/x0x_remix_table.h`. Even slots use the low nibble, odd slots the high
nibble: eight bytes per row, exactly 2048 bytes in a `static const uint8_t` table.
The firmware linker places retained constants in flash/XIP. Until a caller uses
the helpers, its existing section garbage collection can discard them.

`seq/remix.h` exposes only lookup and forward source mapping. Bank/pattern are
0–15; forward chronological slots are 0–63. Pattern length belongs to the caller
and does not clamp source selections within a partial 16-step block.
