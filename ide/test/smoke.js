// Rauchtest der Compiler-Anbindung.
//
//   npm run test:smoke
//
// Läuft unter reinem Node: der Dienst "toolchain" braucht kein Electron mehr.
// Prüft die Prozessgrenze zum Compiler von beiden Seiten: dass wir ihn
// finden, seine Kommandoliste lesen und beide Fehlerklassen korrekt einsortieren.
//
// Ohne gefundenen blitzcc wird der Test übersprungen statt zu scheitern — die
// IDE darf ohne Compiler bauen und laufen, und das gilt auch hier.

'use strict';

const fs = require('fs');
const path = require('path');

const backend = require('../electron/services/toolchain');

const FIXTURES = path.join(__dirname, 'fixtures');
const basename = (p) => String(p).split(/[\\/]/).pop();

let failures = 0;
function check(label, ok, detail) {
	console.log(`${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '  -> ' + detail : ''}`);
	if (!ok) failures++;
}

(async () => {
	// --- Compiler finden ---------------------------------------------------
	const info = await backend.getInfo();
	if (!info.available) {
		console.log('SKIP  blitzcc nicht gefunden — Test übersprungen.');
		console.log('      Pfad via BLITZPATH setzen oder den Compiler nach bin/ bauen.');
		process.exit(0);
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

	// --- Bauen und Starten im Protokoll der Original-IDE ---------------------
	//
	// Der Runner spricht mit dem echten blitzcc wie die Original-IDE (blitzide=1,
	// Zeilen auf "...", Fehler "datei":z:s:z:s:msg, "Executing..."). Das Programm
	// ist ein `End` und beendet sich sofort.
	const { runBuild } = require('../electron/services/build-runner');
	const build = (file, mode, extra = {}) =>
		new Promise((resolve) => {
			const events = [];
			const handle = runBuild({ file, mode, ...extra }, (e) => {
				events.push(e);
				if (e.type === 'exit' || e.type === 'failed') resolve(events);
			});
			if (!handle) resolve(events);
		});
	const types = (events) => events.map((e) => e.type).join(',');

	const checked = await build(path.join(FIXTURES, 'ok.bb'), 'check');
	check('build check ok: Fortschritt, dann Ende 0', /^progress(,progress)*,exit$/.test(types(checked)) && checked.at(-1).code === 0, types(checked));

	const broken = await build(path.join(FIXTURES, 'parse-error.bb'), 'check');
	const err = broken.find((e) => e.type === 'error');
	check('build check Fehler: Ort und Meldung im IDE-Format',
		Boolean(err) && /parse-error\.bb$/i.test(err.file) && err.line >= 1 && err.column >= 1 && err.endLine >= err.line && Boolean(err.message),
		err ? `${basename(err.file)}:${err.line}:${err.column}-${err.endLine}:${err.endColumn} ${err.message}` : types(broken));
	check('build check Fehler: Ende mit Fehlercode', broken.at(-1).type === 'exit' && broken.at(-1).code !== 0, String(broken.at(-1).code));

	const ran = await build(path.join(FIXTURES, 'run-end.bb'), 'run');
	check('build run: Fortschritt, "Executing", Ende 0 (blitzcc wartet auf das Programm)',
		/^progress(,progress)*,running,exit$/.test(types(ran)) && ran.at(-1).code === 0, types(ran));

	const phases = ran.filter((e) => e.type === 'progress').map((e) => e.text);
	check('build run: Fortschritt je Phase des Compilers (Parsing, Checking, Generating C++, Compiling C++)',
		JSON.stringify(phases) === JSON.stringify(['Compiling...', 'Parsing...', 'Checking...', 'Generating C++...', 'Compiling C++...']),
		phases.join(' | '));

	const exeBefore = fs.existsSync(path.join(FIXTURES, 'run-end.exe'));
	check('build run: neben der Quelle bleibt nichts liegen', !exeBefore);

	const argsFile = path.join(FIXTURES, 'run-args.txt');
	fs.rmSync(argsFile, { force: true });
	const args = await build(path.join(FIXTURES, 'run-args.bb'), 'run', { args: ['eins', 'zwei drei'] });
	const written = fs.existsSync(argsFile) ? fs.readFileSync(argsFile, 'latin1') : null;
	check('build run: Arbeitsordner ist der Ordner der Quelle, Programmargumente kommen an',
		/^progress(,progress)*,running,exit$/.test(types(args)) && written !== null && /eins/.test(written) && /zwei drei/.test(written),
		written === null ? types(args) : JSON.stringify(written));
	fs.rmSync(argsFile, { force: true });

	const missing = await build(path.join(FIXTURES, 'ok.bb'), 'check', { compilerPath: path.join(FIXTURES, 'gibt-es-nicht.exe') });
	check('build: fehlender Compiler wird gemeldet', missing.some((e) => e.type === 'failed' || e.type === 'exit'), types(missing));

	console.log(`\n${failures === 0 ? 'Alle Prüfungen bestanden.' : failures + ' fehlgeschlagen.'}`);
	process.exit(failures === 0 ? 0 : 1);
})();
