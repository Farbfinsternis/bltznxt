// Oberflächentest: die gebaute IDE in einem versteckten Electron-Fenster.
//
//   npm run test:ui        (baut zuerst mit Vite)
//
// Prüft das Kriterium von P0 (PLAN.md) durch die echte Oberfläche: eine
// Erweiterung trägt einen Menüpunkt bei, er erscheint, löst seinen Befehl aus;
// dazu Kürzel, Dialog, Editor, Tab-Leiste, Statuszeile und dass Einstellungen
// auf die Platte kommen und eine kaputte Datei gesichert statt überschrieben wird.
//
// Kein Test der Optik — nur der Struktur. Fehler auf der Konsole der Seite
// sind ein Testfehler.

'use strict';

const { app, BrowserWindow, ipcMain } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');

const bridge = require('../electron/bridge');
const store = require('../electron/services/store');

const DIST = path.join(__dirname, '..', 'dist', 'index.html');

let failures = 0;
function check(label, ok, detail) {
	console.log(`${ok ? 'PASS' : 'FAIL'}  ${label}${detail !== undefined && detail !== '' ? '  -> ' + detail : ''}`);
	if (!ok) failures++;
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function waitFor(win, expr, timeout = 15000) {
	const end = Date.now() + timeout;
	while (Date.now() < end) {
		try {
			if (await win.webContents.executeJavaScript(expr)) return true;
		} catch {
			/* Seite noch nicht bereit */
		}
		await sleep(50);
	}
	return false;
}

const js = (win, expr) => win.webContents.executeJavaScript(expr);

async function openWindow(userData) {
	store.setBaseDir(userData);
	const errors = [];
	const win = new BrowserWindow({
		show: false,
		width: 1100,
		height: 800,
		webPreferences: {
			nodeIntegration: false,
			contextIsolation: true,
			preload: path.join(__dirname, '..', 'electron', 'preload.js'),
			webSecurity: false
		}
	});
	win.webContents.on('console-message', (event) => {
		// Electron 34: Argumente (event) mit level/message; ältere: positionsabhängig
		const level = event.level ?? 0;
		const message = event.message ?? '';
		if (level === 'error' || level === 3) errors.push(message);
	});
	await win.loadFile(DIST);
	const started = await waitFor(win, 'Boolean(window.__ide && window.__ide.app.host.list().length)');
	return { win, errors, started };
}

async function scenarioNormal() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	const { win, errors, started } = await openWindow(dir);
	check('Start: IDE geladen', started);
	if (!started) return win.destroy();
	await waitFor(win, 'window.__ide.app.host.list().every((e) => e.state !== "pending")');
	await sleep(300); // Compilersuche und Befehlsliste laufen nebenher

	// --- Erweiterungen -----------------------------------------------------
	const exts = await js(win, 'window.__ide.app.host.list()');
	check('Erweiterungen: alle aktiv', exts.every((e) => e.state === 'active'),
		exts.map((e) => `${e.id}:${e.state}${e.error ? '(' + e.error + ')' : ''}`).join(' '));

	// --- Menüleiste --------------------------------------------------------
	const titles = await js(win, `[...document.querySelectorAll('.menu-title')].map((b) => b.textContent)`);
	check('Menüleiste: Menüs mit Einträgen sichtbar, leere nicht', JSON.stringify(titles) === JSON.stringify(['Edit', 'Help']), titles.join(','));

	// Menü öffnen, Eintrag lesen
	await js(win, `document.querySelector('.menu-title[data-menu="edit"]').click()`);
	const item = await js(win, `(() => {
		const el = document.querySelector('.menu-item[data-command="workbench.toggleToolbars"]');
		return el && { label: el.querySelector('.menu-label').textContent, key: el.querySelector('.menu-key').textContent,
			check: el.querySelector('.menu-check').textContent, role: el.getAttribute('role') };
	})()`);
	check('Menü: Eintrag aus Erweiterung mit Text, Kürzel und Haken',
		Boolean(item) && item.label === 'Show Toolbars' && item.key === 'Shift+Escape' && item.check === '✓',
		JSON.stringify(item));

	// --- Befehl über den Menüpunkt -----------------------------------------
	const before = await js(win, `document.querySelector('.ide').dataset.toolbars === 'on' && !document.querySelector('.statusbar').hidden`);
	await js(win, `document.querySelector('.menu-item[data-command="workbench.toggleToolbars"]').click()`);
	await sleep(100);
	const afterMenu = await js(win, `document.querySelector('.ide').dataset.toolbars === 'off' && document.querySelector('.statusbar').hidden`);
	check('Menüpunkt löst Befehl aus (Leisten ausgeblendet)', before && afterMenu);
	check('Menü schließt nach dem Klick', await js(win, `document.querySelector('.menu-list') === null`));

	// --- Kürzel ---------------------------------------------------------------
	await js(win, `window.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', shiftKey: true, bubbles: true, cancelable: true }))`);
	await sleep(100);
	const afterKey = await js(win, `document.querySelector('.ide').dataset.toolbars === 'on' && !document.querySelector('.statusbar').hidden`);
	check('Kürzel Umschalt+Esc löst denselben Befehl aus (Leisten wieder da)', afterKey);

	// --- Einstellungen kommen auf die Platte ------------------------------------
	await js(win, `window.__ide.app.settings.set('workbench.showToolbars', false)`);
	await sleep(700); // Sicherung ist verzögert (300 ms)
	const file = path.join(dir, 'settings.json');
	let saved = null;
	try { saved = JSON.parse(fs.readFileSync(file, 'utf8')); } catch { /* bleibt null */ }
	check('Einstellungen: settings.json geschrieben',
		Boolean(saved) && saved.version === 1 && saved.settings['workbench.showToolbars'] === false,
		saved ? JSON.stringify(saved.settings) : 'keine Datei');

	// --- About-Dialog -------------------------------------------------------------
	await js(win, `window.__ide.app.settings.set('workbench.showToolbars', true)`);
	// nicht abwarten: der Befehl endet erst, wenn der Dialog geschlossen ist
	await js(win, `void window.__ide.app.run('help.about'); 0`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	const dlg = await js(win, `(() => { const d = document.querySelector('.dialog'); return d && { title: d.querySelector('.dialog-title').textContent, text: d.querySelector('.dialog-text').textContent }; })()`);
	check('About: Dialog mit Version und Compiler', Boolean(dlg) && /^BLTZNXT IDE v/.test(dlg.text) && /Compiler: /.test(dlg.text), dlg ? dlg.text.replace(/\n/g, ' | ') : 'kein Dialog');
	check('Dialog: Kontext dialog.open gesetzt', await js(win, `window.__ide.app.context.get('dialog.open') === true`));

	// Kürzel ruhen, solange ein Dialog offen ist
	await js(win, `window.dispatchEvent(new KeyboardEvent('keydown', { key: 'F', shiftKey: true, bubbles: true }))`);
	// Escape schließt (Standardknopf/Abbruch)
	await js(win, `document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', bubbles: true, cancelable: true }))`);
	await sleep(100);
	check('Dialog: Escape schließt', await js(win, `document.querySelector('.dialog') === null && window.__ide.app.context.get('dialog.open') === false`));

	// --- Editor und Tab-Leiste -------------------------------------------------------
	check('Tab-Leiste: ein namenloses Dokument', await js(win, `document.querySelectorAll('.tab').length === 1 && document.querySelector('.tab-title').textContent === '<untitled>'`));
	check('Editor: Monaco gemountet', await waitFor(win, `document.querySelector('.view[data-view="editor"] .monaco-editor') !== null`, 5000));
	const text = await js(win, `window.__ide.app.services.get('editor').monaco.editor.getModel()?.getValue()`);
	check('Editor: zeigt den Text des Dokuments', typeof text === 'string' && text.includes('Graphics 800, 600'));

	// Eingabe im Editor -> Dokument geändert -> Tab zeigt es
	await js(win, `window.__ide.app.services.get('editor').monaco.editor.getModel().setValue('Print 1\\n')`);
	await sleep(100);
	check('Eingabe im Editor erreicht das Dokument (dirty, Tab markiert)',
		await js(win, `window.__ide.app.documents.active.text === 'Print 1\\n' && window.__ide.app.context.get('document.dirty') === true && document.querySelector('.tab').classList.contains('dirty')`));

	// Änderung am Dokument von außen erreicht den Editor
	await js(win, `window.__ide.app.documents.setText(window.__ide.app.documents.active.id, 'Print 2\\r\\n')`);
	await sleep(100);
	check('Änderung am Dokument erreicht den Editor (LF-normalisiert)',
		await js(win, `window.__ide.app.services.get('editor').monaco.editor.getModel().getValue() === 'Print 2\\n'`));

	// --- Statuszeile / Compiler -----------------------------------------------------------
	const status = await js(win, `document.querySelector('[data-status="toolchain"]')?.textContent ?? null`);
	check('Statuszeile: Compiler-Eintrag', typeof status === 'string' && /blitzcc/.test(status), String(status));

	// Sprache: Befehlsliste kommt an (nur wenn ein Compiler gefunden wurde)
	const info = await js(win, `window.__ide.app.services.get('toolchain').info`);
	if (info && info.available) {
		const ok = await waitFor(win, `window.__ide.app.services.get('toolchain').listCommands().then((l) => l.length > 100)`, 10000);
		check('Toolchain: Befehlsliste über die Brücke', ok);
	} else {
		console.log('SKIP  Toolchain: kein Compiler gefunden');
	}

	// --- Beitrag entfernen -> Menü und Kürzel verschwinden --------------------------------------
	await js(win, `window.__ide.app.host.deactivateAll()`);
	await sleep(100);
	check('Abschalten der Erweiterungen räumt Menüleiste und Ansichten weg',
		await js(win, `document.querySelectorAll('.menu-title').length === 0 && document.querySelector('.monaco-editor') === null`));

	check('Keine Fehler auf der Konsole der Seite', errors.length === 0, errors.slice(0, 3).join(' || '));
	win.destroy();
	fs.rmSync(dir, { recursive: true, force: true });
}

async function scenarioCorruptSettings() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	fs.writeFileSync(path.join(dir, 'settings.json'), '{ das ist kein json', 'utf8');
	const { win, errors, started } = await openWindow(dir);
	check('Kaputte Einstellungen: IDE startet trotzdem', started);
	if (!started) return win.destroy();

	const shown = await waitFor(win, `document.querySelector('.dialog') !== null`, 5000);
	check('Kaputte Einstellungen: Dialog erscheint', shown);
	const bad = path.join(dir, 'settings.json.bad');
	check('Kaputte Einstellungen: Original als settings.json.bad gesichert',
		fs.existsSync(bad) && fs.readFileSync(bad, 'utf8') === '{ das ist kein json');
	check('Kaputte Einstellungen: es gelten die Vorgaben', await js(win, `window.__ide.app.settings.get('editor.tabSize') === 4`));
	check('Kaputte Einstellungen: keine Fehler auf der Konsole', errors.length === 0, errors.slice(0, 3).join(' || '));
	win.destroy();
	fs.rmSync(dir, { recursive: true, force: true });
}

// Ohne diesen Hörer beendet Electron die App, sobald das erste Testfenster zugeht.
app.on('window-all-closed', () => {});

app.whenReady().then(async () => {
	if (!fs.existsSync(DIST)) {
		console.log('FAIL  dist/index.html fehlt — erst "npm run build" (oder "npm run test:ui")');
		app.exit(1);
		return;
	}
	bridge.register(ipcMain);
	try {
		await scenarioNormal();
		await scenarioCorruptSettings();
	} catch (err) {
		console.log('FAIL  Testlauf abgebrochen:', err && err.stack ? err.stack : err);
		failures++;
	}
	console.log(`\n${failures === 0 ? 'Alle Prüfungen bestanden.' : failures + ' fehlgeschlagen.'}`);
	app.exit(failures === 0 ? 0 : 1);
});
