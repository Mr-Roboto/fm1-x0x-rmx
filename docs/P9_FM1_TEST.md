# X0X-RMX v1.0.0 — abschließender FM1-Test

Noch keine Veröffentlichung. Alle Tests gelten für genau das Paket und die SHA-256 aus
[P9_REPORT.md](P9_REPORT.md). Ausgangsfirmware: `FM-1_900`. Ergebnisse pro Punkt mit
Datum, Gerätemodell, iPad-/OS-Version und ggf. Fehlerbeschreibung festhalten.

1. **Vorher sichern und dokumentieren.** Projekt und eigene Samples sichern. Manuelle
   Patternkombination, USB-AUDIO-Einstellung und Inhalte aller drei Sample-Slots notieren.
   Einen Durchlauf mit gespeichertem FOLLOWS KNOB vorbereiten. Kein Factory Reset vor
   der Prüfung des Erhalts vorhandener Einstellungen und Samples.
2. **Reguläres Update.** Lokal mit `python3 tools/fm1_install.py --info` die Ausgangs-ID
   prüfen. `python3 tools/fm1_install.py build/x0x-1.0.0.fwsc` ausführen, während des
   Schreibens verbunden lassen. Danach `FM-1_9010000` und Display `X0X 1.0.0` prüfen.
   Der öffentliche Web-Installer bleibt gesperrt. Reales Update noch nicht automatisiert belegt.
3. **Grundfunktion.** Display, alle Parts, PRESETS/ALGORITHM/SELECT, vier Knobs, Start/Stop,
   Kopfhörer/Line/Speaker und Lautstärkeregler prüfen. Mindestens zehn Minuten spielen;
   keine Abstürze, Knackser oder Aussetzer. PERF und Dropout-Anzeige bei dichter Wiedergabe
   einschließlich bestehender Morph-Funktion beobachten; Ergebnisse protokollieren.
4. **USB-Audio am iPad.** FM1 direkt wie gewohnt anschließen, AudioShare als Aufnahme-App.
   Stereoaufnahme starten, Hardwarelautstärke drehen. Mit gespeichertem FOLLOWS KNOB muss
   USB weiterhin folgen. FULL auswählen und speichern: USB-Pegel bleibt beim Drehen gleich,
   lokale Ausgänge reagieren. Ausschalten/einschalten: Einstellung bleibt erhalten.
5. **Factory Reset.** Erst nach Backups und Erhaltstest: Factory Reset ausführen. USB AUDIO
   muss FULL anzeigen und sofort entsprechend arbeiten. FREE PLAY und KEYS enthalten keine
   zusätzlichen Parameter. Die Projektpatterns werden beim Factory Reset zurückgesetzt.
6. **Manuelle Startkombination.** Per PRESETS auf den Part-Seiten z.B. 909=P2, 808=P5,
   303A=P9, 303B=P13, BREAK=P17 wählen. SAVE, ausschalten, einschalten: exakt dieselben fünf
   Patternnummern. HOME-Pattern-Taste wählen, SAVE, Neustart: alle fünf entsprechend gesetzt.
7. **AUTOSAVE und SAVE bei PLAY.** Neue Mischung einstellen, anhalten und mindestens vier
   Sekunden ohne Eingabe warten (bei kürzlichem Autosave bis zu 20 Sekunden Abstand).
   AUTOSAVED abwarten, aus/ein, Mischung prüfen. Während PLAY manuell andere Patterns wählen
   und SAVE drücken: SAVES WHEN STOPPED; STOP, SAVED abwarten, aus/ein, neue Mischung prüfen.
8. **Song und Chain.** Eine gespeicherte manuelle Mischung als Referenz notieren. Song mit
   abweichenden Patterns mehrere Takte spielen, STOP und SAVE/AUTOSAVE, aus/ein: manuelle
   Referenz bleibt. Chain über HOME-Tasten definieren; ihre erste manuelle Wahl wird zur
   Startkombination. Automatische Chain-Schritte abwarten, stoppen und speichern, aus/ein:
   erster manuell gewählter Start bleibt, nicht der letzte automatische Chain-Schritt.
9. **Legacy-Projekt.** Gesicherte ältere Projektdatei ohne Startkombination laden: alle Parts
   starten bei P1, vorhandene Sounds/Noten/Remix/APPLY bleiben erhalten. Neue Kombination
   wählen, speichern und Neustart prüfen. Nicht mit neuer Firmware gespeicherte Projekte
   auf ältere Firmware zurückschreiben; Downgrade-Lesbarkeit ist nicht zugesichert.
10. **Breaks und eigene Samples.** 909 GR und 808 GR bei 96 BPM mehrfach vollständig hören:
    zwei Takte, klar unterschiedliche Grooves, keine hörbar störenden Loopübergänge oder
    Verzerrung. Auch andere Tempi und Slice-Trigger prüfen. In jeden der drei User-Slots
    erkennbare eigene Samples laden und abspielen. Unter Windows ggf. `FM-1 Midi 2` im
    Browser-Uploader testen. Danach dasselbe reguläre Firmwareupdate erneut durchführen:
    Inhalte aller drei Slots müssen unverändert spielbar sein. Uploader-Löschen/Ersetzen
    ist ein separater ausdrücklicher Schreibvorgang, kein Firmwareupdate.
11. **303-Seiten und Remix.** Für A und B: SEQUENCE → REMIX → KEYS, KEYS nur FREE PLAY.
    Für 909/808/303A/303B auf REMIX links nach rechts APPLY/BANK/PATTERN/REMIX prüfen.
    Werte unabhängig setzen, speichern, Neustart; Timing STEP/BEAT/BAR/LOOP und A1 neutral
    prüfen. BREAK bleibt ohne Remix. Keine neue Tastenbelegung erwarten.
12. **Freigabeentscheidung.** Alle Ergebnisse, Auffälligkeiten, Paket-Hash und offene Punkte
    zurückmelden. Erst nach erfolgreicher Prüfung und ausdrücklicher Freigabe den offiziellen
    Stable-Release `v1.0.0`, Download und öffentlichen Installer aktivieren.
