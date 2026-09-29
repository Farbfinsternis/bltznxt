// Dienst "files" im Main-Prozess: Bytes und Sicherungskopien, an echten Dateien.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const require = createRequire(import.meta.url);
const { api, cleanScratch, scratchRoot } = require('../../electron/services/files.js');
const host = require('../../electron/services/host.js');

const tmp = () => fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-files-'));

test('files: schreiben und lesen sind byte-gleich (auch 0x00-0xFF)', async () => {
	const dir = tmp();
	const file = path.join(dir, 'all.bb');
	const bytes = Uint8Array.from({ length: 256 }, (_, i) => i);
	await api.write(file, bytes);
	assert.deepEqual([...fs.readFileSync(file)], [...bytes]);
	const read = await api.read(file);
	assert.equal(read.path, path.resolve(file));
	assert.deepEqual([...read.bytes], [...bytes]);
	fs.rmSync(dir, { recursive: true });
});

test('files: lesen einer fehlenden Datei wirft mit Code, exists sagt nein', async () => {
	const dir = tmp();
	await assert.rejects(() => api.read(path.join(dir, 'gibt-es-nicht.bb')), { code: 'ENOENT' });
	assert.equal(await api.exists(path.join(dir, 'gibt-es-nicht.bb')), false);
	fs.rmSync(dir, { recursive: true });
});

test('files: Sicherungskopien rotieren wie im Original (_bak1, _bak2)', async () => {
	const dir = tmp();
	const file = path.join(dir, 'a.bb');
	const put = (text) => api.write(file, Buffer.from(text));
	const save = async (text) => {
		await api.backup(file, 2);
		await put(text);
	};
	const text = (p) => fs.readFileSync(p, 'utf8');

	await save('eins'); // erstes Speichern: es gibt noch nichts zu sichern
	assert.equal(fs.existsSync(`${file}_bak1`), false);

	await save('zwei');
	assert.equal(text(`${file}_bak1`), 'eins');

	await save('drei');
	assert.equal(text(`${file}_bak1`), 'zwei');
	assert.equal(text(`${file}_bak2`), 'eins');

	await save('vier');
	assert.equal(text(file), 'vier');
	assert.equal(text(`${file}_bak1`), 'drei');
	assert.equal(text(`${file}_bak2`), 'zwei');
	assert.equal(fs.existsSync(`${file}_bak3`), false); // nur zwei Kopien
	fs.rmSync(dir, { recursive: true });
});

test('files: 0 Kopien schaltet Sicherungen aus, ungültige Zahlen ebenso', async () => {
	const dir = tmp();
	const file = path.join(dir, 'b.bb');
	await api.write(file, Buffer.from('x'));
	for (const n of [0, -3, NaN, undefined, 'abc']) await api.backup(file, n);
	assert.equal(fs.existsSync(`${file}_bak1`), false);
	fs.rmSync(dir, { recursive: true });
});

test('files: resolve macht absolute Pfade, leere Pfade sind ein Fehler', async () => {
	assert.equal(await api.resolve('a/../b.bb'), path.resolve('b.bb'));
	await assert.rejects(() => api.resolve(''), /leerer Pfad/);
});

test('files: Scratch-Datei je Tab, ungültige Schlüssel abgelehnt, aufräumen', async () => {
	const a = await api.scratchFile('doc1');
	const b = await api.scratchFile('doc2');
	assert.notEqual(a, b);
	assert.ok(a.endsWith(path.join('doc1', 'untitled.bb')));
	assert.ok(fs.existsSync(path.dirname(a)));
	await assert.rejects(() => api.scratchFile('../boese'), /ungültiger Schlüssel/);
	await assert.rejects(() => api.scratchFile(''), /ungültiger Schlüssel/);

	// Ordner einer beendeten Sitzung verschwindet, der eigene erst mit own
	const dead = path.join(scratchRoot(), '2147483000');
	fs.mkdirSync(path.join(dead, 'doc1'), { recursive: true });
	await cleanScratch();
	assert.equal(fs.existsSync(dead), false);
	assert.equal(fs.existsSync(path.dirname(a)), true);
	await cleanScratch({ own: true });
	assert.equal(fs.existsSync(path.dirname(a)), false);
});

test('files: liegengebliebene Bau-Ordner beendeter blitzcc werden weggeräumt, laufende nicht', async () => {
	const root = path.join(os.tmpdir(), 'bltznxt-ide');
	const dead = path.join(root, 'unittest-2147483000');
	const alive = path.join(root, `unittest-${process.pid}`);
	fs.mkdirSync(dead, { recursive: true });
	fs.mkdirSync(alive, { recursive: true });
	fs.writeFileSync(path.join(dead, 'x.exe'), 'x');
	await cleanScratch();
	assert.equal(fs.existsSync(dead), false);
	assert.equal(fs.existsSync(alive), true);
	fs.rmSync(alive, { recursive: true });
});

test('host: Startdateien aus argv (ungepackt und gepackt), nur vorhandene Dateien', () => {
	const dir = tmp();
	const real = path.join(dir, 'prog.bb');
	fs.writeFileSync(real, 'Print 1');
	host.setLaunchArgs(['electron.exe', 'app', real, '--flag', path.join(dir, 'fehlt.bb'), dir], false);
	return host.api.launchFiles().then((files) => {
		assert.deepEqual(files, [real]);
		host.setLaunchArgs(['ide.exe', `"${real}"`], true);
		return host.api.launchFiles();
	}).then((files) => {
		assert.deepEqual(files, [real]);
		fs.rmSync(dir, { recursive: true });
	});
});
