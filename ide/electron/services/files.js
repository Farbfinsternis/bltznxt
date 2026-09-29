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
const os = require('os');
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

// ---------------------------------------------------------------------------
// Namenlose Dokumente: eine Datei je Tab im Temp-Ordner
// ---------------------------------------------------------------------------
//
// Ein namenloses Dokument muss zum Bauen in eine Datei. Das Original benutzt
// dafür eine gemeinsame `<Blitz>\tmp\tmp.bb`; hier bekommt jeder Tab seinen
// eigenen Ordner, damit mehrere namenlose Programme gleichzeitig gebaut werden
// können:
//
//   %TEMP%\bltznxt-ide\scratch\<Prozess>\<Schlüssel>\untitled.bb
//
// Beim Beenden wird der eigene Ordner gelöscht, beim Start dazu die Ordner
// abgestürzter Sitzungen (deren Prozess es nicht mehr gibt).

const scratchRoot = () => path.join(os.tmpdir(), 'bltznxt-ide', 'scratch');
const SCRATCH_KEY_RE = /^[A-Za-z0-9_-]+$/;

/** @returns {Promise<string>} Pfad der Datei; der Ordner ist angelegt */
async function scratchFile(key) {
	if (typeof key !== 'string' || !SCRATCH_KEY_RE.test(key)) throw new Error(`files: ungültiger Schlüssel "${key}"`);
	const dir = path.join(scratchRoot(), String(process.pid), key);
	await fs.promises.mkdir(dir, { recursive: true });
	return path.join(dir, 'untitled.bb');
}

function processAlive(pid) {
	try {
		process.kill(pid, 0);
		return true;
	} catch (err) {
		return err.code === 'EPERM'; // gibt es, gehört nur jemand anderem
	}
}

/**
 * blitzcc legt ein Programm ohne -o in `%TEMP%\bltznxt-ide\<Name>-<Prozess>` ab und
 * löscht es danach selbst. Wurde es dabei beendet (Absturz, Stopp von außen),
 * bleibt der Ordner: hier werden die weggeräumt, deren blitzcc nicht mehr läuft.
 */
async function cleanBuildDirs() {
	const root = path.join(os.tmpdir(), 'bltznxt-ide');
	let names = [];
	try {
		names = await fs.promises.readdir(root);
	} catch {
		return;
	}
	for (const name of names) {
		const m = /^.+-(\d+)$/.exec(name);
		if (!m || name === 'scratch' || processAlive(Number(m[1]))) continue;
		await fs.promises.rm(path.join(root, name), { recursive: true, force: true }).catch(() => {});
	}
}

/**
 * Löscht den eigenen Scratch-Ordner (`own`) und die Ordner beendeter Sitzungen.
 * @param {{ own?: boolean }} [opts]
 */
async function cleanScratch({ own = false } = {}) {
	await cleanBuildDirs();
	let names = [];
	try {
		names = await fs.promises.readdir(scratchRoot());
	} catch {
		return;
	}
	for (const name of names) {
		const pid = Number(name);
		const mine = pid === process.pid;
		if (!Number.isInteger(pid)) continue;
		if (mine ? !own : processAlive(pid)) continue;
		await fs.promises.rm(path.join(scratchRoot(), name), { recursive: true, force: true });
	}
}

module.exports = {
	api: { resolve: async (p) => resolve(p), read, write, exists, backup, scratchFile },
	cleanScratch,
	scratchRoot
};
