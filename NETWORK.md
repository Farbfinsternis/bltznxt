# BLTZNXT — Netzwerk (Network Update)

Stand: 2026-10-27 · Konzept, noch kein Code. Ergänzt: Handshake mit Programmkennung, Schalter
von außen, Listen-Server. Release-Plan: als Nächstes kommt das Network Update
(nur Mehrspieler), danach IDE.

Blitz3D hatte zwei Netzwerk-Schichten: rohe TCP- und UDP-Streams (bei uns seit 2026-09-30
vorhanden, [bb_socket.h](src/compiler/bb_socket.h)) und darüber 14 Befehle auf **DirectPlay**
(`StartNetGame` … `NetMsgData$`). DirectPlay gibt es auf heutigen Systemen nur noch als
abgeschaltetes Legacy-Feature. BLTZNXT baut deshalb **die Befehle mit denselben Namen und
demselben Verhalten nach, aber auf eigenem UDP**. Nachgebaut wird das Verhalten, nicht die
Bibliothek. Der größere Rahmen (WebRTC, Treffpunkt, Plugins) steht in [VISION.md](VISION.md),
Abschnitt Netzwerk; dieses Papier ist Phase 1.

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
- Ein Spiel tritt nur einem Host **desselben Programms** bei, auch mit direkter IP. Maßgeblich ist
  die Programmkennung (Vorgabe: Name der Exe). Wer seine Exe umbenennt, setzt sie mit
  `NetGameApp`; sonst scheitert der Beitritt mit einer Meldung, die beide Namen zeigt.
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

Die Texte der Original-Hilfe (`F:\dev\Blitz3D\help\commands\2d_commands\*.htm`) sind die Vorlage.
Was dort offen bleibt, steht unter „Messen am Original“.

| Befehl | Verhalten |
|---|---|
| `StartNetGame()` | Auto-Modus (unten). 0 = fehlgeschlagen, 1 = beigetreten, 2 = Host |
| `HostNetGame(gamename$)` | Öffnet den Port, beantwortet Suchanfragen mit `gamename$`. 2 = Host, 0 = fehlgeschlagen (Port belegt) |
| `JoinNetGame(gamename$, serverIP$)` | Beitritt beim Host unter `serverIP$`. Leere IP: im LAN nach `gamename$` suchen. 1 = beigetreten, 0 = fehlgeschlagen |
| `StopNetGame()` | Verlässt das Spiel; alle lokalen Spieler gehen (101 bei den anderen). Ist man Host, übernimmt ein anderer (Host-Migration) |
| `CreateNetPlayer(name$)` | Neuer lokaler Spieler; der Host vergibt die ID. Alle anderen bekommen 100. Mehrere lokale Spieler je Programm sind erlaubt (das Beispiel `SendNetMsg.bb` legt fünf an) |
| `DeleteNetPlayer(id)` | Entfernt einen lokalen Spieler, die anderen bekommen 101 |
| `NetPlayerName$(id)` | Name des Spielers |
| `NetPlayerLocal(id)` | 1, wenn der Spieler zu diesem Programm gehört |
| `SendNetMsg type, data$, from, to [, reliable]` | Typ 1–99; `to` = 0 an alle; `reliable` = zugestellt und in Reihenfolge |
| `RecvNetMsg()` | Holt die nächste Nachricht aus der Schlange, 1 wenn eine da war |
| `NetMsgType()` | 1–99 Nutzer, 100 Spieler dazu, 101 Spieler weg, 102 dieses Programm ist jetzt Host, 200 Sitzung verloren |
| `NetMsgFrom()`, `NetMsgTo()`, `NetMsgData$()` | Felder der zuletzt geholten Nachricht |

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

**Programmkennung**: Vorgabe ist der Name der Exe ohne Endung, ohne Unterscheidung von Groß- und
Kleinschreibung. Ein neuer Befehl `NetGameApp id$` setzt sie ausdrücklich, für umbenannte Exes
oder für verschiedene Programme, die zusammen spielen sollen. Sie gilt überall: bei der Suche im
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
  3. `gamename$` verschieden (nur wenn nicht leer) → „name“; ob das Original hier ablehnt, ist
     Messfrage 4
  4. Spiel voll oder Host beendet gerade → „closed“

  `JoinNetGame` gibt dann 0 zurück und schreibt den Grund auf stderr, etwa
  `JoinNetGame: host runs 'ffire', this program is 'lobby'` (Fehlerregel: Meldung auf stderr,
  kein Dialog). Kommt gar keine Antwort, wird `JOIN` 3 s lang wiederholt, dann 0 mit
  `JoinNetGame: no answer from 1.2.3.4:2350`. `OFFER` trägt Kennung und Version ebenfalls,
  damit die Suche Fremdes gar nicht erst auflistet.
- **Zuverlässig** (`reliable` = 1): Sequenznummer je Verbindung, Bestätigung, Neusendung nach
  etwa 1,5 × Laufzeit (anfangs 100 ms), Zustellung in Reihenfolge. **Unzuverlässig**: einmal
  senden, keine Reihenfolge. Systemereignisse gehen immer zuverlässig.
- **Größe**: Nachrichten über etwa 1200 Bytes werden zerlegt und wieder zusammengesetzt. Geht
  bei einer unzuverlässigen Nachricht ein Teil verloren, fällt die ganze Nachricht weg.
- **Lebenszeichen**: `PING` alle 250 ms; eine Gegenstelle gilt nach 5 s ohne Antwort als weg
  (Wert vorläufig, siehe Messen).

### Stern über den Host

Jeder Client hat genau eine Verbindung, zum Host. Eine Nachricht an Spieler X oder an alle geht
zum Host, der sie weiterreicht. Für das Programm sieht das aus wie bei DirectPlay. Der Host kennt
die vollständige Spielerliste und schickt sie beim Beitritt mit; Änderungen verteilt er als
`PLAYER_ADD`/`PLAYER_DEL`.

### Host-Migration

Alle kennen die Liste der Programme in Beitrittsreihenfolge, samt IP und Port. Fällt der Host
weg (`BYE` oder Zeitüberschreitung), wird das älteste verbliebene Programm Host:
- Es bekommt 102 und für jeden Spieler des alten Hosts 101.
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
Missbrauch, Impressum). Mitgeliefert wird `tools/listserver/`:
- `games.php` mit SQLite: läuft auf jedem billigen Webspace, eine Datei hochladen genügt.
- Kurze Anleitung, auch für Alternativen (Cloudflare Worker, kleines Python- oder Go-Programm),
  die nur die Schnittstelle oben erfüllen müssen.

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

Offen: ob der Listen-Server bei der Anmeldung prüfen soll, ob der Host von außen erreichbar ist,
und das in der Liste vermerkt. Mit PHP allein geht das nur eingeschränkt.

### Was die Laufzeit dafür braucht

- Einen HTTP-Client mit HTTPS, weil viele Webhoster nur noch HTTPS erlauben: unter Windows
  WinHTTP (Teil des Systems), unter Linux libcurl.
- `miniupnpc` für Stufe 2.
- Datenschutz in die Nutzer-Doku: Wer öffentlich hostet, macht seine IP für alle sichtbar, die
  die Liste abrufen; der Betreiber des Listen-Servers speichert IPs.

## Messen am Original

Vor dem Bauen am Original messen, mit zwei Exes aus `F:\dev\Blitz3D` auf einem Rechner oder
zwei. **Voraussetzung**: Auf dem Messrechner muss das Windows-Feature DirectPlay eingeschaltet
sein (Windows-Features → Legacykomponenten). Das macht der Nutzer selbst.

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

Wo die Messung nicht möglich ist, gilt der Text der Original-Hilfe und das plausibelste
Verhalten; das wird im DEVLOG vermerkt.

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
| 1 | Messen am Original (nach Freigabe DirectPlay durch den Nutzer) | Fragen oben beantwortet, Ergebnis im DEVLOG |
| 2 | Transport + Zuverlässigkeit (`bb_net.h`), I/O-Thread, Verlust-Schalter | Test: 10 000 zuverlässige Nachrichten bei 20 % Verlust kommen vollständig und in Reihenfolge an |
| 3 | Sitzung: Host, Join, Spielerliste, Nachrichtenschlange, die 14 Befehle | Hilfe-Beispiele `SendNetMsg.bb` + `RecvNetMsg.bb` laufen gegeneinander |
| 4 | Spielsuche im LAN, Auto-Modus, Handshake mit Kennung, Schalter von außen, neue Befehle | Auto-Modus- und `REJECT`-Tests grün; Lobby-Beispiel läuft |
| 5 | Host-Migration, Abbruch, 200 | Test: Host beenden, ein Client wird Host, Spiel geht weiter |
| 6 | Listen-Server: HTTP-Client (WinHTTP/libcurl), Anmelden/Lebenszeichen/Abmelden, `NetGameServer`, Referenz `tools/listserver/games.php` | Tests gegen den lokalen Referenz-Server grün; zwei Rechner finden sich über einen echten Webspace |
| 7 | UPnP mit `miniupnpc` | Am echten Router: Weiterleitung entsteht beim Hosten und verschwindet beim Beenden |
| 8 | Doku: Nutzer-Doku „Netzwerk – Unterschiede zu Blitz3D“ (inkl. Listen-Server, NAT, Datenschutz), KNOWN_ISSUES, roadmap, Release Notes, Website | Unterschiede an allen Stellen gleich beschrieben |
| 9 | Friendly Fire: Mehrspieler auf den Net-Befehlen oder eigenem UDP (Entscheidung offen) | Siehe FRIENDLY_FIRE.md, Abschnitt Mehrspieler |
