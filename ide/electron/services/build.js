// Dienst "build": bauen, prüfen, erstellen, starten — und dem Renderer berichten.
//
// `start` gibt sofort zurück; alles Weitere kommt als Ereignis "build:event"
// mit dem `token`, den der Renderer mitgegeben hat (so kann er zuhören, bevor
// das erste Ereignis kommt). Ereignisse siehe build-runner.js.
//
// Ein Programm läuft unabhängig von der IDE weiter, auch wenn deren Fenster
// schließt; `stop` beendet es ausdrücklich.

'use strict';

const { BrowserWindow } = require('electron');

const { EVENT } = require('../channels');
const { runBuild } = require('./build-runner');
const { splitCommandLine } = require('../build-protocol');

/** @type {Map<string, { handle: { kill(): void }, sender: import('electron').WebContents }>} */
const runs = new Map();

function emit(sender, token, event) {
	const win = sender ? BrowserWindow.fromWebContents(sender) : null;
	if (win && !win.isDestroyed()) win.webContents.send(EVENT, 'build:event', { token, ...event });
}

/**
 * @param {{ token: string, file: string, mode: 'run' | 'check' | 'publish', debug?: boolean, output?: string, commandLine?: string, compilerPath?: string }} opts
 * @returns {Promise<{ started: boolean, pid?: number }>}
 */
async function start(opts) {
	const sender = this && this.sender;
	if (!opts || typeof opts.token !== 'string' || !opts.token) throw new Error('build: token fehlt');
	if (runs.has(opts.token)) throw new Error(`build: token "${opts.token}" läuft schon`);

	const handle = runBuild(
		{
			file: opts.file,
			mode: opts.mode,
			debug: Boolean(opts.debug),
			output: opts.output,
			args: splitCommandLine(opts.commandLine),
			compilerPath: opts.compilerPath
		},
		(event) => {
			if (event.type === 'exit' || event.type === 'failed') runs.delete(opts.token);
			emit(sender, opts.token, event);
		}
	);
	if (!handle) return { started: false };
	runs.set(opts.token, { handle, sender });
	return { started: true, pid: handle.pid };
}

/** Beendet einen Lauf (token) oder alle Läufe dieses Fensters (ohne token). */
async function stop(token) {
	const sender = this && this.sender;
	let n = 0;
	for (const [t, run] of [...runs]) {
		if (token !== undefined && t !== token) continue;
		if (token === undefined && sender && run.sender !== sender) continue;
		run.handle.kill();
		n++;
	}
	return n;
}

module.exports = { api: { start, stop }, stopAll: () => stop.call({}) };
