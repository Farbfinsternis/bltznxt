// Erweiterung "build-run": Ablauf gegen eine Plattform-Attrappe, in die der Test
// die Ereignisse von blitzcc einspielt. Kein Prozess, kein Electron, kein DOM.

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { createApp } from '../../src/app.js';
import files from '../../src/extensions/files/index.js';
import buildRun from '../../src/extensions/build-run/index.js';

const bytes = (text) => Uint8Array.from(Buffer.from(text, 'latin1'));

function fakePlatform() {
	const fsx = new Map();
	const calls = [];
	const handlers = new Map();
	const answers = { save: [] };
	const key = (p) => String(p).replace(/\//g, '\\');
	const platform = {
		hasBackend: true,
		fsx,
		calls,
		answers,
		store: { async read() { return null; }, async write() {}, async remove() {} },
		on(name, fn) {
			handlers.set(name, fn);
			return () => handlers.delete(name);
		},
		/** Ein Ereignis von blitzcc einspielen, für den letzten gestarteten Bauvorgang. */
		build(event) {
			const start = [...calls].reverse().find((c) => c[0] === 'build' && c[1] === 'start');
			handlers.get('build:event')({ token: start[2].token, ...event });
		},
		lastStart: () => [...calls].reverse().find((c) => c[0] === 'build' && c[1] === 'start')?.[2],
		startResult: { started: true },
		async invoke(service, method, ...args) {
			calls.push([service, method, ...args]);
			if (service === 'files') {
				if (method === 'resolve') return key(args[0]);
				if (method === 'read') {
					const p = key(args[0]);
					if (!fsx.has(p)) throw new Error('ENOENT');
					return { path: p, bytes: fsx.get(p) };
				}
				if (method === 'write') { fsx.set(key(args[0]), Uint8Array.from(args[1])); return key(args[0]); }
				if (method === 'backup') return;
				if (method === 'scratchFile') return `C:\\tmp\\scratch\\${args[0]}\\untitled.bb`;
			}
			if (service === 'dialog' && method === 'save') return answers.save.shift() ?? null;
			if (service === 'build' && method === 'start') return platform.startResult;
			if (service === 'build' && method === 'stop') return 1;
			if (service === 'host') return [];
			throw new Error(`unerwarteter Aufruf ${service}.${method}`);
		}
	};
	return platform;
}

// Die Dienste, die build-run von anderen Erweiterungen erwartet
function stubs(platform) {
	const diagnostics = [];
	const revealed = [];
	const dialogs = { shown: [], queue: [], async message(m) { dialogs.shown.push(m); return dialogs.queue.shift() ?? m.cancelId ?? 'ok'; }, prompts: [], promptAnswers: [], async prompt(p) { dialogs.prompts.push(p); return dialogs.promptAnswers.shift() ?? null; } };
	const status = { text: {}, setText(id, t) { status.text[id] = t; }, clear(id) { delete status.text[id]; } };
	const panel = { shown: [], show(id) { panel.shown.push(id); } };
	return {
		diagnostics, revealed, dialogs, status, panel,
		extensions: [
			{
				id: 'toolchain',
				contributes: { settings: { 'toolchain.compilerPath': { type: 'string', default: '' } } },
				activate(ctx) {
					ctx.context.set('toolchain.available', true);
					ctx.services.provide('toolchain', { info: { available: true, path: 'C:\\bin\\blitzcc.exe' } });
				}
			},
			{
				id: 'editor',
				activate(ctx) {
					ctx.services.provide('editor', {
						setDiagnostics: (docId, owner, list) => diagnostics.push({ docId, owner, list }),
						clearDiagnostics: (owner) => diagnostics.push({ cleared: owner }),
						reveal: (line, column) => revealed.push([line, column])
					});
				}
			}
		]
	};
}

async function world() {
	const platform = fakePlatform();
	const s = stubs(platform);
	const app = createApp({ platform, extensions: [...s.extensions, files, buildRun] });
	app.services.provide('dialogs', s.dialogs);
	app.services.provide('statusbar', s.status);
	app.services.provide('panel', s.panel);
	await app.start();
	return { app, platform, s, docs: app.documents, build: app.services.get('build'), fileOps: app.services.get('files') };
}

const settle = () => new Promise((r) => setTimeout(r, 10));
const put = (w, path, text) => w.platform.fsx.set(path, bytes(text));

// ------------------------------------------------------------------ Start

test('build-run: F5 speichert geänderte benannte Dateien, startet blitzcc mit dem Pfad, merkt sich die Datei', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'Print 1');
	put(w, 'C:\\p\\b.bb', 'Print 2');
	const a = await w.fileOps.openPath('C:\\p\\a.bb');
	const b = await w.fileOps.openPath('C:\\p\\b.bb');
	w.docs.setText(a.id, 'Print 10');
	w.docs.activate(b.id);

	assert.equal(await w.build.run(), true);
	// a wurde gespeichert, b war unverändert
	assert.equal(Buffer.from(w.platform.fsx.get('C:\\p\\a.bb')).toString('latin1'), 'Print 10');
	const start = w.platform.lastStart();
	assert.equal(start.file, 'C:\\p\\b.bb');
	assert.equal(start.mode, 'run');
	assert.equal(start.debug, true); // Original: Debug ist an
	assert.equal(start.commandLine, '');
	assert.equal(w.app.state.get('build.lastFile'), 'C:\\p\\b.bb');
	assert.equal(w.app.context.get('build.compiling'), true);
	assert.equal(w.app.context.get('build.hasLast'), true);
});

test('build-run: F7 prüft nur (Modus check), Programm erstellen fragt nach dem Ziel', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.check();
	assert.equal(w.platform.lastStart().mode, 'check');
	w.platform.build({ type: 'exit', code: 0 });
	await settle();

	w.platform.answers.save.push(null); // Abbruch im Speichern-Dialog: nichts startet
	assert.equal(await w.build.publish(), false);
	const dialog = w.platform.calls.filter((c) => c[0] === 'dialog' && c[1] === 'save').at(-1);
	assert.equal(dialog[2].defaultPath, 'C:\\p\\a.exe');

	w.platform.answers.save.push('C:\\out\\spiel'); // ohne Endung: .exe wird ergänzt
	assert.equal(await w.build.publish(), true);
	assert.deepEqual([w.platform.lastStart().mode, w.platform.lastStart().output], ['publish', 'C:\\out\\spiel.exe']);
});

test('build-run: ein namenloser Tab geht in den Temp-Ordner, bleibt aber namenlos', async () => {
	const w = await world();
	const doc = w.fileOps.newFile();
	w.docs.setText(doc.id, 'Print "Grüße"\n');
	await w.build.run();
	const start = w.platform.lastStart();
	assert.match(start.file, /^C:\\tmp\\scratch\\doc\d+\\untitled\.bb$/);
	assert.equal(doc.uri, null);
	assert.equal(doc.kind, 'scratch');
	assert.equal(w.app.state.get('build.lastFile'), undefined); // wie im Original nur für benannte Dateien
	// in Windows-1252 mit CRLF geschrieben, Umlaut ein Byte
	assert.deepEqual([...w.platform.fsx.get(start.file)], [0x50, 0x72, 0x69, 0x6e, 0x74, 0x20, 0x22, 0x47, 0x72, 0xfc, 0xdf, 0x65, 0x22, 0x0d, 0x0a]);
});

test('build-run: zwei namenlose Tabs bekommen zwei verschiedene Dateien', async () => {
	const w = await world();
	const a = w.fileOps.newFile();
	const b = w.fileOps.newFile();
	w.docs.setText(a.id, 'a');
	w.docs.setText(b.id, 'b');
	await w.build.run();
	const first = w.platform.lastStart().file;
	w.platform.build({ type: 'exit', code: 0 });
	await settle();
	w.docs.activate(a.id);
	await w.build.run();
	assert.notEqual(w.platform.lastStart().file, first);
});

test('build-run: Debug-Schalter und Kommandozeile gehen mit', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.app.commands.execute('program.debug');
	w.app.settings.set('program.commandLine', '-w "a b"');
	await w.build.run();
	assert.equal(w.platform.lastStart().debug, false);
	assert.equal(w.platform.lastStart().commandLine, '-w "a b"');
	assert.equal(w.app.context.get('program.debugOn'), false);
});

test('build-run: Programm-Kommandozeile über den Dialog setzen; Abbruch ändert nichts', async () => {
	const w = await world();
	w.s.dialogs.promptAnswers.push('level1', null);
	await w.app.commands.execute('program.commandLine');
	assert.equal(w.app.settings.get('program.commandLine'), 'level1');
	await w.app.commands.execute('program.commandLine');
	assert.equal(w.app.settings.get('program.commandLine'), 'level1');
	assert.equal(w.s.dialogs.prompts[1].value, 'level1'); // zeigt den jetzigen Wert
});

// ---------------------------------------------------------------- Ereignisse

test('build-run: Fortschritt, Executing, Ende — Kontext und Statuszeile folgen', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	assert.equal(w.s.status.text['build.status'], 'Compiling...');
	assert.equal(w.app.commands.isEnabled('program.run'), false); // baut noch

	w.platform.build({ type: 'progress', text: 'Compiling...' });
	w.platform.build({ type: 'running' });
	await settle();
	assert.equal(w.app.context.get('build.compiling'), false);
	assert.equal(w.app.context.get('program.running'), true);
	assert.equal(w.s.status.text['build.status'], 'Program running');
	assert.equal(w.app.commands.isEnabled('program.run'), true); // während das Programm läuft, darf man weiterarbeiten
	assert.equal(w.app.commands.isEnabled('program.stop'), true);

	w.platform.build({ type: 'exit', code: 0 });
	await settle();
	assert.equal(w.app.context.get('program.running'), false);
	assert.equal(w.s.status.text['build.status'], undefined);
	const texts = w.build.lines.map((l) => l.text);
	assert.equal(texts[0], 'Building a.bb'); // vom ersten Augenblick an zu sehen
	assert.equal(texts[1], 'Compiling...');
	assert.match(texts[2], /^Built in \d+\.\d s\.$/);
	assert.equal(texts[3], 'Executing...');
	assert.ok(w.s.panel.shown.includes('output')); // der Ausgabe-Reiter kommt beim Start nach vorn
});

test('build-run: F7 ohne Fehler meldet "No errors found."', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.check();
	w.platform.build({ type: 'progress', text: 'Compiling...' });
	w.platform.build({ type: 'exit', code: 0 });
	await settle();
	assert.equal(w.build.lines.at(-1).text, 'No errors found.');
	assert.equal(w.app.context.get('build.compiling'), false);
});

test('build-run: ein Fehler setzt Marker und Cursor, öffnet den Ausgabe-Reiter und zeigt einen Dialog', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'If x = 1 THEN THEN');
	const doc = await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'progress', text: 'Compiling...' });
	w.platform.build({ type: 'error', file: 'C:\\p\\a.bb', line: 1, column: 15, endLine: 1, endColumn: 19, message: "Unexpected token 'THEN'" });
	w.platform.build({ type: 'exit', code: 255 });
	await settle();

	const marker = w.s.diagnostics.find((d) => d.owner === 'blitzcc');
	assert.equal(marker.docId, doc.id);
	assert.deepEqual(marker.list, [{ line: 1, column: 15, endLine: 1, endColumn: 19, severity: 'error', message: "Unexpected token 'THEN'" }]);
	assert.deepEqual(w.s.revealed.at(-1), [1, 15]);
	assert.ok(w.s.panel.shown.includes('output'));
	assert.equal(w.s.dialogs.shown.at(-1).text, "Unexpected token 'THEN'");
	assert.equal(w.build.lines.at(-1).text, "a.bb:1:15: Unexpected token 'THEN'");
	assert.equal(w.app.context.get('build.compiling'), false);
	assert.equal(w.app.context.get('program.running'), false);
	// der Fehler ist die Meldung; kein zusätzlicher "exited with code"
	assert.equal(w.build.lines.filter((l) => /exited with code/.test(l.text)).length, 0);
});

test('build-run: Fehler in einer anderen Datei (Include) öffnet sie und setzt den Marker dort', async () => {
	const w = await world();
	put(w, 'C:\\p\\main.bb', 'Include "lib.bb"');
	put(w, 'C:\\p\\lib.bb', 'kaputt');
	const main = await w.fileOps.openPath('C:\\p\\main.bb');
	await w.build.run();
	w.platform.build({ type: 'error', file: 'C:\\p\\lib.bb', line: 1, column: 1, endLine: 1, endColumn: 7, message: 'bad' });
	await settle();
	const lib = w.docs.find('C:\\p\\lib.bb');
	assert.ok(lib, 'lib.bb wurde geöffnet');
	assert.equal(w.docs.active.id, lib.id);
	assert.equal(w.s.diagnostics.find((d) => d.owner === 'blitzcc').docId, lib.id);
	assert.notEqual(lib.id, main.id);
});

test('build-run: Fehler im namenlosen Tab landet im Tab, nicht in der Temp-Datei', async () => {
	const w = await world();
	const doc = w.fileOps.newFile();
	w.docs.setText(doc.id, 'x = ');
	await w.build.run();
	const file = w.platform.lastStart().file;
	w.platform.build({ type: 'error', file, line: 1, column: 5, endLine: 1, endColumn: 5, message: 'Expecting expression' });
	await settle();
	assert.equal(w.docs.list().length, 1); // kein zweiter Tab für die Temp-Datei
	assert.equal(w.s.diagnostics.find((d) => d.owner === 'blitzcc').docId, doc.id);
	assert.equal(w.build.lines.at(-1).text, '<untitled>:1:5: Expecting expression');
});

test('build-run: ein Fehler ohne Ort (etwa "Compiler environment error") wird gemeldet', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'error', message: 'C++ compilation failed - run blitzcc from a terminal to see why' });
	await settle();
	assert.equal(w.s.dialogs.shown.at(-1).text, 'C++ compilation failed - run blitzcc from a terminal to see why');
	assert.equal(w.s.diagnostics.filter((d) => d.owner === 'blitzcc').length, 0);
});

test('build-run: der Fehlerdialog lässt sich abschalten, der Ausgabe-Reiter bleibt', async () => {
	const w = await world();
	w.app.settings.set('build.errorDialog', false);
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'error', file: 'C:\\p\\a.bb', line: 1, column: 1, endLine: 1, endColumn: 2, message: 'nein' });
	await settle();
	assert.equal(w.s.dialogs.shown.length, 0);
	assert.ok(w.s.panel.shown.includes('output'));
});

test('build-run: der nächste Bauvorgang löscht Ausgabe und Marker', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'error', file: 'C:\\p\\a.bb', line: 1, column: 1, endLine: 1, endColumn: 2, message: 'nein' });
	w.platform.build({ type: 'exit', code: 255 });
	await settle();
	assert.ok(w.build.lines.length > 0);
	await w.build.run();
	assert.deepEqual(w.build.lines.map((l) => l.text), ['Building a.bb']); // der alte Fehler ist weg
	assert.ok(w.s.diagnostics.some((d) => d.cleared === 'blitzcc'));
});

test('build-run: blitzcc lässt sich nicht starten — Meldung, nichts bleibt hängen', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	w.platform.startResult = { started: false };
	assert.equal(await w.build.run(), false);
	w.platform.build({ type: 'failed', message: 'blitzcc not found.' });
	await settle();
	assert.equal(w.app.context.get('build.compiling'), false);
	assert.match(w.s.dialogs.shown.at(-1).text, /blitzcc not found/);
});

test('build-run: eine Ausnahme beim Start setzt den Zustand zurück', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	const original = w.platform.invoke;
	w.platform.invoke = async (service, method, ...args) => {
		if (service === 'build' && method === 'start') throw new Error('IPC kaputt');
		return original(service, method, ...args);
	};
	assert.equal(await w.build.run(), false);
	assert.equal(w.app.context.get('build.compiling'), false);
	assert.match(w.s.dialogs.shown.at(-1).text, /IPC kaputt/);
});

// ------------------------------------------------------------ F6, Stop, Sperren

test('build-run: F6 öffnet die zuletzt gebaute Datei und startet sie', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	put(w, 'C:\\p\\b.bb', 'y');
	const a = await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'exit', code: 0 });
	await settle();
	await w.fileOps.close(a.id);
	await w.fileOps.openPath('C:\\p\\b.bb');
	assert.equal(w.docs.list().length, 1);

	assert.equal(await w.build.rerun(), true);
	assert.equal(w.platform.lastStart().file, 'C:\\p\\a.bb'); // wieder geöffnet und gebaut
	assert.equal(w.docs.active.uri, 'C:\\p\\a.bb');
});

test('build-run: F6 ist gesperrt, solange es nichts Gebautes gibt', async () => {
	const w = await world();
	assert.equal(w.app.commands.isEnabled('program.rerun'), false);
});

test('build-run: Stop beendet die Programme; das Ende wird nicht als Fehler gemeldet', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	await w.fileOps.openPath('C:\\p\\a.bb');
	await w.build.run();
	w.platform.build({ type: 'running' });
	await settle();
	await w.app.commands.execute('program.stop');
	assert.ok(w.platform.calls.some((c) => c[0] === 'build' && c[1] === 'stop'));
	w.platform.build({ type: 'exit', code: 1 });
	await settle();
	assert.equal(w.build.lines.filter((l) => l.kind === 'error').length, 0);
	assert.equal(w.build.lines.at(-1).text, 'Program stopped.');
	assert.equal(w.app.context.get('program.running'), false);
});

test('build-run: Befehle sind gesperrt ohne Dokument und während blitzcc baut', async () => {
	const w = await world();
	const enabled = (id) => w.app.commands.isEnabled(id);
	assert.deepEqual(['program.run', 'program.check', 'program.publish', 'program.stop'].map(enabled), [false, false, false, false]);
	assert.equal(enabled('program.commandLine'), true);
	assert.equal(enabled('program.debug'), true);
	w.fileOps.newFile();
	assert.deepEqual(['program.run', 'program.check', 'program.publish'].map(enabled), [true, true, true]);
	await w.build.run();
	assert.deepEqual(['program.run', 'program.check', 'program.publish'].map(enabled), [false, false, false]);
});

test('build-run: abgebrochenes Speichern vor dem Bauen startet nichts', async () => {
	const w = await world();
	put(w, 'C:\\p\\a.bb', 'x');
	const doc = await w.fileOps.openPath('C:\\p\\a.bb');
	w.docs.setText(doc.id, 'Grüße → €');
	w.s.dialogs.queue.push('cancel'); // Zeichen, die 1252 nicht kennt -> Rückfrage -> Abbruch
	assert.equal(await w.build.run(), false);
	assert.equal(w.platform.lastStart(), undefined);
});
