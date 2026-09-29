// Dienst "files": Dateien als Bytes.
//
// Der Dienst weiß nichts von Kodierung, Zeilenenden oder Blitz3D — er liest
// und schreibt Bytes, wie sie sind. Was sie bedeuten, entscheidet der
// Renderer (src/core/encoding.js). So bleibt die Regel "Bytes hinein, dieselben
// Bytes heraus" an einer Stelle prüfbar.
//
// Sicherungskopien folgen dem Original (blitzide/mainframe.cpp, MainFrame::save):
// `datei.bb_bak1`, `datei.bb_bak2`, ... — vor jedem Schreiben rutscht die alte
// Kopie eine Nummer weiter, die jetzige Datei wird `_bak1`.

'use strict';

const fs = require('fs');
const path = require('path');

/** Absoluter, bereinigter Pfad (ohne die Datei anzufassen). */
function resolve(p) {
	if (typeof p !== 'string' || !p) throw new Error('files: leerer Pfad');
	return path.resolve(p);
}

/** @returns {Promise<{ path: string, bytes: Buffer }>} */
async function read(p) {
	const full = resolve(p);
	const bytes = await fs.promises.readFile(full);
	return { path: full, bytes };
}

async function write(p, bytes) {
	const full = resolve(p);
	await fs.promises.writeFile(full, Buffer.from(bytes));
	return full;
}

async function exists(p) {
	try {
		await fs.promises.access(resolve(p));
		return true;
	} catch {
		return false;
	}
}

async function copyIfExists(from, to) {
	try {
		await fs.promises.copyFile(from, to);
		return true;
	} catch (err) {
		if (err.code === 'ENOENT') return false;
		throw err;
	}
}

/**
 * Rotiert die Sicherungskopien und kopiert die jetzige Datei nach `_bak1`.
 * Gibt es die Datei noch nicht (erstes Speichern), passiert nichts.
 * @param {string} p
 * @param {number} count  wie viele Kopien (0 = keine)
 */
async function backup(p, count) {
	const full = resolve(p);
	const n = Math.max(0, Math.min(99, Math.floor(Number(count) || 0)));
	if (n === 0) return;
	for (let k = n; k > 1; k--) {
		await copyIfExists(`${full}_bak${k - 1}`, `${full}_bak${k}`);
	}
	await copyIfExists(full, `${full}_bak1`);
}

module.exports = {
	api: { resolve: async (p) => resolve(p), read, write, exists, backup }
};
