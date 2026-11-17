# Test-Chat für das Network-Update – Konzept

Stand: 2026-11-17 · Entscheidungen 1–7 vom Nutzer getroffen (Abschnitt „Entscheidungen“).
Arbeitstitel: **Netchat**.

Zeichen: **✔ entschieden** · **◇ Vorschlag** · **? offen**

## Zweck

Paket 6.7 verlangt einen echten Versuch über zwei Anschlüsse: Ein Rechner hinter Carrier-NAT
hostet, ein anderer Anschluss tritt über IPv6 bei (NETWORK.md, Abschnitt „IPv6“). Ein Laptop am
Hotspot oder ein einzelner Tester prüft genau einen Anschluss und ist schwer zu organisieren.

Stattdessen ✔ ein kleiner Chat, den sich jeder herunterladen kann (Idee des Nutzers,
2026-11-17): ein einziger Raum, wer das Programm startet, ist drin. Das Programm schreibt einen
Bericht, den der Tester schickt oder im BLTZNXT-Discord postet. So kommen viele Anschlüsse
zusammen: verschiedene Router und Anbieter, nur IPv4, IPv6, Carrier-NAT. Genau diese Mischung
erwartet das Network-Update nach dem Release.

Nebenbei ist der Chat ein Beispielprogramm für die Net-Befehle (`samples/netchat/`).

Leitlinie ✔: **vollkommen barrierefrei** – sonst testet es niemand. Ein Doppelklick genügt;
keine Installation, keine Einstellung, kein Konto, keine Frage, die man verstehen muss.

## Ablauf für den Tester

1. ZIP herunterladen, auspacken, `netchat.exe` doppelklicken.
2. Windows fragt womöglich zweimal (siehe „Hürden, die bleiben“). Die Anleitung zeigt beides mit
   Bild.
3. Ein kurzer Startbildschirm: was der Test ist, was übertragen wird (Datenschutz, ein Absatz),
   ein vorgeschlagener Name (zufällig, etwa „Tester-4821“), den man ändern kann. Enter startet.
4. Der Chat: oben die Nachrichten, rechts die Teilnehmer, unten die Eingabezeile, ganz unten eine
   Statuszeile („verbunden – 5 im Raum“ / „du bist Host“ / „suche Raum …“).
5. Esc beendet. Danach zeigt das Programm die Zusammenfassung des Berichts und darunter:
   „Enter: Bericht anonym an bltznxt.com senden · Esc: nicht senden“. Nach dem Senden kurz
   „Danke!“, dann ist das Programm zu. Der Bericht liegt zusätzlich als Datei neben der EXE.

Auch wer gleich wieder schließt oder nie jemanden trifft, hat einen nützlichen Bericht: „allein,
niemand erreichbar“ ist ein Messergebnis.

## Raumlogik

Es gibt ✔ einen Raum. Technisch ist er ein Net-Spiel mit eigener Programmkennung
(`NetGameApp "netchat"`) beim Listen-Server auf bltznxt.com.

◇ Beim Start:

1. `FindNetGames` (LAN und Listen-Server).
2. Alle gefundenen Räume der Reihe nach probieren, größter zuerst (`JoinNetGame` mit
   `NetGameIP$`; das Klopfen läuft dabei von selbst). Der erste, der klappt, gilt.
3. Klappt keiner: selbst hosten (`HostNetGame "Netchat"`).

Folge: Erreicht ein Neuer den Host nicht – etwa nur mit IPv4 zu einem Host hinter Carrier-NAT –
macht er einen zweiten Raum auf. Das ist Absicht, denn es ist selbst ein Messergebnis, und der
Bericht hält fest, welcher Raum warum nicht erreichbar war.

◇ Zusammenführen: Wer allein in seinem eigenen Raum sitzt, sucht alle 60 s erneut und tritt
einem anderen erreichbaren Raum bei (`StopNetGame`, dann Schritt 2). So bleiben nicht dauerhaft
viele Ein-Personen-Räume übrig.

Geht der Host, übernimmt der älteste Teilnehmer (Host-Migration). Ist er von außen nicht
erreichbar, zerfällt der Raum für manche; auch das steht im Bericht.

Rolle des Entwicklers ✔: Der Nutzer startet den Chat zu Testbeginn als Erster auf seinem
Rechner und lässt ihn laufen, solange der Test dauert. Dann hostet ein Rechner hinter
Carrier-NAT – genau der Fall aus 6.7 –, kein Tester ist allein, und jeder Beitritt über IPv6
nach Klopfen ist der gesuchte Beweis.

Damit der Host-Rechner tagelang durchhält, ◇ hält Netchat als Host den Rechner wach
(Energiesparen aus, solange es läuft; Bildschirm darf aus) und erholt sich selbst: Verliert der
Host die Anmeldung beim Listen-Server, meldet er sich neu an (das tut die Laufzeit schon).
Startet der Rechner neu, übernimmt ein Tester per Host-Migration; startet der Nutzer Netchat
wieder, tritt er dem größten Raum bei.

## Der Bericht

### Diagnose-Log in der Laufzeit

Was der Bericht braucht, schreibt die Laufzeit heute nur verstreut auf stderr oder gar nicht.
◇ Neu in `bb_net.h`: eine Einstellung `log = datei` (`.net`-Datei) bzw. `BLTZNXT_NET_LOG`, die
Ereignisse des Netzcodes mit Zeit seit Programmstart in eine Datei schreibt. Das hilft auch
später beim Support, unabhängig vom Chat. Netchat schaltet sie ein.

Ereignisse:

| Ereignis | Beispiel |
|---|---|
| Version, System | `BLTZNXT 0.8.0, Windows 10.0.26200` |
| Eigene Adressen, als Klasse | `IPv4: privat (LAN) · IPv6: global` |
| Sockets | `Client IPv4+IPv6, Spielport IPv4+IPv6` |
| UPnP | `UPnP: Router weitergeleitet, eigene Adresse Carrier-NAT` |
| Listen-Server | `angemeldet in 0,4 s, ip6 gemeldet` / `Liste: 3 Räume (2 mit IPv6)` |
| Beitritt je Adresse | `Beitritt Raum 1: IPv6 nach Klopfen 3,1 s ✓ · IPv4 keine Antwort` |
| Klopfen | `geklopft: angenommen` / Host: `2 Klopfer beantwortet` |
| Sitzung | `Host-Migration: neuer Host über IPv4 erreicht` / `Verbindung verloren nach 12 min` |
| Ende | `Dauer 14 min, höchstens 6 im Raum, 3 Räume gesehen` |

### Datenschutz im Bericht ✔ (Grundsatz)

Der Bericht soll bedenkenlos öffentlich im Discord stehen können. Deshalb:

- **keine Adressen und keine Ports fremder Rechner**, auch nicht gekürzt. Adressen erscheinen
  nur als Klasse: IPv4 privat / Carrier-NAT (100.64.0.0/10) / öffentlich; IPv6 global /
  link-local / keine.
- **keine Namen** anderer Teilnehmer, **kein Chat-Inhalt** – nur Zahlen (wie viele im Raum).
- Zeiten relativ zum Programmstart; das Datum des Tests ohne Uhrzeit.

Oben steht eine Zusammenfassung von fünf Zeilen (das, was man im Discord liest), darunter die
Ereignisliste.

### Wie die Berichte ankommen ✔

Über bltznxt.com (Entscheidung 3): `games.php` bekommt eine Aktion `a=report` und hängt jeden
Bericht an eine Datei an, die der Nutzer vom Webspace holt und weitergibt. Für den Tester ist
das ein Tastendruck.

◇ Einzelheiten:

| Punkt | Vorschlag |
|---|---|
| Aufruf | `POST a=report&app=netchat&v=<Protokoll>&text=<Bericht>` → `ok` oder `err <grund>` |
| Datei | `reports.log` neben `games.php`, je Bericht ein Block mit Trennzeile und Eingangsdatum (Tag, ohne Uhrzeit) |
| Keine Absenderdaten | Der Server schreibt **keine** IP-Adresse und keine Uhrzeit dazu; der Bericht ist schon anonym (siehe oben). Die IP steht nur kurz in der Tabelle `hits` für die Grenze je Minute, wie bei jeder Anfrage |
| Schutz | `.htaccess` sperrt `*.log` wie `*.sqlite`; die Datei holt nur der Nutzer per FTP oder Dateimanager des Webspace |
| Grenzen | Bericht höchstens 16 KB; höchstens 5 Berichte je IP und Stunde; ist `reports.log` größer als 5 MB, `err full` |
| Inhalt prüfen | Steuerzeichen außer Zeilenumbruch und Tab werden entfernt; der Server prüft nicht, ob Adressen drinstehen (das verhindert der Client) |
| Gleiche Fassung im Prüfstand | `listserver.js` bekommt `a=report` ebenso (Datei per `--reports <pfad>`), damit die Testreihe den Weg prüft |
| Nicht gesendet | Wird Netchat hart beendet oder sendet nicht (keine Verbindung), bleibt der Bericht als Datei liegen; ◇ beim nächsten Start bietet Netchat an, ihn nachzureichen |

Gesendet wird nur nach Enter auf die Frage am Ende; der Startbildschirm sagt vorher, dass am Ende
ein anonymer Bericht angeboten wird und was drinsteht.

## Oberfläche

◇ Ein Fenster, 1024×640, kein Vollbild (dann stört weder Fokuswechsel noch Auflösung). Gut lesbare Schrift, hoher Kontrast,
keine Farbe als einzige Information (Status auch als Text). Alles mit der Tastatur: tippen,
Enter sendet, Esc beendet. Maus nur zum Scrollen.

Sprache ✔ (Entscheidung 5): Deutsch und Englisch nach Systemsprache, Bericht immer englisch,
damit er überall lesbar ist. ◇ Die Systemsprache liefert Windows über `GetUserDefaultUILanguage`
(per `CallDLL` oder einer kleinen Laufzeitfunktion, noch zu klären).

## Missbrauch

Ein offener Raum mit Fremden zieht früher oder später Spam oder Beleidigungen an.

- ◇ Nachrichten höchstens 200 Zeichen, höchstens eine je Sekunde (im Programm, beim Senden).
- ◇ Namen höchstens 16 Zeichen, nur Buchstaben, Ziffern, `-` und `_`.
- ◇ „Ignorieren“ per Klick auf einen Namen (nur auf dem eigenen Rechner).
- ✔ Freitext mit diesen Grenzen (Entscheidung 2).
- ◇ Nach dem Test (siehe „Testzeit“) bleibt der Chat als Beispielprogramm im Repository, ohne
  öffentlichen Raum.

Sicherheit: Der Netzcode nimmt im Test Pakete von Fremden aus dem Internet an. Der Leser in
`bb_net.h` prüft jede Länge (`bb_NetR_`); vor dem Test ◇ einmal gezielt mit kaputten und zu
großen Paketen beschießen (Prüfstand), damit ein fremdes Paket kein Programm abstürzen lässt.

## Hürden, die bleiben

- **SmartScreen**: Windows warnt vor einer unsignierten EXE („Der Computer wurde durch Windows
  geschützt“ → „Weitere Informationen“ → „Trotzdem ausführen“). Lässt sich nur mit einem
  Code-Signatur-Zertifikat vermeiden. Die Anleitung zeigt es mit Bild.
- **Windows-Firewall**: Beim ersten Start fragt Windows, ob das Programm Verbindungen annehmen
  darf. Wer ablehnt, kann trotzdem beitreten, nur nicht hosten – der Bericht vermerkt, dass der
  Spielport von außen nicht ankam.
- **Virenscanner** melden selbstgebaute EXEs gelegentlich fälschlich. ◇ Prüfsumme (`.sha256`)
  dazulegen wie bei den Releases.

## Verteilung

◇ ZIP mit `netchat.exe`, `LIESMICH.txt` (Deutsch) und `README.txt` (Englisch), je mit den zwei
Bildern zu SmartScreen und Firewall. Download als GitHub-Release `netchat-test-1` (nicht
„latest“), Link im Discord und auf bltznxt.com. Ein Bild für Startbildschirm und Release kommt
vom Nutzer (Entscheidung 7, wie beim Release-Theming).

## Testzeit ✔

Der Test läuft, bis der Nutzer `games.php` umbenennt (Entscheidung 4). Dann ist der Vermittler
nicht mehr erreichbar, und niemand findet den Raum mehr. Der Nutzer startet den Test auf seinem
Rechner und lässt ihn laufen; damit ist immer mindestens ein Host im Raum.

◇ Netchat erkennt das Ende und sagt es freundlich: Antwortet bltznxt.com mit HTTP 404 (Datei
weg), zeigt der Startbildschirm „Der Test ist beendet – danke fürs Mitmachen!“ statt einen
eigenen Raum zu eröffnen. Kommt gar keine Antwort, heißt es „bltznxt.com nicht erreichbar –
Internetverbindung prüfen“, denn das kann auch am Tester liegen. Dafür braucht das Programm den
HTTP-Status der letzten Abfrage beim Listen-Server. ◇ Neuer Befehl `NetGameServerStatus()`:
200 bei Erfolg, den HTTP-Fehler (etwa 404), 0 ohne Antwort. Er nützt jedem Spiel mit eigenem
Suchdialog, nicht nur Netchat; mit C1 zu bauen.

## Wann 6.7 als erledigt gilt

◇ Mindestens ein Bericht zeigt: Beitritt über IPv6 nach Klopfen in einen Raum, dessen Host hinter
Carrier-NAT sitzt. Dazu eine Übersicht aller Berichte in NETWORK.md: wie viele Anschlüsse mit
IPv6, mit Carrier-NAT, mit erfolgreichem UPnP, wie viele Beitritte über welche Familie geklappt
haben und welche Fälle scheiterten. Was dabei auffällt, wird als BUG-Eintrag in die Buglist.

## Arbeitspakete

| # | Paket | Fertig, wenn |
|---|---|---|
| C1 ✅ | Diagnose-Log in der Laufzeit (`log =`, `BLTZNXT_NET_LOG`, Befehl `NetGameLog`), Adressklassen, keine Adressen im Text; Befehl `NetGameServerStatus()` – **erledigt 2026-11-17**, Fall `tests/net/netlog` | Test in `tests/net/` liest das Log eines Falls und findet die Ereignisse, aber keine Adresse |
| C2 ✅ | Prüfstand gegen kaputte Pakete | Fall, der zufällige und zu große Pakete an Host und Client schickt; beide laufen weiter – **erledigt 2026-11-17**: `tests/net/fuzz.js` und Fall `fuzz` (rund 70 000 Pakete an den Host, 30 000 an den Client, über 127.0.0.1 und ::1, Startwerte 1–3 grün); dazu Grenzen in `bb_net.h` gegen Speicherüberlauf (Empfangsfenster 4096, Nachricht 16 MB, 32 halbe unzuverlässige Nachrichten, 64 Programme, 4096 Spieler, 256 Funde, Name 1024 Byte, Warteschlange 64 MB), beim Host Spielerkennung passend zum Absender; zu weit vorausliegende Pakete werden nicht mehr bestätigt, und der Sender bleibt im Empfangsfenster (sonst scheiterte `reliable` bei 20 % Verlust) |
| C3 | Netchat: Startbildschirm, Raumlogik, Chat, Teilnehmer, Status | Zwei bis drei Programme über 127.0.0.1 und ::1 im Prüfstand; von Hand mit Tastatur bedienbar |
| C4 | Bericht: Zusammenfassung + Ereignisse, Datei neben der EXE, Frage am Ende, Senden, Nachreichen beim nächsten Start | Bericht eines Laufs enthält keine Adresse, keinen Namen, keinen Chat-Text |
| C5 | Listen-Server: `a=report` in `games.php` und `listserver.js`, `.htaccess` sperrt `*.log` | Test gegen `listserver.js` (Bericht kommt an, ohne IP; Grenzen); `games.php` auf bltznxt.com per curl |
| C6 | Paket: ZIP, LIESMICH/README mit Bildern, `.sha256`, Release `netchat-test-1` | Auf einem frischen Windows-Konto: Doppelklick bis Chat ohne weitere Hilfe |
| C7 | Test starten (Nutzer hostet), laufen lassen, `games.php` umbenennen, Berichte auswerten in NETWORK.md, 6.7 abhaken | Siehe „Wann 6.7 als erledigt gilt“ |

## Entscheidungen (vom Nutzer 2026-11-17)

| # | Frage | Entscheidung |
|---|---|---|
| 1 | Name des Programms | ✔ „Netchat“ als Arbeitstitel; für die Ankündigung ein Name mit BLTZNXT-Bezug |
| 2 | Freitext oder vorgefertigte Nachrichten? | ✔ Freitext mit Grenzen (200 Zeichen, 1/s) und „Ignorieren“ |
| 3 | Wie kommen die Berichte an? | ✔ Über bltznxt.com: `games.php` schreibt die Berichte in eine Datei, die der Nutzer weitergibt |
| 4 | Testzeit | ✔ Läuft, bis der Nutzer `games.php` umbenennt; der Nutzer startet den Test und lässt seinen Rechner laufen |
| 5 | Sprache der Oberfläche | ✔ Deutsch/Englisch nach Systemsprache, Bericht englisch |
| 6 | Räume zusammenführen (allein → alle 60 s neu suchen)? | ✔ Ja |
| 7 | Theming wie bei Releases (Bild vom Nutzer)? | ✔ Ja |
