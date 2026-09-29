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
const dialogService = require('../electron/services/dialog');
const hostService = require('../electron/services/host');
const windowService = require('../electron/services/window');

// Die Datei-Dialoge des Systems würden blockieren: der Test beantwortet sie selbst.
const dialogStub = { openAnswers: [], saveAnswers: [] };
dialogService.setImpl({
	open: async () => dialogStub.openAnswers.shift() ?? null,
	save: async () => dialogStub.saveAnswers.shift() ?? null
});

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
	// wie electron-main.js: Schließen nur mit Zustimmung des Renderers
	windowService.installCloseGuard(win, bridge.send);
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

const key = (win, spec) =>
	js(win, `window.dispatchEvent(new KeyboardEvent('keydown', Object.assign({ bubbles: true, cancelable: true }, ${JSON.stringify(spec)})))`);
const clickDialog = (win, id) => js(win, `document.querySelector('.dialog-button[data-id="${id}"]').click()`);
const dialogOpen = (win) => js(win, `document.querySelector('.dialog') !== null`);
const setEditorText = (win, text) =>
	js(win, `window.__ide.app.services.get('editor').monaco.editor.getModel().setValue(${JSON.stringify(text)})`);

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
	check('Menüleiste: Menüs mit Einträgen sichtbar, leere nicht',
		JSON.stringify(titles) === JSON.stringify(['File', 'Edit', 'Help']), titles.join(','));

	// Datei-Menü: Reihenfolge und Kürzel des Originals
	await js(win, `document.querySelector('.menu-title[data-menu="file"]').click()`);
	const fileMenu = await js(win, `[...document.querySelectorAll('.menu-dropdown > .menu-item, .menu-dropdown > .menu-separator')].map((el) =>
		el.classList.contains('menu-separator') ? '-' : el.querySelector('.menu-label').textContent + (el.querySelector('.menu-key').textContent ? ' [' + el.querySelector('.menu-key').textContent + ']' : '') + (el.classList.contains('disabled') ? ' (aus)' : ''))`);
	check('Datei-Menü wie im Original angeordnet, gesperrt was nichts tun kann',
		fileMenu.join('|') === 'New [Ctrl+N]|Open... [Ctrl+O]|-|Close [Ctrl+F4] (aus)|Close All (aus)|-|Save [Ctrl+S] (aus)|Save As... (aus)|Save All (aus)|-|Next File [Ctrl+Tab] (aus)|Previous File [Ctrl+Shift+Tab] (aus)|-|Exit',
		fileMenu.join('|'));
	await js(win, `document.body.click()`);

	// Edit-Menü und Kürzel aus P0
	await js(win, `document.querySelector('.menu-title[data-menu="edit"]').click()`);
	const item = await js(win, `(() => {
		const el = document.querySelector('.menu-item[data-command="workbench.toggleToolbars"]');
		return el && { label: el.querySelector('.menu-label').textContent, key: el.querySelector('.menu-key').textContent,
			check: el.querySelector('.menu-check').textContent, role: el.getAttribute('role') };
	})()`);
	check('Menü: Eintrag aus Erweiterung mit Text, Kürzel und Haken',
		Boolean(item) && item.label === 'Show Toolbars' && item.key === 'Shift+Escape' && item.check === '✓',
		JSON.stringify(item));
	const before = await js(win, `document.querySelector('.ide').dataset.toolbars === 'on' && !document.querySelector('.statusbar').hidden`);
	await js(win, `document.querySelector('.menu-item[data-command="workbench.toggleToolbars"]').click()`);
	await sleep(100);
	check('Menüpunkt löst Befehl aus (Leisten ausgeblendet)',
		before && await js(win, `document.querySelector('.ide').dataset.toolbars === 'off' && document.querySelector('.statusbar').hidden`));
	check('Menü schließt nach dem Klick', await js(win, `document.querySelector('.menu-list') === null`));
	await key(win, { key: 'Escape', shiftKey: true });
	await sleep(100);
	check('Kürzel Umschalt+Esc löst denselben Befehl aus (Leisten wieder da)',
		await js(win, `document.querySelector('.ide').dataset.toolbars === 'on' && !document.querySelector('.statusbar').hidden`));

	// --- Einstellungen kommen auf die Platte ------------------------------------
	await js(win, `window.__ide.app.settings.set('workbench.showToolbars', false)`);
	await sleep(700); // Sicherung ist verzögert (300 ms)
	let saved = null;
	try { saved = JSON.parse(fs.readFileSync(path.join(dir, 'settings.json'), 'utf8')); } catch { /* bleibt null */ }
	check('Einstellungen: settings.json geschrieben',
		Boolean(saved) && saved.version === 1 && saved.settings['workbench.showToolbars'] === false,
		saved ? JSON.stringify(saved.settings) : 'keine Datei');
	await js(win, `window.__ide.app.settings.set('workbench.showToolbars', true)`);

	// --- About-Dialog -------------------------------------------------------------
	// nicht abwarten: der Befehl endet erst, wenn der Dialog geschlossen ist
	await js(win, `void window.__ide.app.run('help.about'); 0`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	const dlg = await js(win, `(() => { const d = document.querySelector('.dialog'); return d && { text: d.querySelector('.dialog-text').textContent }; })()`);
	check('About: Dialog mit Version und Compiler', Boolean(dlg) && /^BLTZNXT IDE v/.test(dlg.text) && /Compiler: /.test(dlg.text), dlg ? dlg.text.replace(/\n/g, ' | ') : 'kein Dialog');
	check('Dialog: Kontext dialog.open gesetzt', await js(win, `window.__ide.app.context.get('dialog.open') === true`));
	await key(win, { key: 'n', ctrlKey: true }); // Kürzel ruhen, solange ein Dialog offen ist
	await js(win, `document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', bubbles: true, cancelable: true }))`);
	await sleep(100);
	check('Dialog: Escape schließt, Kürzel hatten nichts ausgelöst',
		await js(win, `document.querySelector('.dialog') === null && window.__ide.app.context.get('dialog.open') === false && window.__ide.app.documents.list().length === 0`));

	// --- Ohne Datei: Willkommensansicht -------------------------------------------------
	check('Ohne offene Datei: keine Tab-Leiste, Willkommensansicht',
		await js(win, `document.querySelector('.tabs').hidden && !document.querySelector('.welcome').hidden && /Ctrl\\+N/.test(document.querySelector('.welcome-hint').textContent)`));
	check('Symbolleiste: Neu, Öffnen, Speichern, Schließen (die zwei letzten gesperrt)',
		await js(win, `[...document.querySelectorAll('.toolbar-button')].map((b) => b.dataset.command + (b.disabled ? ':aus' : '')).join()`)
			=== 'file.new,file.open,file.save:aus,file.close:aus');

	// --- Neu (Ctrl+N), Editor, Tab-Leiste -----------------------------------------------------
	await key(win, { key: 'n', ctrlKey: true });
	await waitFor(win, `document.querySelectorAll('.tab').length === 1`, 3000);
	check('Ctrl+N: namenloses Dokument, Tab, Editor sichtbar',
		await js(win, `document.querySelector('.tab-title').textContent === '<untitled>' && document.querySelector('.welcome').hidden`));
	check('Editor: Monaco gemountet', await waitFor(win, `document.querySelector('.view[data-view="editor"] .monaco-editor') !== null`, 5000));
	check('Symbolleiste: Speichern und Schließen jetzt frei',
		await js(win, `[...document.querySelectorAll('.toolbar-button')].every((b) => !b.disabled)`));

	// Eingabe im Editor -> Dokument -> Tab
	await setEditorText(win, 'Print "Größe → €"\n');
	await sleep(100);
	check('Eingabe im Editor erreicht das Dokument (dirty, Tab markiert, Titel)',
		await js(win, `window.__ide.app.documents.active.dirty && document.querySelector('.tab').classList.contains('dirty') && document.title.startsWith('* ')`));

	// --- Speichern: "→" kann Windows-1252 nicht -> Rückfrage; Abbrechen, dann UTF-8 --------------
	const target = path.join(dir, 'spiel.bb');
	dialogStub.saveAnswers.push(target, target);
	await js(win, `void window.__ide.app.run('file.save'); 0`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	const warn = await js(win, `document.querySelector('.dialog-text').textContent`);
	check('Speichern: nicht darstellbares Zeichen wird gemeldet', /→/.test(warn), warn.replace(/\n/g, ' | '));
	await clickDialog(win, 'cancel');
	await sleep(150);
	check('Abbrechen schreibt nichts', !fs.existsSync(target));

	// Text ohne "→": als Windows-1252 mit CRLF, Umlaut ein Byte
	await setEditorText(win, 'Print "Größe €"\n');
	await js(win, `void window.__ide.app.run('file.save'); 0`);
	await waitFor(win, `window.__ide.app.documents.active.uri !== null`, 3000);
	const onDisk = [...fs.readFileSync(target)];
	check('Speichern: Windows-1252 und CRLF, Umlaut und Euro als einzelne Bytes',
		JSON.stringify(onDisk) === JSON.stringify([...Buffer.from([0x50,0x72,0x69,0x6e,0x74,0x20,0x22,0x47,0x72,0xf6,0xdf,0x65,0x20,0x80,0x22,0x0d,0x0a])]),
		Buffer.from(onDisk).toString('hex'));
	check('Nach dem Speichern: Tab heißt spiel.bb, nicht mehr geändert',
		await js(win, `document.querySelector('.tab-title').textContent === 'spiel.bb' && !document.querySelector('.tab').classList.contains('dirty')`));

	// zweites Speichern: Sicherungskopie
	await setEditorText(win, 'Print 2\n');
	await js(win, `void window.__ide.app.run('file.save'); 0`);
	await waitFor(win, `!window.__ide.app.documents.active.dirty`, 3000);
	check('Speichern legt datei.bb_bak1 mit dem alten Inhalt an',
		fs.existsSync(`${target}_bak1`) && fs.readFileSync(`${target}_bak1`).length === 17);

	// --- Öffnen -----------------------------------------------------------------------------
	const other = path.join(dir, 'umlaut.bb');
	fs.writeFileSync(other, Buffer.from([0x3b, 0x20, 0xe4, 0xf6, 0xfc, 0x0d, 0x0a, 0x50, 0x72, 0x69, 0x6e, 0x74, 0x20, 0x31, 0x0d, 0x0a]));
	dialogStub.openAnswers.push([other]);
	await key(win, { key: 'o', ctrlKey: true });
	await waitFor(win, `document.querySelectorAll('.tab').length === 2`, 3000);
	check('Ctrl+O: Datei geöffnet, ANSI-Umlaute gelesen',
		await js(win, `window.__ide.app.documents.active.text === '; äöü\\nPrint 1\\n' && window.__ide.app.documents.active.encoding === 'windows-1252'`));
	check('Editor zeigt die geöffnete Datei',
		await js(win, `window.__ide.app.services.get('editor').monaco.editor.getModel().getValue(1) === '; äöü\\nPrint 1\\n'`));

	// dieselbe Datei nochmal: kein zweiter Tab
	dialogStub.openAnswers.push([other.toUpperCase().replace('.BB', '.bb')]);
	await key(win, { key: 'o', ctrlKey: true });
	await sleep(300);
	check('Dieselbe Datei (andere Schreibweise) öffnet keinen zweiten Tab', await js(win, `document.querySelectorAll('.tab').length === 2`));

	// Zuletzt geöffnet: Untermenü
	await js(win, `document.querySelector('.menu-title[data-menu="file"]').click()`);
	const recent = await js(win, `[...document.querySelectorAll('.menu-sub .menu-label')].map((e) => e.textContent)`);
	check('Datei > Recent Files: neueste zuerst', recent.length === 2 && recent[0] === other && recent[1] === target, recent.join(' | '));
	await js(win, `document.body.click()`);

	// Tabs wechseln (Strg+Tab)
	await key(win, { key: 'Tab', ctrlKey: true });
	await sleep(100);
	check('Ctrl+Tab wechselt zum nächsten Tab', await js(win, `window.__ide.app.documents.active.title === 'spiel.bb'`));

	// --- Zustand kommt auf die Platte --------------------------------------------------------------
	await sleep(600);
	let state = null;
	try { state = JSON.parse(fs.readFileSync(path.join(dir, 'state.json'), 'utf8')); } catch { /* null */ }
	check('Zustand: state.json mit zuletzt geöffneten Dateien',
		Boolean(state) && Array.isArray(state.state['files.recent']) && state.state['files.recent'][0] === other);

	// --- Schließen mit Rückfrage -----------------------------------------------------------------------
	await setEditorText(win, 'Print 3\n');
	await key(win, { key: 'F4', ctrlKey: true });
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	check('Ctrl+F4 mit Änderungen: Rückfrage Ja/Nein/Abbrechen',
		await js(win, `[...document.querySelectorAll('.dialog-button')].map((b) => b.dataset.id).join()`) === 'yes,no,cancel');
	await clickDialog(win, 'cancel');
	await sleep(100);
	check('Abbrechen: Tab bleibt', await js(win, `document.querySelectorAll('.tab').length === 2`));

	// Fenster mit dem Kreuz schließen: gleiche Rückfrage, Abbrechen hält das Fenster
	win.close();
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	check('Fenster schließen mit Änderungen: Rückfrage erscheint, Fenster bleibt', !win.isDestroyed() && await dialogOpen(win));
	await clickDialog(win, 'cancel');
	await sleep(150);
	check('Abbrechen: Fenster bleibt offen, der unveränderte Tab ist zu (von hinten nach vorn)',
		!win.isDestroyed() && await js(win, `document.querySelectorAll('.tab').length === 1`));

	// Schließen-Knopf im Tab, dann "Nein" verwirft
	await js(win, `document.querySelector('.tab.active .tab-close').click()`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 3000);
	await clickDialog(win, 'no');
	await sleep(150);
	check('Nein verwirft: Tab weg, Datei unverändert',
		await js(win, `document.querySelectorAll('.tab').length === 0`) && fs.readFileSync(target, 'utf8').startsWith('Print 2'));

	// Fenster schließen ohne Änderungen: schließt ohne Rückfrage
	const closed = new Promise((resolve) => win.once('closed', () => resolve(true)));
	win.close();
	const closedInTime = await Promise.race([closed, sleep(4000).then(() => false)]);
	check('Fenster schließen ohne Änderungen: schließt', closedInTime);

	check('Keine Fehler auf der Konsole der Seite', errors.length === 0, errors.slice(0, 3).join(' || '));
	fs.rmSync(dir, { recursive: true, force: true });
}

async function scenarioLaunchFilesAndAbschalten() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	const start = path.join(dir, 'start.bb');
	fs.writeFileSync(start, 'Print "Start"\r\n');
	hostService.setLaunchArgs(['electron', 'app', start], false, dir);
	const { win, errors, started } = await openWindow(dir);
	check('Start mit Dateiname: IDE geladen', started);
	if (!started) return win.destroy();
	check('Start mit Dateiname: Datei ist geöffnet',
		await waitFor(win, `window.__ide.app.documents.list().length === 1 && window.__ide.app.documents.active.uri === ${JSON.stringify(start)}`, 5000));
	check('Start mit Dateiname: Editor zeigt sie',
		await waitFor(win, `window.__ide.app.services.get('editor').monaco.editor.getModel()?.getValue(1) === 'Print "Start"\\n'`, 5000));

	// Status: Compiler-Eintrag und Befehlsliste
	const status = await js(win, `document.querySelector('[data-status="toolchain"]')?.textContent ?? null`);
	check('Statuszeile: Compiler-Eintrag', typeof status === 'string' && /blitzcc/.test(status), String(status));
	const info = await js(win, `window.__ide.app.services.get('toolchain').info`);
	if (info && info.available) {
		check('Toolchain: Befehlsliste über die Brücke',
			await waitFor(win, `window.__ide.app.services.get('toolchain').listCommands().then((l) => l.length > 100)`, 10000));
	} else {
		console.log('SKIP  Toolchain: kein Compiler gefunden');
	}

	// Abschalten der Erweiterungen räumt alles weg
	await js(win, `window.__ide.app.host.deactivateAll()`);
	await sleep(100);
	check('Abschalten der Erweiterungen räumt Menüleiste und Ansichten weg',
		await js(win, `document.querySelectorAll('.menu-title').length === 0 && document.querySelector('.monaco-editor') === null`));

	check('Keine Fehler auf der Konsole der Seite', errors.length === 0, errors.slice(0, 3).join(' || '));
	win.destroy();
	hostService.setLaunchArgs(['electron', 'app'], false, dir);
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
		await scenarioLaunchFilesAndAbschalten();
		await scenarioCorruptSettings();
	} catch (err) {
		console.log('FAIL  Testlauf abgebrochen:', err && err.stack ? err.stack : err);
		failures++;
	}
	console.log(`\n${failures === 0 ? 'Alle Prüfungen bestanden.' : failures + ' fehlgeschlagen.'}`);
	app.exit(failures === 0 ? 0 : 1);
});
