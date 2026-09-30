# BLTZNXT — Friendly Fire

Stand: 2026-09-28 · Schritt 7 läuft — der Umfang ist erreicht: Bewegung, drei Waffen, Items, Anzeige, 3D-Klang; Waffen (der Raketenwerfer von GPT-6 Astra), Zielscheiben und Items sind Modelle von Kenney (CC0), Arena und Klänge noch Platzhalter

Ein kleiner Arena-Shooter im Stil von Quake III, geschrieben als **gewöhnliches Blitz3D-Programm**:
Blitz-Code, Blitz-Befehle, der Blitz3D-kompatible Renderer. Neu sind nur die Daten — Karte, Waffen
und Items kommen aus Blender, als glTF (`.glb`).

Das Projekt beantwortet eine Frage, die blox-n-balls nicht beantworten kann: **Kann man mit BLTZNXT
heute etwas Neues bauen, mit heutigen Werkzeugen?** Alte Programme laden weiter ihre `.x`- und
`.3ds`-Dateien; wer neu anfängt, exportiert aus Blender und bekommt dasselbe Blitz3D.

## Umfang

„Fertig" heißt:

| Teil | Inhalt |
|---|---|
| Karte | Eine Arena, in Blender gebaut, mit gebackenen Lightmaps |
| Bewegung | Laufen, Springen, Luftkontrolle, Strafen — das Gefühl von Quake III, nicht seine Formeln |
| Waffen | Drei, je mit eigener Munition |
| Items | Munition für jede Waffe, Rüstung, Gesundheit; erscheinen nach einer Zeit wieder |
| Oberfläche | Anzeige von Gesundheit, Rüstung, Munition und aktueller Waffe |

Vorschlag für die Waffen, weil sie drei verschiedene Techniken abdecken:

| Waffe | Art | Technik |
|---|---|---|
| Maschinengewehr | Sofort-Treffer, schnelle Folge, wenig Schaden | `LinePick` je Schuss |
| Raketenwerfer | Geschoss mit Flächenschaden, Rocket-Jump | Entity mit Kollision, Explosion mit Radius |
| Railgun | Sofort-Treffer, langsam, viel Schaden, sichtbare Spur | `LinePick`, Spur als Sprite-Kette oder Mesh |

**Nicht im Umfang:** eine zweite Karte, Spielmodi, Menüs über das Nötigste hinaus, Bots,
Mehrspieler. Gegner sind eine offene Entscheidung (unten).

## glTF im alten Pfad

Der glTF-Lader liest aus der Datei, was der Blitz3D-Renderer darstellen kann, und lässt den Rest
liegen. `LoadMesh("arena.glb")` und `LoadAnimMesh("waffe.glb")` funktionieren wie bei `.x`.

| glTF | Blitz3D |
|---|---|
| Knoten mit Name und Hierarchie | Entity bzw. Pivot, `EntityName`, `FindChild`, `GetChild` |
| Mesh-Primitive | Surface |
| Material: Grundfarbe, Alpha | Brush: `BrushColor`, `BrushAlpha` |
| `baseColorTexture` (UV-Satz 0) | Texturschicht 0 |
| Lightmap auf `TEXCOORD_1` | Texturschicht 1, multipliziert |
| `doubleSided` | FX 16 |
| `alphaMode` MASK / BLEND | Textur-Flag 4 bzw. Alpha-Blend |
| emissiv | FX 1 (Näherung) |
| Vertexfarben | Vertexfarben, FX 2 |
| Skinning, Animationen | Blitz-Knochen und Animationssequenzen (`Animate`, `AnimSeq` …) |
| Metallic, Roughness, Normal Maps, KHR-Erweiterungen | im alten Pfad ignoriert — der moderne Pfad liest sie später aus derselben Datei |

glTF ist rechtshändig, Y oben, in Metern; Blitz3D linkshändig. Der Lader spiegelt eine Achse und
kehrt die Umlaufrichtung um, wie der `.3ds`-Lader es mit seinem Achsentausch schon tut. Die
Einzelheiten werden am Original mit einem `.x`-Gegenstück gemessen, nicht geraten.

## Die Karte in Blender

Die Karte ist eine einzige `.glb`. Alles, was das Spiel über sie wissen muss, steckt in den
**Objektnamen** — damit reichen `EntityName`, `CountChildren` und `GetChild`, und es braucht keinen
neuen Befehl.

| Blender-Objekt | Name | Wirkung im Spiel |
|---|---|---|
| Sichtbare Geometrie | frei | Gerendert, nicht kollidierend |
| Kollisionsgeometrie | endet auf `-col` | Unsichtbar, `EntityType` für die Kollision; einfacher als die sichtbare |
| Spawnpunkt | `spawn` | Leeres Objekt; Position und Blickrichtung des Spielers — der Spieler blickt entlang der +Y-Achse des Empties |
| Zielscheibe | `ziel` | Leeres Objekt; dort schwebt eine Scheibe, auf die man schießen kann |
| Waffe | `weapon_mg`, `weapon_rl`, `weapon_rail` | Leeres Objekt; dort liegt die Waffe |
| Munition | `ammo_mg`, `ammo_rl`, `ammo_rail` | Leeres Objekt |
| Rüstung | `armor_25`, `armor_50`, `armor_100` | Leeres Objekt, Zahl = Punkte |
| Gesundheit | `health_25`, `health_50`, `health_100` | Leeres Objekt, Zahl = Punkte |

Blender hängt bei Kopien `.001`, `.002` an; das Spiel liest nur den Teil vor dem Punkt.

Weitere Regeln:

- **Maßstab:** 1 Blender-Einheit = 1 Meter = 1 Blitz-Einheit. Spielerhöhe etwa 1,8.
- **Lightmap:** in Cycles gebacken, auf einen zweiten UV-Satz namens `Lightmap`; die gebackene
  Textur liegt als Occlusion-Textur am Material, damit der Export sie mitnimmt.
- **Export:** glTF Binary (`.glb`), „+Y Up", Modifier anwenden, Custom Properties dürfen mit,
  werden aber nicht gebraucht.
- **Sichtbarkeit:** Für eine Arena genügt das vorhandene Frustum-Culling; kein PVS, keine Portale.

## Waffen in Blender

Eine Waffe ist eine eigene `.glb`, gebaut so, wie man sie in der Hand sieht: Ursprung am Griff,
der Lauf zeigt nach vorn (in Blender −Y, im Spiel +z). Das Spiel lädt sie mit `LoadAnimMesh`,
hängt sie an die Kamera und spielt ihre Animationen mit `Animate`.

| Teil | Regel |
|---|---|
| Mündung | Leeres Objekt `muendung` an der Spitze des Laufs: Mündungsfeuer und Railspur gehen von dort aus |
| Bewegliche Teile | Eigene Objekte unter der Waffe (Läufe, Ringe …), damit eine Animation sie einzeln bewegen kann |
| Animation 0: `feuern` | Ein Schuss, einmal abgespielt; Länge frei, sinnvoll höchstens der Feuertakt (MG 6 Bilder) |
| Animation 1: `heben` | Von unten (erstes Bild) in die Hand (letztes Bild); das Spiel streckt sie auf 0,25 s und spielt sie zum Senken rückwärts in 0,2 s |

Es zählt die **Reihenfolge der Animationen in der Datei**, nicht ihr Name — Blitz3D kennt nur
Sequenznummern. Blender zählt Bilder in seiner eigenen Rate (Vorgabe 24 je Sekunde), das Spiel 60: gezählt wird in
Sekunden, eine Animation läuft im Spiel also so lange wie in Blender.

Freie Assets gibt es unter CC0 unter anderem von Kenney und Quaternius (Modelle) und Poly Haven
(Texturen). Inhalte aus Quake III sind tabu.

## Was BLTZNXT dafür braucht

| Baustein | Stand | Gehört zu |
|---|---|---|
| Kollision, `LinePick`, `EntityType`, `Collisions` | vorhanden | — |
| Sprites, Partikel, mehrere Texturschichten, 2D über 3D | vorhanden | — |
| `FindChild`, `GetChild`, `EntityName`, `CopyEntity` | vorhanden | — |
| Animationssystem (`Animate`, `SetAnimTime`, `AnimSeq`, `ExtractAnimSeq` …), `.b3d` mit Knochen | vorhanden (3D-19) | — |
| glTF-Lader, statisch: Geometrie, Hierarchie, Brushes, zwei UV-Sätze | vorhanden (3D-24) | — |
| glTF-Lader, animiert: Skinning, Animationen auf dem Blitz-System | vorhanden (3D-24) | — |
| 3D-Klang (`CreateListener`, `EmitSound`), Panorama im Mischer | vorhanden (Schritt 6) | — |

## Reihenfolge

| Schritt | Was | Ergebnis |
|---|---|---|
| 1 | Animationssystem, am Original gemessen | Animierte `.x`-Modelle laufen wie in Blitz3D |
| 2 | glTF statisch | Die Arena lädt, mit Lightmap |
| 3 | Spiel: Bewegung und Kollision | Man läuft und springt durch die Arena — **läuft**, Platzhalter-Arena |
| 4 | Spiel: Waffen | Drei Waffen feuern, Treffer und Explosionen — **läuft**, mit Zielscheiben, Modelle von Kenney (CC0) |
| 5 | glTF animiert | Waffenmodelle mit Animation in der Hand — **läuft**, Kenney-Modelle mit Feuer- und Hebeanimation |
| 6 | 3D-Klang | Schüsse und Items sind räumlich zu hören — **läuft**, mit Platzhalterklängen |
| 7 | Spiel: Items und Anzeige | Aufsammeln, Wiedererscheinen, HUD — Umfang erfüllt — **läuft**, mit Platzhaltern |

Schritt 3 kann mit einem Platzhalter beginnen, sobald Schritt 2 steht; die Waffen in Schritt 4
dürfen bis Schritt 5 statisch sein.

## Stand im Repository

`samples/friendlyfire/`:

| Datei | Inhalt |
|---|---|
| `friendlyfire.bb` | Hauptprogramm: fester Takt zu 1/60 s, Maus und Tastatur, Anzeige (F1); die Maus wird erst nach einem Klick ins Fenster gefangen und bleibt dann im Fenster, Tab gibt sie frei, Esc beendet |
| `spieler.bb` | Bewegung: Reibung, Beschleunigung am Boden und in der Luft, Sprung, Schwerkraft; Kollision als Ellipsoid (0,4 × 0,9) mit `Collisions …,2,3`; Bodenprüfung per `LinePick` |
| `karte.bb` | Karte laden: `-col` wird unsichtbare Kollisionsgeometrie, `spawn` der Startpunkt, andere leere Objekte werden Marken (`ziel`, später die Items); ohne `-col` kollidiert die sichtbare Geometrie |
| `waffen.bb` | MG und Railgun per `LinePick`, Raketen als Entity mit Kollision, Explosion mit Flächenschaden und Rückstoß; Wechsel, Munition, Waffe in der Hand |
| `ziele.bb` | Zielscheiben an den Marken `ziel`: drehen sich, blitzen bei Treffern, zerplatzen, kommen nach 5 s wieder |
| `klang.bb` | Listener an der Kamera, Klänge mit `Load3DSound`, jeder an seiner Entity per `EmitSound`: Schüsse an der Mündung, Schub an der Rakete (mit `LoopSound`), Treffer an der Scheibe, Explosionen an einem Pivot an ihrem Ort |
| `effekte.bb` | Funken, Rauch, Feuerball, Einschlagflecken, Railspur, Explosionslicht; die Texturen erzeugt das Programm |
| `daten/arena.glb` | Platzhalter-Arena aus `werkzeug/arena.py`: Boden, Wände, Säule, Block, Rampe, Treppe, zwei Plattformen, sechs Zielmarken |
| `daten/mg.glb`, `rail.glb`, `ziel.glb` | Aus Kenneys Blaster Kit (CC0), aufbereitet von `werkzeug/kenney.py` nach den Regeln „Waffen in Blender“: Größe, Mündung, der vordere Teil als eigener Knoten `lauf` (beim MG stößt er zurück, bei der Railgun fährt er zurück und dreht sich beim Nachladen), Animationen `feuern` und `heben`; Textur in der Datei; danach von `werkzeug/veredeln.py` (Blender) neu abgewickelt und veredelt |
| `daten/rl.glb` | Der Raketenwerfer: ein Modell von GPT-6 Astra, in Blender gebaut (blauer Steinlauf, glühender Kern). `werkzeug/astra.py` macht es aus `werkzeug/astra/weapon.glb` spielfertig: Standardwürfel entfernt, gedreht, auf 0,30 m gebracht, `muendung`, erfundene Animationen `feuern` und `heben` (das Original ist nicht animiert), Netz mit Blender von 15 500 auf 6 000 Dreiecke vereinfacht, Textur von 4096² auf 1024² verkleinert (JPEG), mit Blender neu abgemischt (Fugenschatten, Kantenlicht, Risse, Rost) und die Leuchttextur daraufgerechnet, weil der Renderer keine kennt (17 MB → 0,8 MB) |
| `daten/klang/*.wav` | Platzhalterklänge aus `werkzeug/klaenge.py`, aus Rauschen und Sinustönen gerechnet |
| `items.bb` | Items an den Marken `weapon_…`, `ammo_…`, `armor_N`, `health_N`: drehen sich, werden bei Berührung genommen, wenn sie etwas bringen, und kommen nach Quake-Zeiten wieder |
| `anzeige.bb` | Munition, Leben und Rüstung groß am unteren Rand, die eigenen Waffen, Meldungen beim Aufsammeln, rotes Aufblitzen bei Schaden |
| `daten/items/*.glb` | Aus `werkzeug/kenney.py`: Koffer aus dem Blaster Kit in den Farben der Waffen (Munition), Rundschild aus Mini Dungeon (Rüstung: grün, gelb, rot), Herz aus dem Platformer Kit (Gesundheit: gelb, orange, groß und blau); umgefärbt, indem die Texturkoordinaten in ein anderes Feld der Farbtafel rücken. Waffen-Items sind die Waffenmodelle selbst |
| `werkzeug/glb.py` | Gemeinsames der Werkzeuge: Körper bauen, `.glb` schreiben, auch mit eingebetteter Textur |
| `werkzeug/veredeln.py` | Blender-Skript, das Modelle veredelt: backt Ambient Occlusion und Kantenlicht und legt prozedurale Risse, Facetten und Rost nach Materialfarbe darüber, weil der Renderer Tiefe nur über die Farbe zeigen kann; Modus `astra` (vorhandene UVs) und `neu` (neu abwickeln, Kenneys Farbtafel aufs neue Layout backen) |
| `werkzeug/astra.py`, `astra_blender.py`, `textur.ps1` | Bringt das Astra-Modell nach `daten/rl.glb`; das Netz vereinfacht Blender im Hintergrund (`astra_blender.py`, Umgebungsvariable `BLENDER`), die Textur rechnet `textur.ps1` (Windows PowerShell, System.Drawing) |
| `figuren.bb`, `daten/figur.glb`, `werkzeug/figur.py` | Die Spielerfiguren für den Mehrspieler (Weg A): ein Roboter aus 15 Teilen an festen Gelenken, 240 Dreiecke, ohne Animation in der Datei. `figuren.bb` dreht die Teile selbst: Schritt aus der Geschwindigkeit (120° je Meter, Beine ±50°, Arme gegenläufig, Rumpf lehnt sich in den Lauf), Sprungpose, Zielen mit der Waffe vor der Brust (Rumpf und Kopf nehmen den halben Blickwinkel), Rückstoß 0,15 s, Umfallen auf den Rücken in 0,6 s. Farbe über `EntityColor` auf die weißen Teile (Stiefel, Visier, Handschuhe, Rucksack bleiben dunkel als Kind `<teil>_dunkel`), Waffenmodell am Knoten `waffe`, Namensschild als Sprite am Knoten `name`, alle Teile `EntityPickMode 2` (`Figur_Von`, `Figur_Kopf`). Die Namen der Teile sind die Schnittstelle: eine andere Figur läuft im Spiel, wenn sie wieder `bein_l`, `bein_r`, `torso`, `kopf`, `arm_l`, `arm_r`, `waffe`, `name` hat. `tests/test_friendlyfire_figur.bb` prüft Teile, Schritt, Luft, Zielen, Rückstoß, Treffer, Tod |
| `werkzeug/kenney.py` | Lädt die Kenney-Pakete nach `werkzeug/kenney/` (nicht im Repository), schreibt daraus Waffen (ohne Raketenwerfer), Zielscheibe und Items als Rohmodelle nach `werkzeug/roh/` (nicht im Repository) und lässt `veredeln.py` daraus die Dateien in `daten/` machen; ohne Blender (`BLENDER`) werden sie nur kopiert |

Werte der Bewegung: die von Quake III, umgerechnet über die Spielergröße (56 Einheiten = 1,8 m,
eine Einheit ≈ 3,2 cm): Laufen 10,3 m/s, Absprung 8,7 m/s, Schwerkraft 25,7 m/s², Sprunghöhe
1,4 m; Stufen bis 25 cm geht man hinauf. `tests/test_friendlyfire_bewegung.bb` steuert den Spieler mit künstlicher Eingabe
durch die Arena und prüft Fallen, Laufen, Wand, Sprung, Block, Rampe hinauf und hinab, Treppe und
Stehen am Hang.

Werte der Waffen, ebenso aus Quake III umgerechnet; Zeiten in ganzen Takten zu 1/60 s:

| Waffe | Takt | Schaden | Sonst |
|---|---|---|---|
| MG | 0,1 s | 7 | Streuung 1,4 Grad |
| Raketenwerfer | 0,8 s | 100 direkt, bis 100 als Fläche in 3,85 m | Rakete 28,9 m/s |
| Railgun | 1,5 s | 100 | Spur aus zwei gekreuzten Bändern und Funken |

Der Rückstoß einer Explosion ist 0,16 m/s je Schadenspunkt, von der Explosion zur Spielermitte und
0,77 m nach oben gerichtet — ein Rocket-Jump trägt so gut 9 m hoch, eine Rakete aus dem Stand
knapp 5 m. Wechsel: 0,2 s senken, 0,25 s heben, erst wenn die Waffe feuerbereit ist.
`tests/test_friendlyfire_waffen.bb` feuert mit künstlicher Eingabe und prüft Takt, Treffer,
Zerstören und Wiedererscheinen, Wechsel, Railspur, Raketenflug, Direkt- und Flächentreffer,
Rocket-Jump, die Rakete an der Wand und den Wechsel bei leerer Waffe, dazu die Animationen der
Modelle: Sequenz, Rückstoß des Laufs beim MG, Drehung des Railgun-Laufs, halb gesenkt beim Wechsel.

Items, Leben und Rüstung, ebenfalls nach Quake III; alle Zeiten in ganzen Takten:

| Item | Wirkung | Wieder da nach |
|---|---|---|
| `weapon_mg/rl/rail` | Waffe; Munition auf 40 / 10 / 10, wer mehr hat, bekommt einen Schuss dazu; eine neue Waffe wird gleich genommen | 5 s |
| `ammo_mg/rl/rail` | 50 / 5 / 10 Schuss, auch ohne die Waffe; höchstens 200 | 40 s |
| `armor_N` | N Rüstung, höchstens 200 | 25 s |
| `health_N` | N Leben bis 100, ab N = 100 bis 200; bei vollem Leben bleibt es liegen | 35 s |

Man erscheint mit 125 Leben, ohne Rüstung und nur mit dem MG (100 Schuss). Leben und Rüstung über
100 klingen um 1 je Sekunde ab. Die Rüstung fängt zwei Drittel jedes Schadens ab (aufgerundet),
solange sie reicht. Die eigene Rakete schadet halb, stößt aber voll: ein Rocket-Jump kostet
ohne Rüstung bis zu 50 Leben. Bei 0 Leben ist man tot und erscheint nach 2 s am Start neu; wer aus
der Welt fällt, stirbt. `tests/test_friendlyfire_items.bb` stellt den Spieler auf die Items und
prüft Aufnahme, Grenzen, Wiederkehr, Abklingen, Rüstung, Eigenschaden, Tod und Neuerscheinen.

## Mehrspieler (Plan, 2026-09-30)

Bis 8 Spieler, UDP, ein Spieler hostet (Listen-Server), die anderen treten übers Internet bei. Ein
Vermittler auf dem eigenen Webspace (nur PHP) führt die Liste. Grundlagen und Alternativen:
[VISION.md](VISION.md), Abschnitt Netzwerk.

### Was in der Engine fehlt

| Was | Warum |
|---|---|
| UDP-Befehle (`CreateUDPStream`, `SendUDPMsg`, `RecvUDPMsg` …) | Schnappschüsse brauchen unzuverlässige Pakete; ohnehin Phase 1 aus VISION.md (alte Programme) |
| HTTP/HTTPS-Abfrage, z. B. `HttpGet$` / `HttpPost$` über WinHTTP | Blitz kennt nur TCP; Webspace leitet meist auf HTTPS um, per Hand gesprochenes HTTP über Port 80 scheitert dann |
| Portfreigabe (UPnP / NAT-PMP / PCP), im Spiel oder als Befehl | damit der Host erreichbar wird, ohne dass jemand im Router klickt |

STUN (öffentliche Server) ist mit den UDP-Befehlen selbst zu sprechen, der Rest des Netzcodes ist Spiel.

### Der Vermittler (PHP, ohne UDP, ohne Dauerprozess)

PHP beantwortet HTTP-Anfragen; es kann keine UDP-Pakete empfangen und nichts weiterleiten. Es
reicht für Liste und Verabredung, nicht für ein Relay.

| Aufruf | Wirkung |
|---|---|
| `announce` (Host, alle 30 s) | trägt das Spiel ein (Name, Karte, Spieler, Version, Port); die öffentliche IP nimmt PHP aus `REMOTE_ADDR`, nicht vom Host; liefert Token und offene Beitrittswünsche zurück |
| `list` | offene Spiele, jünger als 90 s, mit IP:Port, Spielerzahl und Version |
| `join` (Client) | meldet dem Host die öffentliche UDP-Adresse des Clients (per STUN ermittelt), damit beide gleichzeitig Pakete schicken (UDP-Hole-Punching) |
| `close` | Spiel austragen |

Ablage in SQLite oder Datei mit Ablaufzeit; Token je Host, Begrenzung der Aufrufe, Versionsprüfung.

**Wer muss erreichbar sein?** Nur der Host. Clients verbinden von innen nach außen und kommen
durch jeden Router. Der Host ist erreichbar über IPv6, eine UPnP-Freigabe oder einen
Hole-Punch zu einem einfachen NAT. Scheitert er an CGNAT oder symmetrischem NAT (bei DS-Lite häufig),
hilft ohne Relay nichts — PHP kann keins sein. Das Spiel prüft das beim Hosten (Adresse bei zwei
STUN-Servern vergleichen) und sagt es: „erreichbar“, „eingeschränkt“, „nicht erreichbar“. Für
diese Fälle bleibt ein VPN unter Freunden (Tailscale, ZeroTier) oder später ein Relay auf einem
Server mit Dauerprozess.

### Der Netzcode (8 Spieler)

- Der Host rechnet die Welt in den festen 60-Hz-Takten; `spieler.bb` ist schon deterministisch.
- Clients schicken ihre Eingabe je Takt (Tasten, Blickwinkel, Nummer), sagen die eigene Bewegung
  voraus und gleichen sie mit dem Host ab; die anderen werden aus Schnappschüssen interpoliert
  (20–30 je Sekunde; ein Spieler etwa 20 Byte, also grob 5 KB/s zu jedem Client).
- Zuverlässig (Nummer, Bestätigung, Wiederholung) gehen nur Ereignisse: Beitritt, Tod, Item
  genommen, Waffenwechsel, Chat. Schnappschüsse dürfen verloren gehen.
- Schüsse zählt der Host. Sofort-Treffer später mit Ausgleich der Verzögerung; Raketen sind
  Objekte des Hosts, die Clients sehen sie.
- Gegner sind andere Spieler statt Zielscheiben; Punkte je Abschuss, Ergebnistafel, Erscheinen
  fern voneinander. Eigenschaden der Rakete gilt fort (daher der Name).
- Endet die Runde, wenn der Host geht, ist das für den Anfang in Ordnung.

### Die Spieler sehen sich nicht: es fehlen Figuren

Ohne Figur bleibt der Mehrspieler ein Geisterspiel — man hört Schüsse und sieht niemanden. Drei Wege:

| Weg | Was | Für | Gegen |
|---|---|---|---|
| **A: Figur aus Formen, vom Programm bewegt** | Ein Roboter aus wenigen Teilen (Rumpf, Kopf, zwei Arme, zwei Beine, Hand mit Waffe), in Blender per Skript gebaut wie die Arena; Laufen, Zielen, Sterben rechnet das Programm aus Geschwindigkeit und Blickwinkel | Keine fremden Assets, kein Rig, sofort; läuft im alten Renderer; Teilnamen als feste Schnittstelle | Steif; sieht nach Platzhalter aus |
| **B: freie Figur** (CC0, z. B. Kenney oder Quaternius) mit Skelett und Animationen | Der glTF-Lader kann Skinning und Animationen | Echte Gelenke | Stil, Lizenz und Animationsumfang sind zu prüfen |
| **C: Astra baut sie** mit Rig und Animationen in Blender | Passt zum Raketenwerfer | Hängt von Astras Ergebnis ab | Aufwand beim Prüfen |

Empfehlung: **A zuerst**, damit der Netzcode jemanden zum Sehen hat, mit festen Namen für die Teile
(`torso`, `kopf`, `arm_l`, `arm_r`, `bein_l`, `bein_r`, `hand`, `muendung`), damit B oder C später
ohne Änderung im Spiel einsetzbar sind. Die Waffenmodelle sind schon da und kommen an die Hand
(verkleinert). Die acht Spielerfarben kommen über `EntityColor` auf eine helle Grundtextur, nicht
über acht Texturen. Dazu gehören, weil die Figur allein nicht reicht: ein Name über dem Kopf,
Mündungsfeuer an der Figur, 3D-Schritte und -Schüsse (gibt es), ein Blobschatten, eine
Todesanimation (Umfallen) und ein Trefferkörper (Kasten wie der des Spielers) für `LinePick`.

### Reihenfolge

| Schritt | Was | Ergebnis |
|---|---|---|
| 1 | UDP-Befehle, dann Test mit zwei Programmen auf einem Rechner | Pakete laufen |
| 2 | Figur (Weg A) und Namen | Andere sind sichtbar — **läuft**, `figuren.bb` |
| 3 | Netzcode auf einem Rechner (Host + Clients als getrennte Programme), LAN und direkte IP | 2–8 Spieler im Lokalnetz |
| 4 | Portfreigabe und Erreichbarkeitsprüfung, Beitrittscode | Freunde treten ohne Liste bei |
| 5 | HTTP-Befehl, PHP-Vermittler, Liste im Menü | Öffentliche Spiele |
| 6 | Feinschliff: Ausgleich der Verzögerung, Abbruch und Wiederverbinden, Schummelschutz der Grundsorte | spielbar übers Internet |

## Offene Entscheidungen

- [ ] **Gegen wen spielt man?** Zielscheiben gibt es seit Schritt 4 (entschieden 2026-09-28); offen, ob danach Zeitrennen, Bots (Wegpunkte als leere
  Objekte `waypoint` in Blender) oder Mehrspieler (Listen-Server, siehe VISION.md, Abschnitt
  Netzwerk)?
- [ ] **Assets:** Waffen, Zielscheiben und Items aus CC0-Paketen von Kenney (2026-09-28); offen: Arena und Klänge.
- [x] **Wo lebt das Spiel:** im Repository unter `samples/friendlyfire/` (entschieden 2026-09-27).
- [x] **Name:** Friendly Fire (entschieden 2026-09-30; Arbeitstitel war „Leuchtturm“ — so heißt das Spiel noch in älteren Commits und im DEVLOG).
