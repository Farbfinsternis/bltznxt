// redecode: Zeilen aus blitzcc unter Windows und Linux richtig lesen
// (LINUX_PORT.md, L5).

import test from 'node:test';
import assert from 'node:assert';
import { createRequire } from 'node:module';

// Der Runner ist ein CommonJS-Modul des Main-Prozesses
const { redecode } = createRequire(import.meta.url)('../../electron/services/build-runner.js');

// So kommt eine Zeile im Runner an: jedes Byte als ein latin1-Zeichen
const asRead = (bytes) => Buffer.from(bytes).toString('latin1');

test('Windows: die Zeile bleibt, wie sie ist', () => {
	const line = asRead([0x4b, 0xe4, 0x73, 0x65]);   // "Käse" in Windows-1252
	assert.strictEqual(redecode(line, 'win32'), line);
});

test('Linux: reines ASCII bleibt', () => {
	assert.strictEqual(redecode('Compiling...', 'linux'), 'Compiling...');
});

test('Linux: UTF-8 (Pfad in einer Meldung von blitzcc) wird UTF-8', () => {
	const line = asRead(Buffer.from('"/home/x/grüße.bb":1:5:1:5:unexpected token', 'utf8'));
	assert.strictEqual(redecode(line, 'linux'), '"/home/x/grüße.bb":1:5:1:5:unexpected token');
});

test('Linux: Programmausgabe in Windows-1252 wird richtig gelesen', () => {
	assert.strictEqual(redecode(asRead([0x4b, 0xe4, 0x73, 0x65]), 'linux'), 'Käse');
	assert.strictEqual(redecode(asRead([0x80, 0x20, 0x35]), 'linux'), '€ 5');
});
