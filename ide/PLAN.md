# Plan: BLTZNXT IDE

Stand 2026-09-29. Ziel der ersten Ausbaustufe: **kann, was die Original-IDE von Blitz3D kann** —
so wie beim Compiler zuerst Kompatibilität, dann mehr. Und: **nichts ist in Stein gemeißelt.**
Alles Folgende ist ein Arbeitsplan, kein Vertrag; wo ein Punkt sich als falsch erweist, wird er
geändert und hier nachgezogen.

Quelle der Bestandsaufnahme: `blitzide/` und `debugger/` im offiziellen Repo
(github.com/blitz-research/blitz3d, gelesen 2026-09-29: `mainframe.cpp`, `editor.cpp`, `libs.cpp`,
`prefs.cpp`, `funclist.cpp`, `blitzide.rc`, `debugger/*`), dazu die Installation unter `F:\dev\Blitz3D`.

---

## 1. Was die Original-IDE kann

Die Original-IDE ist klein (rund 3000 Zeilen MFC). Ein Fenster mit Tabs; ein Tab ist ein Editor
oder die Hilfe. Ein Projekt gibt es nicht: die IDE kennt nur Dateien, und die Umgebung eines
Programms ist der Ordner seiner Quelldatei.

### Datei
| Funktion | Verhalten im Original |
|---|---|
| Neu (Strg+N) | Tab „`<untitled>`“, beliebig viele |
| Öffnen (Strg+O) | Dialog, Filter `.bb`, Bilder, Klänge, 3D-Modelle, alle Dateien. **Medien werden nicht geöffnet**, sondern an `mediaview.exe` übergeben |
| Schließen / Alle schließen | Strg+F4; bei Änderungen Ja/Nein/Abbrechen |
| Speichern / Speichern unter / Alle speichern | Strg+S; **Sicherungskopien** `datei.bb_bak1`, `_bak2` (Einstellung `edit_backup`, Standard 2) |
| Nächster / vorheriger Tab | Strg+Tab, Strg+Umschalt+Tab |
| Drucken | Strg+P |
| Zuletzt geöffnet | 10 Einträge, zuletzt benutzte oben |
| Start mit Dateiname | `blitzide datei.bb` öffnet sie |
| Datei schon offen | wechselt zum vorhandenen Tab (Pfadvergleich ohne Groß-/Kleinschreibung) |
| Beenden | fragt bei jedem geänderten Tab nach |

### Bearbeiten
Ausschneiden, Kopieren, Einfügen, Alles auswählen, Suchen (Strg+F), Weitersuchen (F3), Ersetzen
(Strg+R); Rechtsklick zeigt das Bearbeiten-Menü; **Tab / Umschalt+Tab** rücken markierte Zeilen
ein und aus; **Enter übernimmt die Einrückung** der Zeile; „Symbolleiste anzeigen“ (Umschalt+Esc)
blendet Symbol- und Statusleiste zusammen aus. Statuszeile: `Row:12 Col:4 *` (Stern = geändert).

### Editor
- Färbung in **sieben** Farben: Hintergrund, Zeichenkette, Bezeichner, Schlüsselwort,
  Kommentar, Zahl (auch `$FF`), Standard.
- **Schlüsselwörter werden zur Schreibweise der Befehlsliste korrigiert** (`graphics` → `Graphics`),
  sobald der Cursor das Wort verlässt. Die Liste kommt bei jedem Start von `blitzcc +k`.
- **Drei Listen unter dem Editor**: `funcs`, `types`, `labels` — Klick springt zur Zeile. Sie
  werden zeilenweise nachgeführt, nicht durch einen Parser der ganzen Datei.
- Schnellhilfe (F1): zeigt die Signatur des Wortes unter dem Cursor in der Statuszeile; **zweites
  F1 auf demselben Wort** öffnet die Hilfeseite des Befehls.

### Programm
| Funktion | Verhalten |
|---|---|
| Programm starten (F5) | speichert alle geänderten **benannten** Dateien, ruft `blitzcc -q [-d] "datei.bb" [Kommandozeile]` |
| Nochmal starten (F6) | öffnet die zuletzt gebaute Datei und startet sie |
| Fehler suchen (F7) | `-c`: nur übersetzen |
| Programm erstellen… | `-o "x.exe"`; warnt, wenn Debug an ist |
| Kommandozeile… | Text, der an das Programm geht (`cmd_line`) |
| Debug an/aus | `-d`; Standard **an** |
| **Namenloser Tab** | Text wird nach `<Blitz>\tmp\tmp.bb` geschrieben, gebaut, der Tab bleibt namenlos. So läuft „schnell etwas ausprobieren“ ohne Speichern. |

Bau-Ablauf (`compile()`): Umgebungsvariable `blitzide=1`, stdout+stderr in **einer** Leitung
gelesen. Zeilen, die auf `...` enden, sind Fortschritt (Balken im Dialog „Compiling“). Beginnt
eine Zeile mit `"`, ist es ein Fehler `"datei":z:s:z:s:Meldung`: die IDE öffnet die Datei, setzt den
Cursor und zeigt die Meldung in einem Dialog. `Executing…` beendet den Dialog; das Programm läuft
als eigener Prozess.

### Hilfe
Ein **HTML-Tab** mit `help\index.html`: Start (Strg+H), Zurück, Vor. Ein Link auf eine `.bb`-Datei
(Beispiele) öffnet sie im Editor. Die Befehlsseiten liegen unter `help\commands\2d_commands\` und
`3d_commands\`. „Über“-Dialog.

### Einstellungen (`cfg\blitzide.prefs`)
Textdatei, Schlüssel und Wert: `prg_debug`, `prg_lastbuild`, `win_rect`, `win_maximized`,
`win_notoolbar`, `font_editor/tabs/debug`, sieben `rgb_*`, `edit_tabs` (Tabbreite, 4),
`edit_blkcursor`, `edit_backup`, `img_toolbar`, `cmd_line`, `file_recent` (bis 10). Es gibt
**keinen Einstellungsdialog**; man bearbeitet die Datei. Ein unbekannter Schlüssel setzt alles auf
die Vorgabe zurück.

### Start
`blitzcc -q` (jede Ausgabe = „Compiler environment error“), `blitzcc -v` (Versionen), `blitzcc +k`
(Befehle). Arbeitsverzeichnis ist zunächst `<Blitz>\samples`.

### Debugger (eigenes Fenster, vom Laufzeitsystem gestartet)
Schnittstelle `Debugger` in `debugger.h`: `debugRun`, `debugStop`, `debugStmt(pos,file)`,
`debugEnter(frame,env,func)`, `debugLeave`, `debugLog`, `debugMsg(msg,serious)`, `debugSys`.
Bedienung: **Run, Stop, Step over, Step into, Step out, End**. Anzeige: Quelltext mit aktueller
Anweisung, Aufrufliste, Variablenbaum (global, lokal, Objekte/Typen), Debug-Log
(`DebugLog`-Ausgaben), Laufzeitfehler als Meldung. Das gibt es bei uns **heute nicht**
(KNOWN_ISSUES: „Debug switch is ignored“) und braucht Arbeit im Compiler — eigene Phase.

### Bewusst nicht Teil der Parität
Funktionsumfang der Editions „Blitz2D/Plus“ (`#ifdef PLUS`), Toolbar-Bitmaps als Format,
Windows-RichEdit-Eigenheiten (Textlimit), der Hilfe-Inhalt selbst (gehört zum Blitz3D-Paket).

---

## 2. Architektur: klein im Kern, alles andere Erweiterung

Leitsatz: **Der Kern kennt keine Funktion der IDE.** Datei-Tabs, Bauen, Hilfe, Gliederung,
Einstellungen, Debugger sind alle *eingebaute Erweiterungen*, die genau dieselbe Schnittstelle
benutzen wie eine spätere fremde. Was eine eingebaute Erweiterung nicht darf, darf der Kern
nicht heimlich anbieten — dann fehlt dem Kern ein Erweiterungspunkt, und den bauen wir.

```
ide/
  electron-main.js          Prozess-Start, Fenster; sonst nichts
  electron/                 MAIN-Prozess: alles, was das Betriebssystem berührt
    services/               fs, toolchain (blitzcc), process, settings-store, dialogs …
    bridge.js               EIN generischer Kanal; Dienste melden sich an
  src/                      RENDERER
    core/                   Kern (framework-frei, in Node testbar)
      commands.js           Befehlsregister
      contributions.js      Erweiterungspunkte
      services.js           Dienste nach Namen
      events.js             Ereignisse
      settings.js           Einstellungen (geschichtet, versioniert)
      documents.js          Dokumentmodell
      extensions.js         Laden / Aktivieren
    shell/                  Fenster: Menüleiste, Symbolleiste, Tabs, Panels, Statuszeile
    extensions/             eingebaut (später: auch aus einem Benutzerordner)
      files/  editor/  language-blitz/  build-run/  outline/
      help/   settings/  recent/        debugger/ (spät)
    platform/               Brücke zum Backend (heute schon da)
```

### Erweiterungspunkte (Contribution Points)
Eine Erweiterung ist `{ id, version, activate(ctx), contributes }` und darf beitragen:

- **Befehle** (`id`, Titel, Handler, `enabledWhen`) — Menüs, Tastenkürzel, Symbolleiste,
  Kontextmenü und später eine Befehlspalette verweisen **nur auf Befehls-IDs**. Genau wie die
  `ID_*` des Originals, nur offen.
- **Menüs / Symbolleistenknöpfe / Kontextmenüs / Tastenkürzel** — deklarativ, als Daten. Ein
  Kürzel umlegen heißt Daten ändern, nicht Code.
- **Ansichten** (Tab-Inhalt, Seitenleiste, unteres Panel): Editor, Hilfe, Gliederung, Ausgabe,
  Debugger. Eine Ansicht ist eine Funktion `mount(element, ctx)`; kein Kern-Wissen darüber.
- **Statuszeilen-Einträge** (Cursorposition, Compiler-Version, Debug-Schalter …).
- **Einstellungen** mit Schema (Typ, Vorgabe, Beschreibung) — der Einstellungsdialog wird aus den
  Schemata der Erweiterungen erzeugt.
- **Sprachdienste** (Färbung, Hover, Vervollständigung, Symbole, Diagnosen) — dünne Schicht über
  Monaco, damit auch Monaco austauschbar bleibt.
- **Toolchain-Adapter** (siehe unten).
- **Dokumenttypen**: `bb` öffnet einen Editor, `png`/`wav`/`x` einen Medienbetrachter (oder wie im
  Original ein externes Programm).

### Dienste und Ereignisse
Erweiterungen holen sich Dienste per Namen (`ctx.services.get('documents')`), nicht per Import
einer Nachbarn-Datei. Ereignisse (`document:saved`, `build:started`, `build:finished`,
`settings:changed` …) entkoppeln die Erweiterungen voneinander. Damit kann jede Erweiterung
entfernt, ersetzt oder ergänzt werden, ohne dass eine andere bricht.

### Trennung Renderer / Main
Der Renderer kennt nur `platform/` (heute schon so). Die Brücke bekommt **einen** generischen
Kanal `invoke(dienst, methode, args)` mit Anmeldung im Main-Prozess statt einer festen Liste von
IPC-Kanälen. Ein neuer Dienst braucht dann keine Änderung an `preload.js`. Ein späterer Wechsel
auf Tauri ersetzt den Transport, nicht die IDE.

### Werkzeugkette: die IDE kennt nur die Kommandozeile
Bleibt so wie heute: kein Include, kein Linken, kein Wissen aus `src/compiler/`. Neu ist die
Form: ein **Toolchain-Adapter** übersetzt zwischen IDE und Compiler. Der Standardadapter spricht
das Protokoll der Original-IDE (`blitzide=1`, `-q`, `-c`, `-o`, `+k`, Fehlerzeile
`"datei":z:s:z:s:msg`) — dann läuft die IDE gegen **jeden** blitzcc, auch den Originalen. Ein
zweiter Adapter kann später reichere Ausgaben nutzen (strukturierte Fehler, Warnungen, Debug),
sobald der Compiler sie anbietet; die IDE fragt Fähigkeiten ab, statt eine Version anzunehmen.

### „Kein Projekt“ ist eine Eigenschaft, keine Einschränkung
- **Arbeitsordner = Ordner der aktiven Quelldatei.** Dort läuft das Programm (das tut
  `blitzcc` schon), dort werden `Include`-Pfade und Medien relativ aufgelöst. Es gibt keine
  Projektdatei.
- **Namenlose Dokumente** (`scratch`) sind vollwertige Dokumente mit einem Ordner in
  `%TEMP%\bltznxt-ide\`; jeder Tab bekommt seine Datei (Original: eine gemeinsame `tmp.bb`,
  dadurch keine zwei namenlosen Programme gleichzeitig baubar). Aufräumen beim Beenden und beim
  nächsten Start. Speichern verschiebt sie an den gewählten Ort.
- Später, **optional**: eine Ordnerübersicht ab dem Arbeitsordner und ein `.bltznxt`-Ordner für
  Einstellungen pro Ordner (Kommandozeile, Debug). Das ist eine Erweiterung; ohne sie
  funktioniert alles wie im Original.

### Einstellungen und Zustand
- **Einstellungen** (JSON, geschichtet: Vorgabe → Benutzer → optional Ordner). Unbekannte
  Schlüssel bleiben erhalten und lösen nichts aus (Original: Totalverlust). Schema-Version im
  Dokument, Migrationen als Funktionen.
- **Zustand** (zuletzt geöffnete Dateien, Fensterposition, letzter Build) getrennt von
  Einstellungen: der Benutzer bearbeitet das eine, die IDE schreibt das andere.
- **Import von `blitzide.prefs`** (Farben, Schrift, Tabbreite, Sicherungen, Kommandozeile,
  zuletzt geöffnet) beim ersten Start, wenn die Original-IDE gefunden wird.
- Farben und Schrift sind **Themes** (Daten), das „Blitz3D-Klassik“-Theme ist die
  Original-Vorgabe (`#225588` mit den sieben Farben von oben).

### Was wir bewusst nicht festlegen
- **Kein UI-Framework** im Kern. Der Kern ist reines JavaScript ohne DOM; die Shell ist dünn
  und ersetzbar. Ob später Lit, Preact oder nichts, entscheidet sich an der Shell, nicht am Kern.
- **Monaco** ist heute der Editor, aber nur hinter der Sprachdienst-/Editor-Schicht
  (`extensions/editor`). Andere Editoren (CodeMirror) blieben möglich.
- **Erweiterungen von außen** (aus einem Benutzerordner) erst nach der Parität. Die
  Manifest-Form soll dann schon stimmen; die Schnittstelle ist bis dahin als *experimentell*
  markiert und darf sich ändern.
- **Sprache der Oberfläche**: Texte kommen aus einer Tabelle (`t('file.save')`), nicht aus dem
  Code — Englisch und Deutsch sind später Daten.

---

## 3. Phasen

Jede Phase endet mit etwas, das sich **ausprobieren und testen** lässt. Reine Kern-Module werden
mit `node --test` geprüft (kein Electron nötig); die Oberfläche mit dem Smoke-Test
(`ide/test/smoke.js`), der dafür erweitert wird.

### P0 — Fundament ✓ (2026-09-29)
- Kern: Befehle, Erweiterungspunkte, Dienste, Ereignisse, Kontextschlüssel mit `when`-Ausdrücken,
  Tastenkürzel, Einstellungen, Dokumentmodell, Menümodell, Erweiterungs-Host — `ide/src/core/`.
- Shell: Menüleiste, Symbolleiste, Tab-Leiste, Ansichten, unteres Panel, Statuszeile, Dialoge —
  `ide/src/shell/`, aus Beiträgen gefüllt; ein Menü ohne Einträge bleibt unsichtbar.
- Einheitliche Brücke `invoke(dienst, methode, ...)`; `backend.js` ist der Dienst `toolchain`
  geworden, dazu der Dienst `store` (Einstellungsdatei).
- Eingebaute Erweiterungen: `workbench` (Menügerüst, „Show Toolbars“, About), `toolchain`,
  `language-blitz`, `editor`.
- Smoke-Test repariert (BUG-121) und auf reines Node umgestellt.
- **Geprüft durch:** `npm test` (Kern und Zusammenbau, 42), `npm run test:smoke` (Compiler-
  Anbindung), `npm run test:ui` (echte Oberfläche in Electron). Das Kriterium — ein Menüpunkt
  aus einer Erweiterung erscheint und löst seinen Befehl aus — steht in beiden Ebenen.

**Beim Bauen entschieden** (nicht im ursprünglichen Plan):
- `contributes` einer Erweiterung darf eine Funktion `contributes(ctx)` sein, damit Befehle auf
  Einstellungen und Dienste zugreifen, ohne Platzhalter.
- `Document.text` hat immer LF; Kodierung und Zeilenende (`eol`) gelten erst beim Schreiben.
  „Geändert?“ ist damit ein Textvergleich, und kein Editor muss das Zeilenende der Datei kennen.
- Die Menüleiste ist HTML aus dem Menümodell (nicht das native Electron-Menü): Sperren, Haken
  und Kürzel kommen aus demselben Modell wie Symbolleiste und Kürzel, ohne Umweg über IPC.
- Der Compilerpfad ist eine Einstellung der IDE und reist mit jedem Aufruf zum Dienst
  `toolchain`; der Dienst selbst hat keinen Zustand außer einem Cache.
- Ein Dokument ohne Datei behält seinen Text im Speicher; Temp-Ordner je Tab (Entscheidung 3)
  kommen mit P1/P3, wenn es zum Bauen etwas zu schreiben gibt.

### P1 — Dateien ✓ (2026-09-29)
Neu, Öffnen (mehrere Dateien), Schließen, Alle schließen, Speichern (unter, alle), Tabs mit
Änderungsmarke, Strg+Tab, zuletzt geöffnet (10), Sicherungskopien `_bakN`, Rückfrage beim Schließen
und Beenden (auch mit dem Kreuz des Fensters), Start mit Dateiname, doppelte Datei erkennen
(ohne Rücksicht auf Groß-/Kleinschreibung), namenlose Dokumente (`scratch`), Fenstertitel mit dem
Pfad, Willkommensansicht ohne offene Datei. Alles in der Erweiterung `files`
(`ide/src/extensions/files/`), dazu die Dienste `files`, `dialog`, `host` und `window` im
Main-Prozess und `core/encoding.js`, `core/state.js`.

**Dateikodierung war die erste Falle** und ist gelöst: `.bb`-Dateien sind Bytes (Windows-1252),
Monaco denkt in UTF-16. Gelesen wird verlustfrei (BOM → UTF-8 mit BOM; gültiges UTF-8 mit
Nicht-ASCII → UTF-8; sonst Windows-1252, das jedes der 256 Bytes kennt), geschrieben in derselben
Kodierung mit dem Zeilenende der Datei. Zeichen, die die Kodierung nicht kennt (etwa „→“ in
Windows-1252), werden nicht ersetzt: die IDE fragt, ob als UTF-8 gespeichert werden soll.
**Geprüft durch:** alle 256 Bytes, Umlaut-Datei, UTF-8 mit/ohne BOM, ungültiges UTF-8, 1-MB-Datei
(`test/unit/encoding.test.js`); Öffnen → Ändern → Speichern ergibt nur die geänderte Stelle
(`files-extension.test.js`); Speichern in der echten Oberfläche schreibt `Größe €` als
`47 72 F6 DF 65 20 80` mit CRLF (`ui-smoke.js`).

**Beim Bauen entschieden:**
- Gemischte Zeilenenden (CRLF und LF in einer Datei) kommen nach dem Speichern mit dem
  häufigeren heraus; `decodeFile` meldet `mixedEol`, die IDE warnt bisher nicht. Eine unveränderte
  Datei wird nie geschrieben, bleibt also byte-gleich.
- Schließen aller Tabs (auch beim Beenden) geht wie im Original von hinten nach vorn; bricht der
  Benutzer bei einem Tab ab, bleiben die schon geschlossenen zu.
- Dateien mit NUL-Bytes fragen vor dem Öffnen nach; Medien über den Betrachter kommen mit P4.
- Ein Speichern unter auf eine in einem anderen Tab offene Datei wird verweigert.
- Temp-Ordner je namenlosem Tab (Entscheidung 3) braucht erst das Bauen; namenlose Dokumente
  leben bis dahin im Speicher (P3).
- Zustand (zuletzt geöffnet, letzter Ordner) liegt in `state.json`, getrennt von `settings.json`.
- Neue Einstellungen: `files.backups` (2), `files.defaultEncoding`, `files.defaultEol`.

**Noch offen aus P1:** Drucken (P5), Dateien per Ziehen ins Fenster öffnen.

### P2 — Editor ✓ (2026-09-29)
Färbung in den sieben Farben der Original-IDE (Theme „Blitz3D Classic“ ist die Vorgabe, dazu Dark
und Light), Schreibweise der Schlüsselwörter, Ein-/Ausrücken markierter Zeilen (Tab, Umschalt+Tab),
Enter übernimmt die Einrückung, Suchen/Weitersuchen/Ersetzen, Ausschneiden/Kopieren/Einfügen/Alles
auswählen, das Rechtsklickmenü ist das Bearbeiten-Menü, Statuszeile `Row:12 Col:4 *`, Gliederung
funcs/types/labels unter dem Editor. Neue Erweiterungen `themes` und `outline`; `editor` und
`language-blitz` sind ausgebaut.

**Geprüft durch:** reine Logik in `test/unit/editor-logic.test.js` (Schreibweise, Gliederung,
Themes, passive Kürzel); in der echten Oberfläche (`ui-smoke.js`): Farben aus dem DOM gelesen,
Theme-Wechsel, Schreibweise beim Tippen, „Rückgängig“ ohne Endlosschleife, eine geöffnete Datei
bleibt beim Durchklicken unverändert, Zwischenablage über die echte Windows-Zwischenablage,
Rechtsklickmenü, Suchfeld, Gliederung mit Klick-Sprung.

**Beim Bauen entschieden:**
- Themes sind ein Beitragspunkt (`themes`), die Auswahl die Einstellung `workbench.theme`; ein
  Theme trägt die sieben Editorfarben, optional Monaco-Überschreibungen und CSS-Variablen der
  Oberfläche. Der Editor definiert daraus Monaco-Themes, der Workbench setzt die Oberflächenfarben.
- Die Schreibweise fasst nur Zeilen an, die der Benutzer selbst bearbeitet hat: eine geöffnete Datei
  bleibt auch beim Durchklicken byte-gleich. Kommentare, Zeichenketten und Zahlen bleiben
  unberührt; das Wort am Cursor erst, wenn der Cursor weiterrückt. Die Schlüsselwörter kommen
  aus `blitzcc +k` (neuer Dienstaufruf `symbols()`); ohne Compiler gibt es keine Korrektur.
- Kürzel können `passive` sein (nur Anzeige im Menü): Strg+X/C/V/A/Z/Y behandelt das Textfeld selbst.
- Zwischenablage über `webContents.copy/cut/paste` (Dienst `window`): Monacos eigene Aktionen
  klappen ohne echte Benutzergeste nicht, und ein Menüklick ist auf dem Weg dorthin keine mehr.
- Menüklicks nehmen dem Editor den Fokus nicht (`mousedown` wird nicht weitergegeben).
- Gliederung: eingerückte Deklarationen zählen mit, nur ganze Wörter (das Original sieht nur
  Spalte 0 und würde `typeName = 3` für einen Typ halten).
- Nicht wie das Original: Rückgängig/Wiederholen im Menü, „Find Previous“, Bracket-Färbung aus.
- Eine Schrift wie die Original-Bitmapschrift „blitz“ gibt es nicht; `editor.fontFamily` ist
  einstellbar (Vorgabe Consolas).

### P3 — Bauen und Starten ✓ (2026-09-30)
F5 (Run program), F6 (Run program again), F7 (Check for errors), Create Executable…, Program
Command Line…, Debug Enabled? (gespeichert, geht als `-d` mit), dazu Stop program (Umschalt+F5).
Vor dem Bauen werden geänderte benannte Dateien gespeichert; ein Fehler öffnet die Datei, setzt
den Cursor, zeichnet einen **Marker** in den Editor, steht im **Ausgabe-Reiter** (klickbar) und
erscheint wie im Original in einem Meldungsdialog. Namenlose Tabs bauen aus einer Datei im
Temp-Ordner. Nach dieser Phase kann man **schreiben, F5, spielen**. Erweiterung `build-run`,
Dienste `build` (Main-Prozess) und `files.scratchFile`; das Protokoll der Original-IDE steht in
`electron/build-protocol.js`.

**Geprüft durch:** Protokoll-Parser an festen Zeilen (`build-protocol.test.js`); die Erweiterung
gegen eine Plattform-Attrappe mit eingespielten Compiler-Ereignissen (`build-run.test.js`, 20
Fälle); der Runner gegen den **echten blitzcc** (`test/smoke.js`: Prüfen, Fehler mit Ort, Starten,
Arbeitsordner, Argumente); und in der echten Oberfläche (`ui-smoke.js`): F5 auf Datei und
namenlosem Tab, F7 mit Fehler → Marker/Cursor/Dialog/Ausgabe, Kommandozeile, Stopp, F6, Erstellen.

**Beim Bauen entschieden:**
- Die IDE spricht mit blitzcc genau wie die Original-IDE (`blitzide=1`, `-q [-d] [-c | -o x] datei`,
  Zeilen auf `...`, Fehler `"datei":z:s:z:s:msg`, `Executing...`); dadurch läuft sie auch gegen
  den originalen blitzcc. Fehler kommen mit Anfang **und** Ende, der Marker deckt die Spanne ab.
- Ein Programm läuft unabhängig von der IDE weiter; blitzcc wartet auf sein Ende (deshalb bleibt
  „läuft“ bis dahin sichtbar, Statuszeile „Program running“). Während es läuft, kann man weiter
  bauen (mehrere Programme gleichzeitig).
- **Stop** ist neu (Original: nur im Debugger). Beendet wird der Prozessbaum (`taskkill /T`); da
  blitzcc dann seinen Temp-Ordner nicht mehr löschen kann, räumt die IDE ihn selbst weg, beim
  Stoppen und beim Start (Ordner beendeter blitzcc-Prozesse).
- Namenlose Tabs: `%TEMP%\bltznxt-ide\scratch\<Prozess>\doc<Nr>\untitled.bb`; Zeichen, die
  Windows-1252 nicht kennt, gehen dort als UTF-8 hinein. Ein Fehler in dieser Datei erscheint im
  Tab, nicht in einer zweiten Datei. Relative Pfade und `Include` finden dort nichts, bis man speichert
  (Entscheidung 3, Abschnitt 4).
- F6 merkt sich nur benannte Dateien (`state.json`: `build.lastFile`), wie das Original.
- Die Warnung des Originals beim Erstellen mit aktivem Debug („langsamere Programme“) entfällt: bei
  uns hat `-d` keine Wirkung (KNOWN_ISSUES). Der Meldungsdialog bei Fehlern ist Vorgabe wie im
  Original, `build.errorDialog` schaltet ihn ab.
- Neu gegenüber P2: Symbolleiste komplett (Ausschneiden, Kopieren, Einfügen, Suchen, Starten), Dialog
  mit Eingabefeld (`dialogs.prompt`), Dienst `panel.show(id)`.

### Seitenleiste mit den Dateien ✓ (2026-09-30, auf Wunsch eingeschoben)
Die Original-IDE hat keine Dateiübersicht; der Plan führte sie unter „Danach“ als spätere
Erweiterung. Nach dem Ausprobieren am Leuchtturm (acht Dateien, drei Ordner) vorgezogen.
Erweiterung `explorer` (`ide/src/extensions/explorer/`), Ansichtsort `sidebar` in der Shell, Trenner
zum Ziehen (Breite wird gemerkt), Strg+B blendet aus und ein.

- **Kein Projekt, ein Ordner**: die Liste folgt der aktiven Datei — deren Ordner steht oben. Liegt die
  Datei schon im gezeigten Ordner (ein Include im Unterordner), bleibt sie stehen; ein namenloser
  Tab ändert nichts. „Open Folder…“ (Datei-Menü) wählt einen Ordner fest, der Pin-Knopf löst ihn.
- Klick öffnet die Datei; Bilder, Klänge, Modelle im Standardprogramm (Dienst `shell`, nur für
  Medienformate — ein Klick startet nie ein Programm). Pfeiltasten, Enter.
- Ausgeblendet (`explorer.hide`): `*.exe`, `*.dll`, `*.bb_bak*`, `.git`, `__pycache__` u.a.
- Aktualisiert beim Speichern, nach einem Bau, beim Zurückkehren ins Fenster und auf Befehl — ohne
  Dateiwächter. Aufgeklappte Ordner werden gemerkt.
- Noch nicht: Neue Datei/Ordner, Umbenennen, Löschen, Kontextmenü, Ziehen und Ablegen.

### P4 — Hilfe
HTML-Tab mit Start/Zurück/Vor (Strg+H), F1-Schnellhilfe (Signatur in der Statuszeile), zweites F1
öffnet die Befehlsseite, Link auf `.bb` öffnet im Editor. Ort der Hilfe einstellbar (Original:
`<Blitz>\help`); ohne Hilfe funktioniert alles andere. Medien-Dateien werden an einen Betrachter
übergeben (`mediaview.exe`, wenn vorhanden).

### P5 — Einstellungen und Politur
Einstellungsseite aus den Schemata, Import `blitzide.prefs`, Schrift/Farben/Tabbreite/
Sicherungsanzahl/Blockcursor, Symbolleiste ein/aus (Umschalt+Esc), Fenstergröße merken, Drucken,
„Über“. Paketierung als installierbare Anwendung (offene Frage: Werkzeug).

### P6 — Paritätsprüfung
Zeile für Zeile Abschnitt 1 durchgehen, Original und BLTZNXT-IDE nebeneinander (Original-IDE gibt
es lokal, `F:\dev\Blitz3D`, und die Testinstallation mit BLTZNXT). Beispiele der Original-Samples
(BirdDemo, blox-n-balls, Leuchtturm) schreiben, bauen, starten. Abweichungen sind entweder Fehler
oder eine dokumentierte Entscheidung.

### P7 — Debugger (braucht den Compiler)
Erst **Protokoll entwerfen**, dann bauen; beides gehört in `KNOWN_ISSUES.md` erst dann weg.
- Compiler/Laufzeit: `-d` erzeugt Anweisungs- und Funktionsmarken, ein Kanal (stdio oder Pipe)
  meldet `stmt`, `enter`, `leave`, `log`, `error` und nimmt `run`, `stop`, `step*`, `end` an.
  Laufzeitfehler mit Zeile gibt es dank BUG-Politik schon auf stderr.
- IDE: Debugger als Erweiterung: Schritt-Befehle, aktuelle Zeile im Editor, Aufrufliste,
  Variablenbaum, Debug-Log. Setzt erst auf, wenn die Kanalbeschreibung steht.

### Danach (jeweils eine Erweiterung, keine Reihenfolge)
Sprung zur Definition und Vervollständigung eigener Funktionen/Typen über `Include`-Grenzen,
Ordnerübersicht, Haltepunkte, Snippets, Formatierer, Themes, Erweiterungen von außen,
mehrere Fenster, Git-Anzeige, Live-Vorschau von Medien.

---

## 4. Entschiedene Fragen (2026-09-29)

1. **Hilfe: beides, Original zuerst.** Die IDE nutzt die Original-Hilfe (`help\`), wenn sie sie
   findet; der Pfad ist einstellbar. Zusätzlich erzeugt sie aus `blitzcc +k` eine eigene Kurzhilfe
   (Signaturen für F1, Befehlsliste), damit sie auch ohne Blitz3D-Installation nützlich ist. Die
   Original-Seiten liefern wir nicht aus. Eine eigene Volldokumentation ist eine spätere Erweiterung.
2. **Oberfläche: Englisch, Deutsch als Zweitsprache.** Englisch ist Vorgabe; alle Texte kommen aus
   einer Tabelle (`t('file.save')`). Deutsch wird eine zweite Datei, sobald die Texte stehen.
3. **Namenlose Dokumente: Temp-Ordner je Tab.** Jeder namenlose Tab bekommt eine eigene Datei in
   `%TEMP%\bltznxt-ide\`. Relative Medienpfade gehen dort erst nach dem Speichern. Spätere
   Einstellung „letzter Arbeitsordner“ bleibt möglich, ist aber keine Vorgabe.
4. **Paket: ZIP zuerst, Installer später.** Entpacken und starten wie beim Leuchtturm-Release.
   Die Paketform ändert nichts an der IDE selbst.
5. **Kodierung: Windows-1252, außer die Datei ist gültiges UTF-8.** Beim Öffnen wird erkannt;
   gespeichert wird in derselben Kodierung, Zeilenenden bleiben wie vorgefunden. Eine unveränderte
   Datei bleibt byte-gleich (Test in P1).

## 5. Regeln für die Arbeit daran

- **Kein Feature im Kern.** Wenn etwas nicht als Erweiterung geht, fehlt ein Erweiterungspunkt.
- **Daten vor Code** bei Menüs, Kürzeln, Themes, Einstellungen, Texten.
- **Jede Phase gegen das Original geprüft**, nicht gegen die eigene Erinnerung: Quelle
  `blitzide/*.cpp` oder die Original-IDE selbst laufen lassen.
- Der Compiler bleibt eine Kommandozeile; kein IDE-Feature darf Compiler-Interna voraussetzen.
- Dieser Plan wird beim Arbeiten nachgeführt (Abschnitt 3 abhaken, neue Fragen in Abschnitt 4).
