// Dienst "store" im Main-Prozess: kleine Dateien im Benutzerordner der IDE.
//
// Hier liegen die Einstellungen (`settings.json`) und später der Zustand
// (`state.json`). Der Dienst weiß nichts über deren Inhalt — er liest und
// schreibt Text. Namen sind flach und einfach (keine Pfade), damit der
// Renderer nichts außerhalb des Benutzerordners erreichen kann.
//
// Geschrieben wird atomar (temporäre Datei, dann umbenennen): ein Absturz
// mitten im Schreiben lässt die alte Datei ganz und nicht halb zurück.

'use strict';

const fs = require('fs');
const path = require('path');

const NAME_RE = /^[A-Za-z0-9][A-Za-z0-9._-]*$/;

let baseDir = null;

/** Wird beim Start vom Main-Prozess gesetzt (app.getPath('userData')) — oder von Tests. */
function setBaseDir(dir) {
	baseDir = dir;
}

function fileFor(name) {
	if (!baseDir) throw new Error('store: kein Basisordner gesetzt');
	if (typeof name !== 'string' || !NAME_RE.test(name)) {
		throw new Error(`store: ungültiger Name "${name}"`);
	}
	return path.join(baseDir, name);
}

/** @returns {Promise<string|null>} Inhalt, oder `null`, wenn es die Datei nicht gibt */
async function read(name) {
	try {
		return await fs.promises.readFile(fileFor(name), 'utf8');
	} catch (err) {
		if (err.code === 'ENOENT') return null;
		throw err;
	}
}

async function write(name, text) {
	const file = fileFor(name);
	await fs.promises.mkdir(baseDir, { recursive: true });
	const tmp = `${file}.${process.pid}.tmp`;
	await fs.promises.writeFile(tmp, String(text), 'utf8');
	await fs.promises.rename(tmp, file);
}

async function remove(name) {
	try {
		await fs.promises.unlink(fileFor(name));
	} catch (err) {
		if (err.code !== 'ENOENT') throw err;
	}
}

module.exports = {
	api: { read, write, remove },
	setBaseDir
};
