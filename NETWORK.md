# BLTZNXT — Netzwerk (Network Update)

Stand: 2026-10-27 · **Umgesetzt** in [bb_net.h](src/compiler/bb_net.h), Nutzer-Doku in
[NETWORKING.md](NETWORKING.md), Tests in [tests/net/](tests/net/). UPnP gebaut und am echten
Router geprüft 2026-10-29, `games.php` auf echtem Webspace geprüft 2026-10-29. Neu: Konzept
IPv6 (Abschnitt „IPv6“, 2026-10-29), Entscheidungen getroffen, Bau steht aus. Release-Plan: als Nächstes kommt das Network Update
(nur Mehrspieler), danach IDE.

Blitz3D hatte zwei Netzwerk-Schichten: rohe TCP- und UDP-Streams (bei uns seit 2026-09-30
vorhanden, [bb_socket.h](src/compiler/bb_socket.h)) und darüber 14 Befehle auf **DirectPlay**
(`StartNetGame` … `NetMsgData$`). DirectPlay gibt es auf heutigen Systemen nur noch als
abgeschaltetes Legacy-Feature. BLTZNXT baut deshalb **die Befehle mit denselben Namen und
demselben Verhalten nach, aber auf eigenem UDP**. Nachgebaut wird das Verhalten, nicht die
Bibliothek. Der größere Rahmen (WebRTC, Treffpunkt, Plugins) steht in [VISION.md](VISION.md),
Abschnitt Netzwerk; dieses Papier ist Phase 1.

## Umsetzung: was anders kam als geplant (2026-10-27)

Der Nutzer hat für die Umsetzung freie Hand gegeben: Blitz3D-Kompatibilität ist hier nicht mehr
Pflicht, sauberes Funktionieren geht vor. Abgleich Konzept gegen Ergebnis:

| Punkt | Konzept | Umgesetzt | Warum |
|---|---|---|---|
| Programmkennung, Vorgabe | Name der Exe | **leer** = gemeinsamer Namensraum aller BLTZNXT-Programme | Mit dem Exe-Namen fänden sich ein Host-Programm und ein *anderes* Client-Programm nie, genau der Fall der Hilfe-Beispiele (`SendNetMsg.bb` hostet, `RecvNetMsg.bb` tritt per `StartNetGame` bei) und vieler alter Server/Client-Paare. DirectPlay ließ ebenfalls alle Blitz-Programme einander sehen. `NetGameApp` grenzt ein |
| `JoinNetGame`, Spielname | genau wie im Original (leerer Name findet nichts) | leerer Name passt auf jedes Spiel, Vergleich ohne Groß-/Kleinschreibung | bequemer, kein altes Programm verlässt sich auf das Scheitern |
| Nachrichtentyp | Byte mit Vorzeichen wie gemessen | ganze Zahl; Typ 0 wird verworfen | die Byte-Kürzung ist ein Artefakt, kein Verhalten, auf das man bauen will |
| Nachricht an eigenen Spieler | kommt nie an (gemessen) | kommt an; an alle (0) weiterhin nicht beim Absender | eine ausdrücklich adressierte Nachricht soll ankommen (geteilter Bildschirm mit mehreren lokalen Spielern) |
| Host-Migration | ältestes Programm; sonst 200 | dazu: antwortet der Kandidat 8 s nicht, übernimmt der nächste | fällt der Kandidat mit dem Host weg, ginge sonst die ganze Sitzung verloren |
| Referenz-Listen-Server | `games.php`, Alternativen beschrieben | `games.php` **und** `listserver.js` (Node.js) | auf dem Entwicklungsrechner gibt es weder PHP noch ein funktionierendes Python; Node ist da und prüft die Schnittstelle in der Testreihe. `games.php` am 2026-10-29 auf echtem Webspace (Apache, HTTPS) geprüft |
| Neuer Befehl | — | `NetGameHost()` | nach einer Migration muss ein Programm fragen können, ob es jetzt Host ist |
| Einstellung | join, server, app, port | dazu `lan = 0` (keine LAN-Suche), nur für Tests `bind`, `drop` | `lan` für reine Internet-Spiele und zum Prüfen des Listen-Servers |
| UPnP | `miniupnpc`, auch NAT-PMP | **eigener kleiner IGD-Client** in `bb_net.h` (2026-10-29): SSDP-Suche, Geräte­beschreibung, SOAP `AddPortMapping`/`DeletePortMapping`/`GetExternalIPAddress`, eigener Thread; Leihfrist 1 h mit Erneuerung, bei Fehler 725 dauerhaft. Kein NAT-PMP | Die Laufzeit steckt als Header in jedem Programm; `miniupnpc` hätte eine vorgebaute Bibliothek für jedes Programm gebraucht. Der benötigte Teil von UPnP sind gut 200 Zeilen. NAT-PMP/PCP bräuchte die Gateway-Adresse (iphlpapi); erst nachziehen, wenn ein Router nur das kann |
| Neue Befehle (UPnP) | — | `NetGameUPnP enable` (Vorgabe an), `NetGameForwarded()` (1/0/−1); Einstellung `upnp = 0/1/URL` | ein reines LAN-Spiel soll den Port nicht ins Internet öffnen müssen; ein Spiel soll zeigen können, ob es von außen erreichbar ist. Die URL überspringt die Suche (Tests gegen `tests/net/fake_igd.js`, Router, die SSDP nicht beantworten). Carrier-NAT (externe Adresse privat oder 100.64/10) zählt als 0 mit Meldung |
| Suche im LAN | Broadcast | Broadcast an 255.255.255.255, an x.y.z.255 jeder eigenen Adresse (/24 angenommen) und an 127.0.0.1 | Netze mit anderer Maske erreicht nur der allgemeine Broadcast |
| HTTPS | WinHTTP / libcurl | WinHTTP unter Windows; anderswo nur http:// über einen eigenen Socket | blitzcc baut heute nur unter Windows |

Gemessen an den Arbeitspaketen: 2 (Zuverlässigkeit) mit 10 000 Nachrichten und 1 MB bei 20 %
Verlust, 3–6 durch die Testfälle in `tests/net/`. Nicht erfüllt: „Hilfe-Beispiele
`SendNetMsg.bb` + `RecvNetMsg.bb` laufen gegeneinander“ — `SendNetMsg.bb` (und
`DeleteNetPlayer.bb`) sind in der Hilfe beschädigt (`player\x` wurde zu `playerx`), auch der
Original-Compiler lehnt sie ab; die übrigen 12 Beispiele übersetzen. „Lobby-Beispiel“: siehe
`samples/network/`. „Zwei Rechner über einen echten Webspace“: zwei Programme auf einem Rechner fanden sich über den Webspace (DEVLOG 2026-10-29); der Beitritt über die öffentliche Adresse scheitert an diesem Anschluss am Carrier-NAT, nicht an der Liste.

## Entscheidungen (2026-10-27)

| # | Entscheidung | Grund |
|---|---|---|
| 1 | Die 14 Befehle behalten Namen, Parameter und Rückgabewerte | Alte Programme kompilieren unverändert |
| 2 | **Kein Dialog.** `StartNetGame()` arbeitet im **Auto-Modus**: kurz im LAN suchen, gefundenes Spiel beitreten (1), sonst selbst hosten (2) | Der Dialog war Win32-Technik von 1999 (Modem, IPX, seriell); der Auto-Modus hält alte Programme im LAN lauffähig |
| 3 | **Neue Befehle für die Spielsuche**, damit man den Beitritts-Dialog selbst in BLTZNXT baut | Wer eine Lobby will, baut sie im Stil seines Spiels |
| 4 | Eigenes Protokoll auf UDP; originale Blitz3D-Exes und BLTZNXT-Exes verbinden sich **nicht** miteinander | Es reicht, wenn zwei BLTZNXT-Programme miteinander reden |
| 5 | Stern über den Host statt Netz jeder mit jedem | Einfacher, NAT-freundlicher, passt zum Listen-Server von [Friendly Fire](FRIENDLY_FIRE.md) |
| 6 | Die Unterschiede zu Blitz3D werden **klar und deutlich dokumentiert** (Nutzer-Doku, Release Notes, Website) | Wer ein altes Projekt mitbringt, soll vorher wissen, was anders ist |
| 7 | Die **Programmkennung** wird auch beim Beitritt geprüft (Handshake), nicht nur bei der Suche | Mit direkter IP oder an einem weitergeleiteten Port soll ein Programm nicht blind einem fremden beitreten |
| 8 | Ein **Schalter von außen** (Umgebungsvariable oder Datei neben der Exe) steuert `StartNetGame()`: feste IP oder Listen-Server | Alte Programme bekommen Internet-Mehrspieler ohne Umbau |
| 9 | **Listen-Server** als zweite Quelle der Spielsuche, mit Referenz-Server zum Selbstbetrieb; dazu UPnP am Router. Hole Punching erst mit dem Treffpunkt (Phase 2) | Bewährtes Prinzip aus Blitz-Zeiten (damals PHP + Datenbank); jeder Entwickler betreibt seinen eigenen, BLTZNXT braucht keinen zentralen Dienst |

## Was der Nutzer wissen muss (Grundlage für die Nutzer-Doku)

- `StartNetGame()` zeigt keinen Dialog. Es sucht etwa eine Sekunde im LAN nach einem Spiel
  **desselben Programms**. Gefunden: beitreten, Rückgabe 1. Nichts gefunden: hosten, Rückgabe 2.
- `HostNetGame` und `JoinNetGame` funktionieren wie im Original. `JoinNetGame(name$, "")` mit
  leerer IP sucht im LAN.
- Für einen eigenen Beitritts-Dialog gibt es neue Befehle (`FindNetGames` und Verwandte, unten).
- Ein Spiel tritt nur einem Host **mit derselben Programmkennung** bei, auch mit direkter IP. Maßgeblich ist
  die Programmkennung (Vorgabe: leer, alle BLTZNXT-Programme sehen einander). Wer sie mit
  `NetGameApp` setzt, findet nur noch Spiele mit derselben Kennung; ein Beitritt bei einer anderen scheitert mit einer Meldung, die beide zeigt.
- Nur TCP/IP-Netze, genauer UDP. Modem, IPX und serielle Verbindungen gibt es nicht mehr.
- LAN und direkte IP gehen sofort. Fürs Internet gibt es einen **Listen-Server**, den man selbst
  betreibt (Referenz liegt bei). Der Host muss für andere erreichbar sein: BLTZNXT bittet den
  Router per UPnP um eine Weiterleitung; wo das nicht geht (manche Router, Carrier-NAT,
  Mobilfunk), braucht es eine Portweiterleitung von Hand.
- Alte Programme bekommen Internet ohne Umbau: Umgebungsvariable oder Datei neben der Exe nennt
  eine feste IP oder einen Listen-Server (Abschnitt „Schalter von außen“).
- Wer ein Spiel über einen Listen-Server öffentlich hostet, macht seine IP für alle sichtbar, die
  die Liste abrufen.
- Ein BLTZNXT-Programm spielt nicht mit einem Programm, das mit dem originalen Blitz3D
  kompiliert wurde.
- Die Windows-Firewall fragt beim ersten Hosten, ob das Programm Verbindungen annehmen darf.

## Die 14 Befehle in BLTZNXT

Die Texte der Original-Hilfe (`F:\dev\Blitz3D\help\commands\2d_commands\*.htm`) sind die Vorlage,
die Messung vom 2026-10-27 (unten, „Messergebnisse“) legt das Verhalten fest. Die Signaturen
stammen aus der Laufzeit des Originals.

| Befehl | Verhalten |
|---|---|
| `StartNetGame()` | Auto-Modus (unten). 0 = fehlgeschlagen, 1 = beigetreten, 2 = Host |
| `HostNetGame(game_name$)` | Öffnet den Port, beantwortet Suchanfragen mit `game_name$`. 2 = Host, 0 = fehlgeschlagen (Port belegt). Läuft schon ein Spiel: Laufzeitfehler „Multiplayer game already started“ |
| `JoinNetGame(game_name$, ip_address$)` | Beitritt beim Host unter `ip_address$`. Leere IP oder `"0.0.0.0"`: im LAN suchen; `"localhost"` geht auch. `game_name$` muss genau passen, ein leerer Name findet nichts. 1 = beigetreten, 0 = fehlgeschlagen |
| `StopNetGame()` | Verlässt das Spiel; die anderen bekommen 101 für jeden lokalen Spieler. Ist man Host, übernimmt ein anderer (Host-Migration). Danach darf man wieder hosten oder beitreten |
| `CreateNetPlayer(name$)` | Neuer lokaler Spieler; der Host vergibt die ID. Alle anderen Programme bekommen 100, das eigene nicht. Mehrere lokale Spieler je Programm sind erlaubt (das Beispiel `SendNetMsg.bb` legt fünf an) |
| `DeleteNetPlayer(player)` | Entfernt einen lokalen Spieler, die anderen bekommen 101. Unbekannte ID: nichts |
| `NetPlayerName$(player)` | Name des Spielers; `"<all>"` für 0, `"<unknown>"` für unbekannte und gelöschte IDs |
| `NetPlayerLocal(player)` | 1, wenn der Spieler zu diesem Programm gehört, sonst 0 (auch für unbekannte IDs) |
| `SendNetMsg type, msg$, from_player [, to_player = 0, reliable = 1]` | `to_player` = 0 an alle; zuverlässig ist die **Vorgabe**. Der Typ geht als Byte mit Vorzeichen über die Leitung (150 kommt als −106 an, 300 als 44), Typ 0 kommt nie an. Nachrichten an eigene Spieler oder an alle erreichen das eigene Programm nicht. Unbekannte `from`- oder `to`-ID: kein Fehler |
| `RecvNetMsg()` | Holt die nächste Nachricht aus der Schlange, 1 wenn eine da war |
| `NetMsgType()` | 1–99 Nutzer, 100 Spieler dazu, 101 Spieler weg, 102 dieses Programm ist jetzt Host, 200 Sitzung verloren. Vor dem ersten `RecvNetMsg` 0 |
| `NetMsgFrom()`, `NetMsgTo()`, `NetMsgData$()` | Felder der zuletzt geholten Nachricht. Bei 100/101 ist `NetMsgFrom` der betroffene Spieler, bei 102 −1; `NetMsgTo` 0, `NetMsgData$` leer |

`RecvNetMsg`, `SendNetMsg` und `CreateNetPlayer` ohne laufendes Spiel (auch nach `StopNetGame`)
sind im Original ein Laufzeitfehler „Multiplayer game not started“. BLTZNXT meldet das wie jeden
Laufzeitfehler auf stderr und beendet das Programm.

**Die Nachrichtenschlange ist das Herzstück.** Nutzer- und Systemnachrichten (100, 101, 102,
200) landen in einer einzigen Schlange in der Reihenfolge ihres Eintreffens; `RecvNetMsg` holt
eine heraus, die `NetMsg…`-Befehle lesen sie aus. Programme sehen so genau das Muster, das sie
aus Blitz3D kennen.

**Spieler-IDs** vergibt der Host. Sie sind nie 0, werden in einer Sitzung nicht wiederverwendet
und sind groß, wie die DPIDs von DirectPlay, damit niemand sie als Index missbraucht.

**`data$`** wird mit Längenangabe übertragen; `Chr(0)` und beliebige Bytes überleben.

### Auto-Modus von `StartNetGame()`

0. Nennt der Schalter von außen eine feste IP (`join`), wird nur dort beigetreten:
   `JoinNetGame("", ip)`, Rückgabe 1 oder 0. Keine Suche, kein Hosten.
1. Suchanfrage per UDP-Broadcast an den Spielport, Antworten etwa 1000 ms sammeln. Ist ein
   Listen-Server eingestellt (Schalter oder `NetGameServer`), wird er gleichzeitig abgefragt.
2. Nur Antworten mit derselben **Programmkennung** zählen (unten), damit ein Spiel nicht
   versehentlich einem anderen BLTZNXT-Programm beitritt.
3. Mindestens ein Spiel gefunden: einem Spiel im LAN den Vorzug geben, sonst dem mit der
   schnellsten Antwort beitreten, Rückgabe 1. Scheitert der Beitritt (Host hinter NAT nicht
   erreichbar), das nächste versuchen.
4. Keins gefunden: nach einer zufälligen Pause von 0–500 ms noch einmal suchen; dann
   `HostNetGame(<Programmname>)`, Rückgabe 2. Mit Listen-Server meldet sich der neue Host dort an.

Die zweite Suche entschärft den Fall, dass zwei Rechner gleichzeitig starten und beide hosten.
Ganz ausschließen lässt er sich nicht; das steht als Grenze in der Nutzer-Doku.

**Programmkennung**: Vorgabe ist leer (gemeinsamer Namensraum, siehe „Umsetzung“), ohne Unterscheidung von Groß- und
Kleinschreibung. `NetGameApp id$` setzt sie ausdrücklich, damit sich nur Programme desselben Spiels
finden. Sie gilt überall: bei der Suche im
LAN, beim Listen-Server (dort ist sie der Namensraum) und beim Beitritt im Handshake, auch mit
direkter IP.

### Schalter von außen

Damit alte Programme ohne Umbau übers Internet spielen und man neue ohne Neukompilieren testen
kann, lesen die Net-Befehle beim ersten Aufruf Einstellungen von außen. Quellen, die erste
gewinnt:

1. Umgebungsvariablen `BLTZNXT_NET_JOIN`, `BLTZNXT_NET_SERVER`, `BLTZNXT_NET_APP`,
   `BLTZNXT_NET_PORT`
2. Datei `<Exe-Name>.net` neben der Exe, eine Einstellung je Zeile:
   ```
   ; Kommentar
   join   = 1.2.3.4:2350          ; StartNetGame tritt nur hier bei
   server = https://example.org/games.php
   app    = ffire
   port   = 2350
   ```
3. Was das Programm selbst setzt (`NetGameServer`, `NetGameApp`, `NetGamePort`)

Von außen gewinnt also vor dem Code: Wer testen oder einen anderen Server nehmen will, muss das
Programm nicht anfassen. `join` wirkt nur auf `StartNetGame()`; `HostNetGame` und `JoinNetGame`
mit IP tun, was der Code sagt. Unbekannte Schlüssel und kaputte Zeilen meldet die Laufzeit auf
stderr und überspringt sie.

## Neue Befehle (NEXT, Vorschlag)

Zum Bauen eines eigenen Beitritts-Dialogs. Die Zählung beginnt bei 1, wie bei `CountHostIPs`
und `HostIP`.

| Befehl | Verhalten |
|---|---|
| `FindNetGames([timeout_ms])` | Sucht im LAN und, falls eingestellt, beim Listen-Server (Vorgabe 1000 ms), merkt sich eine gemeinsame Liste, gibt die Anzahl zurück. Blockiert für die Dauer |
| `NetGameName$(index)` | Name des Spiels, wie beim Hosten angegeben |
| `NetGameIP$(index)` | IP des Hosts als Text mit Port, falls nicht der Vorgabe-Port (`"1.2.3.4:2351"`); passt direkt in `JoinNetGame`. Bei Einträgen vom Listen-Server die öffentliche IP |
| `NetGamePlayers(index)` | Anzahl der Spieler darin |
| `NetGameLAN(index)` | 1, wenn das Spiel im LAN gefunden wurde, 0 vom Listen-Server |
| `NetGameApp id$` | Programmkennung setzen (siehe Auto-Modus) |
| `NetGamePort port` | Spielport ändern, vor dem ersten Net-Befehl (Vorgabe siehe Protokoll) |
| `NetGameServer url$` | Listen-Server einstellen; `""` schaltet ihn ab. Ab dann meldet `HostNetGame` sich dort an und `FindNetGames` fragt ihn mit ab |

`JoinNetGame` versteht deshalb auch `"ip:port"` im zweiten Parameter.

Offen: ob `FindNetGames` später auch eine nicht blockierende Form braucht (Liste füllt sich,
während der Dialog läuft). Für Phase 1 reicht die blockierende.

Mitgeliefert wird ein Beispiel **„Lobby“** (`samples/network/lobby.bb`): Spiele suchen, Liste
anzeigen, beitreten oder hosten, mit `Text` und Maus gezeichnet. Es ist die Vorlage für den
eigenen Dialog.

## Protokoll

Eigenes Protokoll auf **einem UDP-Socket** je Programm.

- **Port**: fester Spielport, Vorgabe 2350 (aus dem alten DirectPlay-Bereich 2300–2400),
  änderbar mit `NetGamePort`. Clients senden von einem freien Port, damit mehrere Programme auf
  einem Rechner laufen (wichtig für Tests).
- **Kopf jedes Pakets**: Kennung `BNX`, Protokollversion, Paketart, Sequenznummer, Bestätigung
  (letzte empfangene Nummer + 32 Bit Bitfeld), Nutzdaten.
- **Paketarten**: `DISCOVER`/`OFFER` (Suche), `JOIN`/`WELCOME`/`REJECT` (Beitritt),
  `PLAYER_ADD`/`PLAYER_DEL`, `MSG`, `ACK`, `PING`/`PONG`, `BYE`, `HOST_CHANGE`.
- **Handshake**: `JOIN` trägt Protokollversion, Programmkennung und `gamename$`. Der Host prüft
  in dieser Reihenfolge und antwortet bei einem Fehler mit `REJECT` und Grund:
  1. Protokollversion verschieden → „protocol“ (verschiedene BLTZNXT-Stände)
  2. Programmkennung verschieden → „app“
  3. `gamename$` verschieden → „name“ (das Original lehnt ab, auch bei leerem Namen; nur der
     Auto-Modus von `StartNetGame()` fragt intern ohne Namen)
  4. Spiel voll oder Host beendet gerade → „closed“

  `JoinNetGame` gibt dann 0 zurück und schreibt den Grund auf stderr, etwa
  `JoinNetGame: host runs 'ffire', this program is 'lobby'` (Fehlerregel: Meldung auf stderr,
  kein Dialog). Kommt gar keine Antwort, wird `JOIN` 3 s lang wiederholt, dann 0 mit
  `JoinNetGame: no answer from 1.2.3.4:2350`. `OFFER` trägt Kennung und Version ebenfalls,
  damit die Suche Fremdes gar nicht erst auflistet.
- **Zuverlässig** (`reliable` = 1): Sequenznummer je Verbindung, Bestätigung, Neusendung nach
  etwa 1,5 × Laufzeit (anfangs 100 ms), Zustellung in Reihenfolge. **Unzuverlässig**: einmal
  senden, keine Reihenfolge. Systemereignisse gehen immer zuverlässig.
- **Größe**: Nachrichten über etwa 1200 Bytes werden zerlegt und wieder zusammengesetzt, bis
  mindestens 1 MB (das Original überträgt 1 MB, zuverlässig wie unzuverlässig). Geht bei einer
  unzuverlässigen Nachricht ein Teil verloren, fällt die ganze Nachricht weg.
- **Lebenszeichen**: `PING` alle 250 ms; eine Gegenstelle gilt nach 5 s ohne Antwort als weg.
  Das Original erkennt einen abgerissenen Host auch nach 30 s nicht und blockiert dabei
  (Messergebnisse); hier ist BLTZNXT bewusst besser.

### Stern über den Host

Jeder Client hat genau eine Verbindung, zum Host. Eine Nachricht an Spieler X oder an alle geht
zum Host, der sie weiterreicht. Für das Programm sieht das aus wie bei DirectPlay. Der Host kennt
die vollständige Spielerliste und schickt sie beim Beitritt mit; Änderungen verteilt er als
`PLAYER_ADD`/`PLAYER_DEL`.

### Host-Migration

Alle kennen die Liste der Programme in Beitrittsreihenfolge, samt IP und Port. Fällt der Host
weg (`BYE` oder Zeitüberschreitung), wird das älteste verbliebene Programm Host. (Im Original
wurde in der einzigen Messung mit zwei Kandidaten das zuletzt beigetretene Host; eine feste Regel
lässt sich daraus nicht ablesen, und kein Programm kann sich darauf verlassen.)
- Es bekommt für jeden Spieler des alten Hosts 101, danach 102 mit `NetMsgFrom` = −1.
- Die anderen verbinden sich neu mit ihm und bekommen ebenfalls 101 für die Spieler des alten
  Hosts.
- Wer den neuen Host nicht erreicht, bekommt 200.

### Ablauf im Programm

Netzwerk-I/O läuft in einem **eigenen Thread**: Empfangen, Bestätigen, Neusenden, Lebenszeichen.
Fertige Nachrichten legt er in die Schlange, `RecvNetMsg` holt sie im Hauptthread ab. So reißt
die Verbindung nicht ab, wenn ein Programm eine Weile nicht `RecvNetMsg` aufruft (Ladebildschirm,
`WaitKey`), wie bei DirectPlay, das ebenfalls eigene Threads hatte.

### Transport-Schnittstelle

Sitzungsschicht und Zuverlässigkeit sprechen nur mit einer kleinen Schnittstelle (Paket an
Adresse senden, Paket empfangen, Broadcast). Heute steckt UDP dahinter, in Phase 2 WebRTC
(VISION.md). Alte Programme bekommen dann Internet-Mehrspieler ohne Änderung.

## Listen-Server

Das Prinzip aus Blitz-Zeiten: Spiele melden sich bei einem Skript im Netz an, das sie in eine
Datenbank schreibt; wer ein Spiel sucht, fragt das Skript nach der Liste. Heute heißt das
Master- oder Listen-Server. Er vermittelt nur, das Spiel selbst läuft weiter direkt zwischen
den Spielern.

### Ablauf

- `HostNetGame` meldet das Spiel an (Kennung, Name, Port, Spielerzahl) und bekommt ein
  Kürzel zurück. Damit schickt es alle 30 s ein Lebenszeichen mit der aktuellen Spielerzahl,
  aus dem I/O-Thread, ohne das Programm aufzuhalten.
- `StopNetGame` meldet ab. Einträge ohne Lebenszeichen löscht der Server nach 90 s; ein
  abgestürzter Host verschwindet so von selbst.
- Bei einer Host-Migration meldet sich der neue Host unter demselben Spielnamen an.
- `FindNetGames` und der Auto-Modus fragen die Liste ab, gefiltert nach Programmkennung.
- Die **öffentliche IP** trägt der Server ein, aus der Absenderadresse der Anfrage. Der Host
  kennt hinter dem Router nur seine interne Adresse.
- Ist der Server nicht erreichbar, läuft alles weiter wie ohne ihn (nur LAN), mit einer Meldung
  auf stderr.

### Schnittstelle (HTTP, Vorschlag)

Einfache HTTP-Anfragen an eine einzige URL, Antwort als Text, eine Zeile je Eintrag, Felder durch
Tabulator getrennt. So einfach, dass man sie auch mit eigenem Blitz-Code über
`OpenTCPStream` sprechen könnte.

| Anfrage | Parameter | Antwort |
|---|---|---|
| `GET ?a=list` | `app` | je Spiel: `name  ip  port  players`; leere Antwort = keine Spiele |
| `POST a=host` | `app`, `name`, `port`, `players`, `v` (Protokollversion) | `ok <kürzel>` oder `err <grund>` |
| `POST a=beat` | `key`, `players` | `ok` oder `err gone` (dann neu anmelden) |
| `POST a=bye` | `key` | `ok` |

Text in den Parametern ist URL-kodiert. Das Kürzel ist zufällig und lang genug, dass niemand
fremde Einträge ändern oder löschen kann. Der Server liefert nur Einträge mit derselben `app`
und derselben Protokollversion.

### Referenz-Server

Jeder Entwickler betreibt seinen eigenen; BLTZNXT betreibt keinen zentralen Dienst (Betrieb,
Missbrauch, Impressum). Mitgeliefert wird `listserver/`:
- `games.php` mit SQLite: läuft auf jedem billigen Webspace, eine Datei hochladen genügt.
- `listserver.js` für Node.js ohne Pakete; beide erfüllen dieselbe Schnittstelle (README daneben).

Schutz gegen Missbrauch im Referenz-Server: Längen begrenzen (Name, Kennung), höchstens wenige
Einträge je IP, Anfragen je IP und Minute begrenzen, alte Einträge löschen.

### Erreichbarkeit: NAT

Früher hatte der Rechner am Modem oft selbst eine öffentliche IP. Heute steht fast jeder hinter
einem Router, manche hinter Carrier-NAT (DS-Lite, Mobilfunk). Die Liste zeigt das Spiel, der
Beitritt scheitert, wenn der Router die Pakete nicht durchlässt. Drei Stufen:

| Stufe | Was | Wann |
|---|---|---|
| 1 | Portweiterleitung von Hand | Immer möglich; steht in der Nutzer-Doku |
| 2 | **UPnP/NAT-PMP**: `HostNetGame` bittet den Router selbst um eine Weiterleitung (Bibliothek `miniupnpc`, BSD-Lizenz), `StopNetGame` gibt sie zurück | **Network-Update**. Hilft bei den meisten Heim-Routern, nicht bei Carrier-NAT |
| 3 | **Hole Punching**: der Server nennt beiden Seiten gleichzeitig die öffentliche UDP-Adresse der anderen, beide senden sofort | Später, mit dem Treffpunkt (VISION.md, Phase 2). Braucht einen kleinen UDP-Dienst; ein PHP-Skript sieht nur HTTP über TCP, nicht die UDP-Adresse |
| 4 | **IPv6**: ohne NAT, nur die Firewall des Routers steht im Weg | Konzept 2026-10-29, Abschnitt „IPv6“. Hilft gerade bei Carrier-NAT, wenn beide Seiten IPv6 haben |

Offen: ob der Listen-Server bei der Anmeldung prüfen soll, ob der Host von außen erreichbar ist,
und das in der Liste vermerkt. Mit PHP allein geht das nur eingeschränkt.

### Was die Laufzeit dafür braucht

- Einen HTTP-Client mit HTTPS, weil viele Webhoster nur noch HTTPS erlauben: unter Windows
  WinHTTP (Teil des Systems), unter Linux libcurl.
- `miniupnpc` für Stufe 2.
- Datenschutz in die Nutzer-Doku: Wer öffentlich hostet, macht seine IP für alle sichtbar, die
  die Liste abrufen; der Betreiber des Listen-Servers speichert IPs.

## IPv6 (Konzept 2026-10-29)

### Warum

IPv4-Adressen sind aufgebraucht. Neue Anschlüsse bekommen immer öfter keine eigene öffentliche
IPv4-Adresse mehr, sondern teilen sich eine beim Anbieter (Carrier-NAT: DS-Lite, Mobilfunk,
viele Glasfaser- und Kabelanschlüsse). Dahinter kann **niemand** hosten: keine Weiterleitung von
Hand, kein UPnP hilft, weil der Router selbst keine öffentliche Adresse hat. Am 2026-10-29 am
Anschluss des Entwicklers gemessen: der Router meldet 100.98.x.x (Carrier-NAT), derselbe
Anschluss hat aber eine öffentliche IPv6-Adresse.

Bei IPv6 hat jeder Rechner eine eigene öffentliche Adresse; es gibt kein NAT. Im Weg steht nur
die Firewall des Routers, die unaufgeforderte Pakete von außen abweist. Damit ist IPv6 für einen
wachsenden Teil der Spieler der einzige Weg, übers Internet zu hosten.

Grenze: Es geht nur, wenn **beide** Seiten IPv6 haben. Wer nur IPv4 hat, erreicht einen Host
hinter Carrier-NAT weiterhin nicht; dafür bleiben Hole Punching und ein Relais (Stufe 3 oben,
VISION.md Phase 2).

### Ausgangslage im Code

IPv4 ist in `bb_net.h` an vielen Stellen fest eingebaut:

| Stelle | Heute |
|---|---|
| Adressen | `sockaddr_in` überall: Peers, Beitritt, Angebote, Vergleich, Text |
| Rechnerliste für die Host-Migration | `bb_NetMachine_::ip` ist ein `uint32_t`; WELCOME und MADD tragen ihn als 4 Byte |
| Sockets | `socket(AF_INET, …)` für Client und Host, `BIND` nimmt nur IPv4 |
| Suche im LAN | Broadcast an 255.255.255.255 und x.y.z.255 |
| Listen-Server | speichert die Absenderadresse der Anmeldung; der Client liest nur IPv4 (`inet_pton(AF_INET, …)`) |
| `NetGameIP$`, `JoinNetGame`, `BLTZNXT_NET_JOIN` | `"ip:port"` – bei IPv6 mehrdeutig |
| UPnP | nur IPv4-Weiterleitung (`WANIPConnection`) |

Kompatibilität ist kein Thema: das Net-Protokoll und das Format des Listen-Servers sind noch in
keinem Release. Sie dürfen sich ändern, ohne dass alte Fassungen mitspielen müssen.

### Plan

**1. Adressen.** Ein eigener Typ `bb_NetAddr_` (Familie, 16 Byte Adresse, Port) ersetzt
`sockaddr_in` in Peers, Angeboten, Beitritt und Rechnerliste. Vergleich, Text und Umwandlung in
die Socket-Adresse an einer Stelle. IPv4 bleibt ein eigener Fall, keine IPv4-in-IPv6-Adressen.

**2. Zwei Sockets je Rolle.** Je ein IPv4- und ein IPv6-Socket für den Client-Verkehr und, beim
Hosten, für den Spielport (derselbe Port in beiden Familien). Der I/O-Thread liest beide. Ein
einziger „Dual-Mode“-Socket wäre kürzer, aber Broadcast über ihn ist unsicher, und die Suche im
LAN braucht Broadcast. Lässt sich der IPv6-Socket nicht öffnen (IPv6 im System aus), läuft alles
wie heute über IPv4, ohne Meldung.

**3. Jeder Rechner nennt alle seine Adressen.** Beim Beitritt schickt ein Programm seine Ports
und seine globale IPv6-Adresse mit; der Host trägt zusätzlich die Absenderadresse ein, wie er
sie sieht. WELCOME und MADD tragen je Rechner eine Liste von Adressen statt einer IPv4-Adresse.
Bei einer Host-Migration versuchen die Clients alle Adressen des Kandidaten. So kommt auch ein
Client nur mit IPv4 zu einem neuen Host, der beides hat, und umgekehrt.

**4. Listen-Server mit zwei Adressen.** Ein Listen-Server, der nur über IPv4 erreichbar ist (wie
der Webspace auf bltznxt.com), sieht die IPv6-Adresse des Hosts nie. Deshalb schickt der Host
sie bei der Anmeldung selbst mit (`ip6=`); die IPv4-Adresse nimmt der Server wie bisher aus der
Verbindung. Kommt die Anmeldung über IPv6, nimmt er die IPv6-Adresse aus der Verbindung und
lässt `ip6=` weg. Die Liste bekommt ein sechstes Feld:
`name<TAB>ip<TAB>port<TAB>players<TAB>token<TAB>ip6`; leere Felder, wo eine Familie fehlt.
`games.php` und `listserver.js` werden gleich geändert.

Missbrauch: Eine angegebene IPv6-Adresse prüft niemand. Ein Host könnte die Adresse eines
Fremden eintragen; Clients schickten dann ein paar JOIN-Pakete dorthin. Das ist wenig Verkehr
und endet nach drei Sekunden. Der Server nimmt nur globale Adressen an (2000::/3), keine aus
dem LAN.

**5. Beitreten mit beiden Familien.** `JoinNetGame` mit einem Spiel aus der Liste schickt JOIN
an alle Adressen des Spiels zugleich, IPv6 zuerst; die erste Antwort gewinnt. Die doppelten
JOINs fängt der Host schon heute über die Nonce ab. `NetGameIP$` gibt die bevorzugte Adresse
zurück: IPv6, wenn das Spiel eine hat und dieser Rechner selbst eine globale IPv6-Adresse hat,
sonst IPv4. Wird sie an `JoinNetGame` gegeben, findet die Laufzeit das Spiel in der zuletzt
gesuchten Liste wieder und nimmt die anderen Adressen dazu.

**6. Schreibweise.** IPv6 mit Port in eckigen Klammern: `[2a03:ed00::1]:2350`; ohne Port auch
ohne Klammern. Gilt für `NetGameIP$`, `JoinNetGame`, `join =` und `BLTZNXT_NET_JOIN`.

**7. Die Firewall des Routers.** Ohne NAT bleibt sie das einzige Hindernis. Drei Wege, in
dieser Reihenfolge:

| Weg | Wie | Grenze |
|---|---|---|
| a. Klopfen über den Listen-Server | Der Client meldet beim Listen-Server „ich will zu Spiel X, von Adresse Y“ (`a=knock`). Der Host fragt die Klopfer ab und schickt jedem ein paar Pakete; das öffnet seine Firewall für genau diese Adresse. Der Client schickt währenddessen JOIN. Weil IPv6 nicht übersetzt wird, ist die Adresse, die der Client meldet, die, von der er sendet | Nur mit Listen-Server. Der Host muss ihn oft fragen (Entscheidung 2), sonst dauert der Beitritt so lange wie das Abfrageintervall |
| b. Lücke per UPnP | Wie die IPv4-Weiterleitung, mit dem Dienst `WANIPv6FirewallControl` (IGD 2): `AddPinhole` beim Hosten, `DeletePinhole` beim Ende, Leihfrist mit Erneuerung | Nicht jeder Router kann es oder hat es an. Ob der Router des Entwicklers (miniupnpd) es kann, zeigt der Versuch |
| c. Von Hand | Freigabe in der Router-Oberfläche, in der Nutzer-Doku beschrieben | Immer möglich, aber umständlich |

Weg a ist der wichtigste: er geht mit jedem Router und ohne Einstellungen, und er ist die
IPv6-Fassung des Hole Punching. Für IPv4 hinter NAT reicht er nicht, dort kennt der
Listen-Server die übersetzte UDP-Adresse des Clients nicht; das bleibt beim Treffpunkt.

**8. Suche im LAN.** Bleibt bei IPv4-Broadcast. Jedes Heimnetz hat IPv4, auch hinter
Carrier-NAT; ein Netz nur mit IPv6 ist im LAN selten. IPv6-Multicast (`ff02::1`) kommt erst
dazu, wenn es jemand braucht.

**9. Datenschutz.** Windows nimmt für ausgehende Verbindungen wechselnde („temporäre")
IPv6-Adressen. Der Host meldet die Adresse, die das System für ausgehende Pakete wählt; sie
verrät wie eine IPv4-Adresse den Anschluss, aber nicht dauerhaft den Rechner. In die Nutzer-Doku.

### Entscheidungen (vom Nutzer übernommen 2026-10-29)

| # | Frage | Entscheidung |
|---|---|---|
| 1 | IPv6 noch ins Network-Update, vor dem Release? | **Ja.** Ohne IPv6 bleibt das Hosten übers Internet für einen wachsenden Teil der Spieler versperrt, und Protokoll und Listenformat lassen sich jetzt noch ohne Rücksicht ändern |
| 2 | Wie oft fragt ein Host den Listen-Server nach Klopfern? | **Alle 3 s.** Ein Beitritt dauert dann höchstens etwa 3 s. Das sind 20 Anfragen je Minute und Host; die Grenze im Referenz-Server (60 je IP und Minute) reicht dafür. Auf einem kleinen Webspace sind auch zehn gleichzeitig gehostete Spiele kein Problem |
| 3 | Weg b (UPnP-Lücke) gleich mitbauen oder erst, wenn Weg a nicht reicht? | **Mitbauen**, wenn der Router des Entwicklers es kann – dann lässt es sich prüfen; sonst weglassen |
| 4 | IPv6-Multicast für die Suche im LAN? | **Nein**, siehe Punkt 8 |

### Prüfstand

- Alle Fälle in `tests/net/` laufen zusätzlich über `::1` (`BLTZNXT_NET_BIND=::1`).
- Neue Fälle: gemischt (ein Client nur IPv4, einer nur IPv6, Host mit beidem), Host-Migration
  von einem IPv6-Host zu einem Client, der nur über IPv4 zu erreichen ist, Beitritt mit beiden
  Adressen (die erste Antwort gewinnt, kein doppelter Spieler), Schreibweise `[…]:port`.
- Listen-Server: Anmeldung mit `ip6=`, Feld 6 in der Liste, `a=knock` und Abfrage durch den
  Host, Ablehnung nicht globaler Adressen; gegen `listserver.js` in der Testreihe und von Hand
  gegen `games.php` auf bltznxt.com.
- UPnP-Lücke gegen `tests/net/fake_igd.js` (dort `WANIPv6FirewallControl` nachbauen).
- Echter Beweis: zwei Anschlüsse mit IPv6. Der Entwicklerrechner hinter Carrier-NAT hostet, der
  zweite tritt bei – etwa ein Laptop am Hotspot eines Mobiltelefons (Mobilfunk hat meist
  IPv6) oder der externe Tester. Vorher fragen: der Test öffnet die Firewall des Routers.

### Arbeitspakete IPv6

| # | Paket | Fertig, wenn |
|---|---|---|
| 6.1 | Adresstyp, zwei Sockets je Rolle, `BIND` mit IPv6, Schreibweise `[…]:port` | Alle bisherigen `tests/net`-Fälle grün über `127.0.0.1` und über `::1` |
| 6.2 | Adresslisten je Rechner in JOIN, WELCOME, MADD; Migration über alle Adressen | Gemischter Fall und Migration über die Familie hinweg grün |
| 6.3 | Listen-Server: `ip6=`, Feld 6, Beitritt an alle Adressen, `NetGameIP$` bevorzugt IPv6 | Tests gegen `listserver.js` grün; `games.php` auf bltznxt.com zeigt Feld 6 |
| 6.4 | Klopfen (`a=knock`, Abfrage durch den Host, Pakete an den Klopfer) | Test: Host, dessen Pakete von Fremden die Testumgebung verwirft, bis er geklopft hat |
| 6.5 | UPnP-Lücke (`AddPinhole`/`DeletePinhole`), nur wenn Entscheidung 3 so fällt | Test gegen nachgebauten Router; Versuch am echten Router |
| 6.6 | Doku: NETWORKING.md (IPv6, Schreibweise, Firewall, Datenschutz), README des Listen-Servers | Unterschiede an allen Stellen gleich beschrieben |
| 6.7 | Echter Versuch über zwei Anschlüsse | Ein Rechner hinter Carrier-NAT hostet, ein anderer Anschluss tritt über IPv6 bei |

Für 6.4 braucht der Prüfstand eine nachgebaute Firewall: ein Testschalter im Laufzeitsystem
(wie `DROP`), der Pakete von Adressen verwirft, an die das Programm noch nichts geschickt hat.

## Messen am Original

Gemessen am 2026-10-27 mit Blitz3D 1.108 auf einem Rechner über 127.0.0.1. Messprogramme in
[tests/network/original/](tests/network/original/), Ablauf mit
`bash scripts/measure_network.sh [pfad/zu/Blitz3D] [szenario …]` (etwa 5 Minuten, öffnet
Fenster), Ergebnis in `build/netmess/ergebnis.txt`. **Voraussetzung**: Das Windows-Feature
DirectPlay (Legacykomponenten) ist eingeschaltet.

Fragen:
1. Bekommt der Host beim eigenen `CreateNetPlayer` eine 100? Bekommen neue Spieler 100 für die
   schon vorhandenen?
2. Kommt `SendNetMsg` an einen lokalen Spieler oder an alle (`to` = 0) beim Absender selbst an?
3. Was liefern `NetMsgFrom`, `NetMsgTo`, `NetMsgData$` bei 100/101/102/200?
4. `JoinNetGame` mit falschem `gamename$`, mit leerer IP, mit `"0.0.0.0"` (so steht es im
   Hilfe-Beispiel)?
5. Was passiert bei `SendNetMsg` ohne `CreateNetPlayer`, mit fremder `from`-ID, mit Typ 0 oder
   über 99?
6. `NetPlayerName$` und `NetPlayerLocal` mit unbekannter ID: Leerstring/0 oder Laufzeitfehler?
7. Größte Nachricht; Verhalten bei sehr vielen Nachrichten pro Frame.
8. Host-Migration: wer wird Host, was bekommen die anderen, wie lange dauert es?
9. Wie schnell erkennt DirectPlay einen abgerissenen Spieler (Kabel ziehen, Prozess beenden)?
10. Was bekommt man nach `StopNetGame`, und darf man danach erneut hosten oder beitreten?

### Messergebnisse (2026-10-27)

| # | Ergebnis |
|---|---|
| 1 | Der Host bekommt keine 100 für eigene Spieler. Ein Beitretender bekommt **direkt nach `JoinNetGame`, noch vor `CreateNetPlayer`**, je eine 100 für jeden vorhandenen Spieler, für eigene Spieler keine |
| 2 | Nein. Weder lokal an lokal (H1 → H2) noch an alle kommt beim eigenen Programm etwas an |
| 3 | 100/101: `NetMsgFrom` = betroffener Spieler, `NetMsgTo` = 0, `NetMsgData$` leer. 102: `NetMsgFrom` = −1. 200 trat nicht auf. `NetPlayerName$` eines gegangenen Spielers: `"<unknown>"` |
| 4 | Falscher oder leerer Name: 0 nach etwa 8,4 s. `"localhost"`, `"0.0.0.0"` und leere IP treten bei (die beiden letzten suchen im LAN). Kein Dialog |
| 5 | Ohne Spieler, mit fremder `from`- oder unbekannter `to`-ID: kein Fehler. Typ = Byte mit Vorzeichen (−1 → −1, 150 → −106, 200 → −56, 255 → −1, 300 → 44); Typ 0 kommt nicht an; selbst gesendete 100–102 kommen als solche an, mit Daten. Ohne Spiel: Laufzeitfehler „Multiplayer game not started“ |
| 6 | `NetPlayerName$`: `"<unknown>"` für unbekannte, `"<all>"` für 0. `NetPlayerLocal`: 0. Kein Fehler |
| 7 | 1 MB kommt an, zuverlässig wie unzuverlässig (dauert über Loopback 2–3 s). 300 unzuverlässige und 300 zuverlässige Nachrichten ohne Pause: alle da, in Reihenfolge |
| 8 | Host verlässt per `StopNetGame`: die anderen bekommen sofort 101 für seine Spieler, der neue Host danach 102. Neuer Host wurde der zuletzt beigetretene (eine Messung). Nachrichten fließen danach weiter |
| 9 | Host-Prozess hart beendet: **keine** 101 und keine 102 innerhalb von 17 s; die Clients hängen in DirectPlay-Aufrufen bis zu 16 s, `StopNetGame` hängt danach ganz |
| 10 | Nach `StopNetGame` erneut beitreten geht (alle 100 kommen wieder). `RecvNetMsg` nach `StopNetGame`: Laufzeitfehler. `HostNetGame` bei laufendem Spiel: „Multiplayer game already started“ |

Dazu: `HostNetGame` braucht 0,5–0,8 s, `JoinNetGame` 5,7–6,2 s auch über Loopback, `StopNetGame`
etwa 3 s. BLTZNXT darf überall schneller sein. Spieler-IDs sind groß (rund 219 Millionen) und
werden in einer Sitzung nach unten vergeben. Der Dialog von `StartNetGame` gehörte zu Blitz
selbst („Please enter a name for the new game“, „<no games found>“), nur die IP-Abfrage kam
von DirectPlay („DirectPlay Request“).

Nicht gemessen: Groß-/Kleinschreibung beim Spielnamen, `DeleteNetPlayer` mit der ID eines
fremden Spielers, Host-Wahl mit mehr Kandidaten, echtes LAN mit zwei Rechnern.

## Prüfstand

- Die 14 Hilfe-Beispiele des Originals (`help\commands\2d_examples\*.bb`). 11 davon rufen
  `StartNetGame()` auf. Paare: `SendNetMsg.bb` hostet (fünf lokale Spieler, Rundsendungen),
  `RecvNetMsg.bb`/`NetMsgType.bb` treten per `StartNetGame()` bei und zeigen die Nachrichten.
  Genau dieses Paar muss im Auto-Modus ohne Änderung zusammen laufen.
- Automatische Tests in `tests/` mit zwei oder mehr Prozessen auf einem Rechner (Loopback):
  Beitritt, Spieler dazu/weg, zuverlässige Reihenfolge unter künstlichem Verlust, Zerlegung
  großer Nachrichten, Host-Migration, Auto-Modus mit und ohne vorhandenen Host,
  Programmkennung trennt zwei Programme, `REJECT` mit jedem Grund und Meldung auf stderr,
  Schalter von außen (Umgebungsvariable schlägt Datei schlägt Code), `JoinNetGame` mit
  `"ip:port"`.
- Listen-Server: der Referenz-Server lokal gestartet; Anmelden, Lebenszeichen, Ablaufen nach
  90 s, Abmelden, Filter nach Kennung und Version, gemeinsame Liste aus LAN und Server,
  Verhalten bei nicht erreichbarem Server. UPnP lässt sich nur am echten Router prüfen; das
  geschieht von Hand und steht im DEVLOG.
- Ein Testschalter im Laufzeitsystem verwirft einen einstellbaren Anteil der Pakete, damit
  Verlust ohne echtes Netz geprüft wird.

## Arbeitspakete

| # | Paket | Fertig, wenn |
|---|---|---|
| 1 | ~~Messen am Original~~ erledigt 2026-10-27 | Fragen beantwortet (Messergebnisse), DEVLOG |
| 2 | ~~Transport + Zuverlässigkeit (`bb_net.h`), I/O-Thread, Verlust-Schalter~~ erledigt | Test: 10 000 zuverlässige Nachrichten bei 20 % Verlust kommen vollständig und in Reihenfolge an |
| 3 | ~~Sitzung: Host, Join, Spielerliste, Nachrichtenschlange, die 14 Befehle~~ erledigt (Hilfe-Beispiel siehe „Umsetzung“) | Hilfe-Beispiele `SendNetMsg.bb` + `RecvNetMsg.bb` laufen gegeneinander |
| 4 | ~~Spielsuche im LAN, Auto-Modus, Handshake mit Kennung, Schalter von außen, neue Befehle~~ erledigt | Auto-Modus- und `REJECT`-Tests grün; Lobby-Beispiel läuft |
| 5 | ~~Host-Migration, Abbruch, 200~~ erledigt | Test: Host beenden, ein Client wird Host, Spiel geht weiter |
| 6 | ~~Listen-Server~~ erledigt, `games.php` auf echtem Webspace geprüft 2026-10-29: HTTP-Client (WinHTTP/libcurl), Anmelden/Lebenszeichen/Abmelden, `NetGameServer`, Referenz `listserver/games.php` | Tests gegen den lokalen Referenz-Server grün; zwei Rechner finden sich über einen echten Webspace |
| 7 | ~~UPnP, eigener IGD-Client~~ erledigt 2026-10-29: Tests gegen nachgebauten Router grün, am echten Router (miniupnpd) angelegt und wieder gelöscht, Carrier-NAT erkannt (DEVLOG) | Am echten Router: Weiterleitung entsteht beim Hosten und verschwindet beim Beenden (`samples/network/upnpcheck.bb`) |
| 8 | ~~Doku~~ erledigt bis auf Release Notes und Website: Nutzer-Doku „Netzwerk – Unterschiede zu Blitz3D“ (inkl. Listen-Server, NAT, Datenschutz), KNOWN_ISSUES, roadmap, Release Notes, Website | Unterschiede an allen Stellen gleich beschrieben |
| 9 | ~~Erstes Spiel auf den Net-Befehlen~~ erledigt 2026-10-29: **BLTZCRFT** statt Friendly Fire (Entscheidung des Nutzers: einfacher, man sieht nur die anderen, dazu Chat und gemeinsame Blöcke), `samples/bltzcrft/mehrspieler.bb` | Zwei Programme: Beitritt mit der Welt des Hosts, Figur mit Namen, Chat, Block kommt bei beiden an (`bltzcrft.exe -test`) |
| 10 | Friendly Fire: Mehrspieler auf den Net-Befehlen oder eigenem UDP (Entscheidung offen, nach BLTZCRFT) | Siehe FRIENDLY_FIRE.md, Abschnitt Mehrspieler |
