// Rauchtest der Compiler-Anbindung.
//
//   npm run test:smoke
//
// Läuft unter Electron (das Backend braucht app.getPath), öffnet aber kein
// Fenster. Prüft die Prozessgrenze zum Compiler von beiden Seiten: dass wir ihn
// finden, seine Kommandoliste lesen und beide Fehlerklassen korrekt einsortieren.
//
// Ohne gefundenen blitzcc wird der Test übersprungen statt zu scheitern — die
// IDE darf ohne Compiler bauen und laufen, und das gilt auch hier.

'use strict';

const { app } = require('electron');
const path = require('path');

const backend = require('../electron/backend');

const FIXTURES = path.join(__dirname, 'fixtures');
const basename = (p) => String(p).split(/[\\/]/).pop();

let failures = 0;
function check(label, ok, detail) {
	console.log(`${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '  -> ' + detail : ''}`);
	if (!ok) failures++;
}

app.whenReady().then(async () => {
	// --- Compiler finden ---------------------------------------------------
	const info = await backend.getInfo();
	if (!info.available) {
		console.log('SKIP  blitzcc nicht gefunden — Test übersprungen.');
		console.log('      Pfad via BLITZPATH setzen oder den Compiler nach bin/ bauen.');
		app.exit(0);
		return;
	}
	check('getInfo: Compiler gefunden', true, `${info.source}: ${info.path}`);
	check('getInfo: Version gelesen', Boolean(info.version), `v${info.version}`);

	// --- Kommandoliste (die Quelle für Autovervollständigung) --------------
	const cmds = await backend.listCommands();
	check('listCommands: nicht leer', cmds.length > 0, `${cmds.length} Befehle`);

	const left = cmds.find((c) => c.name === 'Left');
	check('listCommands: Signatur zerlegt', Boolean(left) && left.params.length === 2,
		left ? `Left(${left.signature})` : 'Left fehlt');

	// --- Erfolgsfall -------------------------------------------------------
	const ok = await backend.compile(path.join(FIXTURES, 'ok.bb'));
	check('compile ok: status', ok.status === 'ok', ok.status);
	check('compile ok: keine Diagnosen', ok.diagnostics.length === 0, `${ok.diagnostics.length}`);
	check('compile ok: outputPath gesetzt', Boolean(ok.outputPath), basename(ok.outputPath));

	// --- Parse-Fehler: gehört als Marker an die Quelle ---------------------
	const parse = await backend.compile(path.join(FIXTURES, 'parse-error.bb'));
	check('compile parse: status', parse.status === 'parse-error', parse.status);
	check('compile parse: Diagnose vorhanden', parse.diagnostics.length > 0, `${parse.diagnostics.length}`);
	check('compile parse: keine Toolchain-Meldung', parse.toolchain.length === 0, `${parse.toolchain.length}`);
	check('compile parse: kein outputPath', parse.outputPath === null, String(parse.outputPath));

	// --- C++-Fehler: darf NICHT als Quellmarker landen ---------------------
	//
	// Läuft der Transpiler durch und scheitert erst g++, meldet blitzcc die
	// Fehler mit den Zeilennummern der generierten .cpp. Als Editor-Marker auf
	// der .bb gesetzt zeigten sie auf die falsche Zeile.
	const cpp = await backend.compile(path.join(FIXTURES, 'cpp-error.bb'));
	check('compile cpp: status', cpp.status === 'compile-error', cpp.status);
	check('compile cpp: als Toolchain einsortiert', cpp.toolchain.length > 0,
		cpp.toolchain.length ? basename(cpp.toolchain[0].file) : '-');
	check('compile cpp: Quellmarker nur aus .bb',
		cpp.diagnostics.every((d) => /\.bb$/i.test(d.file)),
		cpp.diagnostics.map((d) => `${basename(d.file)}:${d.line}`).join(', ') || 'keine');

	console.log(`\n${failures === 0 ? 'Alle Prüfungen bestanden.' : failures + ' fehlgeschlagen.'}`);
	app.exit(failures === 0 ? 0 : 1);
});
