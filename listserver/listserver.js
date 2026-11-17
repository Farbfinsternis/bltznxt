#!/usr/bin/env node
// Listen-Server fuer die Spielsuche von BLTZNXT (NETWORK.md, Abschnitt Listen-Server).
//
// Ein Spiel meldet sich beim Hosten an (HostNetGame mit NetGameServer), schickt
// alle 30 s ein Lebenszeichen und meldet sich beim Ende ab. FindNetGames fragt
// die Liste ab. Das Spiel selbst laeuft weiter direkt zwischen den Spielern;
// dieser Server vermittelt nur.
//
// Nur Node.js ohne weitere Pakete, Daten im Speicher. Fuer Webspace ohne Node
// gibt es dieselbe Schnittstelle als games.php.
//
//   node listserver.js --port 8080
//   node listserver.js --host 127.0.0.1 --port 28790 --lifetime 30     (Tests)
//
// Schnittstelle, eine URL, Antwort Text (UTF-8, eine Zeile je Spiel):
//
//   GET  ?a=list&app=<id>&v=<version>   name<TAB>ip<TAB>port<TAB>players<TAB>token<TAB>ip6
//   POST a=host&app=&name=&port=&players=&v=&token=&ip6=   ok <key>   | err <grund>
//   POST a=beat&key=&players=                              ok         | err gone
//   POST a=bye&key=                                        ok
//
// ip ist die IPv4-Adresse des Hosts, ip6 seine IPv6-Adresse; leer, wo eine
// fehlt (NETWORK.md, IPv6 Punkt 4). Kommt die Anmeldung ueber IPv4, sieht der
// Server die IPv6-Adresse des Hosts nie: der Host nennt sie selbst (ip6=),
// angenommen wird nur eine globale (2000::/3). Kommt die Anmeldung ueber
// IPv6, gilt die Absenderadresse und ip6= nicht.

'use strict';
const http = require('http');
const net = require('net');
const crypto = require('crypto');

const EXPIRE = 90 * 1000;   // ohne Lebenszeichen faellt ein Eintrag weg
const MAX_PER_IP = 8;       // Eintraege je Absender-IP
const MAX_TEXT = 64;        // Laenge von Name und Kennung
const RATE = 60;            // Anfragen je IP und Minute

const opt = { host: '0.0.0.0', port: 8080, lifetime: 0, trustProxy: false, quiet: false, localIp6: false };
for (let i = 2; i < process.argv.length; i++) {
  const a = process.argv[i];
  if (a === '--host') opt.host = process.argv[++i];
  else if (a === '--port') opt.port = Number(process.argv[++i]);
  else if (a === '--lifetime') opt.lifetime = Number(process.argv[++i]);
  else if (a === '--trust-proxy') opt.trustProxy = true;
  else if (a === '--quiet') opt.quiet = true;
  else if (a === '--allow-local-ip6') opt.localIp6 = true;   // nur Tests: ip6= auch ::1 und LAN
  else { console.error('unknown option ' + a); process.exit(2); }
}

const games = new Map();   // key -> Eintrag
const hits = new Map();    // ip -> Zeitpunkte

const log = (t) => { if (!opt.quiet) console.log(new Date().toTimeString().slice(0, 8), t); };
const clean = (t) => String(t || '').replace(/\s+/g, ' ').trim().slice(0, MAX_TEXT);
function number(t, low, high, def) {
  if (!/^\d+$/.test(String(t || ''))) return def;
  const v = Number(t);
  return v >= low && v <= high ? v : def;
}

// ip6= des Hosts: eine globale IPv6-Adresse (2000::/3), sonst ''
function ip6Param(t) {
  t = String(t || '').trim().toLowerCase();
  if (!net.isIPv6(t) || t.includes('%')) return '';
  if (opt.localIp6) return t;
  const first = parseInt(t.split(':')[0] || '0', 16);
  return t.startsWith('::') || (first & 0xe000) !== 0x2000 ? '' : t;
}

function handle(ip, p) {
  const now = Date.now();
  const recent = (hits.get(ip) || []).filter((t) => now - t < 60000);
  recent.push(now);
  hits.set(ip, recent);
  if (recent.length > RATE) return 'err busy';
  for (const [k, g] of games) if (now - g.seen > EXPIRE) games.delete(k);

  switch (p.get('a')) {
    case 'list': {
      const app = clean(p.get('app')).toLowerCase(), v = p.get('v') || '';
      const lines = [];
      for (const g of games.values())
        if (g.app === app && (!v || g.v === v))
          lines.push([g.name, net.isIPv6(g.ip) ? '' : g.ip, g.port, g.players, g.token,
                      net.isIPv6(g.ip) ? g.ip : g.ip6].join('\t'));
      return lines.length ? lines.join('\n') + '\n' : '';
    }
    case 'host': {
      const port = number(p.get('port'), 1, 65535, null);
      if (port === null) return 'err port';
      let n = 0;
      for (const g of games.values()) if (g.ip === ip) n++;
      if (n >= MAX_PER_IP) return 'err limit';
      const key = crypto.randomBytes(16).toString('hex');
      const g = {
        app: clean(p.get('app')).toLowerCase(), name: clean(p.get('name')), ip, port,
        ip6: net.isIPv6(ip) ? '' : ip6Param(p.get('ip6')),
        players: number(p.get('players'), 0, 65535, 0), v: clean(p.get('v')),
        token: number(p.get('token'), 0, 4294967295, 0), seen: now,
      };
      games.set(key, g);
      log(`host ${ip}:${port}${g.ip6 ? ' ip6 ' + g.ip6 : ''} app='${g.app}' name='${g.name}'`);
      return 'ok ' + key;
    }
    case 'beat': {
      const g = games.get(p.get('key') || '');
      if (!g) return 'err gone';
      g.seen = now;
      g.players = number(p.get('players'), 0, 65535, g.players);
      return 'ok';
    }
    case 'bye': {
      const key = p.get('key') || '', g = games.get(key);
      if (g) { games.delete(key); log(`bye ${g.ip}:${g.port} name='${g.name}'`); }
      return 'ok';
    }
  }
  return 'err action';
}

const server = http.createServer((req, res) => {
  let body = '';
  req.on('data', (c) => { if (body.length < 4096) body += c; });
  req.on('end', () => {
    const p = new URLSearchParams(new URL(req.url, 'http://x').search);
    for (const [k, v] of new URLSearchParams(body)) p.set(k, v);
    let ip = req.socket.remoteAddress || '';
    if (opt.trustProxy && req.headers['x-forwarded-for']) ip = req.headers['x-forwarded-for'].split(',')[0].trim();
    ip = ip.replace(/^::ffff:/, '');
    const text = handle(ip, p);
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8', 'Content-Length': Buffer.byteLength(text) });
    res.end(text);
  });
});
server.listen(opt.port, opt.host, () => log(`listening on ${opt.host}:${opt.port}`));
if (opt.lifetime > 0) setTimeout(() => process.exit(0), opt.lifetime * 1000);
