// Bauen und Starten: blitzcc als Prozess, Zeile für Zeile gelesen.
//
// Kein Electron hier — der Runner bekommt einen Rückruf für Ereignisse und
// lässt sich unter reinem Node prüfen (test/smoke.js). Der Dienst "build"
// (build.js) hängt ihn an die Brücke.
//
// Ereignisse (`onEvent`):
//   { type: 'progress', text }                 "Compiling..."
//   { type: 'error', message, file?, line?, column?, endLine?, endColumn? }
//   { type: 'running' }                        "Executing..." — das Programm läuft
//   { type: 'output', text }                   Zeilen nach "Executing..."
//   { type: 'exit', code, signal }             blitzcc ist fertig (bei Programmen: es ist beendet)
//   { type: 'failed', message }                blitzcc ließ sich nicht starten

'use strict';

const { spawn } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { parseLine, createLineSplitter, buildArgs } = require('../build-protocol');
const { resolveCompiler } = require('./toolchain');

/**
 * @param {{ file: string, mode: 'run' | 'check' | 'publish', debug?: boolean, output?: string, args?: string[], compilerPath?: string }} opts
 * @param {(event: object) => void} onEvent
 * @returns {{ pid: number | undefined, kill(): void } | null}  `null`, wenn der Start scheiterte (das Ereignis 'failed' ist dann schon gesendet)
 */
function runBuild(opts, onEvent) {
	const found = resolveCompiler(opts.compilerPath);
	if (!found.available) {
		onEvent({ type: 'failed', message: 'blitzcc not found. Set the compiler path in the settings or BLITZPATH.' });
		return null;
	}

	let argv;
	try {
		argv = buildArgs(opts);
	} catch (err) {
		onEvent({ type: 'failed', message: err.message });
		return null;
	}

	let child;
	try {
		child = spawn(found.path, argv, {
			// Das Programm läuft im Ordner seiner Quelldatei; blitzcc erbt ihn
			cwd: path.dirname(opts.file),
			env: { ...process.env, blitzide: '1' },
			windowsHide: true
		});
	} catch (err) {
		onEvent({ type: 'failed', message: String(err.message) });
		return null;
	}

	const state = { running: false };
	// stdout und stderr sind für die IDE eine Leitung: gemeinsam aufgereiht
	const splitter = createLineSplitter();

	function handle(lines) {
		for (const raw of lines) {
			const line = parseLine(raw, state);
			if (!line) continue;
			if (line.kind === 'executing') {
				state.running = true;
				onEvent({ type: 'running' });
			} else if (line.kind === 'progress') {
				onEvent({ type: 'progress', text: line.text });
			} else if (line.kind === 'output') {
				onEvent({ type: 'output', text: line.text });
			} else {
				const { kind, ...rest } = line;
				onEvent({ type: 'error', ...rest });
			}
		}
	}

	child.stdout.on('data', (d) => handle(splitter.push(d.toString('latin1'))));
	child.stderr.on('data', (d) => handle(splitter.push(d.toString('latin1'))));
	child.on('error', (err) => onEvent({ type: 'failed', message: String(err.message) }));
	child.on('close', (code, signal) => {
		handle(splitter.flush());
		onEvent({ type: 'exit', code: code === null ? -1 : code, signal: signal || null });
	});

	let killed = false;
	child.on('close', () => {
		// blitzcc räumt sein Arbeitsverzeichnis selbst weg — nicht, wenn wir es
		// mitten im Lauf beendet haben.
		if (killed) removeBuildDir(opts.file, child.pid);
	});

	return {
		pid: child.pid,
		/** Beendet blitzcc samt dem Programm, das es gestartet hat. */
		kill() {
			killed = true;
			killTree(child.pid);
		}
	};
}

/**
 * Das Verzeichnis, in dem blitzcc ohne -o das Programm ablegt:
 * %TEMP%\bltznxt-ide\<Name der Quelle>-<Prozess von blitzcc>
 */
function buildDirFor(file, pid) {
	return path.join(os.tmpdir(), 'bltznxt-ide', `${path.basename(file, path.extname(file))}-${pid}`);
}

function removeBuildDir(file, pid) {
	// Das beendete Programm gibt seine Dateien erst kurz nach dem Ende frei
	fs.promises
		.rm(buildDirFor(file, pid), { recursive: true, force: true, maxRetries: 10, retryDelay: 200 })
		.catch(() => {});
}

/** Prozess und alle seine Kinder beenden (Windows: taskkill /T; sonst das Signal). */
function killTree(pid) {
	if (!pid) return;
	if (process.platform === 'win32') {
		try {
			spawn('taskkill', ['/PID', String(pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
		} catch {
			/* nichts zu tun: der Prozess ist vermutlich schon weg */
		}
	} else {
		try {
			process.kill(pid, 'SIGTERM');
		} catch {
			/* schon beendet */
		}
	}
}

module.exports = { runBuild, killTree };
