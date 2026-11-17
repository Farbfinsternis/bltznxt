#!/usr/bin/env node
// Beschiesst die Net-Befehle mit kaputten Paketen (samples/netchat/KONZEPT.md, C2).
//
//   node fuzz.js --host 127.0.0.1 --target 28780 --fake 28781 [--time 6000] [--seed 1]
//
// Angriff A auf den Host am Spielport --target:
//   - zufaellige Bytes, Pakete mit gueltigem Kopf und kaputtem Inhalt jeder
//     Paketart, uebergrosse Pakete, Laengenfelder, die ueber das Paketende zeigen
//   - tritt regulaer bei und schickt als Teilnehmer kaputte Sitzungsnachrichten
//     (MSG, PADD, PDEL, MADD, MDEL), Pakete weit vor dem Fenster, eine
//     zerlegte Nachricht ueber 16 MB, zu viele unzuverlaessige Teilstuecke
// Angriff B, falscher Host am Port --fake: beantwortet JOIN erst mit kaputten
//   WELCOMEs, dann mit einem gueltigen, und ueberschuettet den Client danach
//   mit kaputten Sitzungsnachrichten.
//
// Alle Zufallszahlen kommen aus einem festen Startwert (--seed), damit ein
// Fund sich wiederholen laesst. Adressen in den Paketen sind beliebig; die
// Programme im Test binden an 127.0.0.1 bzw. ::1 und koennen nichts nach
// draussen schicken.
'use strict';
const dgram = require('dgram');

const opt = { host: '127.0.0.1', target: 28780, fake: 28781, time: 6000, seed: 1, lifetime: 40 };
for (let i = 2; i < process.argv.length; i++) {
  const a = process.argv[i], v = process.argv[++i];
  if (a === '--host') opt.host = v;
  else if (a === '--target') opt.target = Number(v);
  else if (a === '--fake') opt.fake = Number(v);
  else if (a === '--time') opt.time = Number(v);
  else if (a === '--seed') opt.seed = Number(v);
  else if (a === '--lifetime') opt.lifetime = Number(v);
  else { console.error('unknown option ' + a); process.exit(2); }
}
const fam = opt.host.includes(':') ? 'udp6' : 'udp4';
const VERSION = 3;
const K = { DISCOVER: 1, OFFER: 2, JOIN: 3, WELCOME: 4, REJECT: 5, REL: 6, UNREL: 7, ACK: 8, PING: 9, PONG: 10,
            BYE: 11, RESUME: 12, RESUMED: 13, KNOCK: 14 };
const NS = { MSG: 1, PADD: 2, PDEL: 3, MADD: 4, MDEL: 5 };

// xorshift32, fester Startwert
let st = (opt.seed * 2654435761) >>> 0 || 1;
const rnd = () => { st ^= st << 13; st >>>= 0; st ^= st >>> 17; st ^= st << 5; st >>>= 0; return st; };
const ri = (n) => rnd() % n;
const bytes = (n) => { const b = Buffer.alloc(n); for (let i = 0; i < n; i++) b[i] = rnd() & 255; return b; };

class W {
  constructor() { this.p = []; }
  u8(v) { this.p.push(Buffer.from([v & 255])); return this; }
  u16(v) { const b = Buffer.alloc(2); b.writeUInt16LE(v & 0xffff); this.p.push(b); return this; }
  u32(v) { const b = Buffer.alloc(4); b.writeUInt32LE(v >>> 0); this.p.push(b); return this; }
  i32(v) { const b = Buffer.alloc(4); b.writeInt32LE(v | 0); this.p.push(b); return this; }
  str(s) { const b = Buffer.isBuffer(s) ? s : Buffer.from(s); this.u32(b.length); this.p.push(b); return this; }
  raw(b) { this.p.push(b); return this; }
  buf() { return Buffer.concat(this.p); }
}
const pkt = (kind, ver = VERSION) => new W().raw(Buffer.from('BNX')).u8(ver).u8(kind);
// Spielernamen des Fuzzers: nie "H", "F" oder "C"
const fuzzName = () => 'z' + bytes(ri(12)).toString('hex');

// ---- Kaputte Sitzungsnachricht (Inhalt eines REL/UNREL-Pakets)
function badSession(mine) {
  const w = new W();
  switch (ri(9)) {
    case 0: return bytes(ri(200));                                                  // reiner Zufall
    case 1: return w.u8(NS.MSG).i32(30 + ri(9)).i32(rnd()).i32(ri(3) ? 0 : rnd()).str(bytes(ri(300))).buf();
    case 2: return w.u8(NS.MSG).i32(31).i32(rnd()).i32(0).u32(0x7fffffff).buf();     // Laenge zeigt weit hinaus
    case 3: return w.u8(NS.PADD).i32((mine << 16) | (1 + ri(60000))).i32(mine).str(fuzzName()).buf();
    case 4: return w.u8(NS.PADD).i32(rnd()).i32(rnd()).str(bytes(ri(2000))).buf();   // falsche Kennung, langer Name
    case 5: return w.u8(NS.PDEL).i32(rnd()).buf();
    case 6: return w.u8(NS.MADD).i32(rnd()).u8(ri(9)).raw(bytes(ri(60))).buf();       // kaputte Adressliste
    case 7: return w.u8(NS.MDEL).i32(rnd()).buf();
    default: return w.u8(ri(256)).raw(bytes(ri(40))).buf();                          // unbekannte Art
  }
}

// ---- Kaputtes Paket ohne Verbindung
function badPacket() {
  switch (ri(6)) {
    case 0: return bytes(ri(1500));
    case 1: return pkt(ri(256), ri(256)).raw(bytes(ri(100))).buf();
    case 2: return pkt(1 + ri(14)).raw(bytes(ri(64))).buf();
    case 3: return pkt(1 + ri(14)).u32(0xffffffff).raw(bytes(ri(8))).buf();
    case 4: return Buffer.from('BNX').subarray(0, ri(4));
    default: return pkt(K.JOIN).str(bytes(ri(20))).str(bytes(ri(20))).u32(rnd()).u8(ri(256)).raw(bytes(ri(30))).buf();
  }
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const sockets = [];
function sock() {
  const s = dgram.createSocket({ type: fam });
  s.on('error', () => {});
  sockets.push(s);
  return new Promise((r) => s.bind(0, opt.host, () => r(s)));
}
const send = (s, b, port) => new Promise((r) => s.send(b, port, opt.host, () => r()));

let sentA = 0, sentB = 0;

// ---- Angriff A
async function attackHost(until) {
  const loose = [];
  for (let i = 0; i < 4; i++) loose.push(await sock());
  // ohne Verbindung
  for (let i = 0; i < 1500; i++) { await send(loose[i & 3], badPacket(), opt.target); sentA++; if (i % 100 === 0) await sleep(5); }
  for (let i = 0; i < 4; i++) { await send(loose[i], bytes(60000), opt.target); sentA++; }

  // regulaer beitreten
  const j = await sock();
  let mine = 0;
  j.on('message', (m) => {
    if (m.length > 13 && m.toString('latin1', 0, 3) === 'BNX' && m[4] === K.WELCOME) mine = m.readInt32LE(9);
  });
  const nonce = rnd() | 1;
  for (let i = 0; i < 20 && !mine; i++) {
    await send(j, pkt(K.JOIN).str('').str('').u32(nonce).u8(0).buf(), opt.target);
    await sleep(100);
  }
  if (!mine) { console.log('fuzz: host did not let us join'); return; }
  let seq = 1;
  const rel = (payload, last = true) => pkt(K.REL).u32(seq++).u8(last ? 1 : 0).raw(payload).buf();

  while (Date.now() < until) {
    for (let i = 0; i < 200; i++) {
      let b;
      switch (ri(8)) {
        case 0: case 1: case 2: b = rel(badSession(mine)); break;
        case 3: b = pkt(K.REL).u32(seq + 5000 + ri(1000000)).u8(1).raw(bytes(ri(1100))).buf(); break;   // weit voraus
        case 4: b = pkt(K.UNREL).u32(rnd()).u16(ri(65535)).u16(1 + ri(65535)).raw(bytes(ri(1200))).buf(); break;
        case 5: b = pkt(K.UNREL).u32(1).u16(0).u16(1).raw(badSession(mine)).buf(); break;
        case 6: b = pkt(K.ACK).u16(ri(65535)).raw(bytes(ri(200))).buf(); break;
        default: b = pkt(1 + ri(14)).raw(bytes(ri(80))).buf();
      }
      await send(j, b, opt.target);
      sentA++;
    }
    // ab und zu: eine zerlegte Nachricht ueber 16 MB, gedrosselt
    if (ri(4) === 0) {
      const chunk = bytes(1100);
      const n = Math.ceil((17 << 20) / 1100);
      for (let i = 0; i < n && Date.now() < until; i++) {
        await send(j, rel(chunk, false), opt.target);
        sentA++;
        if (i % 400 === 0) await sleep(15);
      }
      await send(j, rel(Buffer.from([NS.MSG]), true), opt.target);
    }
    await sleep(20);
  }
}

// ---- Angriff B: falscher Host
async function fakeHost(until) {
  const s = dgram.createSocket({ type: fam });
  s.on('error', () => {});
  sockets.push(s);
  await new Promise((r) => s.bind(opt.fake, opt.host, r));
  let client = null;
  s.on('message', async (m, rinfo) => {
    if (client || m.length < 5 || m.toString('latin1', 0, 3) !== 'BNX' || m[4] !== K.JOIN) return;
    // JOIN: app, name, nonce
    let p = 5;
    const skipStr = () => { const n = m.readUInt32LE(p); p += 4 + n; };
    try { skipStr(); skipStr(); } catch (e) { return; }
    const nonce = m.readUInt32LE(p);
    client = rinfo;
    const to = (b) => { sentB++; return new Promise((r) => s.send(b, rinfo.port, rinfo.address, () => r())); };
    // erst kaputte WELCOMEs
    await to(pkt(K.WELCOME).u32(nonce ^ 1).i32(2).i32(1).u32(1).str('x').u16(0).u32(0).buf());     // falsche Nonce
    await to(pkt(K.WELCOME).u32(nonce).i32(2).buf());                                             // abgeschnitten
    await to(pkt(K.WELCOME).u32(nonce).i32(2).i32(1).u32(7).str('fake').u16(65535).raw(bytes(50)).buf());
    await to(pkt(K.WELCOME).u32(nonce).i32(2).i32(1).u32(7).str('fake').u16(0).u32(0xffffffff).buf());
    await to(pkt(K.REJECT).u32(nonce ^ 2).u8(ri(256)).str(bytes(30)).buf());                         // fremde Nonce
    // dann ein gueltiges
    await to(pkt(K.WELCOME).u32(nonce).i32(2).i32(1).u32(7).str('fake')
      .u16(2).i32(1).u8(0).i32(0).i32(2).u8(0).i32(1)
      .u32(1).i32((1 << 16) | 1).i32(1).str('zhost').buf());
    // und dann Muell als Host
    let seq = 1;
    while (Date.now() < until) {
      for (let i = 0; i < 150; i++) {
        let b;
        switch (ri(6)) {
          case 0: case 1: case 2: b = pkt(K.REL).u32(seq++).u8(1).raw(badSession(1)).buf(); break;
          case 3: b = pkt(K.UNREL).u32(rnd()).u16(ri(100)).u16(1 + ri(100)).raw(bytes(ri(1200))).buf(); break;
          case 4: b = pkt(K.ACK).u16(ri(65535)).raw(bytes(ri(100))).buf(); break;
          default: b = pkt(1 + ri(14)).raw(bytes(ri(80))).buf();
        }
        if (b.length >= 5 && b[4] === K.BYE) continue;   // kein BYE: sonst sucht der Client einen neuen Host
        await to(b);
      }
      await sleep(15);
    }
  });
}

(async () => {
  const until = Date.now() + opt.time;
  setTimeout(() => process.exit(0), opt.lifetime * 1000);
  await Promise.all([attackHost(until), fakeHost(until + 1000)]);
  await sleep(1200);
  console.log(`fuzz: ${sentA} packets to the host, ${sentB} to the client`);
  for (const s of sockets) s.close();
  process.exit(0);
})();
