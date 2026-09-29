// Erweiterung "files": Abläufe gegen ein virtuelles Dateisystem und eine
// Dialog-Attrappe. Kein Electron, kein DOM.

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { createApp } from '../../src/app.js';
import files from '../../src/extensions/files/index.js';
import { decodeFile, encodeFile } from '../../src/core/encoding.js';

const bytes = (...parts) => Uint8Array.from(parts.flatMap((p) => (typeof p === 'string' ? [...Buffer.from(p, 'latin1')] : p)));

/** Ein Dateisystem im Speicher hinter der Plattform-Schnittstelle. */
function fakePlatform({ launch = [] } = {}) {
	const fsx = new Map(); // Pfad -> Uint8Array
	const calls = [];
	const handlers = new Map();
	const answers = { open: [], save: [] };
	const failWrite = new Set();

	const key = (p) => String(p).replace(/\//g, '\\');
	const platform = {
		hasBackend: true,
		fsx,
		calls,
		answers,
		failWrite,
		store: { async read() { return null; }, async write() {}, async remove() {} },
		on(name, fn) {
			handlers.set(name, fn);
			return () => handlers.delete(name);
		},
		emit: (name, payload) => handlers.get(name)?.(payload),
		async invoke(service, method, ...args) {
			calls.push([service, method, ...args]);
			if (service === 'files') {
				if (method === 'resolve') return key(args[0]);
				if (method === 'read') {
					const p = key(args[0]);
					if (!fsx.has(p)) throw Object.assign(new Error('ENOENT: no such file'), { code: 'ENOENT' });
					return { path: p, bytes: fsx.get(p) };
				}
				if (method === 'write') {
					if (failWrite.has(key(args[0]))) throw new Error('EACCES: permission denied');
					fsx.set(key(args[0]), Uint8Array.from(args[1]));
					return key(args[0]);
				}
				if (method === 'backup') return;
			}
			if (service === 'dialog') return answers[method].length ? answers[method].shift() : null;
			if (service === 'host' && method === 'launchFiles') return launch;
			if (service === 'window' && method === 'close') return;
			throw new Error(`unerwarteter Aufruf ${service}.${method}`);
		}
	};
	return platform;
}

/** Ersetzt den Dialogdienst der Shell; Antworten stehen in einer Warteschlange. */
function fakeDialogs() {
	const shown = [];
	const queue = [];
	return {
		shown,
		queue,
		async message(m) {
			shown.push(m);
			const next = queue.shift();
			return next ?? m.cancelId ?? m.buttons?.[0]?.id ?? 'ok';
		}
	};
}

async function world(opts = {}) {
	const platform = fakePlatform(opts);
	const dialogs = fakeDialogs();
	const app = createApp({ platform, extensions: [files] });
	app.services.provide('dialogs', dialogs);
	await app.start();
	return { app, platform, dialogs, ops: app.services.get('files'), docs: app.documents };
}

const put = (w, path, data) => w.platform.fsx.set(path, data);
const read = (w, path) => [...w.platform.fsx.get(path)];

// -------------------------------------------------------------------- neu

test('files: Neu legt ein namenloses Dokument mit den Vorgaben an, ungespeichert-frei', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	assert.equal(doc.kind, 'scratch');
	assert.equal(doc.encoding, 'windows-1252');
	assert.equal(doc.eol, '\r\n');
	assert.equal(doc.dirty, false);
	assert.equal(w.docs.active.id, doc.id);
	// leeres namenloses Dokument lässt sich ohne Rückfrage schließen
	assert.equal(await w.ops.close(), true);
	assert.equal(w.dialogs.shown.length, 0);
});

test('files: Vorgaben aus den Einstellungen gelten für neue Dateien', async () => {
	const w = await world();
	w.app.settings.set('files.defaultEncoding', 'utf-8');
	w.app.settings.set('files.defaultEol', 'lf');
	const doc = w.ops.newFile();
	assert.deepEqual([doc.encoding, doc.eol], ['utf-8', '\n']);
});

// ----------------------------------------------------------------- öffnen

test('files: Öffnen liest Windows-1252 mit CRLF; Speichern nach einer Änderung ist sonst byte-gleich', async () => {
	const w = await world();
	const original = bytes('; Gr', [0xfc], [0xdf], 'e', [0x0d, 0x0a], 'Print 1', [0x0d, 0x0a]);
	put(w, 'C:\\prog\\a.bb', original);

	const doc = await w.ops.openPath('C:\\prog\\a.bb');
	assert.equal(doc.text, '; Grüße\nPrint 1\n');
	assert.deepEqual([doc.encoding, doc.eol, doc.dirty], ['windows-1252', '\r\n', false]);

	// unverändert speichern: dieselben Bytes
	assert.equal(await w.ops.save(), true);
	assert.deepEqual(read(w, 'C:\\prog\\a.bb'), [...original]);

	// eine Zeile ändern: nur diese Stelle unterscheidet sich
	w.docs.setText(doc.id, '; Grüße\nPrint 2\n');
	assert.equal(await w.ops.save(), true);
	assert.deepEqual(read(w, 'C:\\prog\\a.bb'), [...bytes('; Gr', [0xfc], [0xdf], 'e', [0x0d, 0x0a], 'Print 2', [0x0d, 0x0a])]);
	assert.equal(doc.dirty, false);
});

test('files: dieselbe Datei nicht zweimal öffnen (auch nicht mit anderer Schreibweise)', async () => {
	const w = await world();
	put(w, 'C:\\prog\\a.bb', bytes('x'));
	const a = await w.ops.openPath('C:\\prog\\a.bb');
	const b = await w.ops.openPath('c:\\PROG\\A.bb');
	assert.equal(a.id, b.id);
	assert.equal(w.docs.list().length, 1);
	assert.equal(w.platform.calls.filter((c) => c[1] === 'read').length, 1); // nicht neu gelesen
});

test('files: Lesefehler wird gemeldet, kein Dokument entsteht', async () => {
	const w = await world();
	assert.equal(await w.ops.openPath('C:\\fehlt.bb'), null);
	assert.equal(w.docs.list().length, 0);
	assert.match(w.dialogs.shown[0].text, /Error reading file "C:\\fehlt\.bb"/);
});

test('files: Datei mit NUL-Bytes fragt nach; Abbrechen öffnet nichts', async () => {
	const w = await world();
	put(w, 'C:\\m.dat', Uint8Array.from([1, 0, 2]));
	w.dialogs.queue.push('cancel');
	assert.equal(await w.ops.openPath('C:\\m.dat'), null);
	assert.equal(w.docs.list().length, 0);
	w.dialogs.queue.push('open');
	assert.ok(await w.ops.openPath('C:\\m.dat'));
});

test('files: Öffnen-Dialog: mehrere Dateien, Abbruch, Filter und Startordner', async () => {
	const w = await world();
	put(w, 'C:\\d\\a.bb', bytes('a'));
	put(w, 'C:\\d\\b.bb', bytes('b'));
	w.platform.answers.open.push(['C:\\d\\a.bb', 'C:\\d\\b.bb']);
	const opened = await w.ops.open();
	assert.deepEqual(opened.map((d) => d.title), ['a.bb', 'b.bb']);
	assert.equal(w.docs.active.title, 'b.bb');

	const dialogCall = w.platform.calls.find((c) => c[0] === 'dialog' && c[1] === 'open');
	assert.deepEqual(dialogCall[2].filters.map((f) => f.extensions[0]), ['bb', '*']);

	// zweiter Aufruf beginnt im Ordner der zuletzt geöffneten Datei
	w.platform.answers.open.push(null);
	assert.deepEqual(await w.ops.open(), []);
	const second = w.platform.calls.filter((c) => c[0] === 'dialog' && c[1] === 'open')[1];
	assert.equal(second[2].defaultPath, 'C:\\d');
});

// -------------------------------------------------------------- speichern

test('files: Speichern legt zuerst eine Sicherung an (Anzahl aus den Einstellungen)', async () => {
	const w = await world();
	put(w, 'C:\\a.bb', bytes('x'));
	const doc = await w.ops.openPath('C:\\a.bb');
	w.docs.setText(doc.id, 'y');
	await w.ops.save();
	const order = w.platform.calls.filter((c) => c[0] === 'files' && (c[1] === 'backup' || c[1] === 'write')).map((c) => c[1]);
	assert.deepEqual(order, ['backup', 'write']);
	assert.equal(w.platform.calls.find((c) => c[1] === 'backup')[3], 2);

	w.app.settings.set('files.backups', 5);
	w.docs.setText(doc.id, 'z');
	await w.ops.save();
	assert.equal(w.platform.calls.filter((c) => c[1] === 'backup')[1][3], 5);
});

test('files: Speichern eines namenlosen Dokuments fragt nach dem Ort, ergänzt .bb, macht es zur Datei', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	w.docs.setText(doc.id, 'Print "Grüße"\n');
	w.platform.answers.save.push('C:\\neu\\spiel'); // ohne Endung
	assert.equal(await w.ops.save(), true);

	assert.equal(doc.uri, 'C:\\neu\\spiel.bb');
	assert.equal(doc.kind, 'file');
	assert.equal(doc.dirty, false);
	// genauer: Umlaut als ein Byte 0xFC, Zeilenende CRLF, kein UTF-8
	assert.deepEqual(read(w, 'C:\\neu\\spiel.bb'), [...bytes('Print "Gr', [0xfc], [0xdf], 'e"', [0x0d, 0x0a])]);
});

test('files: Abbruch im Speichern-Dialog ändert nichts', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	w.docs.setText(doc.id, 'x');
	w.platform.answers.save.push(null);
	assert.equal(await w.ops.save(), false);
	assert.equal(doc.uri, null);
	assert.equal(doc.dirty, true);
	assert.equal(w.platform.fsx.size, 0);
});

test('files: Speichern unter gibt der Datei einen neuen Ort, die alte bleibt', async () => {
	const w = await world();
	put(w, 'C:\\a.bb', bytes('x'));
	const doc = await w.ops.openPath('C:\\a.bb');
	w.platform.answers.save.push('C:\\b.bb');
	assert.equal(await w.ops.save({ as: true }), true);
	assert.equal(doc.uri, 'C:\\b.bb');
	assert.deepEqual(read(w, 'C:\\a.bb'), [120]);
	assert.deepEqual(read(w, 'C:\\b.bb'), [120]);
});

test('files: Speichern unter auf eine in einem anderen Tab offene Datei wird verweigert', async () => {
	const w = await world();
	put(w, 'C:\\a.bb', bytes('a'));
	put(w, 'C:\\b.bb', bytes('b'));
	await w.ops.openPath('C:\\a.bb');
	const b = await w.ops.openPath('C:\\b.bb');
	w.platform.answers.save.push('c:\\A.BB');
	assert.equal(await w.ops.save({ doc: b, as: true }), false);
	assert.match(w.dialogs.shown.at(-1).text, /already open in another tab/);
	assert.equal(b.uri, 'C:\\b.bb');
});

test('files: Zeichen, die Windows-1252 nicht kennt: Abbrechen schreibt nichts, UTF-8 wechselt die Kodierung', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	w.docs.setText(doc.id, 'Print "→"\n');
	w.platform.answers.save.push('C:\\u.bb', 'C:\\u.bb');

	w.dialogs.queue.push('cancel');
	assert.equal(await w.ops.save(), false);
	assert.equal(w.platform.fsx.has('C:\\u.bb'), false);
	assert.equal(doc.encoding, 'windows-1252');
	assert.match(w.dialogs.shown.at(-1).text, /→/);

	w.dialogs.queue.push('utf8');
	assert.equal(await w.ops.save(), true);
	assert.equal(doc.encoding, 'utf-8');
	assert.deepEqual(read(w, 'C:\\u.bb'), [...new TextEncoder().encode('Print "→"\r\n')]);
});

test('files: Schreibfehler wird gemeldet, das Dokument bleibt ungespeichert', async () => {
	const w = await world();
	put(w, 'C:\\a.bb', bytes('x'));
	const doc = await w.ops.openPath('C:\\a.bb');
	w.docs.setText(doc.id, 'y');
	w.platform.failWrite.add('C:\\a.bb');
	assert.equal(await w.ops.save(), false);
	assert.equal(doc.dirty, true);
	assert.match(w.dialogs.shown.at(-1).text, /Error writing file "C:\\a\.bb"\nEACCES/);
});

test('files: Alle speichern geht von hinten nach vorn und stoppt beim Abbruch', async () => {
	const w = await world();
	const a = w.ops.newFile();
	const b = w.ops.newFile();
	const c = w.ops.newFile();
	for (const d of [a, b, c]) w.docs.setText(d.id, 'x');
	w.platform.answers.save.push('C:\\c.bb', null); // c wird gespeichert, bei b bricht der Benutzer ab
	assert.equal(await w.ops.saveAll(), false);
	assert.equal(c.uri, 'C:\\c.bb');
	assert.equal(b.uri, null);
	assert.equal(a.uri, null); // kam nicht mehr dran
});

// --------------------------------------------------------------- schließen

test('files: Schließen mit Änderungen — Ja speichert, Nein verwirft, Abbrechen behält', async () => {
	const w = await world();
	put(w, 'C:\\a.bb', bytes('x'));
	const doc = await w.ops.openPath('C:\\a.bb');
	w.docs.setText(doc.id, 'y');

	w.dialogs.queue.push('cancel');
	assert.equal(await w.ops.close(doc.id), false);
	assert.equal(w.docs.list().length, 1);
	assert.match(w.dialogs.shown.at(-1).text, /File C:\\a\.bb has been modified!\nSave changes before closing\?/);

	w.dialogs.queue.push('no');
	assert.equal(await w.ops.close(doc.id), true);
	assert.equal(w.docs.list().length, 0);
	assert.deepEqual(read(w, 'C:\\a.bb'), [120]); // unverändert auf der Platte

	const again = await w.ops.openPath('C:\\a.bb');
	w.docs.setText(again.id, 'z');
	w.dialogs.queue.push('yes');
	assert.equal(await w.ops.close(again.id), true);
	assert.deepEqual(read(w, 'C:\\a.bb'), [122]); // gespeichert
});

test('files: "Ja" mit abgebrochenem Speichern-Dialog schließt nicht', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	w.docs.setText(doc.id, 'x');
	w.dialogs.queue.push('yes');
	w.platform.answers.save.push(null);
	assert.equal(await w.ops.close(doc.id), false);
	assert.equal(w.docs.list().length, 1);
});

test('files: Alle schließen fragt von hinten nach vorn und hört beim Abbruch auf', async () => {
	const w = await world();
	const a = w.ops.newFile();
	const b = w.ops.newFile();
	const c = w.ops.newFile();
	for (const d of [a, b, c]) w.docs.setText(d.id, 'x');
	w.dialogs.queue.push('no', 'cancel'); // c verwerfen, bei b abbrechen
	assert.equal(await w.ops.closeAll(), false);
	assert.deepEqual(w.docs.list().map((d) => d.id), [a.id, b.id]);
});

// ------------------------------------------------------------------- Tabs

test('files: nächster/vorheriger Tab läuft im Kreis', async () => {
	const w = await world();
	const a = w.ops.newFile();
	const b = w.ops.newFile();
	const c = w.ops.newFile();
	assert.equal(w.docs.active.id, c.id);
	w.ops.cycle(1);
	assert.equal(w.docs.active.id, a.id);
	w.ops.cycle(-1);
	assert.equal(w.docs.active.id, c.id);
	w.ops.cycle(-1);
	assert.equal(w.docs.active.id, b.id);
});

test('files: die Befehle sind gesperrt, solange sie nichts tun können', async () => {
	const w = await world();
	const enabled = (id) => w.app.commands.isEnabled(id);
	assert.deepEqual(['file.new', 'file.open', 'file.exit'].map(enabled), [true, true, true]);
	assert.deepEqual(['file.close', 'file.closeAll', 'file.save', 'file.saveAs', 'file.saveAll', 'file.nextTab'].map(enabled),
		[false, false, false, false, false, false]);
	w.ops.newFile();
	assert.deepEqual(['file.close', 'file.closeAll', 'file.save', 'file.saveAs', 'file.saveAll'].map(enabled), [true, true, true, true, true]);
	assert.equal(enabled('file.nextTab'), false); // erst ab zwei Tabs
	w.ops.newFile();
	assert.equal(enabled('file.nextTab'), true);
});

test('files: ohne Backend bleiben Neu und Schließen nutzbar, alles mit Datei ist gesperrt', async () => {
	const platform = { ...fakePlatform(), hasBackend: false };
	const app = createApp({ platform, extensions: [files] });
	app.services.provide('dialogs', fakeDialogs());
	await app.start();
	app.services.get('files').newFile();
	assert.equal(app.commands.isEnabled('file.new'), true);
	assert.equal(app.commands.isEnabled('file.close'), true);
	assert.equal(app.commands.isEnabled('file.open'), false);
	assert.equal(app.commands.isEnabled('file.save'), false);
});

// ------------------------------------------------------- zuletzt geöffnet

test('files: Liste der zuletzt geöffneten — neueste zuerst, ohne Doppelte, höchstens zehn, im Menü', async () => {
	const w = await world();
	for (let i = 1; i <= 12; i++) put(w, `C:\\f${i}.bb`, bytes('x'));
	for (let i = 1; i <= 12; i++) await w.ops.openPath(`C:\\f${i}.bb`);
	assert.equal(w.ops.recent().length, 10);
	assert.equal(w.ops.recent()[0], 'C:\\f12.bb');
	assert.equal(w.ops.recent().at(-1), 'C:\\f3.bb');

	await w.ops.openPath('c:\\F5.BB'); // schon offen, wird aber nicht als neuer Eintrag doppelt geführt
	assert.equal(w.ops.recent().filter((p) => /f5\.bb/i.test(p)).length, 1);

	const items = w.app.contributions.get('menus').filter((m) => m.menu === 'file.recent');
	assert.equal(items.length, 10);
	assert.equal(items[0].command, 'file.openRecent');
	assert.equal(items[0].title, 'C:\\f12.bb');
	assert.deepEqual(items[0].args, ['C:\\f12.bb']);
});

test('files: Zuletzt geöffnet öffnen ein Dokument über den Befehl', async () => {
	const w = await world();
	put(w, 'C:\\r.bb', bytes('x'));
	await w.app.commands.execute('file.openRecent', 'C:\\r.bb');
	assert.equal(w.docs.list()[0].uri, 'C:\\r.bb');
});

// --------------------------------------------------------- Beenden, Start

test('files: Beenden schließt das Fenster erst, wenn alle Dokumente geklärt sind', async () => {
	const w = await world();
	const doc = w.ops.newFile();
	w.docs.setText(doc.id, 'x');

	w.dialogs.queue.push('cancel');
	assert.equal(await w.ops.exit(), false);
	assert.equal(w.platform.calls.some((c) => c[0] === 'window'), false);

	w.dialogs.queue.push('no');
	assert.equal(await w.ops.exit(), true);
	assert.equal(w.platform.calls.some((c) => c[0] === 'window' && c[1] === 'close'), true);
});

test('files: die Aufforderung des Fensters zu schließen läuft über denselben Ablauf', async () => {
	const w = await world();
	w.platform.emit('window:close-requested');
	await new Promise((r) => setTimeout(r, 10));
	assert.equal(w.platform.calls.some((c) => c[0] === 'window' && c[1] === 'close'), true);
});

test('files: Dateien von der Kommandozeile werden beim Start geöffnet', async () => {
	const platform = fakePlatform({ launch: ['C:\\x\\start.bb'] });
	platform.fsx.set('C:\\x\\start.bb', bytes('Print 1'));
	const app = createApp({ platform, extensions: [files] });
	app.services.provide('dialogs', fakeDialogs());
	await app.start();
	await new Promise((r) => setTimeout(r, 10));
	assert.deepEqual(app.documents.list().map((d) => d.uri), ['C:\\x\\start.bb']);
});

test('files: Umlaute und UTF-8 aus einer echten Runde durch Öffnen/Speichern bleiben byte-gleich', async () => {
	const w = await world();
	const utf8 = new TextEncoder().encode('Print "Größe €"\n');
	put(w, 'C:\\u8.bb', utf8);
	const doc = await w.ops.openPath('C:\\u8.bb');
	assert.equal(doc.encoding, 'utf-8');
	assert.equal(doc.eol, '\n');
	w.docs.setText(doc.id, doc.text + ' ');
	w.docs.setText(doc.id, doc.text.slice(0, -1));
	assert.equal(doc.dirty, false);
	await w.ops.save({ doc });
	assert.deepEqual(read(w, 'C:\\u8.bb'), [...utf8]);
	// und der Dekodierer sieht dasselbe
	assert.equal(decodeFile(w.platform.fsx.get('C:\\u8.bb')).text, doc.text);
	assert.ok(encodeFile(doc.text, 'utf-8', '\n').bytes);
});
