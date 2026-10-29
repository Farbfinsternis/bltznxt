# Networking in BlitzNext

Blitz3D had two network layers: TCP and UDP streams, and 14 multiplayer commands built on
DirectPlay (`StartNetGame` … `NetMsgData$`). DirectPlay is a switched-off legacy component on
current Windows, so BlitzNext implements the 14 commands on its own UDP protocol. Programs keep
their code; what changes is listed below.

## The short version

```blitzbasic
; host: HostNetGame("my game")       client: JoinNetGame("my game", "192.168.1.20")
; or simply, on every machine:
If StartNetGame() = 0 Then RuntimeError "No network game"
me = CreateNetPlayer("Player")

While Not KeyHit(1)
	While RecvNetMsg()
		Select NetMsgType()
		Case 100 : Print NetPlayerName(NetMsgFrom()) + " joined"
		Case 101 : Print "a player left"
		Case 102 : Print "this program is the host now"
		Case 1   : Print NetPlayerName(NetMsgFrom()) + ": " + NetMsgData()
		End Select
	Wend
	If KeyHit(57) Then SendNetMsg 1, "hello", me      ; to everyone else
	Delay 10
Wend
StopNetGame
```

## Differences from Blitz3D

| Topic | Blitz3D (DirectPlay) | BlitzNext |
|---|---|---|
| `StartNetGame()` | Dialog: host or join, connection type, IP | **No dialog.** Searches the LAN for about a second, joins a game it finds (returns 1), otherwise hosts one (returns 2). Build your own join dialog with `FindNetGames` |
| Connections | TCP/IP, IPX, modem, serial | UDP over IPv4 only |
| Playing with Blitz3D | — | A BlitzNext program cannot join a game of a program built with the original Blitz3D |
| `JoinNetGame(name$, ip$)` | Exact game name required; empty IP searches the LAN | Empty name joins any game at that address; the name is compared without case; `ip$` may carry a port (`"1.2.3.4:2351"`) or a host name. Empty IP or `"0.0.0.0"` searches the LAN |
| Message type | Sent as a signed byte (150 arrives as -106) | Sent as a full integer. Type 0 is never delivered; use 1–99 for your own messages |
| Message to one of your own players | Never arrives | Arrives (local to local). A message to everyone (`to_player = 0`) still skips the sending program |
| A host that crashes | DirectPlay noticed nothing for at least 17 s and blocked the other programs | Noticed after 5 s; the oldest remaining program becomes host |
| Timing | `JoinNetGame` about 6 s, `StopNetGame` about 3 s | `JoinNetGame` a few milliseconds, `StopNetGame` at most 0.5 s; `StartNetGame` about 1 s when it finds a game, about 2 s before it hosts |

Unchanged: the return values, `reliable = 1` as default, messages of any length (1 MB tested),
a 100 for every player already in the game right after joining, no 100 for your own players,
`NetPlayerName$` returning `<unknown>` for unknown players and `<all>` for 0, and the runtime
errors "Multiplayer game not started" (sending or receiving without a game) and "Multiplayer
game already started" (hosting or joining twice).

## New commands

| Command | What it does |
|---|---|
| `FindNetGames([timeout_ms = 1000])` | Searches the LAN and, if set, the list server; returns the number of games found. Blocks for the timeout |
| `NetGameName$(index)` | Name of game `index` (1 … count) |
| `NetGameIP$(index)` | Its address, with the port when it is not the game port; passes straight into `JoinNetGame` |
| `NetGamePlayers(index)` | Number of players in it |
| `NetGameLAN(index)` | 1 if it was found in the LAN, 0 if it came from the list server |
| `NetGameHost()` | 1 while this program hosts the game (also after a host migration) |
| `NetGameApp id$` | Program ID: only games with the same ID are found or joined. Default: empty, so all BlitzNext programs see each other, as with DirectPlay |
| `NetGamePort port` | Game port, default 2350 |
| `NetGameServer url$` | List server for internet games; `""` switches it off |
| `NetGameUPnP enable` | Ask the router to forward the game port while hosting (UPnP); default on, 0 switches it off, for example for LAN-only games |
| `NetGameForwarded()` | While hosting: 1 if the router forwards the game port, 0 if not, -1 while the answer is still pending (a few seconds after hosting) |

A join dialog in a few lines:

```blitzbasic
n = FindNetGames()
For i = 1 To n
	Print i + ": " + NetGameName(i) + " (" + NetGamePlayers(i) + " players) " + NetGameIP(i)
Next
pick = Input("Join which game (0 = host)? ")
If pick = 0 Then HostNetGame("my game") Else JoinNetGame(NetGameName(pick), NetGameIP(pick))
```

When a join fails, the reason goes to stderr, for example
`JoinNetGame: host runs 'racer', this program is 'shooter'` or
`JoinNetGame: no answer from 1.2.3.4:2350`.

## Settings from outside

Old programs can be pointed at another game or list server without changing their code. The
first source that sets a value wins:

1. Environment variables `BLTZNXT_NET_JOIN`, `BLTZNXT_NET_SERVER`, `BLTZNXT_NET_APP`,
   `BLTZNXT_NET_PORT`, `BLTZNXT_NET_LAN`, `BLTZNXT_NET_UPNP`
2. A file `<program>.net` next to the program, one setting per line:
   ```
   ; comment
   join   = 1.2.3.4:2350      ; StartNetGame joins exactly here
   server = https://example.org/games.php
   app    = racer
   port   = 2350
   lan    = 0                 ; do not search the LAN
   upnp   = 0                 ; do not ask the router to forward the port
   ```
3. What the program sets with `NetGameServer`, `NetGameApp`, `NetGamePort` and `NetGameUPnP`

## Internet games

LAN games and games at a known IP work right away. To find games over the internet, run a
**list server**: games register there when they host and disappear when they end, and
`FindNetGames` lists them. BlitzNext ships two implementations in [listserver/](listserver/):
`games.php` for any web space with PHP and SQLite, and `listserver.js` for Node.js. Each game
author runs their own; there is no central BlitzNext server.

The host must be reachable from outside. Behind a home router that means forwarding the game
port (UDP 2350 unless changed) to the hosting machine. BlitzNext asks the router to do that by
itself (UPnP) whenever a program hosts, also after a host migration, and removes the forwarding
again on `StopNetGame` and when the program ends. The forwarding is leased for an hour and
renewed while the game runs, so a crashed program does not leave it open for long; routers that
only know permanent forwardings get a permanent one. `NetGameForwarded()` tells whether it
worked.

It does not work when UPnP is off in the router (a FRITZ!Box has to allow it per device, under
the port sharing settings of that device), when another computer already has the port, or
behind carrier-grade NAT (DS-Lite, many mobile networks): there the router gets the forwarding,
but its own address is not public. BlitzNext says which case it is on stderr, for example
`Net UPnP: no router answered; to host over the Internet, forward UDP port 2350 to this
computer by hand`. Then forward the port by hand, or host on another machine. NAT hole
punching is planned, not yet built.

While a game is hosted with UPnP, its port is open to the internet, not only to the LAN. A game
meant only for the LAN can switch it off with `NetGameUPnP 0`.

**Privacy:** a game that is hosted through a list server shows its public IP address to
everyone who reads the list, and the operator of the list server stores it while the game
runs.

## How it works

The host is the centre: every client has one connection, to the host, which passes messages on.
Each connection has a reliable channel (sequence numbers, acknowledgements, resending, order)
and an unreliable one; long messages are split and joined again. A background thread keeps the
connections alive even while the program does not call `RecvNetMsg`. When the host leaves or
stops answering for 5 s, the oldest remaining program becomes host and gets a 102; the others
reconnect to it, and everyone gets a 101 for the old host's players. Messages sent during the
switch may be lost, reliable ones from clients are delivered after it. The design notes are in
[NETWORK.md](NETWORK.md) (German).

## Testing

`bash tests/net/run_net_tests.sh` runs several programs at once over 127.0.0.1: joining,
messages, 20 % packet loss, host migration, a crashed host, the LAN search, the list server,
and UPnP against a simulated router (`tests/net/fake_igd.js`). A real router can be checked
with [samples/network/upnpcheck.bb](samples/network/upnpcheck.bb). `bash tests/run_tests.sh` includes it.
