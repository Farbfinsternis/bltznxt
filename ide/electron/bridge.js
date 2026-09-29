// Die Brücke: ein einziger Kanal zwischen Renderer und Betriebssystem.
//
// Der Renderer ruft `invoke(dienst, methode, ...argumente)`. Welche Dienste
// und Methoden es gibt, steht in der Tabelle unten — ein neuer Dienst braucht
// dort eine Zeile und weder eine Änderung an preload.js noch einen neuen
// IPC-Kanal. Aufrufbar ist nur, was ein Dienst ausdrücklich in seinem `api`
// anbietet; alles andere (Hilfsfunktionen, Zustand) bleibt unerreichbar.
//
// Dazu ein Weg in die andere Richtung: `send(win, name, payload)` schickt ein
// Ereignis an den Renderer (etwa "window:close-requested").
//
// Ein Wechsel des Backends (etwa auf Tauri) ersetzt diese Datei, preload.js
// und src/platform/index.js — die Dienste selbst sind gewöhnliche Module.

'use strict';

const { INVOKE: CHANNEL, EVENT: EVENT_CHANNEL } = require('./channels');

/** @type {Record<string, { api: Record<string, Function> }>} */
const services = {
	toolchain: require('./services/toolchain'),
	store: require('./services/store'),
	files: require('./services/files'),
	dialog: require('./services/dialog'),
	host: require('./services/host'),
	window: require('./services/window'),
	build: require('./services/build'),
	shell: require('./services/shell')
};

/** Dienste, die nach dem Start weitere aufnehmen wollen (Erweiterungen im Main-Prozess, später). */
function addService(name, service) {
	if (services[name]) throw new Error(`Dienst "${name}" ist schon vergeben`);
	services[name] = service;
}

/**
 * @param {string} service
 * @param {string} method
 * @param {any[]} args
 * @param {{ sender?: import('electron').WebContents }} [caller]  wer fragt — Methoden sehen es als `this`
 */
async function dispatch(service, method, args, caller = {}) {
	const svc = services[service];
	if (!svc) throw new Error(`Unbekannter Dienst "${service}"`);
	const fn = Object.prototype.hasOwnProperty.call(svc.api, method) ? svc.api[method] : null;
	if (typeof fn !== 'function') throw new Error(`Dienst "${service}" hat keine Methode "${method}"`);
	return fn.apply(caller, Array.isArray(args) ? args : []);
}

/** @param {import('electron').IpcMain} ipcMain */
function register(ipcMain) {
	ipcMain.handle(CHANNEL, (event, service, method, args) => dispatch(service, method, args, { sender: event.sender }));
}

/**
 * Ein Ereignis vom Main-Prozess an den Renderer eines Fensters. Der Renderer
 * hört mit `platform.on(name, fn)`.
 */
function send(win, name, payload) {
	if (!win.isDestroyed()) win.webContents.send(EVENT_CHANNEL, name, payload);
}

module.exports = { CHANNEL, EVENT_CHANNEL, register, addService, dispatch, send };
