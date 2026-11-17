# BlitzNext list server

A list server lets players find internet games. A game registers when it hosts
(`HostNetGame` with `NetGameServer url$` set), sends a heartbeat every 30 s and unregisters
when it ends; `FindNetGames` asks for the list. The game itself still runs directly between
the players; the list server only introduces them. See [NETWORKING.md](../NETWORKING.md).

Two implementations of the same interface:

| File | Runs on | Storage |
|---|---|---|
| `games.php` | any web space with PHP 7.4+ and SQLite (PDO) | `games.sqlite` next to the script |
| `listserver.js` | Node.js, no packages | memory |

## games.php

Upload `games.php` and `.htaccess` into a folder PHP may write to, then set
`NetGameServer "https://example.org/folder/games.php"` in the game (or `server = …` in the
game's `.net` file). The `.htaccess` keeps the database, which holds IP addresses, from being
downloaded on Apache; on other web servers block `*.sqlite` yourself.

## listserver.js

```
node listserver.js --port 8080 [--host 0.0.0.0] [--trust-proxy] [--quiet]
```

`--trust-proxy` takes the client address from `X-Forwarded-For`; use it only behind your own
reverse proxy. `--host ::` listens on IPv6 as well. `--allow-local-ip6` is for the test suite
only: it accepts non-global `ip6` addresses such as `::1`.

## Interface

One URL, plain-text answers, one line per game.

| Request | Parameters | Answer |
|---|---|---|
| `GET ?a=list` | `app`, `v` | per game: `name<TAB>ip<TAB>port<TAB>players<TAB>token<TAB>ip6` |
| `POST a=host` | `app`, `name`, `port`, `players`, `v`, `token`, `ip6` | `ok <key>` or `err <reason>` |
| `POST a=beat` | `key`, `players` | `ok` or `err gone` (register again) |
| `POST a=bye` | `key` | `ok` |
| `POST a=knock` | `app`, `token`, `ip6`, `port` | `ok` or `err <reason>` |
| `POST a=knocks` | `key` | `ok`, then per knock `ip6<TAB>port`; or `err gone` |

The server takes the host's IP address from the request, so a host behind a router is listed
with its public address. `ip` is the IPv4 address, `ip6` the IPv6 address; either may be empty.
A server reached over IPv4 never sees the host's IPv6 address, so the host sends it along as
`ip6`; the server only takes a global one (2000::/3). If the host registers over IPv6, its
sender address goes into `ip6` and the parameter is ignored. A `games.php` database from before
the IPv6 field gets the new column on the first request. Entries without a heartbeat for 90 s
disappear.

Knocking gets a player through the firewall of the host's router, which IPv6 still has even
without NAT. A client that wants to join over IPv6 posts `a=knock` with the game's `app` and
`token` and its own IPv6 address and port (over IPv6 the server takes the sender address). The
host asks `a=knocks` every 3 s, which also counts as a heartbeat, and sends each knocker a few
packets; that opens its firewall for exactly that address. Knocks wait 30 s, at most 16 per
game, and asking clears them.

Limits against abuse: names and IDs up to 64 characters, at most 8 games per IP address, at
most 60 requests per IP address and minute. A knock makes a host send three small packets to
the given address, so knocks only take global IPv6 addresses.
