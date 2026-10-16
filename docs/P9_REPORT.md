# P9 Final Implementation — X0X-RMX v1.0.0 Stable

Stand: 16. Oktober 2026. Ausgangspunkt `e5cf961ccb23f4a8834869b0af6e98c9acde30fb`,
Branch `orb-remix`. `main` bleibt bei `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`.
Keine Veröffentlichung, kein Tag, kein Push, kein öffentlicher Firmwareupload und keine
Aktivierung des öffentlichen Installers. Dies ist die lokale Vorbereitung der offiziellen
Stable-Version; die abschließende Hardwareprüfung ist offen.

## Ergebnis und technische Entscheidungen

- **P9-04:** `project_defaults()` setzt USB FULL. Factory Reset wendet die Settings nun auch
  auf die Engine an. Vorhandene Settings werden weiterhin über die neuen Defaults geladen;
  eine gespeicherte 0 (FOLLOWS KNOB) bleibt 0, eine 1 bleibt 1. Die Auswahl bleibt verfügbar.
- **P9-05:** Fünf Bytes `start_pat[5]` werden an `settings_t` angehängt, hinter `rsv[0]`.
  Manuelle PRESETS- und HOME-Auswahl ändern diese Bytes und markieren das Projekt als dirty.
  SAVE/AUTOSAVE schreiben über die vorhandenen Objekthashes; kein Flashzugriff pro Wechsel.
  Die UI merkt bereits die angeforderte manuelle Kombination, auch vor einem Cue-Wechsel.
  Sequencer, Song und Chain kennen diese Bytes nicht und können sie nicht überschreiben.
  Bei einer HOME-Chain ist die erste manuelle Wahl der Start; der zweite gehaltene Tastendruck
  definiert nur das Ende. Boot wendet die gespeicherten Patterns über bestehende Cues an.
- **Speicherformat:** Settings 28 → 36 B durch fünf Nutzbytes und Alignment, Projekt-RAM
  30.944 → 30.952 B. Alle bisherigen Settings-Offsets bleiben unverändert, insbesondere
  Remix APPLY in `rsv[0]` bei Offset 27. Startfelder liegen bei 28–32, Werte 0–31.
  Fehlende Felder bleiben P1, ungültige Nummern werden zu P1 begrenzt. Das Settings-Objekt
  wächst von 32 auf 40 B inklusive Formatwort; kein neues Objekt und kein neuer Sektor.
  `pattern_t` bleibt 528 B, der gepackte Pattern/Song/Motion-Rohstrom bleibt 29.572 B und
  Format 2 unverändert. Historische Format-1-Projekte werden weiterhin konvertiert.
  **Rückwärtskompatibel bedeutet: neue Firmware liest alte Projekte.** Ältere Firmware
  lehnt die längeren Settings ggf. ab; ein verlustfreier Downgrade ist nicht zugesichert.
- **P9-07:** 909 GR und 808 GR ersetzen die vorherigen zwei 909-Loops. Beide zwei Takte,
  96 BPM, 110.250 Samples bei 22.050 Hz, mono, 4-Bit-IMA-ADPCM, jeweils 55.125 B.
  Host-Generator bindet den vorhandenen 808-Renderer ein. Zwei Vorlaufzyklen erhalten
  Ausklänge; 32 Samples kurze Randrampen und Normalisierung auf 85 % vermeiden harte
  Schnittkanten bzw. Clipping. Keine Erzeugung beim Booten, keine neue Engine im Target.
- **P9-08/09:** Beide 303s: SEQUENCE → REMIX → KEYS; KEYS ausschließlich FREE PLAY.
  Auf 909/808/303A/303B: APPLY / BANK / PATTERN / REMIX von links nach rechts.
  DSP-, Remix- und Sequencer-Algorithmen, Patternparameter und Timing sind unverändert.
- **Uploader:** Zusätzlich eng begrenzte FM-1-Namen einschließlich `FM-1 Midi 2` erkannt.
  Identisch benannte Ein-/Ausgänge und bestehender X0X-SysEx-Handshake bleiben erforderlich.
  Fremde Namen werden nicht angesprochen, falsche Header/Kommandos und ausbleibende
  Antworten akzeptieren kein Gerät. Keine Änderung am Uploadprotokoll.
- **Version:** Reguläre Releaseversionen erhalten keinen automatisch angehängten Status
  mehr. Das existierende Produkt-ID-Schema, SDK-, Loader-, Flash-, RAM-, Pool-, MMIO-
  und Soft-Double-Prüfungen bleiben bestehen. Explizite historische Versionssuffixe bleiben
  explizit; die vorbereitete Version ist ausschließlich 1.0.0 Stable.
- **Testumgebung:** POSIX-Featuredefinition für den C99-Host-Build ergänzt. Alte Struktur-
  und UI-Erwartungen gezielt aktualisiert. Beim historischen LCD-Pixelvergleich wird nur
  der bewusst umbenannte zweite Loop normalisiert, um die bisherigen Pixelchecks zu erhalten.
  Die Suite kann das konkrete Stable-Paket über `X0X_TEST_PACKAGE` prüfen. Fehlende externe
  Referenzen werden als Skip gemeldet; vorhandene Vergleiche dürfen weiterhin fehlschlagen.
  Der geerbte Webtest überspringt nur den nicht vorhandenen Felucca-Editor, prüft aber jetzt
  X0X-Pakete und das bestehende Browser-OTA-Protokoll.

## Speicher und Artefakte

| Messung | Unverändert e5cf961 | Finale 1.0.0 | Rest / Grenze |
|---|---:|---:|---:|
| App-Image | 579.820 B | 579.980 B | 1.584 B bis 581.564 B (`0x8DFBC`) |
| Statisches RAM .data+.bss | 97.324 B | 97.332 B | 972 B bis 98.304 B |
| Audiopool | 334.348 B | 334.348 B | 9.716 B bis 344.064 B |
| Poolreserve über Pflichtminimum 8 KiB | 1.524 B | 1.524 B | Buildprüfung bestanden |
| Built-in-ADPCM-Payload | 110.250 B | 110.250 B | unverändert |
| OTA-Paket | 609.680 B | 609.680 B | festes Paketlayout |
| Loader ota.bin | 6.524 B | 6.524 B | bestehende Implementierung |

Flashdelta +160 B, RAMdelta +8 B. Keine Funktion entfernt und keine kurzfristige DSP-
Optimierung eingeführt. Die geringe Flash-/RAMreserve bleibt ein Entwicklungsrisiko;
alle bestehenden Buildgrenzen sind eingehalten.

- Paket: [x0x-1.0.0.fwsc](../build/x0x-1.0.0.fwsc)
- Produkt-ID: `FM-1_9010000`, Display `X0X 1.0.0`, vorgesehener Tag `v1.0.0`.
- SHA-256: `eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c`
- Prüfsummendatei: [x0x-1.0.0.fwsc.sha256](../build/x0x-1.0.0.fwsc.sha256).
- Firmware-Build: `./build.sh --release 1.0.0`, Log
  [stable-build-final.log](../build/host/p9/stable-build-final.log).
- Vorhermessung: [baseline-build.log](../build/host/p9/baseline-build.log).
- Emulator: `X0X_VERSION=1.0.0 sh web/emu/build.sh`; Endstand in `build/emu/`,
  SHA-256, Version und Quellenrevision in `build/emu/build-info.json`.

## Projekt- und Sample-Kompatibilität

Bestätigt durch reale Host-Speicherung und -Migration: alte Settings, Format-1- und
Format-2-Projekte, Remix/APPLY, Schemes, Song und Motion laden weiterhin. Die neue manuelle
Startkombination ist vom automatischen Spielzustand getrennt. SAVE bei PLAY bleibt auf
STOP verschoben; AUTOSAVE behält seine Ruhezeit und den 20-Sekunden-Abstand.

Die drei User-Slots bleiben exakt bei `0xA0000`, `0xB4000`, `0xC8000`, jeweils `0x14000` B,
Datenoffset 512, unverändertem Header und Protokoll. `slots.c`, Upload-Schreibpfade,
Paketlayout und Loader-Schreibfenster wurden nicht verändert. Der Loader-Test belegt mit
Sentinels über **alle Bytes von 0xA0000 bis 0xDBFFF**, dass ein reguläres Update die Slots
unverändert lässt; ein wiederholtes identisches Update löscht nichts. Das ist ein
simulierter OTA-Nachweis, kein Ersatz für den realen Erhaltstest auf der FM1.

## Automatisierte Tests

Reproduzierbarer Hauptlauf:

```sh
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK \
X0X_TEST_PACKAGE=build/x0x-1.0.0.fwsc X0X_VERSION=1.0.0 tests/run_tests.sh
```

Ergebnis: **74 Prüfgruppen/Szenarien erfolgreich, 3 externe Referenzvergleiche ausgelassen, keine fehlgeschlagene verfügbare Prüfung**.
Vollständige Ergebnisliste: [regression-final.log](../build/host/p9/regression-final.log).
Einzellogs liegen in `build/host/`.

```text
ok   tb3po
  ok   drum-generator
  ok   drum-generator-project
  ok   drum-generator-ui
  ok   remix
  ok   remix-state
  ok   schemes
  ok   fastmath
  ok   sequencer
  ok   encoder
  ok   bass303-ui
  ok   bass303-waveform
  ok   bass303_lite
  ok   uac
  ok   trs
  ok   undo
  ok   lz
  ok   motion
  ok   drum-pan
  ok   stereo
  ok   master
  skip drum909 (external reference unavailable: ../schwung-9W9/src/dsp/er99_engine.c)
  skip drum808 (external reference unavailable: ../schwung-8W8/src/dsp/sc808_engine.cpp)
  skip bass303 (external reference unavailable: ../schwung-303/src/dsp/open303/rosic_Open303.cpp)
  ok   breaks
  ok   storage
  ok   ota-entry
  ok   update-loader
  ok   installer
  ok   upload-images
  ok   web-installer
  ok   break-connect
  ok   builtin-breaks
  ok   breaks-web
  ok   host-build
  ok   emu
  ok   p9-emu
  ok   bass303-extra-emu
  ok   emu-slots
  ok   scheme-emu
  ok   remix-emu
  ok   drum-generator-emu
  ok   scenario-autosave
  ok   scenario-extclock
  ok   scenario-freeplay
  ok   scenario-getting_started
  ok   scenario-groove
  ok   scenario-guard
  ok   scenario-help
  ok   scenario-jam
  ok   scenario-keysound
  ok   scenario-knobs
  ok   scenario-migrate
  ok   scenario-migrate0102
  ok   scenario-patterns
  ok   scenario-perf
  ok   scenario-plock
  ok   scenario-prob
  ok   scenario-probcond
  ok   scenario-probreset
  ok   scenario-remix_motion
  ok   scenario-remix_parity
  ok   scenario-remix_recording
  ok   scenario-remix_ui
  ok   scenario-savewait
  ok   scenario-showcase
  ok   scenario-slices
  ok   scenario-song
  ok   scenario-songrep
  ok   scenario-step303
  ok   scenario-steps64
  ok   scenario-stereo
  ok   scenario-tb3po
  ok   scenario-tour
  ok   scenario-trackmute
  ok   scenario-undo
  ok   scenario-upload
all tests passed
```

Zusätzlich bestanden:

- `tests/release_test.py`: tatsächliches Stable-Paket, Produkt-ID, Firmwareanzeige,
  Loader-Marker, Flashgrenze, Emulator-Version und Hash; Log `build/host/p9/release-artifacts.log`.
- `tests/pages_test.py`: gesamte paketfreie Vorschau, Links/Anker, Bilder/Video,
  Installer-Sperre, keine Firmware, WASM-Provenienz; Log `build/host/p9/pages-final.log`.
- `tests/tb3po_scales_doc_test.py`: alle 22 Skalen und deutschen Notennamen;
  Log `build/host/p9/scales-doc.log`.
- Chromium Gerätefarben: Maus/Touch/Tastatur, Audio, Morph/Remix/Grids, Speicherung,
  Mobilansicht und Release-Sperre; keine Seitenfehler, gemessener Audiopeak 0,273466.
  Ergebnis `build/host/p9/device-browser/results.json`.
- Chromium Factory-Scheme: Erststart, SAVE/Neustart, bestehende Settings, alte/kurze
  Settings, Firmware-/Browserreset und Außenfarb-Isolation; keine Seitenfehler.
  Ergebnis `build/host/p9/factory-browser/results.json`.
- Drei echte neue Chromium-Remix-Aufnahmen; visuell kontrolliert: 909 A1/OFF/BEAT,
  909 B3/ON/BAR und 303A C4/ON/LOOP. Keine Seitenfehler. Die beiden 303-KEYS-Seiten
  und alle vier Remix-Seiten stimmen außerdem im direkten WASM-/Host-Pixelvergleich überein.
- `git diff --check`: keine Whitespacefehler.

Die ursprünglichen Fehlversuche sind in den früheren `build/host/p9/`-Logs nachvollziehbar:
veraltete Testgrößen/Knobzuordnungen, fehlende POSIX-Definition, Sandbox-EPERM für
Kindprozesse und ein geerbter Test für die nicht vorhandene `web/editor.html`. Der finale
Lauf behebt diese Ursachen. Der Felucca-Editor gehört nicht zu X0X und wird ausdrücklich
als nicht anwendbar ausgelassen; Paketparser und Browser-OTA sind erfolgreich geprüft.
Die drei externen DSP-Referenzcheckouts fehlen. Die eigentlichen DSP-Engines wurden in P9
nicht verändert. Der geerbte `tests/target_budget.py` hat weder seinen Felucca-Budgetdatensatz
noch passende X0X-Funktionsnamen und gehört nicht zur X0X-Suite; die Target-Buildprüfungen
für Image/RAM/Pool/Soft-Double/MMIO wurden vollständig ausgeführt.

Audio-Messwerte nach ADPCM-Decodierung:

| Loop | Peak (16 Bit) | RMS | Differenz an Loopgrenze |
|---|---:|---:|---:|
| 909 GR | 25.916 | 4.798,5 | 10 |
| 808 GR | 28.288 | 3.227,3 | 4 |

Deterministische Erzeugung, korrekte Länge, Headroom und unterschiedliche rhythmische
Energieverläufe geprüft (Distanz 2.961,3). Hördateien:
[909](../build/host/p9/909-groove.wav), [808](../build/host/p9/808-groove.wav).
Musikalische Klangqualität, Lautsprecher-/Kopfhörereindruck und echte FM1-Loopübergänge
bleiben Teil des Gerätetests; es wird kein automatisierter Hörnachweis behauptet.

## Hardwarestatus und Veröffentlichungssperre

Aus der Aufgabenstellung liegen erste P9-Beobachtungen auf FM-1_900 vor. Es wurde hier
keine physische FM1 bedient und kein iPad verbunden. Einzelne bestandene Hardwarefälle
werden ohne vorliegendes Protokoll nicht erfunden. Die finale 1.0.0-Geräteprüfung ist offen:
Update von FM-1_900, AudioShare/USB, bestehende USB-Einstellungen/FULL-Reset, Startpatterns,
Song/Chain, Sample-Erhalt, Bedienung und längere Wiedergabe ohne Dropouts/Abstürze.

Die [kompakte FM1-Anleitung](P9_FM1_TEST.md) ist dafür vorbereitet. Die
[Releasebeschreibung und Veröffentlichungsschritte](RELEASE_1.0.0.md) sind lokal vorbereitet.
Nach bestandenen Gerätetests werden deren Ergebnisse ergänzt; erst mit ausdrücklicher
Freigabe folgen Tag v1.0.0, GitHub-Stable-Release (Pre-release aus), Firmwareupload und
Installer-/Pages-Aktivierung mit erhaltener Pages-Historie. Danach öffentliche Links,
Checksumme, Identität und Emulator prüfen. `main` bleibt unverändert; kein Force-Push.

Das Video bleibt unverändert als frühere Emulatoraufnahme gekennzeichnet. Der Hinweis auf
noch ausstehende Erstvalidierung wurde im umgebenden Text auf den tatsächlichen Stand
korrigiert: erste P9-Beobachtungen vorhanden, abschließende 1.0.0-Prüfung offen. Eine spätere
Neufassung der Schlusskarte sollte diesen Stand ebenfalls ausdrücken. Historische
Upstream-Versionsnamen in der Versionshistorie bleiben als historische Namen erhalten;
sie bezeichnen nicht die neue Stable-Firmware.

## Geänderte Dateien

Die vier vorgefundenen unversionierten Benutzerdateien (`docs/P82_SCREENSHOT_REVIEW.md`,
`docs/P83_VIDEO_PLAN.md`, Root-Banner und dessen Zone.Identifier) bleiben unangetastet und
werden nicht mit committed. Buildartefakte und Testlogs bleiben unter `build/`.

- `BUILDING.md`
- `README.md`
- `docs/MANUAL.md`
- `docs/NEW.md`
- `docs/P9_FM1_TEST.md`
- `docs/P9_REPORT.md`
- `docs/PAGES.md`
- `docs/RELEASE_1.0.0.md`
- `docs/REMIX_DESIGN.md`
- `docs/img/p82/05-remix-909-off-a1.png`
- `docs/img/p82/06-remix-909-on-b3.png`
- `docs/img/p82/19-remix-303a-on-c4.png`
- `docs/img/p82/README.md`
- `docs/manual.html`
- `firmware/src/app/project.c`
- `firmware/src/app/ui.c`
- `firmware/src/app/x0x.h`
- `host/build_host.sh`
- `tests/host/break_connect_test.mjs`
- `tests/host/builtin_breaks_test.mjs`
- `tests/host/drum_gen_test.c`
- `tests/host/drum_gen_ui_test.c`
- `tests/host/p9_emu_test.mjs`
- `tests/host/p9_test.c`
- `tests/host/remix_state_test.c`
- `tests/host/remix_ui_test.c`
- `tests/host/run_remix_state.sh`
- `tests/host/scheme_ui_test.c`
- `tests/host/tb3po_ui_test.c`
- `tests/install_test.py`
- `tests/ldr_test.c`
- `tests/pages_test.py`
- `tests/release_test.py`
- `tests/run_tests.sh`
- `tests/scenarios/freeplay.x0x`
- `tests/scenarios/migrate.x0x`
- `tests/scenarios/remix_motion.x0x`
- `tests/scenarios/remix_parity.x0x`
- `tests/scenarios/remix_recording.x0x`
- `tests/scenarios/remix_ui.x0x`
- `tools/build.py`
- `tools/gen_builtin_break.c`
- `tools/gen_builtin_break.sh`
- `tools/make_pages.py`
- `web/emu/build.sh`
- `web/test_web.mjs`
- `web/x0x_breaks.html`
