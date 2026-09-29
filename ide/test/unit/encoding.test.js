// Kodierung: was hereinkommt, kommt byte-gleich wieder heraus.

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { decodeFile, encodeFile } from '../../src/core/encoding.js';

const bytesOf = (...parts) => Uint8Array.from(parts.flatMap((p) => (typeof p === 'string' ? [...Buffer.from(p, 'latin1')] : p)));
const same = (a, b) => assert.deepEqual([...a], [...b]);

test('encoding: jedes der 256 Bytes übersteht Lesen und Schreiben (Windows-1252)', () => {
	const all = new Uint8Array(256);
	for (let i = 0; i < 256; i++) all[i] = i;
	// 0x00 erlaubt: es ist "binary", aber verlustfrei
	const r = decodeFile(all);
	assert.equal(r.encoding, 'windows-1252');
	const back = encodeFile(r.text, r.encoding, r.eol);
	assert.ok(back.bytes, 'muss kodierbar sein');
	// Zeilenenden werden normalisiert (0x0A 0x0D im Bereich) — deshalb ohne diese Bytes vergleichen
	const clean = all.filter((b) => b !== 10 && b !== 13);
	const r2 = decodeFile(clean);
	same(encodeFile(r2.text, r2.encoding, '\n').bytes, clean);
});

test('encoding: typische Blitz3D-Datei mit Umlauten (ANSI) bleibt Windows-1252 und byte-gleich', () => {
	const src = bytesOf('; Gr', [0xfc], 'n und Gr', [0xf6], [0xdf], 'e', [0x0d, 0x0a], 'Print "', [0xe4], [0xf6], [0xfc], '"', [0x0d, 0x0a]);
	const r = decodeFile(src);
	assert.equal(r.encoding, 'windows-1252');
	assert.equal(r.eol, '\r\n');
	assert.equal(r.text, '; Grün und Größe\nPrint "äöü"\n');
	same(encodeFile(r.text, r.encoding, r.eol).bytes, src);
});

test('encoding: gültiges UTF-8 mit Nicht-ASCII bleibt UTF-8, mit und ohne BOM', () => {
	const plain = new TextEncoder().encode('Print "Grüße €"\n');
	const r = decodeFile(plain);
	assert.equal(r.encoding, 'utf-8');
	assert.equal(r.text, 'Print "Grüße €"\n');
	same(encodeFile(r.text, r.encoding, r.eol).bytes, plain);

	const withBom = Uint8Array.from([0xef, 0xbb, 0xbf, ...plain]);
	const rb = decodeFile(withBom);
	assert.equal(rb.encoding, 'utf-8-bom');
	assert.equal(rb.text, 'Print "Grüße €"\n'); // BOM ist nicht im Text
	same(encodeFile(rb.text, rb.encoding, rb.eol).bytes, withBom);
});

test('encoding: ungültiges UTF-8 und "BOM ohne UTF-8" werden als 1252 gelesen, nie geworfen', () => {
	const bad = Uint8Array.from([0xc3, 0x28, 0x41]); // ungültige Folge
	assert.equal(decodeFile(bad).encoding, 'windows-1252');
	const badBom = Uint8Array.from([0xef, 0xbb, 0xbf, 0xff, 0xfe]);
	const r = decodeFile(badBom);
	assert.equal(r.encoding, 'windows-1252');
	same(encodeFile(r.text, r.encoding, '\n').bytes, badBom);
});

test('encoding: reines ASCII ist Windows-1252 (Blitz-Vorgabe)', () => {
	assert.equal(decodeFile(bytesOf('Print 1\n')).encoding, 'windows-1252');
	assert.equal(decodeFile(new Uint8Array(0)).encoding, 'windows-1252');
});

test('encoding: Zeilenenden erkennen, mischen melden, Vorgabe bei keinem', () => {
	assert.equal(decodeFile(bytesOf('a\r\nb\r\n')).eol, '\r\n');
	assert.equal(decodeFile(bytesOf('a\nb\n')).eol, '\n');
	assert.equal(decodeFile(bytesOf('a\r\nb\r\n')).mixedEol, false);

	const mixed = decodeFile(bytesOf('a\r\nb\r\nc\n'));
	assert.equal(mixed.eol, '\r\n');
	assert.equal(mixed.mixedEol, true);
	assert.equal(mixed.text, 'a\nb\nc\n');

	assert.equal(decodeFile(bytesOf('a\rb\rc\r')).text, 'a\nb\nc\n'); // altes Mac-Format
	assert.equal(decodeFile(bytesOf('kein Umbruch'), { defaultEol: '\n' }).eol, '\n');
	assert.equal(decodeFile(bytesOf('kein Umbruch')).eol, '\r\n');
});

test('encoding: schreiben setzt das Zeilenende der Datei', () => {
	same(encodeFile('a\nb\n', 'windows-1252', '\r\n').bytes, bytesOf('a\r\nb\r\n'));
	same(encodeFile('a\nb\n', 'windows-1252', '\n').bytes, bytesOf('a\nb\n'));
	same(encodeFile('a\nb', 'utf-8', '\r\n').bytes, bytesOf('a\r\nb'));
});

test('encoding: nicht darstellbare Zeichen werden gemeldet statt ersetzt', () => {
	const r = encodeFile('Print "→ und 日本"', 'windows-1252', '\r\n');
	assert.equal(r.bytes, undefined);
	assert.deepEqual(r.unmappable, ['→', '日', '本']);
	assert.ok(encodeFile('Print "→"', 'utf-8', '\r\n').bytes); // in UTF-8 geht es
	assert.ok(encodeFile('Print "€ „“"', 'windows-1252', '\r\n').bytes); // 1252-Sonderzeichen gehen

	// astrale Zeichen (Emoji) sind in 1252 nicht darstellbar, brechen aber nichts
	assert.deepEqual(encodeFile('😀', 'windows-1252', '\n').unmappable, ['😀']);
	same(encodeFile('😀', 'utf-8', '\n').bytes, new TextEncoder().encode('😀'));
});

test('encoding: NUL-Bytes werden als "binary" gemeldet', () => {
	assert.equal(decodeFile(Uint8Array.from([0x50, 0x00, 0x51])).binary, true);
	assert.equal(decodeFile(bytesOf('Print 1')).binary, false);
});

test('encoding: unbekannte Kodierung beim Schreiben ist ein Fehler', () => {
	assert.throws(() => encodeFile('a', 'ebcdic', '\n'), /Unbekannte Kodierung/);
});

test('encoding: große Datei (1 MB) ohne Argumentlisten-Überlauf', () => {
	const big = new Uint8Array(1_000_000).fill(0x41);
	big[500_000] = 0xe4;
	const r = decodeFile(big);
	assert.equal(r.text.length, 1_000_000);
	same(encodeFile(r.text, r.encoding, '\n').bytes, big);
});
