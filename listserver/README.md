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
reverse proxy.

## Interface

One URL, plain-text answers, one line per game.

| Request | Parameters | Answer |
|---|---|---|
| `GET ?a=list` | `app`, `v` | per game: `name<TAB>ip<TAB>port<TAB>players<TAB>token` |
| `POST a=host` | `app`, `name`, `port`, `players`, `v`, `token` | `ok <key>` or `err <reason>` |
| `POST a=beat` | `key`, `players` | `ok` or `err gone` (register again) |
| `POST a=bye` | `key` | `ok` |

The server takes the host's IP address from the request, so a host behind a router is listed
with its public address. Entries without a heartbeat for 90 s disappear. Limits against abuse:
names and IDs up to 64 characters, at most 8 games per IP address, at most 60 requests per IP
address and minute.
