# BLTZNXT — Vision

Stand: 2026-09-26

Der Wert von BLTZNXT liegt in der Kompatibilität, nicht in PBR: zwanzig Jahre Blitz3D-Code
laufen wieder auf Maschinen ohne DX7. Das kann keine andere Engine nachbauen, und das NEXT
sollte diese Position ausbauen statt sie zu verlassen.

## Die Wette

Der Burggraben ist Phase 1, nicht Phase 2. PBR gibt es 2026 überall: Godot, Unity, Raylib,
three.js — niemand wechselt die Engine wegen eines Cook-Torrance-Shaders.

Was nur BLTZNXT hat: Blitz3D-Code, Tutorials, Forenschnipsel und fertige Spiele aus zwanzig
Jahren laufen unverändert. Diese Position kann sich niemand mehr erarbeiten, weil niemand mehr
bereit ist, x87-Rundung, D3D7-Füllregeln und DirectDraw-Timing nachzumessen. Genau das ist die
Arbeit der letzten Monate.

Daraus folgt die Rahmung des NEXT: nicht „BLTZNXT wird auch eine moderne Engine", sondern
**„dein alter Code läuft weiter, und du kannst ihn Datei für Datei modernisieren"**. Das ist ein
Versprechen, das keine andere Engine geben kann.

## Phase 1: wann ist „fertig"?

„Fertig" braucht ein Kriterium, das kein Bugzähler ist. 46 offene Punkte klingen nach Ziellinie,
aber jedes neue Programm bringt neue Funde — blox-n-balls allein hat ein Dutzend geliefert, die
MD2-Arbeit vier weitere.

Besser ein Korpus: *diese* Installationsdemos und *diese* fünf Community-Spiele laufen
pixelvergleichbar durch. Das ist Phase 1, fertig, eingefroren. Alles danach ist Pflege, nicht
Projektziel.

Eine Lücke steht dem Anspruch „jeder existierende Blitz3D-Code" noch im Weg, eine zweite ist
bewusst herausgeschnitten:

| Punkt | Stand | Entscheidung |
|---|---|---|
| Zielplattformen | Der Compiler hängt an der Windows-API (WEAK-24), `build_linux.sh` baut nicht | Offen: wenn „aktuelle Systeme" Linux, macOS oder den Browser einschließt, ist das Phase 1 und nicht NEXT |
| Userlibs (`.decls`) | Nicht unterstützt, `userlibs/` bleibt leer | Entschieden am 2026-09-22: harter Schnitt, Ersatz im NEXT — siehe unten |

## Userlibs: Schnitt jetzt, Neubau im NEXT

Das alte Userlib-System wird nicht nachgebaut. Blitz3D erweitert seinen Befehlssatz über
32-Bit-Windows-DLLs, die in `userlibs/*.decls` deklariert werden; die Schnittstelle schreibt
`_stdcall` vor und übergibt rohe Adressen von Banks und Objects, ohne Größe und ohne Typ.

Der Grund für den Schnitt ist inhaltlich, nicht technisch. Userlibs dienten vor allem dazu,
Blitz3D beizubringen, was es nicht konnte — bis hin zu einer kompletten Ogre3D-Anbindung. Genau
diese Fähigkeiten baut NEXT selbst ein. Der Anwendungsfall entfällt damit, und mit ihm der Grund,
eine Windows-only-32-Bit-Schnittstelle in eine Engine zu tragen, die auf drei Plattformen laufen
soll.

Machbar wäre es gewesen: ein 32-Bit-Helferprozess mit einer Arena, die beide Prozesse an derselben
Adresse einblenden, hätte den Zeigervertrag erfüllt, ohne BLTZNXT selbst auf 32-Bit umzustellen.
Der Preis wäre eine Windows-Sonderkonstruktion mit Restfällen gewesen — prozesslokale
GDI-Kontexte, Fenster-Subclassing — für einen Zweck, der wegfällt.

Programme, die `.decls` benutzen, laufen also nicht. Das ist bewusst in Kauf genommen.

Der Ersatz im NEXT hat andere Anforderungen:

- Läuft auf Windows, Linux und macOS: `.dll`, `.so`, `.dylib` hinter einem gemeinsamen Lader.
- Stabiles **C-ABI** statt Aufrufkonventions-Folklore. Auf 64-Bit gibt es je Plattform genau eine
  Konvention — `_stdcall` und dekorierte Namen verschwinden damit ersatzlos.
- Geprüfte Übergaben statt roher Adressen: Banks mit Länge, keine Object-Zeiger.
- Das Plugin meldet sich selbst an, mit Versionskennung, statt in einer Textdatei daneben deklariert
  zu werden — eine veraltete Erweiterung wird erkannt, statt abzustürzen.

## Netzwerk: Mehrspieler ohne eigenen Server (Entwurf, 2026-09-26)

Ziel: Online-Spiele, für die der Entwickler keinen Spielserver betreibt. „Ganz ohne Server" geht
dabei im Internet praktisch nie — zwei Rechner hinter Routern (NAT) finden sich nicht von selbst.
Es braucht immer einen **Treffpunkt**, über den sich die Spieler zum ersten Mal erreichen
(Signaling), und für einen Teil der Anschlüsse einen **Umweg** über ein Relay. „Serverless" heißt
hier deshalb: kein eigener Spielserver. Das Spiel läuft zwischen den Spielern, der Treffpunkt ist
klein oder wird von anderen betrieben.

### Ausgangslage

TCP ist vorhanden (`OpenTCPStream`, `CreateTCPServer` …). Es fehlen alle UDP-Befehle und das
DirectPlay-Set von Blitz3D (`StartNetGame`, `HostNetGame`, `JoinNetGame`, `CreateNetPlayer`,
`SendNetMsg`, `RecvNetMsg`, `NetMsgType` …, siehe [KNOWN_ISSUES.md](KNOWN_ISSUES.md)). Genau
dieses Set war in Blitz3D die Mehrspieler-Schicht, und es hatte schon das, was ein Spiel ohne
Server braucht: ein Spieler hostet, die anderen treten bei, Spieler kommen und gehen als
Nachrichten, und fällt der Host weg, übernimmt ein anderer (Host-Migration).

### Die Wege

| Weg | Was | Stärke | Grenze |
|---|---|---|---|
| **UDP + DirectPlay nachbauen** | Die alten Befehle auf eigenem UDP, zuverlässige und schnelle Nachrichten | Alte Mehrspieler-Programme laufen wieder; LAN und direkte IP sofort | Übers Internet scheitert es an NAT, solange kein Treffpunkt dazukommt |
| **WebRTC-Datenkanäle** | Peer-to-Peer mit ICE/STUN, TURN als Umweg; nativ z. B. über `libdatachannel` (C++, MPL-2.0) | Der Standardweg durch NAT; derselbe Transport läuft im Browser | Für einen Teil der Verbindungen ist TURN nötig — das kostet Bandbreite und braucht einen Betreiber |
| **Plattform-Dienste** | Steam Networking Sockets, Epic Online Services (P2P mit Relay und Lobbys) | NAT ist gelöst, nichts selbst zu betreiben | Bindung an Anbieter, SDK und Konto — gehört ins Plugin-System, nicht in den Kern |
| **Serverless im Cloud-Sinn** | Ein Raum je Spiel bei Cloudflare Durable Objects, AWS Lambda o. ä., per WebSocket | Robust gegen NAT und Cheating, kein eigener Betrieb | Technisch Client-Server, und nicht kostenlos |

Der Treffpunkt für WebRTC lässt sich austauschbar machen: LAN-Broadcast, ein Beitrittscode, den
die Spieler austauschen, öffentliche Infrastruktur als Briefkasten (BitTorrent-Tracker,
Nostr-Relays, MQTT-Broker — so arbeitet die JS-Bibliothek Trystero) oder ein eigener kleiner
Signaling-Dienst. STUN-Server gibt es öffentlich; TURN bleibt eine Einstellung, die ein Spiel
setzen kann, aber nicht muss.

### Vorschlag

1. **Phase 1: UDP und DirectPlay nachbauen.** Das ist Kompatibilität, nicht NEXT — alte Programme
   brauchen genau diese Befehle. Vorher am Original messen: Nachrichtentypen und ihre Reihenfolge,
   was beim Beitreten und Verlassen ankommt, Host-Wechsel, Verhalten bei Verbindungsabbruch.
   DirectPlay selbst gibt es auf aktuellen Systemen nicht mehr; nachgebaut wird das Verhalten,
   nicht die Bibliothek.
2. **NEXT: WebRTC als Transport darunter.** Die DirectPlay-Schicht und eine neue, einfache API
   benutzen denselben Weg; alte Programme bekommen Internet-Mehrspieler ohne Änderung, sobald
   ein Treffpunkt eingestellt ist. Im Browser ist es derselbe Transport.
3. **Plugins für Steam und EOS**, sobald das neue Plugin-System steht.

Eine neue API im Stil von Blitz könnte so aussehen (Skizze, nichts davon festgelegt):

```blitzbasic
room = NetOpen("meinspiel", "raum42")   ; hosten oder beitreten, über einen Code
NetSend room, player, bank, reliable     ; zuverlässig oder schnell
While NetRecv(room)
  Select NetEvent(room)
    Case NET_JOIN  : ...
    Case NET_LEAVE : ...
    Case NET_DATA  : ...
  End Select
Wend
```

Dazu gehören zwei Kanalarten (zuverlässig und geordnet, schnell und verlustbehaftet), Ereignisse
für kommende und gehende Spieler, Host-Migration und optional Hilfen für Lockstep oder
Zustandsabgleich. Offen bleibt bei jedem Peer-to-Peer-Modell das Cheating: ohne Server hilft nur
Vertrauen oder gegenseitige Prüfung.

## Die Naht zwischen alt und neu

PBR ist doch ein Architekturthema — nur nicht dort, wo [ROADMAP3D.md](ROADMAP3D.md) es verortet.
Dort steht: „PBR ist kein Architekturthema, nur ein anderes Lighting-Modell im Shader." Für den
Shader stimmt das, für das Ganze nicht.

Vernünftiges PBR braucht lineares Rechnen, IBL und einen Tonemapper. Ohne die drei sieht es mit
acht Punktlichtern und ohne Schatten **schlechter** aus als die jetzige feste Pipeline. Mit ihnen
ändern sich genau die Zahlen, die die Suite festnagelt: BUG-66, BUG-91, BUG-92 und WEAK-25 sichern
gemessene D3D7-Helligkeiten per `ReadPixel`.

Der Farbraum ist also die Naht, an der alt und neu getrennt gehören — nicht das Renderer-Backend.

Und die Trennung gehört **nicht pro Entity**, auch wenn `EntityMetallic` verlockend klingt. Licht,
Ambient, Tonemapping und die Halbpixel-Verschiebung sind Eigenschaften der Szene, nicht des
Objekts; gemischt in einem Bild ergeben sie Resultate, die niemand erklären kann. Besser pro
Programm oder pro Kamera: ein Befehl, mit dem ein Programm einmal den modernen Pfad wählt — ab da
lineare Farben, PBR-Materialien, kein D3D7-Clamp, keine 1/2 − 1/256-Verschiebung. Alte Programme
sagen ihn nie und merken nichts, und die Suite bleibt in zwei sauber getrennte Korpora teilbar.

## Reihenfolge für NEXT

Die Liste GLTF/PBR/Shader ist richtig, aber in dieser Reihenfolge falsch sortiert.

| Schritt | Was | Warum hier |
|---|---|---|
| 1 | Farbraum und Tonemapping als Schalter | Unsichtbar, aber Voraussetzung für alles Weitere. Legt die Naht fest, bevor Features daran hängen |
| 2 | Materialmodell und IBL | Das eigentliche PBR. Mit vorhandenen Meshes testbar, ohne neues Dateiformat |
| 3 | glTF | Der Behälter, der genau diese Materialien transportiert. Vorher gebaut lädt man Modelle, die man nicht korrekt schattieren kann |
| 4 | Eigene Shader | Permanente API-Festlegung — zuletzt, mit Bedacht |
| 5 | Neues Userlib-System | Plattformübergreifend statt Windows-only. Nach den Shadern, weil eine Erweiterung dieselben Materialien ansprechen können soll wie die Engine |
| — | Netzwerk über WebRTC (Entwurf) | Unabhängig von der Grafik-Reihe; setzt den DirectPlay-Nachbau aus Phase 1 voraus. Steam/EOS erst nach Schritt 5 |

GLSL als API freizugeben legt BLTZNXT auf OpenGL fest, solange es das Projekt gibt: kein Vulkan,
kein WebGPU, kein Metal. Das darf eine Entscheidung sein, aber eine bewusste.

## Offene Entscheidungen

Vier Fragen stehen vor dem ersten Schritt, eine fünfte vor dem Netzwerk. Keine davon ist technisch schwer, alle färben auf Jahre
ab.

- [ ] Welcher Korpus definiert „Phase 1 fertig"? Welche Demos, welche Spiele?
- [ ] Was heißt „aktuelle Systeme": Windows allein, oder auch Linux, macOS, Browser?
- [ ] Wo liegt der Schalter zwischen altem und modernem Pfad — pro Programm oder pro Kamera?
- [ ] Wird GLSL die Shader-API, mit der Festlegung auf OpenGL?
- [ ] Netzwerk (Entwurf oben): welcher Treffpunkt ist die Vorgabe, und wer betreibt TURN — das
  Projekt, der Spieleentwickler oder niemand?

Eine weitere ist entschieden: Userlibs gehören nicht zum Anspruch (2026-09-22, siehe oben).

Ein Gedanke zum Schluss, weil er in der Liste fehlt: Wäre es meine Engine, stünde **vor** PBR der
Browser. BLTZNXT ist ein Transpiler nach C++, Emscripten ist damit näher, als es aussieht — und
„dein Blitz3D-Spiel von 2004 läuft in einem Link" schlägt jeden PBR-Screenshot.
