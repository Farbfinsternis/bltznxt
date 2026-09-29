// Dienst "window": das Fenster schließen — aber erst, wenn der Renderer zustimmt.
//
// Das Schließen mit dem Kreuz wird abgefangen und dem Renderer gemeldet
// ("window:close-requested"). Der fragt bei ungespeicherten Dateien nach und
// ruft `close()`, wenn es weitergehen darf. Ohne diese Zustimmung schließt das
// Fenster nicht.

'use strict';

const { BrowserWindow } = require('electron');

/** Fenster, die ohne Rückfrage schließen dürfen. */
const approved = new WeakSet();

/**
 * @param {import('electron').BrowserWindow} win
 * @param {(win: import('electron').BrowserWindow, name: string, payload?: any) => void} send
 */
function installCloseGuard(win, send) {
	win.on('close', (event) => {
		if (approved.has(win)) return;
		event.preventDefault();
		send(win, 'window:close-requested');
	});
}

/** Der Renderer hat zugestimmt: jetzt wirklich schließen. */
function close() {
	const win = this && this.sender ? BrowserWindow.fromWebContents(this.sender) : null;
	if (!win) return;
	approved.add(win);
	win.close();
}

module.exports = { api: { close }, installCloseGuard };
