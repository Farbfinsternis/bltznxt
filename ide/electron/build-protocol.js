// Das Bau-Protokoll der Original-IDE, wie blitzcc es spricht.
//
// Die IDE ruft `blitzcc -q [-d] [-c | -o "x.exe"] "datei.bb" [Argumente]` mit
// der Umgebungsvariablen `blitzide=1` und liest stdout und stderr aus einer
// Leitung (blitzide/mainframe.cpp, MainFrame::compile). Jede Zeile ist
//
//   Compiling...                       Fortschritt: endet auf "..."
//   Executing...                       das Programm läuft jetzt (nur ohne -c und -o)
//   "datei":z:s:z:s:Meldung            ein Fehler; Ort als Anfang und Ende (1-basiert)
//   irgendetwas anderes                ebenfalls ein Fehler, ohne Ort
//
// Das ist der einzige Vertrag zwischen IDE und Compiler — er gilt auch für den
// originalen blitzcc. Dieses Modul ist rein (kein Prozess, kein Electron) und
// lässt sich deshalb an festen Zeilen prüfen.

'use strict';

const ERROR_RE = /^"(.*?)":(\d+):(\d+):(\d+):(\d+):(.*)$/;

/**
 * @typedef {{ kind: 'progress', text: string }
 *   | { kind: 'executing' }
 *   | { kind: 'error', message: string, file?: string, line?: number, column?: number, endLine?: number, endColumn?: number }
 *   | { kind: 'output', text: string }} Line
 */

/**
 * Ordnet eine Zeile der Ausgabe ein.
 * @param {string} raw
 * @param {{ running?: boolean }} [state]  `running`: "Executing..." kam schon — alles Weitere ist Ausgabe, kein Fehler
 * @returns {Line | null}  `null` bei leeren Zeilen
 */
function parseLine(raw, state = {}) {
	const line = String(raw).replace(/\r$/, '');
	if (!line.trim()) return null;

	if (line.startsWith('Executing')) return { kind: 'executing' };
	if (state.running) return { kind: 'output', text: line };

	if (line.startsWith('"')) {
		const m = ERROR_RE.exec(line);
		if (m) {
			return {
				kind: 'error',
				file: m[1],
				line: Number(m[2]),
				column: Number(m[3]),
				endLine: Number(m[4]),
				endColumn: Number(m[5]),
				message: m[6].trim()
			};
		}
		return { kind: 'error', message: line };
	}

	if (line.endsWith('...')) return { kind: 'progress', text: line };
	return { kind: 'error', message: line };
}

/**
 * Zerlegt Ausgabe in Zeilen, auch wenn Blöcke mitten in einer Zeile enden.
 * @returns {{ push(chunk: string): string[], flush(): string[] }}
 */
function createLineSplitter() {
	let rest = '';
	return {
		push(chunk) {
			rest += chunk;
			const parts = rest.split(/\r?\n/);
			rest = parts.pop();
			return parts;
		},
		flush() {
			const last = rest;
			rest = '';
			return last ? [last] : [];
		}
	};
}

/**
 * Die Programmargumente aus dem Feld "Program Command Line": durch Leerzeichen
 * getrennt, Anführungszeichen fassen zusammen (wie bei einer Windows-Kommandozeile).
 * @param {string} text
 * @returns {string[]}
 */
function splitCommandLine(text) {
	const args = [];
	let cur = '';
	let inQuotes = false;
	let has = false;
	for (const ch of String(text || '')) {
		if (ch === '"') {
			inQuotes = !inQuotes;
			has = true;
		} else if (/\s/.test(ch) && !inQuotes) {
			if (has) args.push(cur);
			cur = '';
			has = false;
		} else {
			cur += ch;
			has = true;
		}
	}
	if (has) args.push(cur);
	return args;
}

/**
 * Die Argumente für blitzcc.
 * @param {{ file: string, mode: 'run' | 'check' | 'publish', debug?: boolean, output?: string, args?: string[] }} o
 */
function buildArgs({ file, mode, debug = false, output, args = [] }) {
	const argv = ['-q'];
	if (debug) argv.push('-d');
	if (mode === 'check') argv.push('-c');
	else if (mode === 'publish') {
		if (!output) throw new Error('publish braucht einen Ausgabenamen');
		argv.push('-o', output);
	} else if (mode !== 'run') throw new Error(`Unbekannter Modus "${mode}"`);
	argv.push(file, ...args);
	return argv;
}

module.exports = { parseLine, createLineSplitter, splitCommandLine, buildArgs };
