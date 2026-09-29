// Preload: die einzige Brücke zwischen Renderer und Betriebssystem.
//
// Der Renderer bekommt genau zwei Funktionen (Aufruf und Ereignis) — kein require, kein
// child_process, kein fs. Welche Dienste dahinter stehen, entscheidet
// electron/bridge.js; hier ändert sich nichts, wenn ein Dienst dazukommt.
// Das hält die Angriffsfläche klein und macht einen späteren Wechsel des
// Backends (etwa auf Tauri) zu einem Austausch dieser Datei plus
// ide/src/platform/index.js, nicht der IDE.

'use strict';

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('bltznxt', {
	invoke: (service, method, ...args) => ipcRenderer.invoke('bltznxt:invoke', service, method, args),
	// Ereignisse vom Main-Prozess; gibt die Abmeldefunktion zurück
	on: (name, fn) => {
		const handler = (_event, eventName, payload) => {
			if (eventName === name) fn(payload);
		};
		ipcRenderer.on('bltznxt:event', handler);
		return () => ipcRenderer.removeListener('bltznxt:event', handler);
	}
});
