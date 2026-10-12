# FM1-X0X-RMX – TB-3PO Skalen-Nachschlagewerk

TB-3PO erzeugt für 303A und 303B zufallsbasierte Notenlinien aus einem gewählten
Grundton (ROOT), einer Skala, einer Oktavlage und einem Seed. DENS, ACCENT, SLIDE
und OCTS gestalten Dichte, Artikulation und Tonumfang. **OCT+** erzeugt eine neue
Linie, **OCT-** mutiert ungefähr ein Viertel der Steps. Auch die automatische
Mutation verwendet die gewählte Skala. Die Auswahl ändert eine bereits geschriebene
Linie erst beim Generieren oder Mutieren; sie quantisiert handbearbeitete Noten nicht nachträglich.
Mutation kann vorhandene Noten behalten. Deshalb macht sie eine alte oder von Hand
bearbeitete Linie nicht zwangsläufig vollständig skalenrein.

Die Firmware bietet genau **22 Skalen: vier pentatonische und 18 siebentönige**.
Neue TB-3PO-Konfigurationen starten mit **Minor Pentatonic (`MPEN`), Index 0**.
Diese Standardwahl ändert keine bereits gespeicherte Skalenwahl. Intervalle sind ganzzahlige
Halbtonabstände vom Grundton in zwölfstufiger gleichschwebender Stimmung. Eine
Oktave wird bei der Anzahl der unterschiedlichen Töne nicht zusätzlich gezählt.
Die Tabellen stehen als `const`-Daten in der Firmware; das persistente Projektformat
bekommt weder neue Felder noch eine neue Struktur.

Alle Aussagen zu Charakter, Gefühlen und Einsatz sind **subjektive musikalische
Beobachtungen und Vorschläge**. Rhythmus, Artikulation, Klangfarbe, Register und
Begleitharmonie können den Eindruck stark verändern. Stilnamen sind keine Grenzen.
Die Bezeichnungen Egyptian, Arabic und die Thaat-Namen benennen hier konkrete
Tonvorräte. Sie behaupten keine vollständige Abbildung einer Musikkultur oder Raga-Praxis.

## Übersicht in Firmware-Reihenfolge

Die Indizes beginnen bei 0; angezeigt werden die Kürzel, nicht die Zahlen.

| Index | Vollständiger Skalenname | Display | Töne | Halbtonabstände |
|---|---|---|---|---|
| 0 | Minor Pentatonic | MPEN | 5 | 0 3 5 7 10 |
| 1 | Major Pentatonic | PMAJ | 5 | 0 2 4 7 9 |
| 2 | Egyptian Pentatonic | EGYP | 5 | 0 2 5 7 10 |
| 3 | Hirajoshi | HIRA | 5 | 0 2 3 7 8 |
| 4 | Natural Minor | MIN | 7 | 0 2 3 5 7 8 10 |
| 5 | Major | MAJ | 7 | 0 2 4 5 7 9 11 |
| 6 | Dorian | DOR | 7 | 0 2 3 5 7 9 10 |
| 7 | Phrygian | PHRY | 7 | 0 1 3 5 7 8 10 |
| 8 | Lydian | LYD | 7 | 0 2 4 6 7 9 11 |
| 9 | Mixolydian | MIXO | 7 | 0 2 4 5 7 9 10 |
| 10 | Locrian | LOCR | 7 | 0 1 3 5 6 8 10 |
| 11 | Harmonic Minor | HARM | 7 | 0 2 3 5 7 8 11 |
| 12 | Melodic Minor | MELM | 7 | 0 2 3 5 7 9 11 |
| 13 | Harmonic Major | HMAJ | 7 | 0 2 4 5 7 8 11 |
| 14 | Altered | ALTR | 7 | 0 1 3 4 6 8 10 |
| 15 | Hungarian Minor | GYPS | 7 | 0 2 3 6 7 8 11 |
| 16 | Romanian Minor | ROMN | 7 | 0 2 3 6 7 9 10 |
| 17 | Bhairav Thaat | BHAI | 7 | 0 1 4 5 7 8 11 |
| 18 | Marva Thaat | MARV | 7 | 0 1 4 6 7 9 11 |
| 19 | Purvi Thaat | PURV | 7 | 0 1 4 6 7 8 11 |
| 20 | Todi Thaat | TODI | 7 | 0 1 3 6 7 8 11 |
| 21 | Arabic | ARAB | 7 | 0 2 4 5 6 8 10 |

## Pentatonische Skalen

Fünf Töne lassen mehr Lücken zwischen den möglichen Noten. Das kann kurze Motive
übersichtlich halten und lässt im Mix Platz für Drums, Delay und Begleitakkorde.
Pentatonik ist jedoch nicht immer halbtonfrei: Hirajoshi enthält zwei Halbtonpaare.

### 0. Minor Pentatonic (MPEN)

- Vollständiger Name: **Minor Pentatonic**
- Display: **`MPEN`**
- Anzahl der Töne: **5**
- Halbtonabstände vom Grundton: **0 3 5 7 10**
- Töne bei ROOT C (deutsche Namen): **C Es F G B**

**Charakteristische Intervalle:** Kleine Terz (3), reine Quarte (5), reine Quinte (7) und kleine Septime (10); keine Halbtonschritte innerhalb der zyklischen Skala.

**Musikalischer Eindruck:** Kann direkt, rau und bodenständig wirken. Die reduzierte Auswahl lässt Rhythmus, Akzente und Filterbewegung deutlich hervortreten.

**Einsatzideen:** Acid: kurze Bassmotive mit Slides. Dub und Reggae: sparsame Bassantworten auf Grundton und Quinte. EBM: knappe, wiederholte Riffs.

### 1. Major Pentatonic (PMAJ)

- Vollständiger Name: **Major Pentatonic**
- Display: **`PMAJ`**
- Anzahl der Töne: **5**
- Halbtonabstände vom Grundton: **0 2 4 7 9**
- Töne bei ROOT C (deutsche Namen): **C D E G A**

**Charakteristische Intervalle:** Große Terz (4), reine Quinte (7) und große Sexte (9); Quarte und Septime fehlen.

**Musikalischer Eindruck:** Kann offen, freundlich und klar wirken; weite Sprünge bleiben häufig gut singbar.

**Einsatzideen:** Reggae und Dub: helle Antwortmotive über passenden Durharmonien. Elektronischer Pop: klare Arpeggien. Acid: melodische, weniger dichte Linien.

### 2. Egyptian Pentatonic (EGYP)

- Vollständiger Name: **Egyptian Pentatonic**
- Display: **`EGYP`**
- Anzahl der Töne: **5**
- Halbtonabstände vom Grundton: **0 2 5 7 10**
- Töne bei ROOT C (deutsche Namen): **C D F G B**

**Charakteristische Intervalle:** Große Sekunde (2), Quarte (5), Quinte (7) und kleine Septime (10); keine Terz, dadurch keine eindeutige Dur-/Moll-Terz.

**Musikalischer Eindruck:** Kann schwebend, offen und harmonisch mehrdeutig wirken, weil eine Terz fehlt.

**Einsatzideen:** Dub: offene Ostinati über Grundton-/Quintflächen. Acid und elektronische Musik: Motive mit Quarten und kleinen Septimen; bei Reggae die Akkordterz separat prüfen.

### 3. Hirajoshi (HIRA)

- Vollständiger Name: **Hirajoshi**
- Display: **`HIRA`**
- Anzahl der Töne: **5**
- Halbtonabstände vom Grundton: **0 2 3 7 8**
- Töne bei ROOT C (deutsche Namen): **C D Es G As**

**Charakteristische Intervalle:** Kleine Terz (3), Quinte (7) und kleine Sexte (8); Halbtonschritte D–Es und G–As, große Lücke Es–G.

**Musikalischer Eindruck:** Kann sparsam, fragil und gespannt wirken. Die beiden engen Halbtonpaare kontrastieren mit großen Lücken.

**Einsatzideen:** Elektronische Musik: reduzierte Leads und weite Sprünge. Acid und EBM: markante Halbtonmotive; Dub: einzelne, räumlich verzögerte Antworten.

## Siebentönige Skalen

Sieben Töne erlauben mehr benachbarte Schritte und harmonische Farben. Die größere
Auswahl ändert die möglichen Noten, nicht den Zufallsalgorithmus oder die
Dichte-/Akzent-/Slide-Einstellungen. Für klare Bassmotive kann auch hier eine
sparsame Dichte sinnvoll sein.

### 4. Natural Minor (MIN)

- Vollständiger Name: **Natural Minor**
- Display: **`MIN`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 5 7 8 10**
- Töne bei ROOT C (deutsche Namen): **C D Es F G As B**

**Charakteristische Intervalle:** Kleine Terz (3), kleine Sexte (8) und kleine Septime (10); die natürliche Mollform ohne erhöhten Leitton.

**Musikalischer Eindruck:** Kann dunkel, vertraut und nachdenklich wirken; die kleine Septime lässt weniger Leittondruck als harmonisches Moll entstehen.

**Einsatzideen:** Acid und EBM: Mollbass und wiederkehrende Sequenzen. Dub und Reggae: Basslinien über passenden Mollharmonien. Elektronische Musik: melancholische Arpeggien.

### 5. Major (MAJ)

- Vollständiger Name: **Major**
- Display: **`MAJ`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 4 5 7 9 11**
- Töne bei ROOT C (deutsche Namen): **C D E F G A H**

**Charakteristische Intervalle:** Große Terz (4), große Sexte (9) und große Septime (11); Halbtonschritte E–F und H–C.

**Musikalischer Eindruck:** Kann hell, stabil und zielgerichtet wirken; der Leitton H kann eine Rückkehr nach C nahelegen.

**Einsatzideen:** Reggae und Dub: Bass-/Leadmotive über Durharmonien. Elektronische Musik: helle Arpeggien. Acid: melodische Kontraste zu rauem Sound.

### 6. Dorian (DOR)

- Vollständiger Name: **Dorian**
- Display: **`DOR`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 5 7 9 10**
- Töne bei ROOT C (deutsche Namen): **C D Es F G A B**

**Charakteristische Intervalle:** Kleine Terz (3) mit großer Sexte (9) und kleiner Septime (10); die große Sexte unterscheidet sie von natürlichem Moll.

**Musikalischer Eindruck:** Kann zugleich mollgefärbt und beweglich wirken; die große Sexte kann einen helleren Akzent setzen.

**Einsatzideen:** Dub und Reggae: modale Mollgrooves mit großer Sexte. Acid: bewegliche Basslinien. EBM und elektronische Musik: Mollriffs mit einem helleren A als Kontrast.

### 7. Phrygian (PHRY)

- Vollständiger Name: **Phrygian**
- Display: **`PHRY`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 3 5 7 8 10**
- Töne bei ROOT C (deutsche Namen): **C Des Es F G As B**

**Charakteristische Intervalle:** Kleine Sekunde (1), kleine Terz (3), reine Quinte (7); der Halbton über dem Grundton prägt die Skala.

**Musikalischer Eindruck:** Kann dunkel, eng und drängend wirken, besonders wenn C und Des unmittelbar wechseln.

**Einsatzideen:** EBM und Acid: kurze C–Des-Motive und drängende Sequenzen. Dub: dunkle, isolierte Antworten. Bei Reggae die kleine Sekunde gezielt gegen die Harmonie prüfen.

### 8. Lydian (LYD)

- Vollständiger Name: **Lydian**
- Display: **`LYD`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 4 6 7 9 11**
- Töne bei ROOT C (deutsche Namen): **C D E Fis G A H**

**Charakteristische Intervalle:** Große Terz (4), übermäßige Quarte (6) und große Septime (11); die erhöhte Quarte unterscheidet sie von Dur.

**Musikalischer Eindruck:** Kann weit, schwebend und leicht entrückt wirken; Fis kann über einem C-Grundton eine besondere Spannung erzeugen.

**Einsatzideen:** Elektronische Musik: schwebende Leads und Flächenmotive. Dub: sparsame Fis-Antworten über passender Harmonie. Acid: bewusste Tritonusspannung.

### 9. Mixolydian (MIXO)

- Vollständiger Name: **Mixolydian**
- Display: **`MIXO`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 4 5 7 9 10**
- Töne bei ROOT C (deutsche Namen): **C D E F G A B**

**Charakteristische Intervalle:** Große Terz (4), reine Quinte (7) und kleine Septime (10); Dur mit erniedrigter Septime.

**Musikalischer Eindruck:** Kann gelassen, erdig und groovebetont wirken; die kleine Septime kann den strengen Dur-Leittondruck abschwächen.

**Einsatzideen:** Reggae und Dub: Dur-Grooves mit kleiner Septime. Acid: rollende Bassmotive. Elektronische Musik: modale Ostinati statt starker Leittonauflösung.

### 10. Locrian (LOCR)

- Vollständiger Name: **Locrian**
- Display: **`LOCR`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 3 5 6 8 10**
- Töne bei ROOT C (deutsche Namen): **C Des Es F Ges As B**

**Charakteristische Intervalle:** Kleine Sekunde (1), kleine Terz (3) und verminderte Quinte (6); eine reine Quinte fehlt.

**Musikalischer Eindruck:** Kann instabil und kantig wirken; Grundton und verminderte Quinte können eine feste tonale Verankerung erschweren.

**Einsatzideen:** EBM und experimenteller Acid: instabile, kantige Riffs. Elektronische Musik: Spannungsabschnitte. In Dub eher gezielte Effektnoten als unbesehen über Standardakkorde.

### 11. Harmonic Minor (HARM)

- Vollständiger Name: **Harmonic Minor**
- Display: **`HARM`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 5 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C D Es F G As H**

**Charakteristische Intervalle:** Kleine Terz (3), kleine Sexte (8), große Septime (11); übermäßige Sekunde As–H mit drei Halbtönen.

**Musikalischer Eindruck:** Kann dramatisch und zugespitzt wirken; die Lücke As–H kann markante melodische Sprünge erzeugen.

**Einsatzideen:** Acid und EBM: dramatische Bass-/Leadlinien mit H als Leitton. Dub: markante Delayantworten. Elektronische Musik: gerichtete Mollmelodien.

### 12. Melodic Minor (MELM)

- Vollständiger Name: **Melodic Minor**
- Display: **`MELM`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 5 7 9 11**
- Töne bei ROOT C (deutsche Namen): **C D Es F G A H**

**Charakteristische Intervalle:** Kleine Terz (3), große Sexte (9) und große Septime (11); hier gilt die aufsteigende Form in beiden Bewegungsrichtungen.

**Musikalischer Eindruck:** Kann elegant, beweglich und leicht gespannt wirken; die Mollterz bleibt, während Sexte und Septime heller ausfallen.

**Einsatzideen:** Elektronische Musik: komplexere Mollleads. Acid: flüssige Linien ohne die As–H-Lücke des harmonischen Moll. Dub: gezielte große Sexten und Septimen.

### 13. Harmonic Major (HMAJ)

- Vollständiger Name: **Harmonic Major**
- Display: **`HMAJ`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 4 5 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C D E F G As H**

**Charakteristische Intervalle:** Große Terz (4), kleine Sexte (8) und große Septime (11); übermäßige Sekunde As–H.

**Musikalischer Eindruck:** Kann vertraut und zugleich überraschend wirken; die kleine Sexte verdunkelt die Durterz.

**Einsatzideen:** Elektronische Musik: Durmotive mit unerwartetem As. Acid und EBM: kontrastreiche Sequenzen. Dub: einzelne kleine Sexten als harmonische Farbe.

### 14. Altered (ALTR)

- Vollständiger Name: **Altered**
- Display: **`ALTR`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 3 4 6 8 10**
- Töne bei ROOT C (deutsche Namen): **C Des Es E Ges As B**

**Charakteristische Intervalle:** Kleine Sekunde (1), kleine Terz (3), enharmonisch E (4), verminderte Quinte (6), kleine Sexte (8) und kleine Septime (10); die alterierte Dominantdeutung liest Es als übermäßige Sekunde und E als verminderte Quarte (Fes).

**Musikalischer Eindruck:** Kann dicht, unruhig und stark spannungsreich wirken; eine harmonische Auflösung kann diese Wirkung verändern.

**Einsatzideen:** Experimenteller Acid und EBM: dichte Reibungen und Spannungsriffs. Elektronische Musik: Übergänge über passende Dominantharmonik. Dub: einzelne alterierte Antworten statt dauerhafter Verdichtung.

### 15. Hungarian Minor (GYPS)

- Vollständiger Name: **Hungarian Minor**
- Display: **`GYPS`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 6 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C D Es Fis G As H**

**Charakteristische Intervalle:** Kleine Terz (3), übermäßige Quarte (6), kleine Sexte (8) und große Septime (11); übermäßige Sekunden Es–Fis und As–H.

**Musikalischer Eindruck:** Kann scharf konturiert, dramatisch und ornamental wirken; die zwei großen Lücken erlauben markante Wendungen.

**Einsatzideen:** Acid und EBM: markante Es–Fis- und As–H-Sprünge. Elektronische Musik: ornamentale Leads. Dub: kurze, weit auseinanderliegende Verzögerungsantworten.

### 16. Romanian Minor (ROMN)

- Vollständiger Name: **Romanian Minor**
- Display: **`ROMN`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 3 6 7 9 10**
- Töne bei ROOT C (deutsche Namen): **C D Es Fis G A B**

**Charakteristische Intervalle:** Kleine Terz (3), übermäßige Quarte (6), große Sexte (9) und kleine Septime (10); Dorian mit erhöhter Quarte.

**Musikalischer Eindruck:** Kann beweglich, rau und eigenwillig wirken; Fis setzt einen markanten Kontrast zur Mollterz.

**Einsatzideen:** Acid: bewegliche Mollsequenzen mit Fis. EBM: eigenwillige Ostinati. Dub und elektronische Musik: modale Antworten über einem stabilen Grundton.

### 17. Bhairav Thaat (BHAI)

- Vollständiger Name: **Bhairav Thaat**
- Display: **`BHAI`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 4 5 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C Des E F G As H**

**Charakteristische Intervalle:** Kleine Sekunde (1), große Terz (4), kleine Sexte (8), große Septime (11); übermäßige Sekunden Des–E und As–H.

**Musikalischer Eindruck:** Kann ernst, feierlich und stark gespannt wirken; die Halbtonnähe zum Grundton trifft auf die große Terz.

**Einsatzideen:** Elektronische Musik: langsame, kontrastreiche Motive. Acid und EBM: Des–E-Sprünge. Dub: sparsame Leittonantworten; keine automatische Reggae-Akkordverträglichkeit voraussetzen.

### 18. Marva Thaat (MARV)

- Vollständiger Name: **Marva Thaat**
- Display: **`MARV`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 4 6 7 9 11**
- Töne bei ROOT C (deutsche Namen): **C Des E Fis G A H**

**Charakteristische Intervalle:** Kleine Sekunde (1), große Terz (4), übermäßige Quarte (6), große Sexte (9), große Septime (11); die reine Quinte (7) bleibt enthalten.

**Musikalischer Eindruck:** Kann hell gespannt und schwebend wirken; kleine Sekunde und erhöhte Quarte erzeugen auffällige Reibungen.

**Einsatzideen:** Elektronische Musik: schwebende, gespannte Leads. Acid: C–Des und E–Fis als Motivkerne. EBM: helle Reibungen über einem einfachen Bassfundament.

### 19. Purvi Thaat (PURV)

- Vollständiger Name: **Purvi Thaat**
- Display: **`PURV`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 4 6 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C Des E Fis G As H**

**Charakteristische Intervalle:** Kleine Sekunde (1), große Terz (4), übermäßige Quarte (6), kleine Sexte (8), große Septime (11); gegenüber Marva ist die Sexte erniedrigt.

**Musikalischer Eindruck:** Kann dunkler und dramatischer als Marva wirken; die kleine Sexte verstärkt den Kontrast zum Leitton.

**Einsatzideen:** Acid und EBM: dramatische Riffs mit Des, Fis und As. Elektronische Musik: Spannungsbögen. Dub: isolierte Motive mit viel Raum.

### 20. Todi Thaat (TODI)

- Vollständiger Name: **Todi Thaat**
- Display: **`TODI`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 1 3 6 7 8 11**
- Töne bei ROOT C (deutsche Namen): **C Des Es Fis G As H**

**Charakteristische Intervalle:** Kleine Sekunde (1), kleine Terz (3), übermäßige Quarte (6), kleine Sexte (8), große Septime (11); gegenüber Purvi ist die Terz erniedrigt.

**Musikalischer Eindruck:** Kann fragil, klagend und intensiv wirken; die Mollterz und erhöhte Quarte erzeugen eigenwillige Konturen.

**Einsatzideen:** Elektronische Musik: fragile, langsame Leads. Acid und EBM: enge C–Des-Bewegungen und Es–Fis-Sprünge. Dub: zurückhaltende Antworten mit bewusster Akkordprüfung.

### 21. Arabic (ARAB)

- Vollständiger Name: **Arabic**
- Display: **`ARAB`**
- Anzahl der Töne: **7**
- Halbtonabstände vom Grundton: **0 2 4 5 6 8 10**
- Töne bei ROOT C (deutsche Namen): **C D E F Ges As B**

**Charakteristische Intervalle:** Große Terz (4), reine Quarte (5), verminderte Quinte (6), kleine Sexte (8) und kleine Septime (10); die chromatische Folge E–F–Ges prägt diese konkrete Variante.

**Musikalischer Eindruck:** Kann kantig, kompakt und ungewöhnlich wirken; die enge Mitte E–F–Ges kann chromatische Spannung erzeugen.

**Einsatzideen:** Experimenteller Acid und EBM: E–F–Ges-Motive. Elektronische Musik: chromatisch gefärbte Ostinati. Dub: kurze Reibungstöne; Reggae nur über bewusst passenden Harmonien.

## Grundton und Transposition

ROOT verschiebt jeden Abstand um dieselbe Zahl von Halbtönen. Die Tonklasse ist
`(ROOT + Abstand) mod 12`. Die MIDI-Note berechnet sich aus
`12 × (OCTAVE + 1) + ROOT + Abstand + 12 × gewählte Oktave` und wird auf 0–127 begrenzt.
OCTS bestimmt den zufällig genutzten Oktavumfang. Die Skalenform bleibt beim
Transponieren unverändert.

Beispiel: `MPEN` bei C ergibt C–Es–F–G–B. ROOT D verschiebt jeden Ton um zwei
Halbtöne und ergibt D–F–G–A–C. Bei ROOT A ergibt `MIN` A–H–C–D–E–F–G.
Ein Transpose-Wert der 303-Sequenz ist ein zusätzlicher Wiedergabeparameter;
ROOT setzt dagegen den Grundton für die Notenerzeugung. Ein gleicher Seed mit
sonst gleichen Einstellungen erzeugt bei verändertem ROOT dieselbe rhythmische
und melodische Auswahl, um den Grundtonabstand verschoben, sofern die MIDI-Grenzen
keine Noten beschneiden.

## Internationales B und deutsches H

Das Display verwendet internationale Namen mit Kreuzen:
`C C# D D# E F F# G G# A A# B`. **Internationales B ist deutsches H** (11 Halbtöne
über C). **Deutsches B ist internationales B♭** (10 Halbtöne über C), im Display
also enharmonisch `A#`. Die C-Beispiele oben verwenden deutsche Namen:
Des = C#, Es = D#, Ges = F#, As = G#, B = A#. Fis und Ges sind in dieser Stimmung
dieselbe Tonklasse; die Schreibweise richtet sich nach dem musikalischen Zusammenhang.
Die Tabellen sind Tonklassendarstellungen, keine vollständigen enharmonischen
Notationsanalysen. Bei `ALTR` bezeichnet E denselben Klang wie theoretisches Fes.

## Bewusste Änderung der gespeicherten Skalenindizes

Die sechs Originalskalen behalten ihre Halbtondefinitionen und Kürzel exakt.
Ihre alten numerischen IDs werden jedoch **bewusst nicht beibehalten**. Es gibt
keine automatische Umnummerierung, keine Projektlöschung und keine Änderung der
Speicherstruktur. Ein gespeicherter numerischer Wert wird nach dem Update anhand
der neuen Tabelle gelesen. Dadurch kann eine ältere Skalenwahl eine andere Skala ergeben:

| Gespeicherter Wert | Bisher | Jetzt |
|---|---|---|
| 0 | MIN | MPEN |
| 1 | PHRY | PMAJ |
| 2 | HARM | EGYP |
| 3 | MPEN | HIRA |
| 4 | DOR | MIN |
| 5 | MAJ | MAJ |

Bereits gespeicherte Step-Noten bleiben erhalten. Der geänderte Tonvorrat wirkt bei
**OCT+**, bei Noten-Neuwürfen durch **OCT-** und bei automatischer Mutation.
Wer die bisherige Wahl weiterverwenden möchte, wählt das ursprüngliche Kürzel
vor dem nächsten Generieren oder Mutieren erneut und speichert das Projekt.
Bei gleichem ursprünglichem Kürzel, Seed und übrigen Einstellungen bleiben
Generation und Mutation der sechs Originalskalen deterministisch wie zuvor.

## Bezug zu Roland MC-707 und Mutable Instruments Shruthi-1

Die [offizielle Roland-MC-707-Skalenliste im Update-Handbuch 1.20](https://static.roland.com/assets/media/pdf/MC-707_update_eng03_W.pdf)
enthält unter anderem die üblichen Modi, Pentatoniken und mehrere hier verwendete
benannte Tonvorräte. Sie hilft beim musikalischen Vergleich von Skalenbezeichnungen.
Für dieses Projekt gelten ausschließlich die obigen 22 Definitionen und ihre Reihenfolge;
Geräteindizes werden nicht übernommen.

Das [offizielle Mutable-Instruments-Shruthi-Handbuch](https://pichenettes.github.io/mutable-instruments-diy-archive/shruthi/manual/)
beschreibt ebenfalls eine Scale/raga-Auswahl. Der Shruthi-1 ist damit ein nützlicher
Bezug für das Arbeiten mit begrenzten Tonvorräten in elektronischen Sequenzen.
Diese Referenz behauptet keine identische Auswahl, Reihenfolge oder Generatorfunktion
auf Shruthi-1 und TB-3PO. Auch hier müssen Klang, Grundton und Begleitharmonie musikalisch
aufeinander abgestimmt werden.

## Technische Quellen und Prüfung

- [Firmwaretabellen und Generator](../firmware/src/seq/tb3po.c)
- [Skalenanzahl und Standard](../firmware/src/seq/tb3po.h)
- [Tabellen-, Transpositions- und Determinismusprüfung](../tests/host/tb3po_scale_test.c)
- [UI- und Speicherprüfung](../tests/host/tb3po_ui_test.c)
- [Abgleich dieses Dokuments mit Firmware und C-Tonnamen](../tests/tb3po_scales_doc_test.py)

`sh tests/host/run_tb3po.sh` prüft diese Definitionen zusammen mit der tatsächlichen
UI-Auswahl. `tests/run_tests.sh` führt zusätzlich die vorhandenen Sequencer-,
Remix- und Host-Szenarioprüfungen aus.
