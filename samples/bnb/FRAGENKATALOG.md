# blox-n-balls Remaster – Fragenkatalog

Stand 31.10.2026. Grundlage: alter Code unter `F:\dev\Blitz3D\Games\bnb`
(~9.650 Zeilen in 26 Dateien) und die neuen Grafiken unter `samples/bnb/`.
Zu jeder Frage steht kurz, was der alte Code macht, und mein Vorschlag (**V:**).
Antworten einfach darunter schreiben oder im Chat durchgehen.

## Stand 31.10.: alle Blöcke A–H entschieden

**Noch offen**
- D27: Anordnung der Waren im Shop-Rahmen (entscheide ich, wenn die Icons da sind).

**Lieferliste (du → `_work/fertig/`)**
- Block-Atlas neu (Kacheln in Vielfachen von 8) + Liste der Blocktypen (links→rechts, oben→unten)
- Modelle als `.glb` aus Blender: Block, Paddle, Ball, Bumper, Hindernisse, Wecker;
  Normal Maps schon mit einbetten (Engine kann sie noch nicht, siehe F39)
- Icons für jede Shop-Ware
- Menügrafik ohne eingebrannten Text
- Türen neu (Vorlage `türen_zusammen.jpg`)
- Maskottchen jubelnd (Level geschafft)
- freie TTF-Schrift (oder Vorschlag von mir)

**Kann ich vorher schon bauen (mit Platzhaltern)**
- Grundgerüst: Zustandsmaschine, Hauptschleife, Optionen/Speicherorte
- Levelformat + Konvertierung der 5 alten `.bbl`
- Tisch (`tableborder.glb`), Ball- und Paddle-Physik, Blöcke als Würfel, HUD-Platzhalter

---

## Bestandsaufnahme alter Code (Kurzfassung)

- **Spielprinzip:** 3D-Breakout auf einem Tisch, schräge Kamera. Maus steuert das
  Paddle, der Ball rollt mit Schwerkraft über den Tisch, Abprall über Blitz-Kollisionen
  (Richtung = Kollisionsnormale, recht grob). Pro Level ein Zeitlimit (180–360 s).
- **Raster:** 14×14 Felder, 24 Blocktypen (normal, Punkte 50/75/100, Multiball 1/2/3,
  Extraball, Atomball, Speedball, neue Blöcke, kleines/breites Paddle, Tinyball,
  Bigball, Slowdown, Punkte×2, Zeit ±15 s, Bumper, drei Hindernisarten).
  Normale Blöcke brauchen zwei Treffer (Alpha 1 → 0,5 → weg), beim ersten Treffer wird
  das Modell gegen ein „Restobjekt“ getauscht.
- **Levels:** nur 5 Stück (`lvl01–05.bbl`, binär: 196 Int + Zeit + Musikpfad).
  Editor `bnbed.exe` liegt nur als EXE vor, Quelltext fehlt.
- **Zwischen den Levels:** Wertung (Levelbonus 1000, Schwierigkeitsbonus, Zeitbonus
  500/s) → Shop. **Punkte sind gleichzeitig Währung („Credits“)**: Kaufen senkt den Score.
- **Inventar:** gekaufte/gesammelte Extras, Mausrad wählt, Rechtsklick/Strg löst aus.
- **Sonstiges:** Ballcam (Tab), Tisch anstoßen (Leertaste), Spiegelung des Tischs,
  Türen-Überblendung zwischen allen Screens, 3D-Mauszeiger mit Funken, Screenshot (F12),
  Cheat Strg+N, Debug-Taste F1 → Level 6.
- **Ballast:** i.Score (Online-Highscore per HTTP), Banner-Download, Mehrspieler-Lobby
  („noch nicht implementiert“), Demo-Modus mit Demo-Endscreens, PAK-Archive,
  Konfigprogramm `Konfiguration.exe`, Schreiben in das Windows-Verzeichnis,
  auflösungsabhängige Grafiken (640/800/1024), zwei parallele Startmenüs,
  Spielstände mit XOR-„Verschlüsselung“ und Prüfzahl.
- **Struktur:** fast alles Globals und `Gosub`, Multiball-Code ist eine Kopie der
  Ball-Logik, Blocktypen als Zahlen in riesigen `Select`-Blöcken.

## Bestandsaufnahme neue Assets

| Datei | Inhalt | Größe |
|---|---|---|
| `_work/textures/blox.png` | Atlas mit ~15 Blocktexturen + größere Flächen (Holz, Kacheln, Himmel, Schachbrett …) | 2880² / 27 MB |
| `_work/textures/coin.png` | 4×4-Sprite-Sheet: drehende Münze mit Maskottchen | 2880² / 11 MB |
| `_work/textures/hud.png` | HUD-Leiste, Metall/blau, 10 runde Slots (Bälle?) | 3648×288 |
| `_work/textures/paddle.png` | Paddle-Textur (rotes Fenster, Schrauben) | 3840×2176 / 27 MB |
| `_work/textures/screen.png` | Titelbild: Maskottchen-Kopf im Sand + Schriftzug | 3840×2176 |
| `_work/textures/shop.png` | Shop-Hintergrund: Metallrahmen, Maskottchen stehend | 3840×2176 |
| `_work/textures/some-old-menu.png` | Menütafel mit **deutschem Text inkl. „i.Score“** eingebrannt + Materialflächen | 2048² |
| `_work/textures/tableborder-*.png` | Tischrand Diffuse + Normal Map | 2048² |
| `assets/meshes/tableborder.glb` | Tischrand-Modell (aus `.blend`) | 14 MB |
| `_work/artwork/*` | Maskottchen: stehend, sitzend-traurig, altes Blöcke-Render | bis 20 MB |
| `_work/textures/original/*` | alte Vorlagen (Atlas, Paddle, Panel, Shop, Türen, Auswahl-Animation) | klein |

---

## A. Ziel und Umfang

1. **Wofür ist das Remaster?** Mitgeliefertes Beispiel im `samples/`-Ordner wie
   BLTZCRFT/Friendly Fire, eigenständiges Spiel mit eigenem Release – oder beides?
   **V:** Leuchtturm-Beispiel in `samples/bnb/`, mit eigenem Release-Tag wie `bltzcrft-v0.1.0`.
   ✅ **Entschieden 31.10.:** Beispiel in `samples/bnb/` mit eigenem Release-Tag (`bnb-v0.1.0`).
2. **Werktreue:** Soll es sich spielen wie damals (gleiches Ballgefühl, gleiche Level),
   oder ist es „inspiriert von“ und darf sich spürbar ändern?
   **V:** Kern gleich (Tisch, Maus-Paddle, Blocktypen, Shop), Physik und Bedienung modern.
   ✅ **Entschieden 31.10.:** Frei inspiriert – das Original ist Ausgangspunkt, nicht Vorgabe.
   Alte Level, Werte und Abläufe dürfen sich spürbar ändern.
3. **Name und Schreibweise:** „blox-n-balls“, „Blox'n'Balls“ (wie auf dem Titelbild),
   „BNB“? Wie heißen Ordner, EXE und Fenstertitel?
   ✅ **Entschieden 31.10.:** Titel „Blox'n'Balls“, Ordner `samples/bnb/`, EXE `bloxnballs.exe`.
4. **Sprache im Spiel:** Der alte Code mischt Deutsch und Englisch („LEICHT“ vs.
   „easy“, „Nicht genug Credits“ vs. „Bonus +50“). Nur Deutsch, nur Englisch,
   oder umschaltbar?
   **V:** Englisch im Spiel (passt zu GitHub/Release), Code-Kommentare deutsch wie in den
   anderen Beispielen.
   ✅ **Entschieden 31.10.:** Englisch im Spiel. (Code-Sprache: siehe H46 – alles englisch.)
5. **Welche BLTZNXT-Features soll es vorführen?** (z. B. glTF + Normal Maps,
   Per-Pixel-Licht, Partikel, Sound, Netzwerk.) Davon hängt ab, wie viel Technik wir
   bewusst einbauen.
   ✅ **Entschieden 31.10.:** glTF + Normal Maps, Licht + Schatten, Partikel + Effekte.
   ⚠ Normal Maps erst, wenn die Engine sie kann (siehe F39).
   Netzwerk nicht als Ziel (B9: Mehrspieler ganz gestrichen).
6. **Erster Meilenstein:** Was muss in Version 0.1 drin sein? Mein Vorschlag:
   Tisch + Paddle + Ball + normale Blöcke + 5 alte Level + HUD + Gewinnen/Verlieren.
   Shop, Extras, Editor danach.
   ✅ **Entschieden 31.10.:** Spielbarer Kern für 0.1: Tisch, Paddle, Ball, normale Blöcke,
   HUD, Gewinnen/Verlieren, die 5 alten Level als Startmaterial (dürfen nach A2 umgebaut
   werden). Shop, Extras und Editor in späteren Meilensteinen.

## B. Was fliegt raus, was bleibt

7. **i.Score, Banner-Download, Online-Lobby:** komplett streichen? (Du hast i.Score
   schon genannt.) Gilt das auch für den Menüpunkt in `some-old-menu.png`?
   ✅ **Entschieden 31.10.:** Alles Online streichen (i.Score, Banner, Lobby), auch der
   Menüpunkt in der neuen Menügrafik.
8. **Demo-Modus** (Demo-Schriftzug, Ende nach Level 5, Demo-Endscreens): weg?
   ✅ **Entschieden 31.10.:** Streichen.
9. **Mehrspieler:** Der alte Code hatte nur eine halbe Lobby. Mit dem neuen Network-Update
   wäre z. B. „zwei Spieler, geteilter Bildschirm“ oder „Versus im LAN“ möglich. Jetzt
   ganz weglassen und später eventuell neu denken?
   **V:** Erst mal weglassen, Code so schneiden, dass ein zweites Paddle später passt.
   ✅ **Entschieden 31.10.:** Ganz streichen – kein Mehrspieler, Code nicht darauf auslegen.
10. **Laden/Speichern:** Braucht ein Arcade-Spiel mit wenigen Leveln Spielstände?
    Alternativ nur „Fortsetzen ab erreichtem Level“ automatisch.
    **V:** Kein Speicher-Menü, nur Autospeichern des Fortschritts + Highscore.
    ✅ **Entschieden 31.10.:** Nur Autospeichern (erreichtes Level, Münzen, Inventar),
    „Continue“ im Menü. Kein Speicher-/Lademenü.
11. **Ballcam (Tab), Tisch anstoßen (Leertaste), Tischspiegelung:** behalten?
    ✅ **Entschieden 31.10.:** Alle drei bleiben. Spiegelung darf modern gelöst werden
    (glänzender Boden statt Spiegel-Trick).
12. **Türen-Überblendung zwischen den Screens:** Markenzeichen behalten (neue Türen
    aus `türen_zusammen.jpg` neu rechnen) oder durch etwas Einfacheres ersetzen?
    ✅ **Entschieden 31.10.:** Behalten und neu rechnen (Vorlage `türen_zusammen.jpg`),
    kürzer und nur bei Screenwechseln, nicht bei jedem Klick.
13. **3D-Mauszeiger mit Funken im Menü:** behalten?
    ✅ **Entschieden 31.10.:** Normaler Zeiger, kein 3D-Zeiger mit Funken.
14. **Cheats/Debug-Tasten** (Strg+N, F1): als offizielle Debug-Optionen behalten oder weg?
    ✅ **Entschieden 31.10.:** Nur im Debug-Modus (Schalter/Startparameter), im Release aus.

## C. Spielmechanik

15. **Ballphysik:** Alt: Schwerkraft drückt den Ball auf den Tisch, Richtung = Normale
    der Kollision (Ball fliegt oft „rechtwinklig“ weg). Neu: saubere Reflexion
    (Einfallswinkel = Ausfallswinkel) und Paddle-Treffpunkt bestimmt den Winkel, wie man
    es von Breakout kennt?
    **V:** Eigene 2D-Kollision in der Tischebene (Kreis gegen Rechteck), 3D nur für die
    Darstellung. Dann ist das Verhalten berechenbar und unabhängig von der Framerate.
    ✅ **Entschieden 31.10.:** Eigene 2D-Kollision (Kreis gegen Rechteck) in der Tischebene,
    saubere Reflexion, Paddle-Trefferpunkt bestimmt den Winkel; 3D nur Darstellung.
16. **Paddle-Steuerung:** nur Maus wie früher, oder zusätzlich Tastatur/Gamepad?
    ✅ **Entschieden 31.10.:** Maus und Gamepad (Joystick-Befehle sind in BLTZNXT vorhanden).
    Keine Tastatursteuerung fürs Paddle.
17. **Tempo:** Alt wird die Geschwindigkeit aus den FPS hochgerechnet und steigt pro
    Level. Feste Zeitschritte (Delta-Time) und eine klare Tempokurve pro Level?
    ✅ **Entschieden 31.10.:** Delta-Time; Grundtempo pro Level aus der Leveldatei, steigt
    während des Levels leicht an.
18. **Zwei Treffer pro Block:** Normale Blöcke brauchen zwei Treffer, beim ersten wird
    ein „Restobjekt“ gezeigt (Hedra-Modell). Beibehalten? Wie soll der beschädigte Block
    aussehen (Risse, kleinerer Block, andere Textur)?
    ✅ **Entschieden 31.10.:** Trefferzahl pro Block festlegbar (1–3) in der Leveldatei.
    Beschädigte Blöcke zeigen Risse/Verfall statt Modelltausch.
19. **Blocktypen:** Alle 24 übernehmen? Kandidaten zum Zusammenlegen:
    Punkte 50/75/100 → ein Typ mit Wert; Multiball 1/2/3 → ein Typ mit Anzahl;
    Hindernisse Zylinder/Block/45° → ein Typ mit Form. Gibt es neue Typen, die du willst?
    ✅ **Entschieden 31.10.:** Zusammenlegen zu Typ + Parameter (Punkteblock/Wert,
    Multiball/Anzahl, Hindernis/Form, Zeit/±s, Größe Paddle/Ball). Ca. 12 Typen.
20. **Unzerstörbare Blöcke:** Alt zählen Bumper/Hindernisse beim Abräumen mit, und es
    gibt Fehler dabei (Zähler wird an mehreren Stellen falsch geändert). Gilt: Level ist
    geschafft, wenn alle *zerstörbaren* Blöcke weg sind?
    ✅ **Entschieden 31.10.:** Ja – nur zerstörbare Blöcke zählen, Zähler wird beim Laden
    einmal bestimmt.
21. **Zeitlimit pro Level:** behalten? Was passiert bei Ablauf – alt: Level gilt als
    beendet (!) mit „Time over“, nicht als verloren. Gewollt?
    ✅ **Entschieden 31.10.:** Zeitlimit bleibt. Bei Ablauf geht ein Ball verloren und die
    Uhr startet neu.
22. **Schwierigkeitsgrade:** alt Leicht/Mittel/Schwer = Tempo, Bälle (5/4/3),
    Schwerkraft, Zeit geteilt durch 1/1,5/2. Behalten?
    ✅ **Entschieden 31.10.:** Drei Stufen Easy/Normal/Hard (Tempo, Startbälle, Zeit),
    eigene Highscore-Liste je Stufe.
23. **Leben/Bälle:** Ball verloren kostet −10 Punkte. Behalten?
    ✅ **Entschieden 31.10.:** Nur der Ball, kein Punktabzug. Multiball-Kugeln kosten
    nichts, solange eine übrig ist.
24. **Bonus-Anzeige:** alt fliegen 3D-Zahlen-Sprites hoch (+15, +50 …) und eine
    Textzeile im Panel. Beibehalten?
    ✅ **Entschieden 31.10.:** Nur schwebende Zahlen am Block (neu gerendert). Keine
    HUD-Meldung, kein großer Schriftzug, kein Combo-Zähler.

## D. Shop, Währung, Punkte

25. **Punkte = Währung?** Alt sinkt der Score beim Einkaufen, das bestraft Käufe in der
    Bestenliste. Die neue **Münze** (`coin.png`) legt eine eigene Währung nahe:
    Münzen fallen aus Blöcken und müssen mit dem Paddle gefangen werden, Score bleibt
    unberührt. So gemeint?
    ✅ **Entschieden 31.10.:** Münzen als eigene Währung. Blöcke lassen manchmal Münzen
    fallen, das Paddle muss sie fangen. Score bleibt unberührt.
26. **Shop-Angebot:** alt Atombombe 30.000, Extraball 40.000, +15 s 20.000,
    breites Paddle 15.000, Zeitlupe 17.000. Gleiche Waren, neue Preise?
    ✅ **Entschieden 31.10.:** Erweitertes Angebot: die alten fünf plus alles, was es als
    Block-Extra gibt (Multiball, großer Ball, Punkte×2 …). Keine dauerhaften Upgrades.
    Preise in Münzen, lege ich beim Bau fest.
27. **Shop-Bildschirm:** `shop.png` zeigt Rahmen + Maskottchen. Wo stehen die Waren
    (links/rechts im Rahmen)? Gibt es Warenbilder/Icons, oder rechne ich die aus dem
    Atlas/neu?
    ✅ **Entschieden 31.10.:** Du rechnest je Ware ein Icon neu (Stil coin.png/hud.png).
    ⏳ Offen: Anordnung der Waren im Rahmen von `shop.png`.
28. **Inventar im Spiel:** Mausrad + Rechtsklick wie früher? Wo wird das gewählte Extra
    angezeigt – in der HUD-Leiste?
    ✅ **Entschieden 31.10.:** Wie früher: Mausrad wählt, rechte Maustaste löst aus;
    Gamepad Schultertasten + Knopf. Gewähltes Extra im HUD.

## E. Level und Editor

29. **Level-Format:** Die alten `.bbl` sind binär. Neues, lesbares Textformat
    (z. B. 14 Zeilen mit Zeichen pro Feld + Kopf mit Zeit/Musik)? Die 5 alten Level
    würde ich einmalig konvertieren.
    **V:** Textformat, damit man Level auch im Editor der IDE bauen kann.
    ✅ **Entschieden 31.10.:** Textformat: Kopf (Name, Zeit, Tempo, Musik, Rastergröße), dann
    je Zeile ein Zeichen-Code pro Feld. Die 5 alten `.bbl` werden einmalig konvertiert.
30. **Rastergröße:** 14×14 behalten? Die alten Level nutzen nur die oberen ~9 Zeilen.
    ✅ **Entschieden 31.10.:** Rastergröße pro Level variabel (im Levelkopf).
    ✅ Gelöst in F39: Blöcke skalieren auf die feste Tischfläche.
31. **Neue Level:** Wie viele sollen es zum Start sein? Baust du sie, baue ich Vorschläge,
    oder beides?
    ✅ **Entschieden 31.10.:** 5 alte Level konvertiert + weitere Vorschläge von mir,
    du feilst nach.
32. **Level-Editor:** `bnbed.exe` hat keinen Quelltext. Neu schreiben (als eigenes
    BLTZNXT-Programm im Projekt wie `werkzeug/` bei den anderen Beispielen)? Jetzt oder
    später?
    ✅ **Entschieden 31.10.:** Später, nach 0.1, als eigenes BLTZNXT-Programm in `werkzeug/`.
    Bis dahin reicht das Textformat.

## F. Grafik und Assets

33. **Ordnerstruktur:** Es gibt `assets/` (englisch) neben `_work/`, die anderen Beispiele
    nutzen `daten/`. Bleibt es bei `assets/`? Vorschlag:
    `assets/textures`, `assets/meshes`, `assets/sounds`, `assets/music`, `assets/levels`, `assets/ui`.
    ✅ **Entschieden 31.10.:** `assets/` nach Art: textures, meshes, sounds, music, levels, ui.
34. **`_work/` ins Git?** Die Rohdaten sind riesig (PNGs bis 27 MB, `.glb` 14 MB,
    zusammen weit über 200 MB). Rohdaten nur lokal (`.gitignore`), im Repo nur die
    fertig verkleinerten Assets?
    ✅ **Entschieden 31.10.:** Nein – `_work/` nur lokal (`.gitignore`), ins Repo nur die
    fertigen Assets.
35. **Zielauflösung der Texturen:** 3840×2176 / 2880² sind für Spielgrafik zu groß.
    Auf 1024²–2048² verkleinern und als PNG/JPG ablegen? Machst du das (Qwen/Blender)
    oder soll ich ein Skript schreiben?
    ✅ **Entschieden 31.10.:** Du rechnest Texturen mit Qwen-2.1 Image Edit neu und bringst sie
    mit Affinity Photo in gebräuchliche Formate; GLBs baust und texturierst du in Blender.
    Übergabe in `_work/fertig/`, ich verschiebe von dort nach `assets/`.
36. **Block-Atlas:** Der neue `blox.png` hat ungleich große Felder (15 Blockkacheln +
    große Flächen unten). Welche Kachel gehört zu welchem Blocktyp? Sind die großen
    Flächen für Tischboden/Hintergrund gedacht?
    ✅ **Entschieden 31.10.:** Du baust den Atlas neu (Kacheln in Vielfachen von 8) und
    lieferst eine Liste der Blocktypen, gelesen links→rechts, oben→unten.
37. **Blöcke als 3D-Modell:** Bleibt der Block ein texturierter Würfel, oder kommt ein
    eigenes Modell (abgerundete Kanten, Schrauben) aus Blender?
38. **Fehlende Modelle:** Tischboden, Paddle-Modell, Ball, Bumper, Hindernisse, Wecker
    (Zeit-Bonus), Punkte-Objekte – kommen die neu aus Blender, oder soll ich einfache
    Formen per Code bauen?
    ✅ **Entschieden 31.10.:** Alle Modelle (Block, Paddle, Ball, Bumper, Hindernisse,
    Wecker …) baust du in Blender als `.glb`. Bis dahin nehme ich Platzhalter per Code.
39. **Tischrand-Modell:** Die `.glb` hat 14 MB. Liegt da die Textur eingebettet drin?
    Soll das Modell die Textur selbst mitbringen oder lädt der Code
    `tableborder-diffuse/normal` getrennt?
    **Befund 31.10.:** Ein Mesh `border`, ca. 150 × 225 Einheiten; die Diffuse-Textur
    (2048², 14,3 MB) ist eingebettet, die Normal Map nicht.
    ✅ **Entschieden 31.10.:** Tisch hat feste Größe, Blöcke skalieren auf das Raster
    des Levels (löst E30).
    ⚠ **Engine-Lücke:** BLTZNXT kann noch keine Normal Maps (glTF-Lader liest nur
    `baseColorTexture`, `BrushNormalMap` steht nur in ENGINE_DESIGN.md).
    ✅ **Entschieden 31.10.:** Erst ohne Normal Maps, nachrüsten wenn die Engine den
    modernen Pfad bekommt. Normal Maps trotzdem schon in die `.glb` einbetten.
40. **HUD:** `hud.png` hat links/rechts Felder und 10 runde Slots. Meine Lesart: Score |
    Level | Zeit | 10 Ball-Slots | Extra. Stimmt das? Schrift dafür – eine TTF (welche?)
    oder Bitmap-Font wie früher?
    ✅ **Entschieden 31.10.:** HUD-Lesart stimmt: Score | Level | Zeit oben, 10 Ball-Slots,
    rechts gewähltes Extra; Münzen ins linke untere Feld. Schrift: eine freie TTF.
41. **Menü:** `some-old-menu.png` hat die Menüpunkte als Bild eingebrannt (inkl. i.Score,
    Laden/Speichern). Neu rechnen ohne Text und die Punkte per Schrift darüberlegen?
    Welche Menüpunkte soll es geben?
    ✅ **Entschieden 31.10.:** Menü: Continue, New Game (+ Skill), Highscores, Options,
    Credits, Quit. Menügrafik ohne eingebrannten Text, Schrift darüber.
42. **Maskottchen:** Wo taucht es überall auf? Ideen: Titel (`screen.png`), Shop
    (`shop.png`), Verloren-Screen (sitzend-traurig), Level geschafft (jubelnd – fehlt noch?).
    ✅ **Entschieden 31.10.:** Titel, Shop, Verloren-Screen (sitzend-traurig) und Level
    geschafft (jubelnde Pose – rechnest du noch). Nicht im HUD.
43. **Auflösung/Seitenverhältnis:** Alt fest 640/800/1024 in 4:3, Grafiken je Auflösung
    doppelt. Neu: 16:9, frei skalierend, Fenster und Vollbild?
    ✅ **Entschieden 31.10.:** 16:9, ein Grafiksatz, skaliert auf jede Auflösung; Fenster
    und Vollbild in den Optionen.

## G. Sound und Musik

44. **Musik:** Die alten MP3s (Hauptmenü + 4 Stücke) – sind die von dir und dürfen mit
    ins Repo/Release (Lizenz)? Oder neue Musik?
    ✅ **Entschieden 31.10.:** Alte Musik übernehmen (Lizenz geklärt). BLTZNXT spielt MP3 und
    OGG (dr_mp3, stb_vorbis), die MP3s bleiben also wie sie sind (~7,4 MB) → `assets/music/`.
45. **Soundeffekte:** alte WAVs übernehmen (Lizenz?) oder neue erzeugen? Gilt dasselbe
    für die `.iff`-Dateien?
    ✅ **Entschieden 31.10.:** Alte WAVs übernehmen → `assets/sounds/`. Die `.iff` sind keine
    Sounds, sondern i.Score-Daten (~150 Byte, nur in `iscore.bb` benutzt) und entfallen.

## H. Technik und Code

46. **Code-Aufteilung:** wie bei BLTZCRFT mehrere `.bb`-Dateien nach Thema
    (`bnb.bb`, `tisch.bb`, `ball.bb`, `bloecke.bb`, `extras.bb`, `shop.bb`, `hud.bb`,
    `menue.bb`, `level.bb`, `klang.bb`). Deutsche Dateinamen wie dort?
    ✅ **Entschieden 31.10.:** Nach Thema, englische Dateinamen (`bloxnballs.bb`, `table.bb`,
    `ball.bb`, `blocks.bb`, `extras.bb`, `shop.bb`, `hud.bb`, `menu.bb`, `level.bb`,
    `sound.bb`, `effects.bb`). Auch Bezeichner und Kommentare englisch.
47. **Zustandsmaschine statt Gosub-Sprünge:** Titel → Menü → Spiel → Wertung → Shop →
    Spiel …; je Zustand eine Update/Draw-Funktion. Einverstanden?
    ✅ **Entschieden 31.10.:** Zustandsmaschine, eine Hauptschleife, kein Gosub.
48. **Types statt paralleler Arrays:** Block, Ball (auch Multiball – nur ein Code-Pfad),
    Extra, Partikel als `Type`. Einverstanden?
    ✅ **Entschieden 31.10.:** Types; Hauptball und Multiball über denselben Code.
49. **Einstellungen:** statt `blox.ini` + `Konfiguration.exe` ein Optionsmenü im Spiel
    (Lautstärke, Maus-Empfindlichkeit, Fenster/Vollbild), gespeichert neben der EXE oder in
    `%APPDATA%`?
    ✅ **Entschieden 31.10.:** Optionsmenü im Spiel; Einstellungen, Autospeicherstand und
    Highscores neben der EXE.
50. **Highscore lokal:** Top 10 pro Schwierigkeitsgrad, Name eingeben wie früher?
    ✅ **Entschieden 31.10.:** Top 10 je Stufe mit Namenseingabe, letzter Name wird vorgeschlagen.
51. **Muss es auch mit dem originalen Blitz3D laufen**, oder nur mit BLTZNXT (glTF,
    Normal Maps, PNG mit Alpha nutzen)?
    **V:** Nur BLTZNXT – es ist ein Schaufenster dafür.
    ✅ **Entschieden 31.10.:** Nur BLTZNXT.
52. **Protokoll:** wie bei BLTZCRFT eine `PROTOKOLL.md` im Ordner führen?
    ✅ **Entschieden 31.10.:** Ja, `PROTOKOLL.md` (intern, deutsch) wie bei BLTZCRFT.

---

## Auffälligkeiten im alten Code (zur Info, kein Handlungsbedarf)

- Score-Formel benutzt `scrorepoints` (Tippfehler) → Schwierigkeitsbonus war immer 0.
- `wallcol`/`wallcoll` und `bumpercol`/`bumpercoll` vertauscht → Schutz gegen
  „Ball klemmt an der Wand“ hat nie gegriffen.
- Shop: `scorepoints>pr` statt `>=`, Inventar ohne Obergrenze.
- `lev$<100` vergleicht einen String mit einer Zahl.
- Atomball-Treffer (Typ 9) zieht den Blockzähler zweimal ab, Bumper/Hindernisse
  ziehen ab, obwohl sie nie mitgezählt wurden.
