// Preload: die einzige Brücke zwischen Renderer und Betriebssystem.
//
// Der Renderer bekommt ausschließlich diese vier Funktionen — kein require,
// kein child_process, kein fs. Das hält die Angriffsfläche klein und macht
// einen späteren Wechsel des Backends (etwa auf Tauri) zu einem Austausch
// dieser Datei plus ide/src/platform/index.js, nicht der IDE.

'use strict';

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('bltznxt', {
	getInfo: () => ipcRenderer.invoke('bltznxt:getInfo'),
	listCommands: () => ipcRenderer.invoke('bltznxt:listCommands'),
	compile: (file, opts) => ipcRenderer.invoke('bltznxt:compile', file, opts),
	setCompilerPath: (p) => ipcRenderer.invoke('bltznxt:setCompilerPath', p)
});
