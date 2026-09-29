// Das Bau-Protokoll der Original-IDE, an festen Zeilen.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { parseLine, createLineSplitter, splitCommandLine, buildArgs } = require('../../electron/build-protocol');

test('protocol: Fortschritt endet auf "..."', () => {
	assert.deepEqual(parseLine('Compiling...'), { kind: 'progress', text: 'Compiling...' });
	assert.deepEqual(parseLine('Creating executable "C:\\x\\a.exe"...\r'), { kind: 'progress', text: 'Creating executable "C:\\x\\a.exe"...' });
});

test('protocol: Executing heißt, das Programm läuft', () => {
	assert.deepEqual(parseLine('Executing...'), { kind: 'executing' });
});

test('protocol: Fehler mit Ort — Datei, Anfang, Ende, Meldung', () => {
	const e = parseLine('"C:\\prog\\a.bb":12:5:12:9:Unexpected token \'THEN\'');
	assert.deepEqual(e, { kind: 'error', file: 'C:\\prog\\a.bb', line: 12, column: 5, endLine: 12, endColumn: 9, message: "Unexpected token 'THEN'" });
});

test('protocol: die Meldung darf Doppelpunkte enthalten, der Pfad auch (C:)', () => {
	const e = parseLine('"D:\\a b\\c.bb":3:1:3:4:Expecting: identifier');
	assert.equal(e.file, 'D:\\a b\\c.bb');
	assert.equal(e.message, 'Expecting: identifier');
});

test('protocol: kaputt formatierte Fehlerzeile und jede andere Zeile sind Fehler ohne Ort', () => {
	assert.deepEqual(parseLine('"nur ein Name"'), { kind: 'error', message: '"nur ein Name"' });
	assert.deepEqual(parseLine('"a.bb":x:1:1:1:msg'), { kind: 'error', message: '"a.bb":x:1:1:1:msg' });
	assert.deepEqual(parseLine('Compiler environment error'), { kind: 'error', message: 'Compiler environment error' });
});

test('protocol: nach "Executing" ist alles Ausgabe, nie ein Fehler', () => {
	assert.deepEqual(parseLine('Runtime error: out of memory', { running: true }), { kind: 'output', text: 'Runtime error: out of memory' });
	assert.deepEqual(parseLine('Compiling...', { running: true }), { kind: 'output', text: 'Compiling...' });
});

test('protocol: leere Zeilen werden übergangen', () => {
	assert.equal(parseLine(''), null);
	assert.equal(parseLine('   \r'), null);
});

test('protocol: Zeilenzerleger fügt Blöcke zusammen, die mitten in einer Zeile enden', () => {
	const s = createLineSplitter();
	assert.deepEqual(s.push('Compil'), []);
	assert.deepEqual(s.push('ing...\r\nExecut'), ['Compiling...']);
	assert.deepEqual(s.push('ing...\n'), ['Executing...']);
	assert.deepEqual(s.push('Rest ohne Umbruch'), []);
	assert.deepEqual(s.flush(), ['Rest ohne Umbruch']);
	assert.deepEqual(s.flush(), []);
});

test('protocol: Programmargumente — Leerzeichen trennen, Anführungszeichen fassen zusammen', () => {
	assert.deepEqual(splitCommandLine(''), []);
	assert.deepEqual(splitCommandLine('   '), []);
	assert.deepEqual(splitCommandLine('-fullscreen level1'), ['-fullscreen', 'level1']);
	assert.deepEqual(splitCommandLine('"C:\\my files\\a.txt" b'), ['C:\\my files\\a.txt', 'b']);
	assert.deepEqual(splitCommandLine('a "" b'), ['a', '', 'b']);
	assert.deepEqual(splitCommandLine(undefined), []);
});

test('protocol: Argumente für blitzcc je Modus', () => {
	assert.deepEqual(buildArgs({ file: 'a.bb', mode: 'run' }), ['-q', 'a.bb']);
	assert.deepEqual(buildArgs({ file: 'a.bb', mode: 'run', debug: true, args: ['x', 'y z'] }), ['-q', '-d', 'a.bb', 'x', 'y z']);
	assert.deepEqual(buildArgs({ file: 'a.bb', mode: 'check', debug: true }), ['-q', '-d', '-c', 'a.bb']);
	assert.deepEqual(buildArgs({ file: 'a.bb', mode: 'publish', output: 'C:\\o.exe' }), ['-q', '-o', 'C:\\o.exe', 'a.bb']);
	assert.throws(() => buildArgs({ file: 'a.bb', mode: 'publish' }), /Ausgabenamen/);
	assert.throws(() => buildArgs({ file: 'a.bb', mode: 'fliegen' }), /Unbekannter Modus/);
});
