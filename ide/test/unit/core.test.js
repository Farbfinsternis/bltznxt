// Tests des Kerns. Reines Node, kein Electron, kein DOM:
//
//   npm test

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { createEmitter, createDisposables } from '../../src/core/events.js';
import { createContext, parseWhen } from '../../src/core/context.js';
import { createCommands } from '../../src/core/commands.js';
import { createServices } from '../../src/core/services.js';
import { createContributions } from '../../src/core/contributions.js';
import { createI18n } from '../../src/core/i18n.js';
import { normalizeKey, keyFromEvent, createKeybindings } from '../../src/core/keybindings.js';
import { createSettings, SETTINGS_VERSION } from '../../src/core/settings.js';
import { createDocuments } from '../../src/core/documents.js';
import { buildMenus } from '../../src/core/menus.js';
import { createExtensionHost } from '../../src/core/extensions.js';

// Kein Lärm von absichtlich provozierten Fehlern
const quiet = (fn) => async (t) => {
	const orig = console.error;
	console.error = () => {};
	try {
		await fn(t);
	} finally {
		console.error = orig;
	}
};

// ---------------------------------------------------------------- events

test('events: on/emit/off, once, Hörer mit Fehler sperrt die anderen nicht', quiet(() => {
	const e = createEmitter();
	const seen = [];
	const off = e.on('x', (v) => seen.push(['a', v]));
	e.on('x', () => { throw new Error('boom'); });
	e.on('x', (v) => seen.push(['c', v]));
	e.once('x', (v) => seen.push(['once', v]));
	e.emit('x', 1);
	e.emit('x', 2);
	off();
	e.emit('x', 3);
	assert.deepEqual(seen, [['a', 1], ['c', 1], ['once', 1], ['a', 2], ['c', 2], ['c', 3]]);
}));

test('disposables: umgekehrte Reihenfolge, ein Fehler stoppt nicht', quiet(() => {
	const d = createDisposables();
	const order = [];
	d.add(() => order.push(1));
	d.add(() => { throw new Error('x'); });
	d.add({ dispose: () => order.push(3) });
	d.dispose();
	assert.deepEqual(order, [3, 1]);
	assert.equal(d.size, 0);
}));

// --------------------------------------------------------------- context

test('context: Schlüssel, Operatoren, Vergleiche', () => {
	const c = createContext();
	c.set('a', true);
	c.set('n', 3);
	c.set('mode', 'debug');
	assert.equal(c.evaluate('a'), true);
	assert.equal(c.evaluate('b'), false);
	assert.equal(c.evaluate('!b'), true);
	assert.equal(c.evaluate('a && !b'), true);
	assert.equal(c.evaluate('b || a'), true);
	assert.equal(c.evaluate('!(a && b)'), true);
	assert.equal(c.evaluate("mode == 'debug'"), true);
	assert.equal(c.evaluate("mode != 'debug'"), false);
	assert.equal(c.evaluate('n == 3'), true);
	assert.equal(c.evaluate('a == true'), true);
	// && bindet stärker als ||
	assert.equal(c.evaluate('a || b && b'), true);
});

test('context: leer ist wahr, Funktionen, Änderungsereignis', () => {
	const c = createContext();
	assert.equal(c.evaluate(undefined), true);
	assert.equal(c.evaluate(''), true);
	assert.equal(c.evaluate(() => 0), false);
	const changes = [];
	c.onDidChange((e) => changes.push(e));
	c.set('x', 1);
	c.set('x', 1); // gleicher Wert: kein Ereignis
	c.set('x', 2);
	assert.deepEqual(changes, [{ key: 'x', value: 1 }, { key: 'x', value: 2 }]);
});

test('context: kaputter Ausdruck ist falsch und wirft nie', quiet(() => {
	const c = createContext();
	assert.equal(c.evaluate('a &&'), false);
	assert.equal(c.evaluate('a $ b'), false);
	assert.equal(c.evaluate('(a'), false);
	assert.throws(() => parseWhen('a b'), /Überschuss/);
	assert.throws(() => parseWhen("a == b"), /Anführungszeichen/);
}));

// -------------------------------------------------------------- commands

test('commands: registrieren, ausführen, sperren, abmelden', async () => {
	const context = createContext();
	const cmds = createCommands({ context });
	const calls = [];
	const off = cmds.register({ id: 'demo.run', enabledWhen: 'ready', run: (x) => { calls.push(x); return x * 2; } });

	assert.equal(cmds.has('demo.run'), true);
	assert.equal(cmds.isEnabled('demo.run'), false);
	assert.deepEqual(await cmds.execute('demo.run', 4), { executed: false });
	assert.deepEqual(calls, []);

	context.set('ready', true);
	assert.equal(cmds.isEnabled('demo.run'), true);
	assert.deepEqual(await cmds.execute('demo.run', 4), { executed: true, result: 8 });

	await assert.rejects(() => cmds.execute('nope'), /Unbekannter Befehl/);
	assert.throws(() => cmds.register({ id: 'demo.run', run() {} }), /schon registriert/);
	assert.throws(() => cmds.register({ id: 'x' }), /run-Funktion/);

	off();
	assert.equal(cmds.has('demo.run'), false);
});

test('commands: ein Befehl kann ersetzt werden, altes Abmelden trifft den Nachfolger nicht', () => {
	const cmds = createCommands({ context: createContext() });
	const first = { id: 'x', run: () => 1 };
	const offFirst = cmds.register(first);
	offFirst();
	const offSecond = cmds.register({ id: 'x', run: () => 2 });
	offFirst(); // spätes zweites Abmelden
	assert.equal(cmds.has('x'), true);
	offSecond();
	assert.equal(cmds.has('x'), false);
});

// -------------------------------------------------------------- services

test('services: provide/get/waitFor', async () => {
	const s = createServices();
	assert.throws(() => s.get('a'), /nicht vorhanden/);
	assert.equal(s.tryGet('a'), undefined);
	const late = s.waitFor('a');
	const impl = { hi: 1 };
	const off = s.provide('a', impl);
	assert.equal(await late, impl);
	assert.equal(await s.waitFor('a'), impl); // schon da
	assert.throws(() => s.provide('a', {}), /schon vergeben/);
	off();
	assert.equal(s.has('a'), false);
});

// --------------------------------------------------------- contributions

test('contributions: beitragen, lesen, ändern, entfernen, Besitzer', () => {
	const c = createContributions();
	const seen = [];
	c.onDidChange('menus', (e) => seen.push(e.owner));
	const a = c.contribute('menus', [{ n: 1 }, { n: 2 }], 'ext-a');
	c.contribute('menus', [{ n: 3 }], 'ext-b');
	assert.deepEqual(c.get('menus').map((i) => i.n), [1, 2, 3]);
	assert.deepEqual(c.getEntries('menus').map((e) => e.owner), ['ext-a', 'ext-a', 'ext-b']);
	a.update([{ n: 9 }]);
	assert.deepEqual(c.get('menus').map((i) => i.n), [9, 3]);
	a.dispose();
	assert.deepEqual(c.get('menus').map((i) => i.n), [3]);
	assert.deepEqual(c.get('unbekannt'), []);
	assert.deepEqual(seen, ['ext-a', 'ext-b', 'ext-a', 'ext-a']);
	assert.throws(() => c.contribute('x', 'kein array'), /Liste/);
});

// ------------------------------------------------------------------ i18n

test('i18n: Rückfall auf Englisch, dann auf den Schlüssel; Parameter; Überschreiben', () => {
	const i = createI18n({ language: 'de' });
	const off = i.add({ en: { 'a': 'Alpha', 'b': 'Beta {n}' }, de: { 'a': 'Alfa' } });
	assert.equal(i.t('a'), 'Alfa');
	assert.equal(i.t('b', { n: 2 }), 'Beta 2');
	assert.equal(i.t('c.datei'), 'c.datei');
	assert.equal(i.t('b', {}), 'Beta {n}');
	i.add({ de: { 'a': 'Anders' } });
	assert.equal(i.t('a'), 'Anders');
	i.setLanguage('en');
	assert.equal(i.t('a'), 'Alpha');
	off();
	assert.equal(i.t('a'), 'a'); // Tabelle entfernt, kein Eintrag mehr
});

// ----------------------------------------------------------- keybindings

test('keys: Normalisierung', () => {
	assert.equal(normalizeKey('ctrl+s'), 'Ctrl+S');
	assert.equal(normalizeKey('Shift+Ctrl+Tab'), 'Ctrl+Shift+Tab');
	assert.equal(normalizeKey('shift+esc'), 'Shift+Escape');
	assert.equal(normalizeKey('f5'), 'F5');
	assert.equal(normalizeKey('Strg+F4'), 'Ctrl+F4');
	assert.equal(normalizeKey('Ctrl++'), 'Ctrl++');
	assert.throws(() => normalizeKey('Ctrl+'), /ohne Taste/);
});

test('keys: aus Tastaturereignis', () => {
	assert.equal(keyFromEvent({ key: 's', ctrlKey: true }), 'Ctrl+S');
	assert.equal(keyFromEvent({ key: 'Tab', ctrlKey: true, shiftKey: true }), 'Ctrl+Shift+Tab');
	assert.equal(keyFromEvent({ key: 'F5' }), 'F5');
	assert.equal(keyFromEvent({ key: 'Escape', shiftKey: true }), 'Shift+Escape');
	assert.equal(keyFromEvent({ key: ' ' }), 'Space');
	assert.equal(keyFromEvent({ key: 'Control', ctrlKey: true }), null);
});

test('keybindings: auflösen mit when, letzter Beitrag gewinnt, Anzeige im Menü', quiet(() => {
	const context = createContext();
	const contributions = createContributions();
	const kb = createKeybindings({ contributions, context });
	contributions.contribute('keybindings', [
		{ command: 'a', key: 'ctrl+s' },
		{ command: 'save.all', key: 'Ctrl+Shift+S', when: 'multi' },
		{ command: 'bad', key: 'Ctrl+' }
	], 'one');
	assert.deepEqual(kb.resolve('Ctrl+S'), { command: 'a', args: [] });
	assert.equal(kb.resolve('Ctrl+Shift+S'), null);
	context.set('multi', true);
	assert.equal(kb.resolve('Ctrl+Shift+S').command, 'save.all');

	const other = contributions.contribute('keybindings', [{ command: 'b', key: 'Ctrl+S', args: [1] }], 'two');
	assert.deepEqual(kb.resolve('Ctrl+S'), { command: 'b', args: [1] });
	assert.equal(kb.keyFor('a'), 'Ctrl+S');
	other.dispose();
	assert.equal(kb.resolve('Ctrl+S').command, 'a');
}));

// -------------------------------------------------------------- settings

function settingsWithSchema() {
	const s = createSettings();
	s.registerSchema({
		'editor.tabSize': { type: 'number', default: 4 },
		'editor.font': { type: 'string', default: 'Consolas' },
		'workbench.mode': { type: 'string', enum: ['a', 'b'], default: 'a' }
	});
	return s;
}

test('settings: Vorgabe < Benutzer < Ordner, Ereignisse', () => {
	const s = settingsWithSchema();
	const changes = [];
	s.onDidChange((e) => changes.push([e.key, e.value, e.layer]));
	assert.equal(s.get('editor.tabSize'), 4);
	s.set('editor.tabSize', 8);
	assert.equal(s.get('editor.tabSize'), 8);
	s.setFolderLayer({ 'editor.tabSize': 2 });
	assert.equal(s.get('editor.tabSize'), 2);
	s.reset('editor.tabSize');
	assert.equal(s.get('editor.tabSize'), 2); // Ordner gilt weiter
	s.setFolderLayer({});
	assert.equal(s.get('editor.tabSize'), 4);
	assert.deepEqual(changes.map((c) => c[1]), [8, 2, 4]);
	assert.equal(s.get('unbekannt', 'x'), 'x');
});

test('settings: Typprüfung beim Setzen, falsche Werte aus der Datei werden ignoriert und bleiben erhalten', () => {
	const s = settingsWithSchema();
	assert.throws(() => s.set('editor.tabSize', 'acht'), /Schema/);
	assert.throws(() => s.set('workbench.mode', 'c'), /Schema/);
	const r = s.load(JSON.stringify({ version: 1, settings: { 'editor.tabSize': 'acht', 'fremd.wert': [1] } }));
	assert.equal(r.ok, true);
	assert.equal(s.get('editor.tabSize'), 4); // Vorgabe statt Unsinn
	const out = JSON.parse(s.serialize());
	assert.equal(out.settings['editor.tabSize'], 'acht'); // nicht gelöscht
	assert.deepEqual(out.settings['fremd.wert'], [1]); // Fremdes bleibt
	assert.equal(out.version, SETTINGS_VERSION);
});

test('settings: kaputte Datei ergibt ok:false und leere Einstellungen, nie einen Wurf', () => {
	const s = settingsWithSchema();
	s.set('editor.tabSize', 8);
	for (const text of ['{ nicht json', '[1,2]', 'null']) {
		const r = s.load(text);
		assert.equal(r.ok, false, text);
		assert.ok(r.error);
		assert.equal(s.get('editor.tabSize'), 4);
	}
	assert.equal(s.load('').ok, true);
	assert.equal(s.load(null).ok, true);
});

test('settings: Migration und neuere Version', () => {
	const s = createSettings({ migrations: {} });
	// Version 1 ist die aktuelle: keine Migration nötig, aber eine ältere ohne Migration ist ein Fehler
	const r = s.load(JSON.stringify({ version: 0, settings: {} }));
	assert.equal(r.ok, false);
	assert.match(r.error, /Migration/);

	const withStep = createSettings({ migrations: { 0: (d) => ({ ...d, neu: d.alt }) } });
	const r2 = withStep.load(JSON.stringify({ version: 0, settings: { alt: 5 } }));
	assert.deepEqual([r2.ok, r2.migrated], [true, true]);
	assert.equal(JSON.parse(withStep.serialize()).settings.neu, 5);

	const future = createSettings();
	assert.equal(future.load(JSON.stringify({ version: 7, settings: { x: 1 } })).ok, true);
	assert.equal(JSON.parse(future.serialize()).version, 7); // Nummer bleibt
});

test('settings: Schema abmelden, doppeltes Schema ist ein Fehler', () => {
	const s = createSettings();
	const off = s.registerSchema({ 'a.b': { type: 'boolean', default: true } });
	assert.throws(() => s.registerSchema({ 'a.b': {} }), /schon vergeben/);
	assert.equal(s.get('a.b'), true);
	off();
	assert.equal(s.get('a.b'), undefined);
});

// ------------------------------------------------------------- documents

test('documents: öffnen, aktivieren, ändern, speichern, schließen', () => {
	const d = createDocuments();
	const log = [];
	for (const name of ['opened', 'closed', 'activated', 'changed', 'saved', 'renamed']) {
		d.on(name, (doc) => log.push(name + ':' + (doc ? doc.title : '-')));
	}
	const a = d.open({ uri: 'C:\\prog\\a.bb', text: 'Print 1' });
	assert.equal(a.title, 'a.bb');
	assert.equal(a.dirty, false);
	assert.equal(d.active.id, a.id);

	d.setText(a.id, 'Print 2');
	assert.equal(a.dirty, true);
	d.markSaved(a.id);
	assert.equal(a.dirty, false);

	const b = d.open({ text: 'x' });
	assert.equal(b.kind, 'scratch');
	assert.equal(b.title, '<untitled>');
	assert.equal(b.dirty, true); // namenloser Text ist ungespeichert
	const c = d.open({ text: 'y' });
	assert.equal(c.title, '<untitled 2>');

	d.markSaved(b.id, { uri: 'C:\\prog\\b.bb' });
	assert.equal(b.kind, 'file');
	assert.equal(b.title, 'b.bb');
	assert.equal(b.dirty, false);

	d.close(c.id);
	assert.equal(d.active.id, b.id); // Nachbar wird aktiv
	assert.deepEqual(d.list().map((x) => x.title), ['a.bb', 'b.bb']);
	assert.ok(log.includes('renamed:b.bb'));
});

test('documents: gleicher Pfad (ohne Groß-/Kleinschreibung) öffnet nichts doppelt', () => {
	const d = createDocuments();
	const a = d.open({ uri: 'C:\\Prog\\Game.bb', text: '1' });
	d.open({ text: 'x' });
	const again = d.open({ uri: 'c:\\prog\\game.BB', text: 'egal' });
	assert.equal(again.id, a.id);
	assert.equal(d.list().length, 2);
	assert.equal(d.active.id, a.id);
	assert.equal(again.text, '1'); // vorhandener Text gewinnt

	const strict = createDocuments({ caseInsensitivePaths: false });
	strict.open({ uri: '/a/B.bb' });
	strict.open({ uri: '/a/b.bb' });
	assert.equal(strict.list().length, 2);
});

test('documents: letztes Dokument schließen ergibt kein aktives', () => {
	const d = createDocuments();
	const a = d.open({ text: '' });
	let last = 'unset';
	d.on('activated', (doc) => { last = doc; });
	d.close(a.id);
	assert.equal(d.active, null);
	assert.equal(last, null);
	assert.equal(d.close(999), undefined);
});

test('documents: text hat immer LF, eol gilt erst beim Schreiben', () => {
	const d = createDocuments();
	const a = d.open({ uri: 'x.bb', text: 'a\r\nb\rc\n', eol: '\r\n' });
	assert.equal(a.text, 'a\nb\nc\n');
	assert.equal(a.dirty, false);
	d.setText(a.id, 'a\r\nb\nc\n'); // gleicher Text, nur anderes Zeilenende
	assert.equal(a.dirty, false);
});

test('documents: Kodierung und Zeilenende gehören zum Dokument', () => {
	const d = createDocuments();
	const a = d.open({ uri: 'x.bb', text: 'a', encoding: 'windows-1252', eol: '\r\n' });
	assert.equal(a.encoding, 'windows-1252');
	assert.equal(a.eol, '\r\n');
	d.markSaved(a.id, { encoding: 'utf-8' });
	assert.equal(a.encoding, 'utf-8');
});

// ----------------------------------------------------------------- menus

function menuWorld() {
	const context = createContext();
	const contributions = createContributions();
	const commands = createCommands({ context });
	const keybindings = createKeybindings({ contributions, context });
	const i18n = createI18n();
	i18n.add({ en: { 'menu.file': 'File', 'cmd.save': 'Save', 'cmd.open': 'Open...' } });
	return { context, contributions, commands, keybindings, i18n, build: () => buildMenus({ contributions, commands, keybindings, context, i18n }) };
}

test('menus: Gruppen, Reihenfolge, Trenner, Kürzel, Sperre, Haken', () => {
	const w = menuWorld();
	w.commands.register({ id: 'file.save', title: 'cmd.save', enabledWhen: 'dirty', run() {} });
	w.commands.register({ id: 'file.open', title: 'cmd.open', run() {} });
	w.commands.register({ id: 'view.bar', title: 'Bar', run() {} });
	w.contributions.contribute('menubar', [{ id: 'file', title: 'menu.file', order: 1 }], 'a');
	w.contributions.contribute('keybindings', [{ command: 'file.save', key: 'ctrl+s' }], 'a');
	w.contributions.contribute('menus', [
		{ menu: 'file', command: 'file.save', group: '2_save', order: 1 },
		{ menu: 'file', command: 'file.open', group: '1_open', order: 1 },
		{ menu: 'file', command: 'view.bar', group: '2_save', order: 2, checkedWhen: 'bar' },
		{ menu: 'file', command: 'gibt.es.nicht', group: '3_x' }
	], 'a');

	let [file] = w.build();
	assert.equal(file.title, 'File');
	assert.deepEqual(file.items.map((i) => i.type === 'item' ? i.command : i.type),
		['file.open', 'separator', 'file.save', 'view.bar']);
	const save = file.items.find((i) => i.command === 'file.save');
	assert.equal(save.title, 'Save');
	assert.equal(save.key, 'Ctrl+S');
	assert.equal(save.enabled, false);
	assert.equal(file.items.find((i) => i.command === 'view.bar').checked, false);

	w.context.set('dirty', true);
	w.context.set('bar', true);
	[file] = w.build();
	assert.equal(file.items.find((i) => i.command === 'file.save').enabled, true);
	assert.equal(file.items.find((i) => i.command === 'view.bar').checked, true);
});

test('menus: Untermenü, leere Menüs und Untermenüs entfallen, when blendet aus', () => {
	const w = menuWorld();
	w.commands.register({ id: 'file.recent', title: 'x', run() {} });
	w.commands.register({ id: 'edit.cut', title: 'Cut', run() {} });
	w.contributions.contribute('menubar', [
		{ id: 'file', title: 'menu.file', order: 1 },
		{ id: 'edit', title: 'Edit', order: 2 },
		{ id: 'help', title: 'Help', order: 3 }
	], 'a');
	const recent = w.contributions.contribute('menus', [
		{ menu: 'file', submenu: 'file.recent', title: 'Recent', group: '9' }
	], 'a');
	w.contributions.contribute('menus', [{ menu: 'edit', command: 'edit.cut', when: 'hasSelection' }], 'a');

	// Untermenü ohne Einträge: kein Eintrag, also auch kein Menü "File"
	assert.deepEqual(w.build().map((m) => m.id), []);

	const items = w.contributions.contribute('menus', [
		{ menu: 'file.recent', command: 'file.recent', title: 'C:\\a.bb', args: ['C:\\a.bb'] }
	], 'b');
	let menus = w.build();
	assert.deepEqual(menus.map((m) => m.id), ['file']);
	const sub = menus[0].items[0];
	assert.equal(sub.type, 'submenu');
	assert.equal(sub.items[0].title, 'C:\\a.bb'); // fertiger Text geht durch
	assert.deepEqual(sub.items[0].args, ['C:\\a.bb']);

	w.context.set('hasSelection', true);
	assert.deepEqual(w.build().map((m) => m.id), ['file', 'edit']);

	items.dispose();
	assert.deepEqual(w.build().map((m) => m.id), ['edit']);
	recent.dispose();
});

test('menus: ein Untermenü, das sich selbst enthält, hängt nicht', () => {
	const w = menuWorld();
	w.commands.register({ id: 'c', title: 'C', run() {} });
	w.contributions.contribute('menubar', [{ id: 'm' }], 'a');
	w.contributions.contribute('menus', [
		{ menu: 'm', submenu: 'm' },
		{ menu: 'm', command: 'c' }
	], 'a');
	const [m] = w.build();
	assert.equal(m.items.length, 1);
});

// ------------------------------------------------------------ extensions

function hostWorld(extra) {
	const context = createContext();
	const contributions = createContributions();
	const commands = createCommands({ context });
	const services = createServices();
	const settings = createSettings();
	const i18n = createI18n();
	const events = createEmitter();
	const host = createExtensionHost({ commands, contributions, services, context, settings, i18n, events, extra });
	return { context, contributions, commands, services, settings, i18n, events, host };
}

test('extensions: Beiträge, Befehle, Einstellungen, Texte, Ansichten mit Kontext', async () => {
	const w = hostWorld();
	let mountedWith;
	w.host.add({
		id: 'demo',
		messages: { en: { 'demo.title': 'Demo' } },
		contributes: {
			commands: [{ id: 'demo.go', title: 'demo.title', run: () => 'ok' }],
			settings: { 'demo.on': { type: 'boolean', default: true } },
			menus: [{ menu: 'm', command: 'demo.go' }],
			views: [{ id: 'v', mount: (el, ctx) => { mountedWith = [el, ctx.id]; } }],
			eigenerPunkt: [{ x: 1 }]
		}
	});
	await w.host.activateAll();
	assert.equal(w.host.stateOf('demo'), 'active');
	assert.equal(w.commands.has('demo.go'), true);
	assert.equal(w.settings.get('demo.on'), true);
	assert.equal(w.i18n.t('demo.title'), 'Demo');
	assert.equal(w.contributions.get('menus').length, 1);
	assert.deepEqual(w.contributions.get('eigenerPunkt'), [{ x: 1 }]); // Kern kennt keine feste Liste
	w.contributions.get('views')[0].mount('EL');
	assert.deepEqual(mountedWith, ['EL', 'demo']);

	await w.host.deactivateAll();
	assert.equal(w.host.stateOf('demo'), 'inactive');
	assert.equal(w.commands.has('demo.go'), false);
	assert.equal(w.contributions.get('menus').length, 0);
	assert.equal(w.settings.get('demo.on'), undefined);
	assert.equal(w.i18n.t('demo.title'), 'demo.title');
});

test('extensions: contributes als Funktion bekommt den Kontext', async () => {
	const w = hostWorld();
	let seen;
	w.host.add({
		id: 'fn',
		contributes: (ctx) => ({
			commands: [{ id: 'fn.cmd', run: () => { seen = ctx.id; } }],
			menus: [{ menu: 'm', command: 'fn.cmd' }]
		})
	});
	await w.host.activateAll();
	assert.equal(w.host.stateOf('fn'), 'active');
	await w.commands.execute('fn.cmd');
	assert.equal(seen, 'fn');
	assert.equal(w.contributions.get('menus').length, 1);
});

test('extensions: dependsOn bestimmt die Reihenfolge, Dienste über ctx, Aufräumen', async () => {
	const w = hostWorld({ documents: 'DOCS' });
	const order = [];
	let extraSeen;
	w.host.add({ id: 'b', dependsOn: ['a'], activate(ctx) { order.push('b'); ctx.subscriptions.add(() => order.push('b-off')); } });
	w.host.add({
		id: 'a',
		activate(ctx) {
			order.push('a');
			extraSeen = ctx.documents;
			ctx.services.provide('svc', { v: 1 });
		},
		deactivate() { order.push('a-deactivate'); }
	});
	await w.host.activateAll();
	assert.deepEqual(order, ['a', 'b']);
	assert.equal(extraSeen, 'DOCS');
	assert.deepEqual(w.services.get('svc'), { v: 1 });

	await w.host.deactivateAll();
	assert.deepEqual(order, ['a', 'b', 'b-off', 'a-deactivate']);
	assert.equal(w.services.has('svc'), false);
});

test('extensions: eine kaputte Erweiterung stoppt die anderen nicht und räumt sich weg', quiet(async () => {
	const w = hostWorld();
	const failures = [];
	w.host.onDidFail((f) => failures.push(f.id + ':' + f.state));
	w.host.add({
		id: 'kaputt',
		contributes: { commands: [{ id: 'k.cmd', run() {} }], menus: [{ menu: 'm', command: 'k.cmd' }] },
		activate() { throw new Error('nein'); }
	});
	w.host.add({ id: 'abhaengig', dependsOn: ['kaputt'], contributes: { commands: [{ id: 'x.cmd', run() {} }] } });
	w.host.add({ id: 'fremd', dependsOn: ['gibt-es-nicht'] });
	w.host.add({ id: 'gesund', contributes: { commands: [{ id: 'g.cmd', run() {} }] } });
	await w.host.activateAll();

	assert.equal(w.host.stateOf('kaputt'), 'failed');
	assert.equal(w.host.stateOf('abhaengig'), 'skipped');
	assert.equal(w.host.stateOf('fremd'), 'failed');
	assert.equal(w.host.stateOf('gesund'), 'active');
	assert.equal(w.commands.has('k.cmd'), false); // halb Aktiviertes ist abgebaut
	assert.equal(w.contributions.get('menus').length, 0);
	assert.equal(w.commands.has('x.cmd'), false);
	assert.equal(w.commands.has('g.cmd'), true);
	assert.deepEqual(failures.sort(), ['abhaengig:skipped', 'fremd:failed', 'kaputt:failed']);
}));

test('extensions: Zyklus in dependsOn wird gemeldet statt zu hängen', quiet(async () => {
	const w = hostWorld();
	w.host.add({ id: 'x', dependsOn: ['y'] });
	w.host.add({ id: 'y', dependsOn: ['x'] });
	await w.host.activateAll();
	const states = Object.fromEntries(w.host.list().map((e) => [e.id, e.state]));
	assert.ok(Object.values(states).every((s) => s === 'failed' || s === 'skipped'), JSON.stringify(states));
	assert.ok(w.host.list().some((e) => /Zyklus/.test(e.error || '')));
}));

test('extensions: doppelte id und fehlende id sind Fehler', () => {
	const w = hostWorld();
	w.host.add({ id: 'a' });
	assert.throws(() => w.host.add({ id: 'a' }), /schon hinzugefügt/);
	assert.throws(() => w.host.add({}), /ohne id/);
});

test('extensions: gemeinsamer Ablauf — Menüpunkt aus einer Erweiterung löst einen Befehl aus', async () => {
	// Das Kriterium von P0 aus PLAN.md, ohne Oberfläche: Beitrag -> Modell -> Befehl.
	const w = hostWorld();
	const keybindings = createKeybindings({ contributions: w.contributions, context: w.context });
	let ran = 0;
	w.host.add({
		id: 'demo',
		messages: { en: { 'menu.demo': 'Demo', 'demo.hello': 'Say hello' } },
		contributes: {
			commands: [{ id: 'demo.hello', title: 'demo.hello', run: () => { ran++; } }],
			menubar: [{ id: 'demo', title: 'menu.demo' }],
			menus: [{ menu: 'demo', command: 'demo.hello' }],
			keybindings: [{ command: 'demo.hello', key: 'F9' }]
		}
	});
	await w.host.activateAll();

	const [menu] = buildMenus({ contributions: w.contributions, commands: w.commands, keybindings, context: w.context, i18n: w.i18n });
	assert.equal(menu.title, 'Demo');
	assert.equal(menu.items[0].title, 'Say hello');
	assert.equal(menu.items[0].key, 'F9');
	await w.commands.execute(menu.items[0].command, ...menu.items[0].args);
	const bound = keybindings.resolve(keyFromEvent({ key: 'F9' }));
	await w.commands.execute(bound.command, ...bound.args);
	assert.equal(ran, 2);
});
