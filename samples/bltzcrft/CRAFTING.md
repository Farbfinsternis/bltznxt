# BLTZCRFT – Konzept Handwerk, Werkbank und Werkzeuge

Stand: 2026-11-12. Entwurf, noch nicht gebaut.

Vorbild ist **nicht** das Rezeptgitter von Minecraft, sondern Survival-Handwerk wie in
Conan Exiles oder Enshrouded: Man wählt ein Rezept aus einer Liste, die Zutaten werden
abgezogen, nach einer Weile liegt der Gegenstand im Inventar. Größere Dinge gibt es
nur an einer Station (zuerst die Werkbank).

## Entscheidungen (Nutzer, 12.11.)

1. **Freischalten durch Aufheben.** Ein Rezept wird erst frei, wenn man **jede** seiner
   Zutaten schon einmal aufgehoben hat. Beispiel: Wer einen Ast und einen Steinbrocken
   gesammelt hat, kennt die Steinaxt. Nur einer von beiden genügt nicht.
2. **Haltbarkeit.** Werkzeuge verschleißen bei Benutzung. Ist die Haltbarkeit
   aufgebraucht, ist das Werkzeug **kaputt**, verschwindet aber nicht. An der Werkbank
   lässt es sich gegen einige wenige seiner Materialien reparieren.
3. **Herstellung über Zeit** mit Fortschrittsbalken.
4. **Hunger, Gesundheit und Gegner** kommen langfristig dazu, aber nicht in diesem
   Schritt. Das Konzept lässt Platz dafür (Nahrung und Waffen sind nur weitere
   Gegenstände und Rezepte).

## 1. Unterbau: Gegenstände statt nur Blöcke

Bisher hält ein Inventarplatz eine Blockart (`inv_block`, `inv_anzahl`). Neu:

- **Gegenstandsnummern.** 0 bis `B_ANZAHL - 1` bleiben die Blöcke, darüber
  (ab `G_ERSTER = 64`) kommen Gegenstände, die man nicht setzen kann: Ast,
  Steinbrocken, Werkzeuge. `inv_block` wird zu `inv_ding`. Setzen geht nur, wenn
  die Nummer ein Block ist.
- **Eigenschaften je Gegenstand:** Name, Symbol, Stapelgröße (Werkzeuge 1, sonst 64),
  Höchsthaltbarkeit (0 = keine), Werkzeugart (keine, Spitzhacke, Axt, Schaufel),
  Werkzeugstufe (Hand 0, Stein 1, später Eisen 2).
- **Haltbarkeit je Platz:** neues Feld `inv_haltbar(i)`. Werkzeuge stapeln nicht,
  also reicht ein Wert je Platz. Der Stapel am Mauszeiger braucht ihn ebenso.
- **Symbole:** Blöcke wie bisher als Würfel. Gegenstände bekommen ein flaches,
  selbst gemaltes 16×16-Bild (wie die Blocktexturen, im Stil der Pixel-Texturen),
  hochskaliert auf `SYMBOL_GROESSE`. Unter Werkzeugen ein Haltbarkeitsbalken
  (grün → rot), kaputte Werkzeuge grau mit rotem Kreuz.
- **Gesehen-Liste:** `ding_gesehen(n)`, wird bei jedem Aufheben gesetzt. Daraus
  ergeben sich die freien Rezepte (siehe 4).

## 2. Woher die ersten Materialien kommen

Ohne Werkzeug baut die Hand weder Stein noch Erz ab. Die ersten Zutaten müssen also
mit bloßer Hand zu holen sein:

| Gegenstand | Quelle |
|---|---|
| **Ast** | Laub abbauen: mit 30 % Wahrscheinlichkeit 1 Ast. Außerdem Stamm → 4 Äste von Hand (Rezept). |
| **Steinbrocken** | Lose Steine an der Oberfläche: neuer Pflanzenblock „Steine“ (Härte 0, auf Gras, Sand, Kies und am Ufer, verstreut wie die Blumen) gibt 1–2 Brocken. Kies von Hand gibt mit 25 % einen Brocken. |
| **Holz (Stamm)** | Wie bisher von Hand, langsam. Mit Axt schneller. |
| **Stein (Block)** | Erst mit Spitzhacke. Ergibt den Steinblock zum Bauen; von Hand lässt er sich zu 2 Steinbrocken zerschlagen (Rezept). |
| **Kohle** | Erst mit Spitzhacke aus Kohleerz. |

So entsteht der Weg: Laub und lose Steine → Steinwerkzeuge von Hand → Holz und Stein
abbauen → Werkbank → Kohle → Fackeln.

## 3. Werkzeuge

| Werkzeug | schneller bei | erlaubt Ertrag bei | Haltbarkeit |
|---|---|---|---|
| Steinspitzhacke | Stein, Sandstein, Erze | Stein, Sandstein, Kohle (Eisen braucht später Stufe 2) | 100 |
| Steinaxt | Stamm, Bretter, Werkbank | – | 100 |
| Steinschaufel | Erde, Gras, Sand, Kies, Ton, Schnee | – | 100 |

- **Abbauzeit:** mit dem passenden Werkzeug Härte × 1,5 ÷ 4 (Steinstufe wie in
  Minecraft), sonst wie bisher (Hand). `bl_hand` bleibt die Regel für „Ertrag ohne
  Werkzeug“; neu kommt `bl_werkzeug(b)` (welche Art passt) und `bl_stufe(b)` (welche
  Stufe für Ertrag nötig ist) dazu.
- **Verschleiß:** jeder abgebaute Block mit Härte > 0 kostet das gehaltene Werkzeug 1,
  auch wenn es das falsche Werkzeug ist (wie in Minecraft).
- **Kaputt:** Bei 0 wirkt das Werkzeug wie die bloße Hand, bleibt aber im Platz.
  Meldung „Steinaxt ist kaputt – an der Werkbank reparieren“.
- **In der Hand:** Das Handmodell (`hand.bb`) hält das Werkzeug sichtbar; zuerst
  genügt ein flaches Quad mit dem Symbol, später ein kleines Modell.

## 4. Rezepte und Freischalten

Ein Rezept hat: Ergebnis und Anzahl, bis zu vier Zutaten mit Anzahl, Herstellzeit,
Station (keine = von Hand, oder Werkbank).

**Frei** ist ein Rezept, sobald `ding_gesehen` für **alle** Zutaten gesetzt ist. Dann
erscheint „Neues Rezept: Steinaxt“ am Bildschirm und das Rezept steht in der Liste.
Werkbank-Rezepte erscheinen ebenfalls, wenn sie frei sind, sind aber nur an einer
Werkbank herstellbar (grau mit Hinweis „braucht Werkbank“).

Solange es keinen Spielstand gibt, gilt das Gesehene je Sitzung. Im Mehrspieler
kennt jeder Spieler seine eigenen Rezepte; das Inventar ist ohnehin je Spieler.

### Erste Rezeptliste

**Von Hand:**

| Ergebnis | Zutaten | Zeit |
|---|---|---|
| 4 Äste | 1 Holz (Stamm) | 2 s |
| 4 Bretter (neuer Block) | 1 Holz (Stamm) | 2 s |
| 2 Steinbrocken | 1 Stein | 2 s |
| Steinaxt | 2 Äste, 3 Steinbrocken | 4 s |
| Steinspitzhacke | 2 Äste, 3 Steinbrocken | 4 s |
| Steinschaufel | 2 Äste, 1 Steinbrocken | 3 s |
| 4 Fackeln | 1 Ast, 1 Kohle | 2 s |
| Werkbank (neuer Block) | 4 Bretter, 2 Steinbrocken | 5 s |

**An der Werkbank:**

| Ergebnis | Zutaten | Zeit |
|---|---|---|
| 4 Steinziegel (neuer Block) | 4 Stein | 4 s |
| Reparieren (siehe 6) | – | 3 s |

Die Werkbank hat in diesem Schritt bewusst wenig. Sie bekommt später den Schmelzofen
als nächste Station (Eisenbarren → Eisenwerkzeuge, Stufe 2) und Dinge wie Truhe, Tür,
Leiter. Die 64 Startfackeln fallen weg.

## 5. Herstellen mit Fortschrittsbalken

- **Wo:** im Inventar (E) rechts neben dem Vorrat eine Rezeptliste. Jede Zeile:
  Symbol, Name, Zutaten mit „vorhanden/nötig“ (fehlende rot). Darunter Anzahl
  wählen (1, 5, alle möglichen).
- **Ablauf:** Klick auf „Herstellen“ zieht die Zutaten **sofort** ab und reiht den
  Auftrag in eine Warteschlange (höchstens 8 Aufträge). Der vorderste läuft; ein
  Balken zeigt seinen Fortschritt im Inventar und klein über der Schnellleiste,
  damit man währenddessen weiterspielen kann.
- **Abbrechen:** Rechtsklick auf einen Auftrag gibt die Zutaten zurück.
- **Fertig:** Ergebnis ins Inventar. Ist es voll, wartet der Auftrag mit „Inventar
  voll“ und läuft weiter, sobald Platz ist. Nichts geht verloren.
- **Werkbank-Aufträge** laufen nur, solange man höchstens 4 Blöcke von einer
  Werkbank entfernt steht; geht man weg, pausiert der Balken. Rechtsklick auf die
  Werkbank öffnet das Inventar mit den Werkbank-Rezepten oben.

## 6. Reparieren

- An der Werkbank erscheint für jedes beschädigte oder kaputte Werkzeug im Inventar
  ein Eintrag „Reparieren: Steinaxt“.
- **Kosten:** die Hälfte der Zutaten des Werkzeugs, abgerundet, mindestens 1 je
  Zutatenart. Steinaxt (2 Äste, 3 Brocken) → 1 Ast, 1 Steinbrocken.
- **Ergebnis:** volle Haltbarkeit. Repariert wird ab 1 Punkt Verschleiß; die Kosten
  hängen nicht davon ab, wie stark es abgenutzt ist (einfach und gut lesbar).
- Während der Reparatur liegt das Werkzeug im Auftrag, nicht im Inventar.

## 7. Bausteine im Code

| Datei | Inhalt |
|---|---|
| `dinge.bb` (neu) | Gegenstandsnummern, Eigenschaften, Symbole malen, `ding_gesehen` |
| `rezepte.bb` (neu) | Rezepte als `Data`-Zeilen, Freischalten, Warteschlange, Reparatur |
| `inventar.bb` | Blocknummer → Gegenstandsnummer, Haltbarkeit je Platz, Rezeptliste zeichnen |
| `bloecke.bb` | neue Blöcke Bretter, Werkbank, Steinziegel, lose Steine; `bl_werkzeug`, `bl_stufe` |
| `abbau.bb` | Abbauzeit nach Werkzeug, Verschleiß |
| `welt.bb` | lose Steine verstreuen |
| `hand.bb` | Werkzeug in der Hand |
| `bltzcrft.bb` | Startfackeln weg, F1-Hilfe, Rechtsklick auf Werkbank, `-foto`-Schalter für die Rezeptliste |

## 8. Erster Meilenstein – Reihenfolge

1. ✅ Gegenstände und Haltbarkeit im Inventar (ohne Rezepte), Äste aus Laub, lose Steine.
2. ✅ Werkzeuge mit Abbauzeit und Verschleiß. Kohleerz gibt dabei den Gegenstand Kohle; die Erzblöcke heißen jetzt „Kohleerz“ und „Eisenerz“.
3. ✅ Rezeptliste von Hand mit Freischalten und Fortschrittsbalken; dazu der Block Bretter.
4. Werkbank-Block, Werkbank-Rezepte, Reparatur.
5. Startfackeln weg, Hilfe und README nachziehen, Fotolauf zum Prüfen.

Jeder Schritt ist für sich spielbar und wird einzeln committet.

## Offene Punkte

- **Freischalten:** Hier gilt „jede Zutat einmal aufgehoben“. Die genaue Menge
  muss nicht gleichzeitig im Inventar liegen. Falls doch gemeint: kurz sagen.
- **Laufen Aufträge bei geschlossenem Inventar weiter?** Hier: ja (wie Conan). Die
  Alternative wäre, dass man im Menü warten muss.
