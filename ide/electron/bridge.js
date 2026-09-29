// Die Brücke: ein einziger Kanal zwischen Renderer und Betriebssystem.
//
// Der Renderer ruft `invoke(dienst, methode, ...argumente)`. Welche Dienste
// und Methoden es gibt, steht in der Tabelle unten — ein neuer Dienst braucht
// dort eine Zeile und weder eine Änderung an preload.js noch einen neuen
// IPC-Kanal. Aufrufbar ist nur, was ein Dienst ausdrücklich in seinem `api`
// anbietet; alles andere (Hilfsfunktionen, Zustand) bleibt unerreichbar.
//
// Ein Wechsel des Backends (etwa auf Tauri) ersetzt diese Datei, preload.js
// und src/platform/index.js — die Dienste selbst sind gewöhnliche Module.

'use strict';

const CHANNEL = 'bltznxt:invoke';

/** @type {Record<string, { api: Record<string, Function> }>} */
const services = {
	toolchain: require('./services/toolchain'),
	store: require('./services/store')
};

/** Dienste, die nach dem Start weitere aufnehmen wollen (Erweiterungen im Main-Prozess, später). */
function addService(name, service) {
	if (services[name]) throw new Error(`Dienst "${name}" ist schon vergeben`);
	services[name] = service;
}

async function dispatch(service, method, args) {
	const svc = services[service];
	if (!svc) throw new Error(`Unbekannter Dienst "${service}"`);
	const fn = Object.prototype.hasOwnProperty.call(svc.api, method) ? svc.api[method] : null;
	if (typeof fn !== 'function') throw new Error(`Dienst "${service}" hat keine Methode "${method}"`);
	return fn(...(Array.isArray(args) ? args : []));
}

/** @param {import('electron').IpcMain} ipcMain */
function register(ipcMain) {
	ipcMain.handle(CHANNEL, (_event, service, method, args) => dispatch(service, method, args));
}

module.exports = { CHANNEL, register, addService, dispatch };
