// Dienst "dialog": die Datei-Dialoge des Betriebssystems.
//
// Meldungen und Rückfragen ("Speichern?") zeichnet die IDE selbst
// (src/shell/dialogs.js); hier liegen nur die Dialoge, die das System besser
// kann: Datei öffnen und Datei speichern.
//
// Für Tests austauschbar: `setImpl` ersetzt beide, damit ein Test ohne
// sichtbaren, blockierenden Dialog läuft.

'use strict';

const { dialog, BrowserWindow } = require('electron');

let impl = null;

/** Nur für Tests. `null` stellt die echten Dialoge wieder her. */
function setImpl(next) {
	impl = next;
}

function parentOf(ctx) {
	return ctx && ctx.sender ? BrowserWindow.fromWebContents(ctx.sender) : BrowserWindow.getFocusedWindow();
}

/**
 * @param {{ title?: string, defaultPath?: string, filters?: Array<{name: string, extensions: string[]}>, multi?: boolean }} [opts]
 * @returns {Promise<string[] | null>} gewählte Pfade oder `null` bei Abbruch
 */
async function open(opts = {}) {
	if (impl) return impl.open(opts);
	const result = await dialog.showOpenDialog(parentOf(this), {
		title: opts.title,
		defaultPath: opts.defaultPath || undefined,
		filters: opts.filters,
		properties: ['openFile', ...(opts.multi ? ['multiSelections'] : [])]
	});
	return result.canceled || result.filePaths.length === 0 ? null : result.filePaths;
}

/**
 * @param {{ title?: string, defaultPath?: string, filters?: Array<{name: string, extensions: string[]}> }} [opts]
 * @returns {Promise<string | null>}
 */
async function save(opts = {}) {
	if (impl) return impl.save(opts);
	const result = await dialog.showSaveDialog(parentOf(this), {
		title: opts.title,
		defaultPath: opts.defaultPath || undefined,
		filters: opts.filters
	});
	return result.canceled || !result.filePath ? null : result.filePath;
}

module.exports = { api: { open, save }, setImpl };
