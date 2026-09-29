// Einsortierung der Compiler-Meldungen an fester Ausgabe — unabhängig davon,
// welche Programme der Compiler gerade bis g++ durchlässt (siehe smoke.js).

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { parseDiagnostics, isSourceDiagnostic, statusFromExitCode } = require('../../electron/diagnostics');

// Ausgabe, wie blitzcc sie bei einem C++-Fehler liefert (Zeilennummern der .cpp)
const CPP_FAILURE = [
	'F:/dev/projects/bltznxt/src/compiler/bb_object.h:33:17: error: invalid use of incomplete type',
	'p4.cpp:6:10: error: \'var_x\' was not declared in this scope',
	'p4.bb:0:0: error: compilation failed'
].join('\n');

test('diagnostics: C++-Meldungen sind keine Quellmarker', () => {
	const { diagnostics } = parseDiagnostics(CPP_FAILURE);
	const source = diagnostics.filter(isSourceDiagnostic);
	const toolchain = diagnostics.filter((d) => !isSourceDiagnostic(d));
	assert.equal(toolchain.length, 2);
	assert.deepEqual(source.map((d) => d.file), ['p4.bb']);
	assert.equal(source[0].line, 1); // 0:0 wird für Monaco auf 1 angehoben
});

test('diagnostics: Quellfehler mit Zeile und Spalte', () => {
	const { diagnostics, unparsed } = parseDiagnostics('parse.bb:3:9: error: unexpected token \'THEN\'\n[runtime] etwas\n');
	assert.equal(diagnostics.length, 1);
	assert.deepEqual([diagnostics[0].line, diagnostics[0].column, diagnostics[0].severity], [3, 9, 'error']);
	assert.deepEqual(unparsed, ['[runtime] etwas']); // Rohausgabe geht nicht verloren
});

test('diagnostics: Exit-Codes', () => {
	assert.equal(statusFromExitCode(0), 'ok');
	assert.equal(statusFromExitCode(1), 'parse-error');
	assert.equal(statusFromExitCode(2), 'compile-error');
	assert.equal(statusFromExitCode(-1), 'failed');
});
