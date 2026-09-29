// Die Dateiliste: reine Logik und das Verhalten der Erweiterung (ohne DOM).

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { basename, dirname, joinPath, isInside, isHidden, sortEntries, visibleEntries, isMedia, decideRoot } from '../../src/extensions/explorer/tree.js';
import { createApp } from '../../src/app.js';
import files from '../../src/extensions/files/index.js';
import explorer from '../../src/extensions/explorer/index.js';

// ------------------------------------------------------------------- Pfade

test('explorer: dirname/basename/joinPath für beide Trenner und Laufwerkswurzeln', () => {
	assert.equal(dirname('C:\\game\\main.bb'), 'C:\\game');
	assert.equal(dirname('C:/game/main.bb'), 'C:/game');
	assert.equal(dirname('C:\\main.bb'), 'C:\\');
	assert.equal(dirname('main.bb'), '');
	assert.equal(basename('C:\\game\\main.bb'), 'main.bb');
	assert.equal(basename('C:\\game\\'), 'game');
	assert.equal(joinPath('C:\\game', 'a.bb'), 'C:\\game\\a.bb');
	assert.equal(joinPath('C:/game', 'a.bb'), 'C:/game/a.bb');
	assert.equal(joinPath('C:\\', 'a.bb'), 'C:\\a.bb');
	assert.equal(joinPath('', 'a.bb'), 'a.bb');
});

test('explorer: isInside ohne Rücksicht auf Groß-/Kleinschreibung und Trenner, "game" ist nicht in "game2"', () => {
	assert.equal(isInside('C:\\game', 'C:\\game\\sub\\a.bb'), true);
	assert.equal(isInside('C:\\GAME', 'c:/game/a.bb'), true);
	assert.equal(isInside('C:\\game', 'C:\\game'), true);
	assert.equal(isInside('C:\\game', 'C:\\game2\\a.bb'), false);
	assert.equal(isInside('C:\\', 'C:\\x\\a.bb'), true);
	assert.equal(isInside('', 'C:\\a.bb'), false);
	assert.equal(isInside('C:\\game', null), false);
});

// -------------------------------------------------- Ausblenden und Sortieren

test('explorer: Ausblenden mit * und ?, ohne Rücksicht auf Groß-/Kleinschreibung', () => {
	const hide = ['*.exe', '*.bb_bak*', '.git', 'thumbs.db', 'level?.tmp'];
	assert.equal(isHidden('game.EXE', hide), true);
	assert.equal(isHidden('a.bb_bak1', hide), true);
	assert.equal(isHidden('a.bb', hide), false);
	assert.equal(isHidden('.git', hide), true);
	assert.equal(isHidden('Thumbs.db', hide), true);
	assert.equal(isHidden('level1.tmp', hide), true);
	assert.equal(isHidden('level10.tmp', hide), false);
	assert.equal(isHidden('a(b).bb', ['a(b).bb']), true); // Sonderzeichen sind keine Regelzeichen
	assert.equal(isHidden('x', []), false);
	assert.equal(isHidden('x', undefined), false);
	assert.equal(isHidden('x', [null, '', 5]), false);
});

test('explorer: Ordner zuerst, dann nach Namen mit Zahlen als Zahlen', () => {
	const sorted = sortEntries([
		{ name: 'level10.bb', dir: false },
		{ name: 'Zeta', dir: true },
		{ name: 'level2.bb', dir: false },
		{ name: 'alpha', dir: true },
		{ name: 'Main.bb', dir: false }
	]).map((e) => e.name);
	assert.deepEqual(sorted, ['alpha', 'Zeta', 'level2.bb', 'level10.bb', 'Main.bb']);
	const entries = [{ name: 'b', dir: false }, { name: 'a.exe', dir: false }];
	assert.deepEqual(visibleEntries(entries, ['*.exe']).map((e) => e.name), ['b']);
	assert.equal(entries.length, 2); // die Eingabe bleibt unverändert
});

test('explorer: Medien erkennen', () => {
	for (const n of ['a.PNG', 'b.wav', 'c.x', 'd.3ds', 'e.b3d', 'f.glb']) assert.equal(isMedia(n), true, n);
	for (const n of ['a.bb', 'a.exe', 'x', 'png', 'a.txt', 'a.png.bb']) assert.equal(isMedia(n), false, n);
});

// ------------------------------------------------------------ Wurzelordner

test('explorer: decideRoot — Auswahl gilt, sonst folgt die aktive Datei, im gezeigten Ordner bleibt es', () => {
	assert.equal(decideRoot({ explicitRoot: 'C:\\fest', activePath: 'C:\\x\\a.bb', currentRoot: 'C:\\y' }), 'C:\\fest');
	assert.equal(decideRoot({ activePath: 'C:\\game\\a.bb' }), 'C:\\game');
	assert.equal(decideRoot({ activePath: 'C:\\game\\sub\\inc.bb', currentRoot: 'C:\\game' }), 'C:\\game'); // Include im Unterordner
	assert.equal(decideRoot({ activePath: 'C:\\other\\b.bb', currentRoot: 'C:\\game' }), 'C:\\other');
	assert.equal(decideRoot({ activePath: null, currentRoot: 'C:\\game' }), 'C:\\game'); // namenloser Tab
	assert.equal(decideRoot({}), null);
});

// ----------------------------------------------------------- Erweiterung

function fakePlatform({ tree = {}, state = null, folderAnswers = [] } = {}) {
	const calls = [];
	const key = (p) => String(p).replace(/\//g, '\\');
	const platform = {
		hasBackend: true,
		calls,
		tree,
		folderAnswers,
		store: {
			async read(name) { return name === 'state.json' && state ? JSON.stringify({ version: 1, state }) : null; },
			async write() {},
			async remove() {}
		},
		on: () => () => {},
		async invoke(service, method, ...args) {
			calls.push([service, method, ...args]);
			if (service === 'files' && method === 'resolve') return key(args[0]);
			if (service === 'files' && method === 'list') {
				const entries = platform.tree[key(args[0])];
				if (!entries) throw new Error('ENOENT');
				return entries;
			}
			if (service === 'files' && method === 'read') return { path: key(args[0]), bytes: Uint8Array.from([0x41]) };
			if (service === 'dialog' && method === 'folder') return platform.folderAnswers.shift() ?? null;
			if (service === 'host') return [];
			throw new Error(`unerwarteter Aufruf ${service}.${method}`);
		}
	};
	return platform;
}

async function world(opts) {
	const platform = fakePlatform(opts);
	const app = createApp({ platform, extensions: [files, explorer] });
	app.services.provide('dialogs', { message: async () => 'ok' });
	await app.start();
	return { app, platform, explorer: app.services.get('explorer'), docs: app.documents };
}

const settle = (ms = 30) => new Promise((r) => setTimeout(r, ms));
const names = (w) => w.explorer.rows().map((r) => `${' '.repeat(r.depth)}${r.name ?? r.text}`);

const GAME = {
	'C:\\game': [
		{ name: 'main.bb', dir: false }, { name: 'game.exe', dir: false }, { name: 'SDL3.dll', dir: false },
		{ name: 'data', dir: true }, { name: 'main.bb_bak1', dir: false }, { name: 'a2.bb', dir: false }, { name: 'a10.bb', dir: false }
	],
	'C:\\game\\data': [{ name: 'hero.png', dir: false }, { name: 'level.bb', dir: false }],
	'C:\\other': [{ name: 'b.bb', dir: false }]
};

test('explorer: ohne Datei ist die Liste leer und Aktualisieren gesperrt', async () => {
	const w = await world({ tree: GAME });
	assert.equal(w.explorer.root, null);
	assert.deepEqual(w.explorer.rows(), []);
	assert.equal(w.app.commands.isEnabled('explorer.refresh'), false);
	assert.equal(w.app.commands.isEnabled('explorer.followActive'), false);
});

test('explorer: folgt der aktiven Datei; Ausgeblendetes fehlt, Sortierung stimmt', async () => {
	const w = await world({ tree: GAME });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	assert.equal(w.explorer.root, 'C:\\game');
	assert.deepEqual(names(w), ['data', 'a2.bb', 'a10.bb', 'main.bb']); // ohne .exe, .dll, _bak
	assert.equal(w.app.commands.isEnabled('explorer.refresh'), true);
});

test('explorer: eine Datei in einem Unterordner lässt die Liste stehen, eine anderswo wechselt sie', async () => {
	const w = await world({ tree: GAME });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	w.docs.open({ uri: 'C:\\game\\data\\level.bb', text: 'y' });
	await settle();
	assert.equal(w.explorer.root, 'C:\\game');

	w.docs.open({ uri: 'C:\\other\\b.bb', text: 'z' });
	await settle();
	assert.equal(w.explorer.root, 'C:\\other');
	assert.deepEqual(names(w), ['b.bb']);

	w.fileOps = w.app.services.get('files');
	w.fileOps.newFile(); // namenloser Tab ändert nichts
	await settle();
	assert.equal(w.explorer.root, 'C:\\other');
});

test('explorer: aufgeklappte Ordner (aus dem Zustand) zeigen ihren Inhalt eingerückt', async () => {
	const w = await world({ tree: GAME, state: { 'explorer.expanded': ['C:\\game\\data'] } });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	assert.deepEqual(names(w), ['data', ' hero.png', ' level.bb', 'a2.bb', 'a10.bb', 'main.bb']);
	const rows = w.explorer.rows();
	assert.equal(rows[0].open, true);
	assert.equal(rows[1].path, 'C:\\game\\data\\hero.png');
});

test('explorer: unlesbarer Ordner wird als Hinweis gezeigt, nicht als Fehler', async () => {
	const w = await world({ tree: { 'C:\\game': [{ name: 'gone', dir: true }] }, state: { 'explorer.expanded': ['C:\\game\\gone'] } });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	assert.deepEqual(names(w), ['gone', ' Cannot read this folder']);
});

test('explorer: Ordner wählen setzt die Wurzel fest; der Pin löst sie wieder', async () => {
	const w = await world({ tree: GAME });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();

	w.platform.folderAnswers.push('C:\\other');
	await w.app.commands.execute('explorer.openFolder');
	await settle();
	assert.equal(w.explorer.root, 'C:\\other');
	assert.equal(w.app.state.get('explorer.root'), 'C:\\other');
	assert.equal(w.app.commands.isEnabled('explorer.followActive'), true);

	// eine Datei anderswo ändert die festgewählte Wurzel nicht
	w.docs.open({ uri: 'C:\\game\\data\\level.bb', text: 'y' });
	await settle();
	assert.equal(w.explorer.root, 'C:\\other');

	await w.app.commands.execute('explorer.followActive');
	await settle();
	assert.equal(w.app.state.get('explorer.root'), undefined);
	assert.equal(w.explorer.root, 'C:\\game\\data'); // die aktive Datei ist level.bb
});

test('explorer: Abbruch im Ordnerdialog ändert nichts', async () => {
	const w = await world({ tree: GAME });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	await w.app.commands.execute('explorer.openFolder'); // keine Antwort = Abbruch
	assert.equal(w.explorer.root, 'C:\\game');
	assert.equal(w.app.state.get('explorer.root'), undefined);
});

test('explorer: gespeicherter fester Ordner gilt nach dem Start', async () => {
	const w = await world({ tree: GAME, state: { 'explorer.root': 'C:\\other' } });
	await settle();
	assert.equal(w.explorer.root, 'C:\\other');
	assert.deepEqual(names(w), ['b.bb']);
});

test('explorer: Speichern und das Ende eines Baus lesen den Ordner neu, gebündelt', async () => {
	const w = await world({ tree: GAME });
	const doc = w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	const lists = () => w.platform.calls.filter((c) => c[1] === 'list').length;
	const before = lists();

	w.platform.tree['C:\\game'] = [...GAME['C:\\game'], { name: 'neu.bb', dir: false }];
	w.docs.setText(doc.id, 'y');
	w.docs.markSaved(doc.id);
	w.app.events.emit('build:finished', { ok: true });
	await settle(300);
	assert.equal(lists(), before + 1); // zwei Anlässe, eine Aktualisierung
	assert.ok(names(w).includes('neu.bb'));
});

test('explorer: geänderte Ausblende-Einstellung wirkt sofort', async () => {
	const w = await world({ tree: GAME });
	w.docs.open({ uri: 'C:\\game\\main.bb', text: 'x' });
	await settle();
	w.app.settings.set('explorer.hide', []);
	assert.ok(names(w).includes('game.exe'));
	assert.ok(names(w).includes('main.bb_bak1'));
});
