// Der Zusammenbau (src/app.js) ohne Fenster: Start, Einstellungen, Kontext.

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { createApp } from '../../src/app.js';

function fakePlatform(files = {}) {
	const writes = [];
	return {
		writes,
		files,
		hasBackend: false,
		store: {
			async read(name) { return name in files ? files[name] : null; },
			async write(name, text) { files[name] = text; writes.push(name); },
			async remove(name) { delete files[name]; }
		}
	};
}

// Speichern ohne echte Zeit: der Test löst es von Hand aus
function manualTimer() {
	let pending = null;
	return {
		schedule: (fn) => { pending = fn; return 1; },
		cancel: () => { pending = null; },
		fire: async () => { const fn = pending; pending = null; if (fn) await fn(); },
		get pending() { return pending !== null; }
	};
}

test('app: Start lädt Einstellungen, aktiviert Erweiterungen, meldet app:started', async () => {
	const platform = fakePlatform({
		'settings.json': JSON.stringify({ version: 1, settings: { 'demo.n': 7 } })
	});
	let seen;
	const app = createApp({
		platform,
		extensions: [{
			id: 'demo',
			contributes: { settings: { 'demo.n': { type: 'number', default: 1 } } },
			activate(ctx) { seen = ctx.settings.get('demo.n'); }
		}]
	});
	let started = false;
	app.events.on('app:started', () => { started = true; });
	await app.start();
	assert.equal(seen, 7); // geladen, bevor die Erweiterung aktiviert wird
	assert.equal(started, true);
});

test('app: Einstellungen werden verzögert und gebündelt gesichert', async () => {
	const platform = fakePlatform();
	const timer = manualTimer();
	const app = createApp({
		platform,
		schedule: timer.schedule,
		cancel: timer.cancel,
		extensions: [{ id: 'demo', contributes: { settings: { 'demo.a': { type: 'number', default: 0 } } } }]
	});
	await app.start();
	assert.equal(timer.pending, false);
	app.settings.set('demo.a', 1);
	app.settings.set('demo.a', 2);
	assert.equal(platform.writes.length, 0);
	await timer.fire();
	assert.equal(platform.writes.length, 1);
	assert.equal(JSON.parse(platform.files['settings.json']).settings['demo.a'], 2);
});

test('app: stop sichert Ausstehendes sofort', async () => {
	const platform = fakePlatform();
	const timer = manualTimer();
	const app = createApp({
		platform, schedule: timer.schedule, cancel: timer.cancel,
		extensions: [{ id: 'demo', contributes: { settings: { 'demo.a': { type: 'number', default: 0 } } } }]
	});
	await app.start();
	app.settings.set('demo.a', 5);
	await app.stop();
	assert.equal(JSON.parse(platform.files['settings.json']).settings['demo.a'], 5);
});

test('app: kaputte Einstellungen werden gesichert, gemeldet — nach der Aktivierung', async () => {
	const platform = fakePlatform({ 'settings.json': '{ kaputt' });
	const order = [];
	const app = createApp({
		platform,
		extensions: [{
			id: 'listener',
			activate(ctx) { ctx.events.on('settings:corrupt', (e) => order.push(['corrupt', e.backup])); order.push(['activated']); }
		}]
	});
	await app.start();
	assert.equal(platform.files['settings.json.bad'], '{ kaputt');
	assert.deepEqual(order, [['activated'], ['corrupt', 'settings.json.bad']]);
});

test('app: Dokumentzustand steht als Kontext bereit', async () => {
	const app = createApp({ platform: fakePlatform() });
	await app.start();
	assert.equal(app.context.get('document.active'), false);
	assert.equal(app.context.get('documents.count'), 0);

	const a = app.documents.open({ text: 'x' });
	assert.equal(app.context.get('document.active'), true);
	assert.equal(app.context.get('document.dirty'), true); // namenlos, mit Text
	assert.equal(app.context.get('document.kind'), 'scratch');
	assert.equal(app.context.get('documents.anyDirty'), true);

	app.documents.markSaved(a.id, { uri: 'C:\\x.bb' });
	assert.equal(app.context.get('document.dirty'), false);
	assert.equal(app.context.get('document.kind'), 'file');
	assert.equal(app.context.get('documents.anyDirty'), false);

	app.documents.close(a.id);
	assert.equal(app.context.get('document.active'), false);
});

test('app: run meldet einen Fehler über den Dialogdienst und wirft nicht', async () => {
	const app = createApp({
		platform: fakePlatform(),
		extensions: [{
			id: 'demo',
			messages: { en: { 'error.title': 'Error', 'error.command': 'Command {id} failed: {message}' } },
			contributes: { commands: [{ id: 'demo.boom', run() { throw new Error('kaputt'); } }] }
		}]
	});
	await app.start();
	const shown = [];
	app.services.provide('dialogs', { message: async (m) => { shown.push(m); } });
	const orig = console.error;
	console.error = () => {};
	try {
		const result = await app.run('demo.boom');
		assert.equal(result.executed, false);
	} finally {
		console.error = orig;
	}
	assert.equal(shown.length, 1);
	assert.match(shown[0].text, /demo\.boom.*kaputt/);
});
