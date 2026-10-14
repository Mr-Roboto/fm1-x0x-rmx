<!-- SPDX-License-Identifier: GPL-3.0-only -->
# FM1-X0X-RMX – TB-303 Waveform Design & Mathematics

## Current implementation (English summary)

Morph is implemented on `orb-remix` and remains experimental: physical FM-1 timing,
listening and installation validation are pending. This document preserves the original
German technical analysis and mathematics; the [English manual](MANUAL.md#6-303a-and-303b)
is the user-facing control reference.

- Both 303 voices have TONE, VOICE and EXTRA EDIT pages. WAVE is KNOB 2 on VOICE;
  MORPH is KNOB 1 on EXTRA, alongside ACCDEC, DRVTYPE and TUNE. Only VOLUME remains
  under MORE in SEL's list.
- WAVE selects Saw, Square or Morph. Morph 0/64/127 selects Sharkfin/Trapezoid/
  Notch-Trapezoid. Each voice keeps its own value; new defaults are 64. Older zeroed
  sound bytes load as 0, without an automatic Morph-default migration.
- Only one voice may use Morph at once. Conflicting UI/command/motion requests keep
  the previous accepted Wave; releasing the owner does not activate a rejected request.
  A conflicting loaded pair resolves to A = Morph, B = Saw.
- Changes while Morph sounds use a finite slew of approximately 5 ms, with coefficients
  held for a control chunk. Idle changes settle immediately.
- The implemented oscillator uses cubic four-box BLEP/BLAMP. The triangular kernel
  section is retained design history, not the current correction. Neither build success
  nor the one-voice restriction establishes universal audio quality or hardware headroom.

## Status und verständlicher Einstieg

Wave 3 ist auf `orb-remix` in Produktions-DSP, UI und Projekt-Sound
integriert, bleibt aber **experimentell: FM1-Hardwarevalidierung steht aus**.
Dieses bestehende Dokument enthält die aktuelle Spezifikation sowie die
historische Machbarkeitsanalyse und ursprüngliche Dreieckskern-Mathematik.
Der tatsächlich implementierte **kubische Vier-Box-BLEP/BLAMP-Kern** ist unten
separat definiert und ersetzt die ursprüngliche Korrektur für Wave 3.
Host-Prototyp, fokussierte Regressionen und Builds wurden bereits geprüft;
die damaligen Akzeptanzziele sind dadurch nicht pauschal bestanden.

**Wave = 0 Saw, 1 Square, 2 Morph.** Die vorhandenen Saw-/Square-Ausdrücke und
DSP-Pfade bleiben erhalten. Die dritte Auswahl ist eine Familie, deren Klang
sich mit dem Morph-Regler zwischen drei Formen bewegen lässt:

- **Sharkfin:** eine gebogene Rampe. Sie fällt zunächst langsam, dann immer
  schneller und springt am Zyklusende zurück nach oben. Gegenüber einem linearen
  Sägezahn soll sie eine andere Mischung aus Grundton und Obertönen bieten.
- **Trapezoid:** eine Welle mit zwei ebenen Abschnitten und zwei schrägen Flanken.
  Die Übergänge dauern einen endlichen Teil des Zyklus. Sie soll eine weichere
  Alternative zum Rechteck liefern, ohne ihren deutlichen Basscharakter zu verlieren.
- **Notch-Trapezoid:** dieselbe Trapezwelle mit einer dreieckigen Vertiefung im
  oberen Plateau. Diese Kerbe verändert die Obertonstruktur und soll zusätzliche
  Klangfarben für Acid-Lines eröffnen, ohne harte rechteckige Löcher einzuführen.

Die Namen bezeichnen die unten definierten Geometrien, keine verifizierte
Nachbildung eines anderen Instruments. Der Charakter der Host-Hörbeispiele wurde
freigegeben; das ersetzt weder Hörtests auf dem FM1 noch die noch offenen
Audioqualitäts- und Echtzeitprüfungen.

Die endgültige Morph-Zuordnung lautet **0 = Sharkfin, 64 = Trapezoid,
127 = Notch-Trapezoid**. Zwischenwerte verwenden kontinuierliches Smoothstep-
Blending mit Gain-Kompensation. Default ist 64. Der Morph-Regler liegt in
**EDIT → EXTRA → KNOB 1 (MORPH)** und hat pro Stimme einen eigenen Wert und 5-ms-Slew.

**Verbindliche Architektur:** Höchstens eine von 303A/303B darf `Wave = Morph`
verwenden. Eine konkurrierende Auswahl wird abgewiesen und belässt den zuvor
akzeptierten Wave-Wert; die andere Stimme wird nicht umgestellt. Saw oder Square
auf der aktiven Morph-Stimme gibt die Auswahl frei. Es gibt keine wartende
Anforderung oder spätere automatische Übernahme. Diese Regel gilt auch für
direkte Parameterkommandos und Motion. Speichern übernimmt akzeptierte Wave-
Werte und beide Morph-Werte; ein widersprüchliches geladenes Paar wird
bestimmt auf **A = Morph, B = Saw** aufgelöst.

## Einordnung der Aussagen

| Kennzeichnung | Bedeutung |
|---|---|
| **Quellcodebefund** | In der bestehenden Repository-Implementierung festgestellt |
| **Mathematisch hergeleitet** | Folgt aus den hier angegebenen Gleichungen |
| **Designvorschlag / historischer Entwurf** | Ursprüngliche Planung; nur ausdrücklich als implementiert bezeichnete Teile gelten für den aktuellen Code |
| **Schätzung** | Erwartung ohne Build- oder Laufzeitmessung der Erweiterung |
| **Ungeprüfte Annahme** | Insbesondere musikalische Wirkung oder Geräteverhalten |
| **Ausstehende Messung** | Erst durch Host-Prototyp oder Hardware zu bestimmen |

Die exakten Gleichungen und Koeffizienten der ursprünglichen mathematischen
Designphase bleiben als gekennzeichnete Historie erhalten. Für die aktuelle
Antialiasing-Korrektur gilt der kubische Abschnitt, für Geometrien und
Morph-Zuordnung gelten die unveränderten Definitionen. Näherungswerte ersetzen
keine exakten Definitionen. Einschränkungen bei wechselnder Tonhöhe, Morph-Modulation und
Oversampling sind ausdrücklich Teil der Spezifikation.

## Ziele und technische Grenzen

Prioritäten, in dieser Reihenfolge:

1. Klangqualität und Timing der bestehenden Instrumente erhalten.
2. CPU-Kosten im Audio-Sample-Loop minimieren.
3. Firmware-Image möglichst wenig vergrößern.
4. Zusätzliche statische RAM- und Pool-Nutzung gering halten.
5. Keine großen Wavetables, dynamischen Speicherzuweisungen oder neuen großen Puffer.
6. Projekt-Storage kompatibel halten, insbesondere persistente Strukturgrößen.

Die erste Machbarkeitsphase betrachtete eine strikte Null-RAM-Lösung. Die
anschließende Vorgabe stellt Wartbarkeit und zuverlässige Parameterbehandlung
über fragile Bit-Packing-Tricks: **acht zusätzliche Pool-Bytes** durch die
Erweiterung der beiden Parameterarrays sind grundsätzlich akzeptabel, vorbehaltlich
eines tatsächlichen Builds. Ein separates Smoothing-Float pro Stimme wäre ein
weiterer kleiner, ausdrücklich zu prüfender Bedarf.

Firmware-Arithmetik bleibt single precision; keine per-sample transcendental
functions. `-ffp-contract=off` ist zu erhalten. Bestehende Funktionen dürfen nicht
zur Freigabe von Kapazität reduziert werden. Kapazitätsprobleme müssen berichtet
werden. Diese experimentelle Implementierung liegt auf `orb-remix`; `main` bleibt davon unberührt.

## Bestehende Architektur: Saw, Square und Render-Pfad

**Quellcodebefund:** [`bass303.c`](../firmware/src/dsp/bass303.c) und
[`bass303.h`](../firmware/src/dsp/bass303.h) implementieren zwei unabhängige,
zustandsbehaftete 303-Instanzen. Der Signalpfad ist:

```text
phase oscillator → highpass → TeeBee filter → decimator
                 → post-filter stages → amplitude → optional drive
```

Saw und die transistorförmig geformte „303 square“ werden direkt berechnet. Die
großen mipmapped Wavetables der Open303-Referenz passen nicht zu den Ressourcen
dieses Ports. Für bestehende lokale Variablen gelten derzeit die Ausdrücke:

```text
t = frac(phase + 0.5)
Saw = -(2t - 1 - blep(t, inc))

Square = 0.5 × [fm_tanhf(69.98419960 × (2phase - 1) + 4.37)
                + sq_dc - sq_h2 × blep(phase, inc)]
sq_h2 = 0.5 × sq_h
```

Diese mathematische Schreibweise erlaubt **keine Umordnung des existierenden
C-Ausdrucks**: dessen Floating-point-Operationsfolge muss bestehen bleiben.
Saw hat den wesentlichen aufwärts gerichteten Reset bei `phase = 0.5`. Square
verwendet den vorhandenen Shaper, seine DC-Kompensation und eine BLEP-Korrektur
an der harten Kante bei `phase = 0`.

Der aktuelle Dispatch behandelt `wave == 2` als Morph-Familie, `wave == 0`
als Saw und den verbleibenden gültigen Wert 1 als Square. Parameter-Metadaten
begrenzen Wave auf 0..2. Neue Normalisierung und kubische Korrektur gelten nur
für Morph; Saw/Square behalten ihre bisherigen Ausdrücke und Operationsfolgen.

Standard ist `BASS303_OS = 2`; Compile-time-Konfigurationen 1 und 4 existieren.
Der Overload Guard schaltet beide Stimmen zur Laufzeit auf den Lite-Pfad mit
1× Oversampling. Hüllkurven und Filtersteuerung werden normalerweise alle vier
Ausgabesamples berechnet und dazwischen interpoliert. Der Idle-Pfad schreibt
Nullen und führt Phase/Hüllkurven nach; die bestehende Hold-off-Logik ist zu erhalten.

Die Filter-, Halfband-Decimator-, Envelope-, Phase-, Idle- und Drive-Pfade bleiben
unverändert. Unveränderte Ausdrücke allein garantieren noch keine Bit-Identität:
Compiler-Codegenerierung und Registerbelegung können sich ändern. Deshalb sind
direkte Baseline-Vergleiche und separate Timing-Messungen erforderlich.

## Ergebnisse der ersten Machbarkeitsphase

| Form | Ergebnis | Wesentliche Kosten und Einschränkungen |
|---|---|---|
| Sharkfin | Direkt aus Phase machbar | Niedriges Polynom; BLEP bei Reset und gegebenenfalls BLAMP bei Slope-Jump |
| Trapezoid | Direkt aus Phase machbar | Vier lineare Segmente und Corner-Korrekturen |
| Notch-Trapezoid | Machbar, teuerste der drei Formen | Zusätzliche Kerbengeometrie und Korrekturereignisse |
| Wavehole Triangle | Grundsätzlich machbar, zurückgestellt | Nicht definierte Lochgeometrie, zusätzliche Ereignisse und Testkombinationen |

Als frühe Sharkfin-Konstruktion wurde `2p² − 1` untersucht. Sie benötigt nur eine
Phasenmultiplikation, hat aber Mittelwert `−1/3`, einen Reset und einen
Slope-Mismatch. Die unten spezifizierte Polarität, Ausrichtung und Zentrierung
ersetzen diese frühe Skizze. Eine Exponentialkurve wäre ohne Approximation teurer.

Als Trapezbeschreibung wurde eine geclippte Dreieckswelle untersucht:

\[
\operatorname{triangle}(p)=1-4|p-\tfrac12|,\qquad
\operatorname{trapezoid}(p)=\operatorname{clamp}(g\operatorname{triangle}(p),-1,1),
\quad g>1.
\]

Das ist als Geometrie hilfreich, aber Clipping eines bereits antialiasierten
Signals bleibt nicht automatisch antialiasiert. Die finale Spezifikation
verwendet direkte Segmente und korrigiert deren Corner-Ereignisse.

Eine kontinuierliche Kerbe ist gegenüber einem rechteckigen Loch vorzuziehen:
Ein rechteckiges Loch würde zwei zusätzliche Value-Jumps und BLEPs einführen.
Eine allgemeine dreieckige Kerbe hätte bis zu drei zusätzliche Slope-Ereignisse.
Die finale Platzierung teilt zwei Positionen mit Trapez-Corners und reduziert
die kombinierte Form auf fünf unterschiedliche Ereignispositionen.

Die Machbarkeit ist damit gegeben, die Echtzeitfähigkeit aber **nicht gemessen**.
Insbesondere zwei aktive Stimmen im dichten Gesamtmix und der 1×-Guard-Pfad
müssen geprüft werden. Höheres Oversampling als Ausweichlösung würde CPU kosten.

## Exakte Phase und Wellenformen

Oszillatorphase sei \(p\in[0,1)\). Für alle neuen Formen gilt:

\[
u=\operatorname{frac}(p+\tfrac12).
\]

**Implementierte Geometrie:** Der Zyklus beginnt im oberen Bereich der Formen. Sharkfins
aufwärts gerichteter Reset liegt bei \(p=1/2\), entsprechend dem wesentlichen
Saw-Reset. Trapezoid und Notch-Trapezoid sind dort kontinuierlich. Bestehende
Oszillatorphase wird weder zurückgesetzt noch verändert.

Die folgenden Funktionen sind ideale Continuous-time-Formen vor Antialiasing
und vor Gain-Normalisierung.

### Sharkfin

\[
S(u)=1-3u^2.
\]

Die Kurve fällt zunächst langsam und anschließend steiler. Äquivalent beginnt
man mit \(1-2u^2\), subtrahiert dessen Mittelwert \(1/3\) und multipliziert mit
\(3/2\). Daher ist \(S\) bereits mittelwertfrei.

\[
S'(u)=-6u,\qquad S''(u)=-6.
\]

Am Zyklusübergang sind die rechts-minus-links definierten Sprünge:

\[
J=S(0^+)-S(1^-)=3,
\]

\[
\Delta=S'(0^+)-S'(1^-)=6.
\]

Es sind sowohl Value-Jump als auch Slope-Jump zu korrigieren. Die Nullstelle
liegt bei \(u=1/\sqrt3\).

### Trapezoid

\[
T(u)=
\begin{cases}
1,&0\le u<\tfrac14,\\
3-8u,&\tfrac14\le u<\tfrac12,\\
-1,&\tfrac12\le u<\tfrac34,\\
8u-7,&\tfrac34\le u<1.
\end{cases}
\]

Positives Plateau, fallende Rampe, negatives Plateau und steigende Rampe dauern
jeweils ein Viertel des Zyklus. Die Slopes sind \(0,-8,0,+8\). Die Funktion ist
überall kontinuierlich, auch am Wrap; der Mittelwert ist null.

### Notch-Trapezoid

Die feste dreieckige Kerbe ist:

\[
H(u)=
\begin{cases}
8u,&0\le u<\tfrac18,\\
2-8u,&\tfrac18\le u<\tfrac14,\\
0,&\tfrac14\le u<1.
\end{cases}
\]

**Implementierte Geometrie:** Zentrum \(u=1/8\), Halbbreite \(w=1/8\), volle Breite
\(1/4\) Zyklus und Tiefe 1. Der Support umfasst genau das positive Plateau.
Vor DC-Korrektur erreicht die Kerbe die ursprüngliche Nulllinie; sie erzeugt
kein tiefes negatives Loch. Breite und Tiefe sind keine weiteren Parameter.

Die Fläche der Kerbe ist \(1/8\). Der Mittelwert von \(T-H\) ist deshalb
\(-1/8\). Die mittelwertfreie Form lautet:

\[
N(u)=T(u)-H(u)+\tfrac18.
\]

### Phase-/Amplitude-Landmarks

Alle Werte sind nach analytischer DC-Zentrierung, vor Gain und Antialiasing.

| \(u\) | \(p\) | \(S\) | \(T\) | \(N\) |
|---|---|---|---|---|
| \(0\) | \(1/2\) | \(1\) | \(1\) | \(9/8\) |
| \(1/8\) | \(5/8\) | \(61/64\) | \(1\) | \(1/8\) |
| \(1/4\) | \(3/4\) | \(13/16\) | \(1\) | \(9/8\) |
| \(1/2\) | \(0\) | \(1/4\) | \(-1\) | \(-7/8\) |
| \(3/4\) | \(1/4\) | \(-11/16\) | \(-1\) | \(-7/8\) |
| \(1^-\) | \(1/2^-\) | \(-2\) | \(1\) | \(9/8\) |

**Mathematisch hergeleitet:** Sharkfins Krümmung verändert die harmonische
Zusammensetzung gegenüber der linearen Saw. Trapezoid erfüllt
\(T(u+1/2)=-T(u)\) mit periodischer Fortsetzung und besitzt ideal nur ungerade
Harmonische. Seine endlichen Flanken ergeben einen weicheren Spektralabfall als
harte Rechteckflanken. Die Kerbe bricht die Halbzyklussymmetrie und erzeugt
zusätzlich gerade Harmonische. Die musikalische Eignung auf dem FM1 bleibt ungeprüft.

## Ursprünglicher Dreieckskern: PolyBLEP und kompakte PolyBLAMP-Korrektur

**Historischer Entwurf, nicht die aktuelle Wave-3-Korrektur.** Die folgenden
Gleichungen bleiben zur Nachvollziehbarkeit erhalten. Der quadratische BLEP
und sein integrierter BLAMP entsprechen einem Zwei-Box-Dreieckskern mit
Support ±d. Wave 3 verwendet stattdessen den anschließend spezifizierten
kubischen Vier-Box-Kern. Die bisherige Saw-/Square-Korrektur bleibt unverändert.

Das tatsächliche Phaseninkrement bei der aktuellen Oszillatorrate ist:

\[
d=\frac{f}{44100\,OS_{\mathrm{effective}}}.
\]

Für ein Ereignis bei Phase \(e\) wird verschoben:

\[
t=\operatorname{frac}(u-e).
\]

### Value-Jump: vorhandene Two-point-PolyBLEP

Die vorhandene quadratische Residualfunktion ist:

\[
B(t,d)=
\begin{cases}
2x-x^2-1,&t<d,\quad x=t/d,\\
x^2+2x+1,&t>1-d,\quad x=(t-1)/d,\\
0,&\text{sonst}.
\end{cases}
\]

Für \(J=f(e^+)-f(e^-)\) wird addiert:

\[
\boxed{\frac{J}{2}B(t,d)}.
\]

Die Vorzeichenkonvention entspricht der bestehenden Saw-Korrektur.

### Slope-Jump: integrierte Residualfunktion

Für \(\Delta=f'(e^+)-f'(e^-)\) gilt:

\[
R(t,d)=
\begin{cases}
\dfrac d6(1-t/d)^3,&t<d,\\
\dfrac d6\left(1+(t-1)/d\right)^3,&t>1-d,\\
0,&\text{sonst}.
\end{cases}
\]

Zu addieren ist:

\[
\boxed{\Delta R(t,d)}.
\]

**Wichtig:** \(\Delta\) ist die Ableitung nach Phase, nicht nach Sekunden oder
Sample-Index. Der Faktor \(d\) enthält bereits die Umrechnung auf die
Per-sample-Slope. Die konkrete Residualfunktion folgt durch Integration der
vorhandenen BLEP; die externe BLAMP-Literatur begründet das allgemeine Verfahren.

### Vollständige Correction Events

| Form | Phase \(e\) | Value-Jump \(J\) | Slope-Jump \(\Delta\) |
|---|---|---|---|
| Sharkfin | \(0\) | \(+3\) | \(+6\) |
| Trapezoid | \(0\) | 0 | \(-8\) |
| Trapezoid | \(1/4\) | 0 | \(-8\) |
| Trapezoid | \(1/2\) | 0 | \(+8\) |
| Trapezoid | \(3/4\) | 0 | \(+8\) |
| Notch-Trapezoid | \(0\) | 0 | \(-16\) |
| Notch-Trapezoid | \(1/8\) | 0 | \(+16\) |
| Notch-Trapezoid | \(1/4\) | 0 | \(-16\) |
| Notch-Trapezoid | \(1/2\) | 0 | \(+8\) |
| Notch-Trapezoid | \(3/4\) | 0 | \(+8\) |

Mit \(R_e=R(\operatorname{frac}(u-e),d)\) sind die korrigierten Templates:

\[
S_d=S+\tfrac32B(u,d)+6R_0-\tfrac12d^2,
\]

\[
T_d=T-8R_0-8R_{1/4}+8R_{1/2}+8R_{3/4},
\]

\[
N_d=N-16R_0+16R_{1/8}-16R_{1/4}+8R_{1/2}+8R_{3/4}.
\]

**Mathematisch hergeleitet:** Die Phase-Integration eines Unit-slope-Residuals
ergibt \(d^2/12\). Sharkfins Slope-Event \(+6\) würde daher einen Mittelwert
\(d^2/2\) erzeugen. Der Term **\(-d^2/2\)** ist notwendig. Bei Trapezoid und
Notch-Trapezoid summieren sich die Slope-Jumps zu null.

Für stationäres \(d<1/2\) entsprechen diese Formeln der periodischen Glättung
der idealen Formen mit einem Dreieckskern. Überlappende Residuals müssen deshalb
**addiert** werden. Sie dürfen nicht durch eine gegenseitig ausschließende
Ereignisauswahl unterdrückt werden. Das erlaubt einen zustandslosen Helper ohne
History-Buffer. Die Funktion ist dennoch nicht perfekt bandbegrenzt.

## Implementierter kubischer Vier-Box-BLEP/BLAMP-Kern

**Quellcodebefund:** `cubic_residual()`, `cubic_active_correction()` und
`morph_eval()` in [`bass303.c`](../firmware/src/dsp/bass303.c) übernehmen die
validierte kubische Variante aus
[`waveform_prototype.c`](../tests/host/waveform_prototype.c). Vier gleich breite,
normalisierte Box-Kerne der Phasenbreite \(d\) ergeben durch Faltung einen
kubischen B-Spline-Kern. „Kubisch“ bezeichnet den Glättungskern; dessen BLEP-
Residual ist stückweise quartisch, dessen BLAMP-Residual stückweise quintisch.
Dies ist eine explizite Änderung gegenüber dem ursprünglichen Dreiecksentwurf.

Das Inkrement \(d\) ist das tatsächlich im aktuellen Oversample verwendete,
gegebenenfalls durch den bestehenden Pitch-Control-Pfad interpolierte Inkrement.
Für stationäre Tonhöhe gilt weiterhin \(d=f/(44100\,OS_{\mathrm{effective}})\).
Das laufende Inkrement bestimmt die Korrekturbreite auch nach Guard-Umschaltung.

### Residualfunktionen und Vorzeichen

Ein Event bei \(u=e\) liegt bei \(p_e=\operatorname{frac}(e+1/2)\).
Die Implementierung berechnet den nächsten signierten periodischen Abstand
\(t=p-p_e\), auf \([-1/2,1/2]\) gewrappt. Damit wird die Rundung von
`frac(phase + .5)` nahe dem Reset nicht durch die anschließende Division
verstärkt. Die Raw-Geometrie verwendet weiterhin \(u=\operatorname{frac}(p+1/2)\).
Mit \(r=|t|/d\) lauten Tail und integriertes Tail:

\[
F(r)=\begin{cases}
\frac12-\frac23r+\frac13r^3-\frac18r^4,&0\le r<1,\\
\frac{(2-r)^4}{24},&1\le r<2,\\
0,&r\ge2,
\end{cases}
\]

\[
G(r)=\begin{cases}
\frac7{30}-\frac12r+\frac13r^2-\frac1{12}r^4+\frac1{40}r^5,&0\le r<1,\\
\frac{(2-r)^5}{120},&1\le r<2,\\
0,&r\ge2.
\end{cases}
\]

Die lokale BLEP- und BLAMP-Korrektur ist:

\[
B_c(t,d)=\begin{cases}+2F(r),&t<0,\\-2F(r),&t\ge0,\end{cases}
\qquad R_c(t,d)=dG(r).
\]

Für einen rechts-minus-links Value-Jump \(J\) und Slope-Jump \(\Delta\) wird
\(\frac J2 B_c+\Delta R_c\) addiert. Bei \(t=0\) folgt das negative
BLEP-Vorzeichen aus der rechtsseitigen Raw-Geometrie. Slope-Jumps sind weiterhin
Ableitungen nach Phase. Die Event-Tabelle des ursprünglichen Abschnitts bleibt
gültig; geändert sind die Residualfunktionen, ihre Breite und die Sharkfin-DC-Korrektur.

### Periodische Überlappung und DC

Support ist **±2d**, statt ±d beim Dreieckskern. Unterschiedliche Events werden
immer addiert. Bei \(d>1/4\) kann außerdem die benachbarte periodische Kopie
*desselben* Events beitragen: zusätzlich zum nächsten Abstand \(t\) wird
\(t+1\) für \(t<0\), sonst \(t-1\), ausgewertet. Für \(0<d<1/2\) reicht diese
eine Nachbarkopie. Ein exklusiver Event-Dispatch würde diese Überlappung verlieren.

Seien \(B^c_e\) und \(R^c_e\) die so periodisch summierten Residuals eines Events
bei \(u=e\). Die aktuellen Templates vor Gain sind:

\[
S_d^{(c)}=S+\tfrac32B^c_0+6R^c_0-d^2,
\]

\[
T_d^{(c)}=T-8R^c_0-8R^c_{1/4}+8R^c_{1/2}+8R^c_{3/4},
\]

\[
N_d^{(c)}=N-16R^c_0+16R^c_{1/8}-16R^c_{1/4}
             +8R^c_{1/2}+8R^c_{3/4}.
\]

Das phasenintegrierte Unit-slope-Residual beträgt jetzt \(d^2/6\), gegenüber
\(d^2/12\) im Dreiecksentwurf. Sharkfins Slope-Jump +6 benötigt deshalb
**−d² statt −d²/2**. Im kombinierten normalisierten Evaluator ist das
`-mix.shark * d * d`. Bei Trapezoid und Notch-Trapezoid bleibt die Summe der
Slope-Jumps null. Die analytische stationäre DC-Zentrierung hängt an dieser
geänderten Korrektur; der alte DC-Term wäre für den kubischen Kern falsch.

| Eigenschaft | Ursprünglicher Dreiecksentwurf | Implementiertes Wave 3 |
|---|---|---|
| Kernel | Zwei Boxen, Dreieck | Vier Boxen, kubischer B-Spline |
| Support je lokaler Event-Kopie | ±d | ±2d |
| BLEP / BLAMP | Quadratisch / kubisch | Stückweise quartisch / quintisch |
| Sharkfin-DC vor Gain | −d²/2 | −d² |
| Nachbarbild desselben Events | Für d < 1/2 nicht nötig | Zusätzlich für d > 1/4 |
| Geometrien, Event-Sprünge, Morph-Landmarks | Ursprüngliche Definitionen | Unverändert |

Der breitere, glattere Kern verändert die harmonische Dämpfung und die
Rechenkosten; er ist kein idealer Brickwall-Bandlimit. Die Cubic-Variante wurde
als Host-Prototyp geprüft und bewusst für die experimentelle Integration
gewählt. Bestehende spektrale Fehlkriterien werden dadurch weder umbenannt noch
als bestanden erklärt. Dynamische Tonhöhe, Morph und Ratenwechsel bleiben
lokale Korrekturapproximationen; FM1-CPU-Messungen und Gerätetests stehen aus.

## DC, feste RMS-Normalisierung und Pegel

**Implementierte Level-Policy:** Ziel-RMS der idealen unfiltrierten Formen:

\[
r=1/\sqrt3.
\]

Das entspricht dem RMS einer idealen Saw mit Bereich \([-1,1]\), ohne identische
wahrgenommene Lautheit zur bestehenden Saw oder Square zu behaupten.

| Form | Mittelwert vor DC-Korrektur | Varianz der zentrierten Form |
|---|---|---|
| \(S\) | 0 | \(4/5\) |
| \(T\) | 0 | \(2/3\) |
| \(T-H\) | \(-1/8\) | \(31/64\) |

Die festen Gains lauten exakt:

\[
g_S=\sqrt{5/12}\approx0.6454972244,
\]

\[
g_T=1/\sqrt2\approx0.7071067812,
\]

\[
g_N=8/\sqrt{93}\approx0.8295613558.
\]

Die aktuellen kubisch korrigierten und normalisierten Endpoint-Signale sind:

\[
s_d=g_SS_d^{(c)},\qquad t_d=g_TT_d^{(c)},\qquad n_d=g_NN_d^{(c)}.
\]

Die idealen Endpoint-Bereiche vor Antialiasing sind ungefähr:

- Sharkfin: \([-1.2910,0.6455]\).
- Trapezoid: \([-0.7071,0.7071]\).
- Notch-Trapezoid: \([-0.7259,0.9333]\).

Diese Werte dürfen nicht allein zur Erzwingung eines Unit-Peaks geclippt werden.
Die asymmetrischen Peaks und der höhere Crest Factor von Sharkfin müssen im
Filter-/Drive-Pfad untersucht werden.

## Smoothstep-Morph und Gain-Kompensation

Morph \(m\) wird auf \([0,127]\) begrenzt. Für kontinuierliche Betrachtung und
Smoothing darf \(m\) reell sein. Die gespeicherte Pot-Einstellung ist ganzzahlig.

\[
h(x)=x^2(3-2x).
\]

Smoothstep besitzt exakte Endpunkte und verschwindende erste Ableitungen an
beiden Enden. Die Intervallgewichte sind:

\[
a=h(m/64),\qquad 0\le m\le64,
\]

\[
b=h((m-64)/63),\qquad64\le m\le127.
\]

Gleiche Endpoint-RMS-Werte verhindern keinen Blend-Dip, weil die Formen
korreliert sind. Die hergeleiteten idealen Korrelationen lauten:

\[
\rho_{ST}=\frac{1/4}{\sqrt{(4/5)(2/3)}}\approx0.3423265984,
\]

\[
\rho_{TN}=\frac{13/24}{\sqrt{(2/3)(31/64)}}\approx0.9532062476.
\]

Für beide Intervalle gilt:

\[
\boxed{C_\rho(w)=
\left[(1-w)^2+w^2+2\rho w(1-w)\right]^{-1/2}}.
\]

Die vollständige Morph-Funktion ist:

\[
Y(m)=
\begin{cases}
C_{ST}(a)\left[(1-a)s_d+a\,t_d\right],&m\le64,\\
C_{TN}(b)\left[(1-b)t_d+b\,n_d\right],&m\ge64.
\end{cases}
\]

Dabei verwendet \(C_{ST}\) die Korrelation \(\rho_{ST}\), \(C_{TN}\) entsprechend
\(\rho_{TN}\). Die Endpunkte sind:

\[
Y(0)=s_d,\qquad Y(64)=t_d,\qquad Y(127)=n_d.
\]

Beide Zweige ergeben bei 64 dasselbe Trapezoid. Die ersten Ableitungen nach
kontinuierlichem Morph verschwinden dort ebenfalls. Die statische Funktion hat
also weder einen Value-Jump noch einen zentralen Knick. „Exakter Endpoint“
bezeichnet das spezifizierte **antialiasierte Template**, nicht eine naiv
abgetastete Form mit unkorrigierten Kanten.

**Mathematisch hergeleitet:** Ohne Gain-Kompensation beträgt der größte ideale
RMS-Dip etwa 1.73 dB im ersten und 0.10 dB im zweiten Intervall. Die größte
Kompensation ist ungefähr 1.221 beziehungsweise 1.012. Für die idealen,
stationären, unfiltrierten Formen hält sie den RMS exakt bei \(1/\sqrt3\).

**Quellcodebefund:** `precise_compensation()` wertet die Kompensation außerhalb
des Oversampled-Loops mit `fm_rsqrtf()` und einem zusätzlichen Newton-Schritt aus:
`c * (1.5f - 0.5f * variance * c * c)`. Für Varianz exakt 1 ist das Ergebnis
exakt 1. `morph_prepare()` verwendet an 0, 64 und 127 explizit die festen
Endpoint-Gains, sodass die Reciprocal-square-root-Approximation deren Auswahl
nicht beeinflusst. Die Float-Konstanten sind `0.6454972243679028f`,
`0.7071067811865475f` und `0.8295613557843402f`; die Korrelationskonstanten sind
`0.3423265984407288f` und `0.9532062476387964f`.

Die Korrelationen gelten für ideale Formen. Antialiasing verändert sie mit der
Tonhöhe; Filter und Drive beeinflussen Pegel und wahrgenommene Lautheit weiter.
Keine pitchabhängige oder audio-following Normalisierung in der aktuellen Umsetzung.
Stationäre Blends bleiben analytisch DC-frei. Zeitveränderliche Gewichte können
trotzdem niederfrequente Modulation erzeugen.

### Gemeinsame Berechnung im zweiten Intervall

\[
H_d^{(c)}=H+8R^c_0-16R^c_{1/8}+8R^c_{1/4},
\]

\[
N_d^{(c)}=T_d^{(c)}-H_d^{(c)}+\tfrac18.
\]

Daraus folgt vor Morph-Gain-Kompensation:

\[
(1-b)g_TT_d^{(c)}+b g_NN_d^{(c)}
=\big[(1-b)g_T+b g_N\big]T_d^{(c)}-b g_NH_d^{(c)}+\frac{b g_N}{8}.
\]

Es müssen somit nicht zwei vollständige Endpoint-Oszillatoren separat laufen.
Auch im ersten Intervall ist die gemeinsame BLAMP-Position bei null nur einmal
mit kombiniertem Koeffizienten auszuwerten.

### Implementiertes Morph-Smoothing: 5 ms

Ein eigenes Float pro Stimme hält die geglättete Morph-Position. Das Ziel bleibt
der gespeicherte Integer-Pot.

- Maximale Bewegung: 127 Morph-Einheiten in ungefähr 221 Output-Samples.
- Bei kleinerer verbleibender Distanz exakt auf das Ziel springen.
- Zeitbasis ist die Base-Rate, damit 1×/2×/4× und Guard-Umschaltung dieselbe Dauer haben.
- Mapping und Gain-Kompensation verwenden die geglättete Position.
- Während Morph läuft, rückt `morph_advance()` pro Control-Chunk um höchstens
  `(127.0f / 221.0f) * samples` zum Ziel vor. Danach berechnet `morph_prepare()`
  Gewichte, Gain und Event-Koeffizienten neu und hält sie für den Chunk konstant.
  Es gibt keine zusätzliche Koeffizienteninterpolation zwischen Morph-Chunks.
- Bei erreichtem Ziel werden die Koeffizienten einmal pro Render-Aufruf vorbereitet.
  Ein voller Weg erreicht das Ziel in ungefähr 221 Base-Rate-Samples plus einem
  Control-Chunk. Die Tonhöhen-/Filterinterpolation bleibt unverändert.
- Morph-Änderungen bei Saw/Square ändern deren Oszillator nicht. Im Idle setzt
  der Setter die Morph-Position sofort aufs Ziel; auch Idle-Rendering hält sie dort.

Das ist ein endlicher Slew, kein asymptotischer One-pole, der den Endpoint nie
exakt erreicht. Smoothing reduziert Klicks, garantiert aber keine bandbegrenzte
Modulation. Ein Crossfade zwischen den drei Wave-Modi wäre eine separate,
zusätzlich zu messende Option und gehört nicht zur Minimalimplementierung.

## Antialiasing-Grenzen und Pitch-/Rate-Verhalten

| Situation | Bewertung und erforderliche Prüfung |
|---|---|
| Tiefe MIDI-Noten | Kleine Correction Supports; Präzision von Phase und DC ausdrücklich testen |
| Hohe Noten | Mehr überlappende Residuals, weniger darstellbare Harmonische und geringere Formunterschiede |
| MIDI 0–127 | Akzeptierter Bereich im bestehenden Note-on-Pfad; nicht auf typische Bassnoten beschränken |
| Maximales Tune, Note 127 | Etwa 13.67 kHz, \(d\approx0.310\) bei 1×; unter der Formelgrenze \(1/2\) |
| Überlappende Supports | Sämtliche Events addieren; keine exklusive Auswahl |
| 2×/4× | Tatsächliches Oszillatorinkrement verwenden; bestehende Filterraten und Decimator beibehalten |
| Guard → 1× | Correction Width sofort aus aktuellem Inkrement ableiten; Transition-Transienten messen |
| Morph um 64 | Gemeinsamer Endpoint und verschwindende Mapping-Ableitungen verhindern statischen Sprung |
| Schnelle Automation | Sidebands und tieffrequente Modulation bleiben möglich |
| Zwei Stimmen | Sämtlicher veränderlicher Morph-Zustand pro Instanz; keine mutable static Helpers |

Die feste Geometrie besitzt keine gegen null laufende Rampenbreite und keine
Width-Division als Parameterfunktion. Residuals mindern Aliasing; sie ersetzen
keinen idealen Brickwall-Bandlimit. Bereits im Oszillator zurückgefaltete Aliases
können nachfolgende Ladder-Filter oder Decimator nicht entfernen. Die Drive-
Stufen erzeugen weiterhin zusätzliche Harmonische.

Die jeweilige Kernel-Interpretation und die analytischen DC-Aussagen gelten für
stationäres Inkrement. Bei Slew, Morph-Automation und Rate-Wechseln ist die
lokale Korrektur eine Approximation, deren Transienten geprüft werden müssen.

## CPU, Flash, RAM und Pool

### Rechenaufwand

| Bereich | Aufwand vor weiteren Optimierungen |
|---|---|
| Exakter Sharkfin | Quadratik, eine BLEP- und eine BLAMP-Position |
| Sharkfin → Trapezoid | Beide Raw-Formen, eine BLEP- und vier unterschiedliche BLAMP-Positionen |
| Exakter Trapezoid | Vier Segmente und vier BLAMP-Positionen |
| Trapezoid → Notch-Trapezoid | Gemeinsamer Trapezoid, Kerbe und fünf BLAMP-Positionen |
| Exakter Notch-Trapezoid | Fünf Correction Positions |

Keine per-sample transcendental functions sind nötig. Divisionen der Residuals
fallen nur innerhalb ihres Supports an. Branch-Checks kosten aber auch außerhalb
des Supports Zeit; bei hohen Noten liegen mehr Samples im Support. Ob ein
gemeinsamer Reciprocal günstiger ist, ist **ausstehende Messung**.

Zwei dauerhaft aktive 303-Stimmen benötigen insgesamt die folgenden
Oszillatorauswertungen. In der Anwendung darf nur eine davon die komplexe
Morph-Familie verwenden; bei Standard 2× sind das höchstens 88,200 komplexe
Auswertungen pro Sekunde. Frühere Zwei-Morph-Host-Fälle dienen ausschließlich
Evaluator-/Isolationsprüfungen, nicht als zulässige Anwendungskonfiguration.


| Effective OS | Oscillator Evaluations pro Sekunde |
|---|---:|
| 1× / Lite | 88,200 |
| Standard 2× | 176,400 |
| 4× | 352,800 |

Zusätzliche \(\delta\) Zyklen pro Evaluation kosten ungefähr
\(88{,}200\,OS\,\delta\) Zyklen pro Sekunde, zuzüglich Control-/Block-Arbeit.
Dies ist eine Strukturabschätzung, kein gemessener Geräteprozentsatz.

### Speicher und historischer Build-Befund

**Ursprüngliche Schätzung:** Pot-Wachstum und Smoothing wurden zusammen auf
etwa 16 zusätzliche Pool-Bytes geschätzt. **Implementiert** sind `pot[13]`, ein
Morph-Smoothing-Float und ein separates `uint8_t morph_target` pro Instanz.
Mit Alignment wächst `bass303_t` bei OS2/CTRL4 von 752 auf **764 B**, zusammen
**24 zusätzliche Pool-Bytes** gegenüber dem Stand vor Wave 3. Davon entfallen
16 B auf die DSP-Phase und weitere 8 B auf die Parameterfreigabe.

Der vorbereitete Mix hat zehn Floats (40 B) und liegt lokal im Render-Pfad,
nicht in einer zusätzlichen globalen DSP-Allokation. `engine.c` publiziert Basis-
und effektive Wave-Werte in einem vier Byte großen Wort. Dieses Wort dient der
konsistenten UI-/Save-Rückmeldung, nicht einer separaten Ownership-Verwaltung.
Es gibt keine neuen großen Tabellen, History-Buffer oder dynamischen Allokationen.
Der konkrete Hardware-Stack-Highwater ist noch zu prüfen.

Die Erstphase erwog, Morph in das vorhandene Integer-Feld `wave` zu packen, um
Null-RAM-Wachstum zu erreichen. **Diese Alternative ist nicht die Empfehlung**:
normale Pot-Behandlung und klarer eigener Zustand sind nach der aktualisierten
Ressourcenvorgabe vorzuziehen. Square-/Filter-Zustände werden nicht zweckentfremdet.

**Historischer Befund, kein neuer Build:**
`build/host/tb3po-mpen-firmware.log` meldete:

| Ressource | Gemeldeter Verbrauch | Verbleibend |
|---|---:|---:|
| Firmware-Image | 570,032 B | App-Slot separat prüfen |
| `.data + .bss` | 97,244 / 98,304 B | 1,060 B |
| Pool | 334,324 / 344,064 B | 9,740 B |

Der Build-Checker fordert mindestens 8 KiB Pool-Reserve. Oberhalb dieser Reserve
blieben dort 1,548 B. Eine weitere in der Machbarkeitsphase betrachtete Logdatei,
`build/host/remix-firmware-build.log`, meldete 569,556 B Image bei denselben RAM-
und Pool-Werten. `AGENTS.md` nennt den älteren Baseline-RAM-Wert 97,216 B.
Diese verschiedenen Artefakte sind keine frische Messung des aktuellen Checkouts.
Die Logs sind lokale Build-Artefakte und möglicherweise nicht in jedem Checkout vorhanden.

**Vorhandener Build nach DSP- und UI-Integration:**
`build/host/waveform-ui/firmware.log` berichtet 574,048 B App-Image,
97,244 / 98,304 B `.data+.bss` und 334,348 / 344,064 B Pool. Es verbleiben
1,060 B statische RAM-Reserve und 9,716 B Pool-Reserve, davon 1,524 B oberhalb
der geforderten 8 KiB. Keine Soft-double-Routinen wurden gelinkt.
Der DSP-Phasen-Build hatte 573,088 B Image und 334,340 B Pool; die UI-Phase
kostete somit weitere 960 B Image und 8 B Pool, bei unverändertem statischen RAM.
Der Browser-Emulator wurde mit 654,183 B WASM gebaut. Dies sind bereits
vorliegende Artefakte der Integration, keine neuen Builds dieser Dokumentationsphase
und kein Beleg für FM1-Echtzeitfähigkeit.

## Parameter, Persistence und UI

**Quellcodebefund:** `sound_t` behält 16 Parameterbytes je `(target, voice)`.
Die 303 nutzt jetzt 13 davon. `BASS303_MORPH` ist Index 12; alle vorherigen
Indizes bleiben unverändert. Wave hat die Werte 0 Saw, 1 Square, 2 Morph.
Morph ist 0..127, Default 64, mit den Landmarks Sharkfin (0), Trapezoid (64)
und Notch-Trapezoid (127). Beide Stimmen haben unabhängige Morph-Werte.

- `sound_t`, Projektobjekte, Pattern-/Arrangement-Stream und gespeicherte
  Motion-Lanes wachsen nicht. `pot[13]` ist ausschließlich Runtime-Zustand.
- VOICE behält Accent, Wave, Drive und Slide. In EDITs SEL-Liste liegt Morph
  auf EXTRA nach TONE und VOICE: Morph, Accent Decay, Drive Type und Tune; nur
  Volume bleibt unter MORE. SELECT navigiert,
  ALGORITHM ändert den markierten Wert. Morph bleibt bei Saw/Square gespeichert,
  beeinflusst aber nur die Morph-Familie; es ist nicht aus der Liste ausgeblendet.
- UI und direkte Parameterkommandos gehen über die vorhandene Command Queue.
  Die UI schreibt keine DSP-Parameter direkt. Wave-Anforderungen werden nicht
  vorzeitig als erfolgreich in den Projekt-Sound geschrieben.
- Der gemeinsame Audio-Callback `apply_param()` prüft den effektiven Wave-Wert
  der anderen Stimme. Hat sie Morph, bleibt eine konkurrierende Morph-Anforderung
  ohne Änderung des bisherigen akzeptierten Wave-Werts. Es gibt kein Owner-Objekt,
  keinen Vormerkzustand und kein Erzwingen eines anderen Wave-Werts beim Konkurrenten.
- Saw/Square auf der aktuellen Morph-Stimme löst die Sperre sofort bei Verarbeitung
  des Kommandos. Eine abgewiesene Anforderung wird nicht nachträglich wiederholt;
  eine neue UI-Auswahl oder ein neuer Motion-Step kann frisch anfordern.
- `engine_sync_sound()` übernimmt audio-akzeptierte Basis-Waves in den Main-Loop-
  Mirror, insbesondere vor `project_save()`. Vor Verarbeitung eines Kommandos
  wird noch der zuvor akzeptierte Wave-Wert gespeichert. Eine volle Queue kann
  ein Kommando verwerfen, erzeugt aber keine optimistisch gespeicherte Wave-Auswahl.

### Motion und Rückkehr zum Basis-Sound

Motion verwendet denselben Audio-Callback wie Queue-Kommandos. Sein Apply-Callback
liefert für Wave den tatsächlich effektiven Wert zurück. Die Lane-Runtime hält
bei Ablehnung diesen Wert und bricht die Ramp ab; ein späterer Tick spielt die
abgewiesene Auswahl nicht nach. Die UI-Motion-Anzeige liest den effektiven Wave-
Wert. Eine beim Recording abgewiesene UI-Auswahl nimmt den akzeptierten Basiswert
auf, nicht die erfolglose Morph-Auswahl. Bereits gespeicherte Lane-Steps bleiben
Anforderungen, deren Zulässigkeit bei der Wiedergabe geprüft wird.

Hat Motion eine bisherige Morph-Stimme bereits auf Saw/Square gestellt und
übernimmt die andere Stimme anschließend Morph, wird der veraltete Morph-
Basiswert der ersten Stimme mit ihrem schon effektiven Saw-/Square-Wert abgeglichen.
Ihr DSP wird dabei nicht umgestellt. Dieser Basisabgleich verhindert eine
verdeckte Rückforderung beim Stoppen oder Freigeben von Motion. Für andere
Parameter bleibt die vorhandene Motion-/Basiswertbehandlung erhalten.

### Laden und Speichern der experimentellen Projekte

Neue Defaults setzen Morph für A und B auf 64. **Es wurde keine historische
Wave-3-Projektmigration eingeführt.** Der frühere Vorschlag, alte ungenutzte
Soundbytes über eine Sound-Revision ausdrücklich auf 64 zu migrieren, wurde
für diese frische experimentelle Implementierung verworfen. Alte Nullbytes
an Index 12 werden somit als Morph 0 gelesen; Saw/Square bleiben davon unberührt.
Die bereits vorhandenen Migrationen anderer Sound-Parameter bleiben erhalten.

`engine_sound_validate_bass()` begrenzt geladene Morph-Werte über 127 auf 127,
setzt ungültige Wave-Werte über 2 auf Saw und löst ein widersprüchliches Paar
bestimmt als **A = Morph, B = Saw** auf. Die Morph-Werte beider Stimmen bleiben
unabhängig erhalten. `project_load()` und `engine_apply_sound()` verwenden diese
Validierung; es entsteht kein zusätzlicher gespeicherter Ownership-Zustand.

Ein ganzer Sound wird mit einem gemeinsamen Wave-Paar-Kommando übernommen.
Das Audio gibt zuerst einen alten Owner frei und setzt dann den neuen, sodass
auch ein vorher aktives B die Ladepriorität von A nicht blockiert. Ein vorübergehender
PERF-Factory-Sound überschreibt die akzeptierten Projekt-Waves nicht; die
Wiederherstellung nutzt den Projekt-Sound. SAVE übernimmt akzeptierte Basis-Waves
und die beiden Morph-Potwerte. Gleiche persistente Größen bedeuten keine
klangliche Rückwärtskompatibilität: alte Firmware kann Wave 3 nicht reproduzieren.

## Implementierte C-Architektur

1. Expliziter Morph-Dispatch bei `Wave = 2`; Saw-/Square-Ausdrücke unverändert.
2. `BASS303_MORPH` an Index 12, `pot[13]`, Default 64 und Zustand pro Stimme.
3. Validierte kubische Vier-Box-Residuals, feste Geometrien und präzise Gains.
4. Gewichte und kombinierte Event-Koeffizienten außerhalb des Oversampled-Loops;
   gemeinsamer Trapez-/Kerbenpfad und nur benötigte Templates an Endpunkten.
5. Finiter 5-ms-Slew auf Base-Rate, Morph-Koeffizienten konstant pro Control-Chunk.
6. TeeBee-Filter, Phase, Envelopes, Idle, Drive und Decimator bleiben erhalten.
7. Standard 2×, bestehender Lite-Guard und bestehende 4×-Verarbeitung unverändert;
   keine erzwungene 1×-Verarbeitung für die neue Familie.
8. Zentrale A/B-Exklusivität im Audio-Parameterpfad plus akzeptierter UI-/Save-Mirror.

Unabhängige Kerbenbreite/-tiefe, Mode-Crossfades, weitere Wave-Familien und ein
zusätzlicher MIDI-CC-Parameterpfad gehören nicht zu dieser Implementierung.

## Vorhandene Benchmark- und Testmethoden

- [`bass303_waveform_test.c`](../tests/host/bass303_waveform_test.c) und
  [`run_bass303_waveform.sh`](../tests/host/run_bass303_waveform.sh): kubische
  Prototyp-/Produktions-Bitidentität, Landmark-/Zwischenwerte, zentrale Kontinuität,
  finite Morph-Bewegung, Instanzisolation und Saw-/Square-Baseline bei OS1/2/4
  und CTRL1/4. Zwei-Morph-DSP-Fälle prüfen den Helper ohne Anwendungsexklusivität.
- [`bass303_ui_test.c`](../tests/host/bass303_ui_test.c) und
  [`run_bass303_ui.sh`](../tests/host/run_bass303_ui.sh): echte UI-Referenzen,
  TONE/VOICE/EXTRA und SEL/MORE, Queue-Akzeptanz/Ablehnung, Freigabe, volle Queue, Motion-Rückmeldung,
  Recording, Save vor Command-Verarbeitung, Round-trip, A-Priorität bei
  widersprüchlichem Load und temporärer PERF-Sound.
- [`bass303_test.c`](../tests/host/bass303_test.c): Open303-Referenzvergleich,
  Envelope-, Spectral-centroid- und Glide-Metriken, Control-rate-Vergleich,
  Instanzisolation, Idle-Verhalten, Alias-Analyse und Active-/Idle-Host-Timing.
- [`run_bass303.sh`](../tests/host/run_bass303.sh): Konfigurationen 1×/2×/4×;
  1×-Referenzvergleich ist dort informational und bestimmt nicht den Exit-Status.
  Benötigt externen `schwung-303`-Checkout. Er fehlt lokal; dieser externe
  Vergleich konnte bei der Integration nicht laufen. Der separate gespeicherte
  lokale Saw-/Square-Baselinevergleich oben ist dagegen bestanden.
- [`bass303_lite_test.c`](../tests/host/bass303_lite_test.c): Pegel, Pitch-Proxy
  über Zero-crossings und Umschalttransienten zwischen Normal/Lite. Ein vorhandenes
  Integrationslog meldete Erfolg für den unveränderten Legacy-Guard-Pfad;
  das ist kein allgemeiner Qualitätstest der neuen Formen.
- [`host/x0x_host.c`](../host/x0x_host.c): Stage-Timing mit `X0X_PROFILE=1` beim
  Simulatorlauf. Kernel Instruction Counts sind für macOS implementiert, nicht
  für den betrachteten Linux-Host. Host-Nanosekunden sind keine FM-1-Zyklen.
- [`ui.c`](../firmware/src/app/ui.c): PERF zeigt Stage Shares, Load, Peaks,
  Dropouts und optionale Counter. PERF TEST nutzt Factory/All-five/Dense-Patterns,
  setzt Factory-Sound und deaktiviert Motion sowie Guard. Für die neue Wave-Familie
  benötigt es daher eine gezielte Benchmark-Erweiterung.
- [`perf.x0x`](../tests/scenarios/perf.x0x): prüft insbesondere Testablauf und
  Wiederherstellung des Projekts; erfolgreiche Simulatorwerte sind kein
  Hardware-Headroom-Nachweis.
- [`2026-10-04-performance.md`](plans/2026-10-04-performance.md): historische
  Host-Profile und Optimierungsanalysen. Darin genannte ältere RAM-/Guard-
  Zustände dürfen nicht als aktuelle Ressourcen- oder Hardwaregarantie gelten.

## Bereits vorliegende Ergebnisse und verbleibende Grenzen

Die Logs der abgeschlossenen Integration liegen lokal unter
`build/host/waveform-ui/`. `dsp.log` meldet PASS für OS1/2/4 jeweils bei CTRL1/4:
Saw/Square gegenüber der gepinnten Baseline bitidentisch, kubischer Evaluator
gegenüber dem Host-Prototyp bitidentisch, Morph-Landmarks/Zwischenwerte,
Kontinuität, endliche Zielbewegung und Instanzisolation bestanden. `test.log`
meldet PASS für UI, Command-/Motion-Exklusivität, Recording und Projekt-Persistence.
Der vorhandene Motion-Test und der Legacy-Lite-Test meldeten ebenfalls Erfolg.
Firmware und Browser-Emulator wurden gebaut; Ressourcen stehen im Speicherabschnitt.
Diese Ergebnisse sind Host-/Build-Befunde, keine Messungen des FM1.

Die vorhandenen Berichte `build/host/waveform-prototype-report.md` und
`build/host/waveform-prototype-audio-gate.md` enthalten die vor der Integration
abgeschlossene Audio-Untersuchung. Ihr experimentelles GO ist keine Release-
Freigabe. Bei 2× bleiben unter anderem Open-filter-/Envelope-Stressfälle,
Extremtonhöhen und die unveränderte RAT-Stufe außerhalb einzelner Qualitätsziele.
Die dort gemessenen gepaarten Full-voice-Fehler enthalten neben möglichem Aliasing
auch andere Signal-/Referenzabweichungen; sie sind keine isolierten Aliaspegel.
Die bestehende OS4-Decimator-Auswertungsreihenfolge wurde nicht korrigiert.

Hardware-Timing, Deadline-Marge im dichten Mix, Guard-Häufigkeit, Stack-Highwater,
Geräte-Hörtests und langfristige Stabilität stehen aus. Weder Exklusivität noch
Build-Erfolg lösen alle Audioqualitätsgrenzen. In dieser Dokumentationsphase
wurden keine neuen Tests, Spektralanalysen, WAVs oder Builds erzeugt.

## Ursprüngliche Host-Prototyp-Testmatrix

**Historische Prüfplanung:** Die Tabelle bewahrt die ursprünglichen Kriterien mit aktualisiertem Storage/UI-Eintrag;
sie ist keine Liste pauschal bestandener Tests. Einzelne Aspekte sind inzwischen
mit den oben genannten fokussierten Regressionen geprüft, andere bleiben offene
Qualitäts-/Hardware-Gates. Die ursprüngliche Storage-Migrationsforderung ist
für die frische experimentelle Umsetzung durch die oben dokumentierte einfache
Load-Validierung ersetzt. Double-Präzision ist für Host-Referenz-Mathematik
zulässig, nicht für die Firmware-DSP-Arithmetik.

| Test | Matrix | Vorgeschlagenes Pass/fail-Kriterium |
|---|---|---|
| Mathematischer Evaluator | Alle Landmarks und unmittelbar um jedes Event | Float-Fehler gegen Double-Gleichungen ≤ \(5\times10^{-6}\); Jumps/Slopes stimmen |
| DC | Alle Integer-Morph-Werte; \(d=0.001,0.01,0.05,0.15,0.31\) | Betrag des phasenintegrierten Mittelwerts ≤ \(10^{-5}\), auch bei Support-Überlappung |
| Idealer RMS | Alle 128 Morph-Werte; Corrections nur für diesen Mathematiktest deaktiviert | Abweichung von \(1/\sqrt3\) ≤ 0.001 dB |
| Exakte Endpunkte | Morph 0, 64, 127 | Bit-identisch zum direkten Endpoint-Evaluator im selben Build |
| Zentrale Kontinuität | Kontinuierliche Positionen \(64\pm10^{-3}\) | Maximale Phase-domain-Differenz zwischen beiden Seiten ≤ \(10^{-6}\) |
| Legacy-Erhalt | Saw/Square; Noten, Slides, Accents, Drive, Idle; OS 1/2/4; Lite-Wechsel | Keine unterschiedlichen Output-Float-Bits gegenüber Baseline bei identischen Kommandos |
| Instanzisolation | A/B mit verschiedenen Noten und Morph-Automation | Interleaved Rendering bit-identisch zum jeweiligen isolierten Rendering |
| Blockbehandlung | 1, 4, 64, 128, 256 Samples | Stationärer Oszillator unabhängig von Partitionierung; Automation innerhalb eines Control-Chunks |
| Automation | 0↔127, 63↔65, Target-Steps und Sweeps | Kein NaN/Inf; monotone Smoothing-Position; volles Ziel in 5 ms plus einem Chunk |
| Rate-Wechsel | 1↔2 und 1↔4 bei gehaltenen Noten und Morphing | Kein Phase-Reset; endlicher Output; Transition-Excursion ≤ 1.5× größter stationärer Adjacent-sample-Differenz beider Raten |
| Spektralqualität | Noten 0,24,36,48,60,72,84,96,108,120,127; Tune-Extreme; Morph 0,32,63,64,65,96,127 | Vorläufige Alias-Ziele unten erfüllen |
| Vollständige Stimme | Cutoff-/Resonanz-Extreme, Accents, Slides; Off/Soft/RAT | Endlicher Output, keine neue Instabilität; Hörprüfung akzeptiert |
| Storage/UI (aktualisiert) | Neue Defaults, Round-trip, korrupter Morph, A/B-Konflikt | Persistente Größen unverändert; Default 64; unabhängige Werte, Begrenzung und A-Priorität beim Load |

Spektraltests benötigen eine Fourier-basierte bandbegrenzte Referenz dieser
exakten Formen. Neben musikalischen Frequenzen auch non-coherent Frequenzen
verwenden, damit Aliases nicht auf harmonischen Bins verborgen bleiben. Alias-
Leistung und erwartete harmonische Dämpfung durch die Korrektur getrennt bewerten.

### Vorläufige Spectral-quality-Ziele

Die ursprüngliche Planung schlug für den isolierten korrigierten Oszillator
folgende Alias-to-signal-Obergrenzen vor. Die vorhandenen Host-Berichte verwenden
diese unveränderten Schwellen auch für ihre gekennzeichneten Vergleichsmetriken;
das ist kein Nachweis, dass alle Formen und Betriebspunkte sie erfüllen:

| Pitch-Bereich | 1× | 2×/4× nach passender Decimation |
|---|---:|---:|
| Noten ≤60 | −40 dB | −50 dB |
| Noten 61–84 | −30 dB | −40 dB |
| Noten 85–127 | −15 dB | −25 dB |

Diese Grenzen sind keine Vorhersage erfolgreicher Tests. Bei Verfehlung sind
Geometrie oder Correction Order zu überdenken; Ziele nicht stillschweigend lockern.
Für die bereits vorliegenden Messungen sind Analyseablauf, Referenzfilter und
Auswertefenster in den Host-Berichten festgehalten. Die Metrik eines isolierten
Oszillators darf nicht mit dem gepaarten Fehler einer vollständigen Stimme
gleichgesetzt werden. Diese Dokumentationsphase wiederholt keine Messungen.

## Hardware-Gates und Echtzeitvalidierung

**Quellcodebefund:** Die Firmware rendert 512-Frame-DMA-Hälften, intern in
256-Frame-Aufrufen. Die Deadline beträgt ungefähr 11.6 ms bei 44.1 kHz.
Der Guard aktiviert sich bei einem Block über 92 % oder gemitteltem Load über
85 %; nach drei Sekunden unter 65 % kann er zurückkehren.

Mehr CPU-Last kann den Guard öfter aktivieren. Dadurch kann selbst eine
bit-identische Saw-/Square-Stimme im Mix anders klingen, weil sie dann mit
reduziertem Oversampling läuft. Dieser indirekte Einfluss ist Teil der
Legacy-Erhalt-Anforderung.

**Vorgeschlagenes Release-Gate:** keine Deadline Misses über zehn Minuten eines
dichten Workloads mit einer Morph- und einer Saw-/Square-Stimme, Automation,
Drums, BREAK, FX, USB-Audio und UI-Aktivität. Beide möglichen Morph-Owner
(A und B) und Übergaben sind zu prüfen. Mean, Peak und Guard Engagement separat
berichten. Eine guard-geschützte Messung alleine belegt keine ausreichende Full-quality-Reserve;
Full-quality-Kosten und reguläres Guard-Verhalten getrennt erfassen.

Weitere ausstehende Hardware-Prüfungen:

- Alle drei Endpoint-Shapes und kostspielige Zwischenwerte, verschiedene Pitches.
- 1×-Lite-Verhalten und Rückkehr zu Normal, auch während Morphing und Slides.
- UI still gegenüber animiert/Encoder-Aktivität; Flash-/Instruction-cache-Effekte.
- Stack-Highwater und reale Kapazität auf dem Gerät; Build-Ressourcen und
  Soft-double-Prüfung sind oben bereits berichtet und bei späteren Codeänderungen neu zu prüfen.
- Counter-Verfügbarkeit überprüfen: `fm1_perf.h` beschreibt CPU-Counter mit
  Timer-Fallback und noch nicht hardwareverifizierte Stall-Counter-Annahmen.
- Langfristige Dropout-/Peak-Statistik; nicht allein Durchschnittsload auswerten.

Host-Timing, historische macOS-Profile und Emulatorerfolg sind keine Nachweise
der Echtzeitfähigkeit auf dem FM-1. Nach späteren DSP-/UI-Änderungen sind außerdem
Firmware und Browser-Emulator zu bauen; in dieser Dokumentationsphase nicht.

## Implementierungsstand und nächste Validierung

Geometrien, Level-Policy, Default 64, TONE/VOICE/EXTRA-Navigation, finiter 5-ms-Slew,
kubischer Kern und zentrale Ein-Morph-Stimmen-Regel sind umgesetzt. Die
ursprüngliche Geometrie wurde beibehalten; der Dreieckskern wurde ausdrücklich
ersetzt. Historische Wave-3-Storage-Migrationen wurden bewusst nicht eingeführt.
Baseline-/Prototypidentität und fokussierte UI-/Persistence-Prüfungen liegen vor;
Firmware- und Emulator-Builds sind abgeschlossen.

Noch ausstehend sind die Geräteprüfungen des vorigen Abschnitts und die
Bewertung der bekannten Audioqualitätsgrenzen. Erst deren Ergebnisse erlauben
eine Aussage über Release-Eignung und Hardware-Headroom. Die externe Open303-
Referenz bleibt für Legacy-Qualität relevant, nicht als Referenz der neuen
Geometrien; ihr fehlender lokaler Checkout bleibt eine ausgewiesene Testlücke.

## Risiken, Alternativen und offene Entscheidungen

Die konkreten Formen, breite feste Kerbe, ideale RMS-Level-Policy und
Morph-Gain-Kompensation sind für dieses Experiment festgelegt. Die
A/B-Exklusivität ist eine verbindliche Architekturentscheidung und kein
noch offener CPU-Trade-off. Sie ersetzt keine Messung der Echtzeitreserve und
keine Prüfung der verbleibenden Sonic-Gates, insbesondere bei RAT und
Extremtonhöhen. Automatisierte Morph-Verläufe erzeugen weiterhin mögliche
Sidebands; endlicher Slew garantiert keine bandbegrenzte Modulation.

Die erste Planung schätzte weniger Pool-Zuwachs als die schließlich
implementierten 24 B. Der tatsächliche Build erfüllt die bestehenden
Kapazitätsprüfungen ohne Opfer an anderen Funktionen. Stackbedarf und
CPU-Ausführung auf dem FM1 bleiben trotzdem offen.

Weitere Alternativen wären eine kontinuierliche Sharkfin ohne Value-Reset,
eine approximierte Exponentialkurve, andere Kerbengeometrien oder zusätzliche
Korrekturverfahren. Sie sind nicht Bestandteil der aktuellen Umsetzung und
würden Klang beziehungsweise Rechenkosten verändern. Große Wavetables,
Audio-following-RMS-Normalisierung, neue breite Puffer und duplizierte
komplette Render-Pfade gehören weiterhin nicht zur Minimalarchitektur.

### Warum Wavehole Triangle zurückgestellt wird

Die erste Phase stellte die prinzipielle Machbarkeit fest: Eine Dreiecksbasis ist
günstig, eine Vertiefung oder ein fehlender Abschnitt erzeugt jedoch weitere
Corners und gegebenenfalls Value-Jumps. „Wavehole“ ist im Projekt nicht exakt
definiert. Breite, Position, Tiefe, Polarität und Kontinuität wären neu festzulegen.

Zusätzliche Correction Events, Quality-Tests und Messkombinationen würden die
auf dem FM1 noch ungeprüfte dritte Wave-Familie erweitern, bevor deren Headroom bekannt ist.
Deshalb ist Wavehole Triangle **außerhalb der aktuellen mathematischen
Spezifikation** und bleibt aufgeschoben, bis Klangqualität und Echtzeitreserve
der drei festgelegten Formen nachgewiesen sind. Es ist keine geplante vierte
Auswahl in dieser Minimalimplementierung.

## Quellen und relevante Dateien

### Repository

- [`AGENTS.md`](../AGENTS.md): Architektur-, Ressourcen-, Storage- und Workflow-Vorgaben.
- [`bass303.c`](../firmware/src/dsp/bass303.c): Parameter, BLEP, Oscillator, Filter und Rendering.
- [`bass303.h`](../firmware/src/dsp/bass303.h): Instanzzustand, OS/Control-Rate und Parameterindizes.
- [`fastmath.h`](../firmware/src/dsp/fastmath.h): Single-precision-Mathematik, insbesondere `fm_rsqrtf()`.
- [`x0x_param.h`](../firmware/src/dsp/x0x_param.h): Pot-Descriptors und UI-Namenskonventionen.
- [`engine.h`](../firmware/src/app/engine.h) und [`engine.c`](../firmware/src/app/engine.c): `sound_t`, zwei Stimmen, Command Queue, Motion und Guard.
- [`x0x.h`](../firmware/src/app/x0x.h), [`project.c`](../firmware/src/app/project.c) und [`storage.c`](../firmware/src/storage.c): persistente Daten und Revision/Migration.
- [`motion.h`](../firmware/src/seq/motion.h) und [`motion.c`](../firmware/src/seq/motion.c): Parameter-Automation.
- [`ui.c`](../firmware/src/app/ui.c): TONE/VOICE/EXTRA und SEL/MORE, Page-Kapazitäten und PERF TEST.
- [`main_fm1.c`](../firmware/src/app/main_fm1.c): DMA-Deadline, Render-Blöcke und Load-/Dropout-Messung.
- [`fm1_perf.h`](../firmware/hal/fm1_perf.h): Performance-Counter und deren Einschränkungen.
- [`tools/build.py`](../tools/build.py): Image-/RAM-/Pool-Gates und Soft-double-Prüfung.
- [`host/build_host.sh`](../host/build_host.sh), [`host/x0x_host.c`](../host/x0x_host.c): Simulator und Profiling.
- [`tests/run_tests.sh`](../tests/run_tests.sh), [`bass303_test.c`](../tests/host/bass303_test.c), [`run_bass303.sh`](../tests/host/run_bass303.sh), [`bass303_lite_test.c`](../tests/host/bass303_lite_test.c): vorhandene Prüfmethoden.
- [`perf.x0x`](../tests/scenarios/perf.x0x), [`2026-10-04-performance.md`](plans/2026-10-04-performance.md): Szenario und historische Performance-Analyse.
- [`BUILDING.md`](../BUILDING.md), [`MANUAL.md`](MANUAL.md), [`LICENSING.md`](../LICENSING.md): Build, Bedienung und Attribution.

### Externe mathematische und Engine-Quellen

- Fabián Esqueda, Vesa Välimäki, Stefan Bilbao: **Rounding Corners with BLAMP**,
  DAFx-16 (2016), [Originalpaper](https://www.dafx.de/paper-archive/2016/dafxpapers/18-DAFx-16_paper_33-PN.pdf).
  Grundlage zur Behandlung von Slope-Discontinuities mit BLAMP. Die konkrete
  Two-point-Funktion, Event-Koeffizienten, DC-Terme, Geometrien und Morph-
  Korrelationen dieses Dokuments sind eigene mathematische Ableitungen aus
  der angegebenen bestehenden BLEP und den definierten Formen.
- [schwung-303](https://github.com/charlesvestal/schwung-303): Referenz-Engine mit
  Open303 von Robin Schmidt; bestehende Port-Kommentare nennen insbesondere
  `rosic_Open303`, `rosic_TeeBeeFilter`, `rosic_MipMappedWaveTable` und Drive-Quellen.
  Die neuen Formen sind keine behauptete Nachbildung ihrer Wavetables.

Die Implementierung bewahrt bestehende GPL-3.0-only- und erforderliche
MIT-/Drive-Attribution. Dieses Dokument beschreibt den experimentellen Stand;
es erteilt keine Hardware-/Release-Freigabe und keine Freigabe zum Commit oder Push.
