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

const { app, BrowserWindow, ipcMain, clipboard } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');

const bridge = require('../electron/bridge');
const store = require('../electron/services/store');
const dialogService = require('../electron/services/dialog');
const hostService = require('../electron/services/host');
const windowService = require('../electron/services/window');

// Die Datei-Dialoge des Systems würden blockieren: der Test beantwortet sie selbst.
const dialogStub = { openAnswers: [], saveAnswers: [], folderAnswers: [] };
dialogService.setImpl({
	open: async () => dialogStub.openAnswers.shift() ?? null,
	save: async () => dialogStub.saveAnswers.shift() ?? null,
	folder: async () => dialogStub.folderAnswers.shift() ?? null
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
			webSecurity: false,
			// Ein verstecktes Fenster würde sonst nicht mehr neu zeichnen (requestAnimationFrame ruht)
			backgroundThrottling: false
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
		JSON.stringify(titles) === JSON.stringify(['File', 'Edit', 'Program', 'Help']), titles.join(','));

	// Datei-Menü: Reihenfolge und Kürzel des Originals
	await js(win, `document.querySelector('.menu-title[data-menu="file"]').click()`);
	const fileMenu = await js(win, `[...document.querySelectorAll('.menu-dropdown > .menu-item, .menu-dropdown > .menu-separator')].map((el) =>
		el.classList.contains('menu-separator') ? '-' : el.querySelector('.menu-label').textContent + (el.querySelector('.menu-key').textContent ? ' [' + el.querySelector('.menu-key').textContent + ']' : '') + (el.classList.contains('disabled') ? ' (aus)' : ''))`);
	check('Datei-Menü wie im Original angeordnet, gesperrt was nichts tun kann',
		fileMenu.join('|') === 'New [Ctrl+N]|Open... [Ctrl+O]|Open Folder...|-|Close [Ctrl+F4] (aus)|Close All (aus)|-|Save [Ctrl+S] (aus)|Save As... (aus)|Save All (aus)|-|Next File [Ctrl+Tab] (aus)|Previous File [Ctrl+Shift+Tab] (aus)|-|Exit',
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
	check('Symbolleiste: Neu, Öffnen, Speichern, Schließen | Ausschneiden, Kopieren, Einfügen | Suchen | Starten — ohne Datei gesperrt',
		await js(win, `[...document.querySelectorAll('.toolbar-button')].map((b) => b.dataset.command + (b.disabled ? ':aus' : '')).join()`)
			=== 'file.new,file.open,file.save:aus,file.close:aus,edit.cut:aus,edit.copy:aus,edit.paste:aus,edit.find:aus,program.run:aus');

	// --- Neu (Ctrl+N), Editor, Tab-Leiste -----------------------------------------------------
	await key(win, { key: 'n', ctrlKey: true });
	await waitFor(win, `document.querySelectorAll('.tab').length === 1`, 3000);
	check('Ctrl+N: namenloses Dokument, Tab, Editor sichtbar',
		await js(win, `document.querySelector('.tab-title').textContent === '<untitled>' && document.querySelector('.welcome').hidden`));
	check('Editor: Monaco gemountet', await waitFor(win, `document.querySelector('.view[data-view="editor"] .monaco-editor') !== null`, 5000));
	check('Symbolleiste: mit Datei frei, nur Ausschneiden/Kopieren wollen eine Auswahl',
		await js(win, `[...document.querySelectorAll('.toolbar-button')].slice(0, 8).map((b) => b.dataset.command + (b.disabled ? ':aus' : '')).join()`)
			=== 'file.new,file.open,file.save,file.close,edit.cut:aus,edit.copy:aus,edit.paste,edit.find');

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

const edApi = `window.__ide.app.services.get('editor').monaco`;
// Quelle 'keyboard': nur dann behandelt Monaco das Tippen wie echtes Tippen (Einrückung bei Enter u.a.)
const typeText = (win, text) => js(win, `${edApi}.editor.trigger('keyboard', 'type', { text: ${JSON.stringify(text)} })`);
const editorText = (win) => js(win, `${edApi}.editor.getModel().getValue(1)`);
const setSelection = (win, l1, c1, l2, c2) => js(win, `${edApi}.editor.setSelection({ startLineNumber: ${l1}, startColumn: ${c1}, endLineNumber: ${l2}, endColumn: ${c2} })`);
const cssColor = (hex) => {
	const n = parseInt(hex.slice(1), 16);
	return `rgb(${n >> 16}, ${(n >> 8) & 255}, ${n & 255})`;
};

async function scenarioEditor() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	// Eine Datei mit kleingeschriebenen Schlüsselwörtern: sie darf sich durch bloßes Öffnen und Durchklicken nicht ändern
	const lower = path.join(dir, 'klein.bb');
	fs.writeFileSync(lower, 'graphics 800,600\r\nprint "hallo"\r\n');
	hostService.setLaunchArgs(['electron', 'app', lower], false, dir);
	const { win, errors, started } = await openWindow(dir);
	check('Editor: IDE gestartet', started);
	if (!started) return win.destroy();
	await waitFor(win, `window.__ide.app.documents.active !== null && ${edApi} !== null && ${edApi}.editor.getModel() !== null`, 8000);
	// Befehlsliste vom Compiler abwarten (Schlüsselwörter für Färbung und Schreibweise)
	const info = await js(win, `window.__ide.app.services.get('toolchain').info`);
	const haveCompiler = Boolean(info && info.available);
	if (haveCompiler) await waitFor(win, `window.__ide.app.services.get('toolchain').symbols().then((s) => s.keywords.length > 10)`, 10000);
	await sleep(500);

	// --- Theme "Blitz3D Classic": die sieben Farben der Original-IDE ----------------------
	const classic = { background: '#225588', keyword: '#aaffff', comment: '#ffee00', string: '#00ff66', number: '#33ffdd', identifier: '#ffffff' };
	check('Theme: Vorgabe ist Blitz3D Classic, Hintergrund #225588',
		await js(win, `getComputedStyle(document.querySelector('.monaco-editor-background')).backgroundColor`) === cssColor(classic.background));

	// Färben: Text mit allen Sorten setzen und die Farben aus dem DOM lesen
	await js(win, `${edApi}.editor.getModel().setValue('Graphics 800\\nMeinWert = 5 ; Kommentar\\nPrint "text"\\n')`);
	await waitFor(win, `document.querySelectorAll('.view-line span span').length >= 8`, 4000);
	await sleep(200);
	const colors = await js(win, `(() => {
		const out = {};
		for (const span of document.querySelectorAll('.view-line span span')) {
			// Monaco setzt geschützte Leerzeichen (U+00A0) in die Spans
			const t = span.textContent.split(String.fromCharCode(160)).join(' ').trim();
			if (t) out[t] = getComputedStyle(span).color;
		}
		return out;
	})()`);
	if (haveCompiler) {
		check('Färbung: Befehle und Schlüsselwörter in der Schlüsselwortfarbe (aaffff)',
			colors['Graphics'] === cssColor(classic.keyword) && colors['Print'] === cssColor(classic.keyword), JSON.stringify(colors));
	}
	check('Färbung: Bezeichner weiß, Zahl, Zeichenkette und Kommentar in ihren Farben',
		colors['MeinWert'] === cssColor(classic.identifier) && colors['5'] === cssColor(classic.number) &&
		colors['"text"'] === cssColor(classic.string) && colors['; Kommentar'] === cssColor(classic.comment), JSON.stringify(colors));

	// --- Theme wechseln ---------------------------------------------------------------------
	await js(win, `window.__ide.app.settings.set('workbench.theme', 'light')`);
	await sleep(300);
	check('Theme "Light": Editor und Oberfläche wechseln',
		await js(win, `getComputedStyle(document.querySelector('.monaco-editor-background')).backgroundColor`) === 'rgb(255, 255, 255)' &&
		await js(win, `getComputedStyle(document.documentElement).getPropertyValue('--bg-alt').trim()`) === '#f3f3f3');
	await js(win, `window.__ide.app.settings.set('workbench.theme', 'blitz-classic')`);
	await sleep(300);
	check('Theme zurück: Oberflächenfarben wieder die Vorgabe',
		await js(win, `getComputedStyle(document.documentElement).getPropertyValue('--bg-alt').trim()`) === '#252526');

	// --- Geöffnete Datei bleibt, wie sie ist ------------------------------------------------------
	await js(win, `window.__ide.app.documents.close(window.__ide.app.documents.active.id)`);
	await js(win, `void window.__ide.app.services.get('files').openPath(${JSON.stringify(lower)}); 0`);
	await waitFor(win, `window.__ide.app.documents.active !== null && ${edApi}.editor.getModel() !== null`, 5000);
	await js(win, `${edApi}.editor.setPosition({ lineNumber: 2, column: 3 })`);
	await js(win, `${edApi}.editor.setPosition({ lineNumber: 1, column: 12 })`);
	await sleep(200);
	check('Nur durchklicken ändert eine geöffnete Datei nicht (Schreibweise bleibt, Datei nicht "geändert")',
		await editorText(win) === 'graphics 800,600\nprint "hallo"\n' && await js(win, `window.__ide.app.documents.active.dirty === false`));

	// --- Schreibweise beim Tippen ----------------------------------------------------------------------
	if (haveCompiler) {
		await js(win, `${edApi}.editor.getModel().setValue('')`);
		await js(win, `window.__ide.app.documents.active && 0`);
		await typeText(win, 'graphics');
		check('Schreibweise: das Wort am Cursor bleibt, solange man tippt', await editorText(win) === 'graphics');
		await typeText(win, ' ');
		check('Schreibweise: sobald der Cursor weiterrückt, wird es "Graphics"', await editorText(win) === 'Graphics ');
		await typeText(win, '800 ; print');
		check('Schreibweise: Kommentar bleibt unberührt', await editorText(win) === 'Graphics 800 ; print');

		await js(win, `${edApi}.editor.getModel().setValue('')`);
		await typeText(win, 'if x then print "if"\nendif ');
		check('Schreibweise: mehrere Wörter, Zeichenkette bleibt, Zeile davor wird abgeschlossen',
			await editorText(win) === 'If x Then Print "if"\nEndIf ', await editorText(win));

		// Rückgängig darf die Korrektur nicht endlos neu anwenden
		await js(win, `${edApi}.editor.getModel().setValue('')`);
		await typeText(win, 'print ');
		const afterType = await editorText(win);
		await js(win, `${edApi}.editor.trigger('test', 'undo', null)`);
		await sleep(150);
		const afterUndo = await editorText(win);
		check('Rückgängig: kein Endlos-Korrigieren (Text ist entweder leer oder klein, nicht wieder "Print ")',
			afterType === 'Print ' && afterUndo !== 'Print ', JSON.stringify({ afterType, afterUndo }));
	} else {
		console.log('SKIP  Schreibweise: kein Compiler gefunden');
	}

	// --- Einrücken -----------------------------------------------------------------------------------------
	await js(win, `${edApi}.editor.getModel().setValue('a\\nb\\nc\\n')`);
	await setSelection(win, 1, 1, 3, 2);
	await js(win, `${edApi}.editor.trigger('test', 'tab', null)`);
	check('Tab mit markierten Zeilen rückt ein (mit Tabulator)', await editorText(win) === '\ta\n\tb\n\tc\n');
	await js(win, `${edApi}.editor.trigger('test', 'outdent', null)`);
	check('Umschalt+Tab rückt aus', await editorText(win) === 'a\nb\nc\n');
	await js(win, `${edApi}.editor.getModel().setValue('\\t\\tx')`);
	await js(win, `${edApi}.editor.setPosition({ lineNumber: 1, column: 4 })`);
	await typeText(win, '\n');
	check('Enter übernimmt die Einrückung der Zeile', await editorText(win) === '\t\tx\n\t\t');

	// --- Statuszeile ------------------------------------------------------------------------------------------
	await js(win, `${edApi}.editor.getModel().setValue('eins\\nzwei drei')`);
	await js(win, `${edApi}.editor.setPosition({ lineNumber: 2, column: 6 })`);
	await sleep(150);
	const pos = await js(win, `document.querySelector('[data-status="editor.position"]')?.textContent ?? null`);
	check('Statuszeile: "Row:2 Col:6 *" (Stern = geändert)', pos === 'Row:2 Col:6 *', String(pos));

	// --- Bearbeiten-Menü ------------------------------------------------------------------------------------------
	await js(win, `document.querySelector('.menu-title[data-menu="edit"]').click()`);
	const editMenu = await js(win, `[...document.querySelectorAll('.menu-dropdown > .menu-item, .menu-dropdown > .menu-separator')].map((el) =>
		el.classList.contains('menu-separator') ? '-' : el.querySelector('.menu-label').textContent + (el.querySelector('.menu-key').textContent ? ' [' + el.querySelector('.menu-key').textContent + ']' : '') + (el.classList.contains('disabled') ? ' (aus)' : ''))`);
	check('Bearbeiten-Menü: Anordnung des Originals (Zwischenablage, Auswahl, Suchen, Leisten)',
		editMenu.join('|') === 'Undo [Ctrl+Z]|Redo [Ctrl+Y]|-|Cut [Ctrl+X] (aus)|Copy [Ctrl+C] (aus)|Paste [Ctrl+V]|-|Select All [Ctrl+A]|-|Find... [Ctrl+F]|Find Next [F3]|Replace... [Ctrl+R]|Find Previous [Shift+F3]|-|Show Toolbars [Shift+Escape]|Show Sidebar [Ctrl+B]',
		editMenu.join('|'));
	await js(win, `document.body.click()`);

	// --- Zwischenablage: Kopieren, Ausschneiden, Einfügen über die Befehle ----------------------------------------------
	await js(win, `${edApi}.editor.getModel().setValue('abc def')`);
	await setSelection(win, 1, 1, 1, 4);
	await sleep(100);
	check('Kopieren/Ausschneiden sind bei Auswahl frei, ohne gesperrt', await js(win, `window.__ide.app.commands.isEnabled('edit.copy') && window.__ide.app.commands.isEnabled('edit.cut')`));
	await js(win, `void window.__ide.app.run('edit.copy'); 0`);
	await sleep(200);
	check('Kopieren: Auswahl liegt in der Zwischenablage', clipboard.readText() === 'abc', clipboard.readText());
	await js(win, `void window.__ide.app.run('edit.cut'); 0`);
	await sleep(200);
	check('Ausschneiden: Text weg, in der Zwischenablage', await editorText(win) === ' def' && clipboard.readText() === 'abc');
	clipboard.writeText('XYZ');
	await js(win, `${edApi}.editor.setPosition({ lineNumber: 1, column: 5 })`);
	await js(win, `void window.__ide.app.run('edit.paste'); 0`);
	await sleep(300);
	check('Einfügen aus der Zwischenablage', await editorText(win) === ' defXYZ', await editorText(win));
	await js(win, `void window.__ide.app.run('edit.selectAll'); 0`);
	await sleep(100);
	check('Alles auswählen', await js(win, `${edApi}.editor.getSelection().equalsRange(${edApi}.editor.getModel().getFullModelRange())`));

	// --- Suchen -----------------------------------------------------------------------------------------------------------
	await js(win, `void window.__ide.app.run('edit.find'); 0`);
	await sleep(300);
	check('Suchen: Suchfeld erscheint', await js(win, `document.querySelector('.monaco-editor .find-widget.visible') !== null`));
	await key(win, { key: 'Escape' });
	await js(win, `void window.__ide.app.run('edit.replace'); 0`);
	await sleep(300);
	check('Ersetzen: Ersetzen-Feld erscheint', await js(win, `document.querySelector('.monaco-editor .find-widget.visible .replace-part') !== null && !document.querySelector('.monaco-editor .find-widget .replace-part').hidden`));
	await key(win, { key: 'Escape' });
	// Kürzel Ctrl+F über das Register der Shell, nicht über Monaco
	await js(win, `${edApi}.editor.focus()`);
	await key(win, { key: 'f', ctrlKey: true });
	await sleep(300);
	check('Ctrl+F öffnet das Suchfeld', await js(win, `document.querySelector('.monaco-editor .find-widget.visible') !== null`));
	await key(win, { key: 'Escape' });

	// --- Rechtsklickmenü = Bearbeiten-Menü ------------------------------------------------------------------------------------
	await js(win, `document.querySelector('.monaco-editor .view-lines').dispatchEvent(new MouseEvent('contextmenu', { bubbles: true, cancelable: true, clientX: 300, clientY: 200, button: 2 }))`);
	await sleep(200);
	const ctxItems = await js(win, `[...document.querySelectorAll('.menu-dropdown .menu-label')].map((e) => e.textContent).join()`);
	check('Rechtsklick im Editor zeigt das Bearbeiten-Menü', /Cut,Copy,Paste,Select All,Find/.test(ctxItems), ctxItems);
	await js(win, `document.body.click()`);

	// --- Gliederung ------------------------------------------------------------------------------------------------------------------
	const source = ['Type Player', '\tField x', 'End Type', '', '.start', 'Function Main()', 'End Function', 'Function Helper()', 'End Function', ''].join('\n');
	await js(win, `${edApi}.editor.getModel().setValue(${JSON.stringify(source)})`);
	await sleep(500);
	const tabsText = await js(win, `[...document.querySelectorAll('.outline-tab')].map((b) => b.textContent).join()`);
	check('Gliederung: drei Reiter mit Anzahl (funcs, types, labels)', tabsText === 'funcs (2),types (1),labels (1)', tabsText);
	check('Gliederung: unten im Panel sichtbar', await js(win, `!document.querySelector('.panel').hidden && document.querySelector('.panel-tab.active').textContent === 'Outline'`));
	const funcs = await js(win, `[...document.querySelectorAll('.outline-item')].map((e) => e.textContent + ':' + e.dataset.line).join()`);
	check('Gliederung: Funktionen mit Zeile', funcs === 'Main:6,Helper:8', funcs);
	await js(win, `document.querySelectorAll('.outline-item')[1].click()`);
	await sleep(150);
	check('Klick auf einen Eintrag setzt den Cursor auf die Zeile', await js(win, `${edApi}.editor.getPosition().lineNumber === 8`));
	await js(win, `document.querySelector('.outline-tab[data-tab="types"]').click()`);
	check('Reiter "types" zeigt die Typen', await js(win, `document.querySelector('.outline-item').textContent === 'Player'`));
	// die Gliederung folgt Änderungen
	await js(win, `${edApi}.editor.getModel().setValue('Function Neu()\\nEnd Function\\n')`);
	await sleep(500);
	check('Gliederung folgt dem Text', await js(win, `[...document.querySelectorAll('.outline-tab')].map((b) => b.textContent).join()`) === 'funcs (1),types (0),labels (0)');

	check('Keine Fehler auf der Konsole der Seite', errors.length === 0, errors.slice(0, 3).join(' || '));
	win.destroy();
	hostService.setLaunchArgs(['electron', 'app'], false, dir);
	fs.rmSync(dir, { recursive: true, force: true });
}

async function scenarioBuild() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	const good = path.join(dir, 'hallo.bb');
	const bad = path.join(dir, 'kaputt.bb');
	const slow = path.join(dir, 'lang.bb');
	fs.writeFileSync(good, '; beendet sich sofort\r\nEnd\r\n');
	fs.writeFileSync(bad, 'For i = 1 To 3\r\nPrint i\r\n');
	fs.writeFileSync(slow, '; wartet, bis man es stoppt\r\nDelay 60000\r\nEnd\r\n');
	hostService.setLaunchArgs(['electron', 'app', good], false, dir);
	const { win, errors, started } = await openWindow(dir);
	check('Bauen: IDE gestartet', started);
	if (!started) return win.destroy();
	await waitFor(win, `window.__ide.app.documents.active !== null && window.__ide.app.services.get('toolchain').info?.available === true`, 10000);
	const info = await js(win, `window.__ide.app.services.get('toolchain').info`);
	if (!info || !info.available) {
		console.log('SKIP  Bauen: kein Compiler gefunden');
		win.destroy();
		return;
	}
	const outputLines = () => js(win, `[...document.querySelectorAll('.output-line')].map((e) => e.textContent)`);
	const statusText = () => js(win, `document.querySelector('[data-status="build.status"]')?.textContent ?? null`);

	// --- Programm-Menü -----------------------------------------------------------------------
	await js(win, `document.querySelector('.menu-title[data-menu="program"]').click()`);
	const menu = await js(win, `[...document.querySelectorAll('.menu-dropdown > .menu-item, .menu-dropdown > .menu-separator')].map((el) =>
		el.classList.contains('menu-separator') ? '-' : el.querySelector('.menu-label').textContent + (el.querySelector('.menu-key').textContent ? ' [' + el.querySelector('.menu-key').textContent + ']' : '') + (el.classList.contains('disabled') ? ' (aus)' : '') + (el.querySelector('.menu-check').textContent ? ' ✓' : ''))`);
	check('Programm-Menü wie im Original (dazu Stop), Debug ist an',
		menu.join('|') === 'Run program [F5]|Run program again [F6] (aus)|Check for errors [F7]|Create Executable...|-|Stop program [Shift+F5] (aus)|-|Program Command Line...|Debug Enabled? ✓',
		menu.join('|'));
	await js(win, `document.body.click()`);
	check('Symbolleiste hat Ausschneiden, Kopieren, Einfügen, Suchen, Starten',
		await js(win, `[...document.querySelectorAll('.toolbar-button')].map((b) => b.dataset.command).join()`) ===
		'file.new,file.open,file.save,file.close,edit.cut,edit.copy,edit.paste,edit.find,program.run');

	// --- F5 auf einer benannten Datei ------------------------------------------------------------
	await js(win, `${edApi}.editor.focus()`);
	await key(win, { key: 'F5' });
	check('F5: der Ausgabe-Reiter füllt sich mit "Compiling..." und "Executing..."',
		await waitFor(win, `[...document.querySelectorAll('.output-line')].some((e) => e.textContent === 'Executing...')`, 60000));
	check('F5: Programm beendet, nichts mehr in Arbeit',
		await waitFor(win, `window.__ide.app.context.get('program.running') === false && window.__ide.app.context.get('build.compiling') === false`, 20000));
	const lines = await outputLines();
	check('Ausgabe: Building, die Phasen des Compilers, Built in ... s, Executing',
		lines[0] === 'Building hallo.bb' && lines.includes('Compiling C++...') && lines.some((l) => /^Built in \d+\.\d s\.$/.test(l)) && lines.at(-1) === 'Executing...',
		lines.join(' | '));
	check('Der Ausgabe-Reiter war beim Bauen vorn', await js(win, `document.querySelector('.panel-tab.active').textContent === 'Output'`));
	check('Auto-Scroll: die letzte Zeile ist ganz sichtbar (nicht unter dem Panelrand)',
		await js(win, `(() => {
			const last = document.querySelector('.output-list').lastElementChild.getBoundingClientRect();
			const panel = document.querySelector('.panel-body').getBoundingClientRect();
			return last.bottom <= panel.bottom + 1 && last.top >= panel.top;
		})()`));
	check('Neben der Quelle bleibt nichts liegen', !fs.existsSync(path.join(dir, 'hallo.exe')) && fs.readdirSync(dir).filter((n) => !n.endsWith('.json')).sort().join() === 'hallo.bb,kaputt.bb,lang.bb', fs.readdirSync(dir).sort().join());
	check('F6 ist jetzt frei (es gibt etwas Gebautes)', await js(win, `window.__ide.app.commands.isEnabled('program.rerun')`));

	// --- F7 auf einer Datei mit Fehler: Marker, Cursor, Dialog, Ausgabe ----------------------------------
	dialogStub.openAnswers.push([bad]);
	await key(win, { key: 'o', ctrlKey: true });
	await waitFor(win, `window.__ide.app.documents.active?.title === 'kaputt.bb'`, 5000);
	await js(win, `void window.__ide.app.run('program.check'); 0`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 60000);
	const errText = await js(win, `document.querySelector('.dialog-text').textContent`);
	check('Fehler: Meldungsdialog wie im Original', /NEXT/i.test(errText), errText);
	await clickDialog(win, 'ok');
	await sleep(200);
	const markers = await js(win, `${edApi}.api.editor.getModelMarkers({ owner: 'blitzcc' }).map((m) => [m.startLineNumber, m.startColumn, m.severity === 8])`);
	check('Fehler: roter Marker im Editor an der Stelle des Compilers', markers.length === 1 && markers[0][2] === true, JSON.stringify(markers));
	check('Fehler: Cursor steht an der Fehlerstelle',
		await js(win, `${edApi}.editor.getPosition().lineNumber === ${markers[0]?.[0] ?? 0}`));
	const errLines = await outputLines();
	check('Fehler: Zeile "datei:z:s: Meldung" im Ausgabe-Reiter, angeklickt springt sie',
		errLines.some((l) => /^kaputt\.bb:\d+:\d+: /.test(l)) &&
		await js(win, `document.querySelector('.panel-tab.active').textContent === 'Output' && document.querySelector('.output-line.link') !== null`), errLines.join(' | '));
	await waitFor(win, `window.__ide.app.context.get('build.compiling') === false`, 20000);

	// --- Fehler-Marker verschwinden beim nächsten Bau --------------------------------------------------------
	await js(win, `window.__ide.app.documents.activate(window.__ide.app.documents.find(${JSON.stringify(good)}).id)`);
	await js(win, `void window.__ide.app.run('program.check'); 0`);
	await waitFor(win, `window.__ide.app.context.get('build.compiling') === false && [...document.querySelectorAll('.output-line')].some((e) => e.textContent === 'No errors found.')`, 60000);
	check('Prüfen ohne Fehler: "No errors found.", Marker weg',
		await js(win, `${edApi}.api.editor.getModelMarkers({ owner: 'blitzcc' }).length === 0`));

	// --- Namenloser Tab: läuft aus dem Temp-Ordner, bleibt namenlos ----------------------------------------------------
	await key(win, { key: 'n', ctrlKey: true });
	await waitFor(win, `window.__ide.app.documents.active?.kind === 'scratch'`, 3000);
	await setEditorText(win, '; namenlos\nEnd\n');
	await key(win, { key: 'F5' });
	check('Namenloser Tab: F5 baut und startet ihn',
		await waitFor(win, `[...document.querySelectorAll('.output-line')].some((e) => e.textContent === 'Executing...')`, 60000));
	await waitFor(win, `window.__ide.app.context.get('program.running') === false`, 20000);
	check('Namenloser Tab: bleibt namenlos und ungespeichert', await js(win, `window.__ide.app.documents.active.uri === null && window.__ide.app.documents.active.dirty`));
	// und Fehler im namenlosen Tab landen im Tab
	await setEditorText(win, 'For i = 1 To 3\n');
	await js(win, `void window.__ide.app.run('program.check'); 0`);
	await waitFor(win, `document.querySelector('.dialog') !== null`, 60000);
	await clickDialog(win, 'ok');
	await sleep(200);
	check('Namenloser Tab: der Fehler steht im Tab, es entsteht kein zweiter',
		await js(win, `${edApi}.api.editor.getModelMarkers({ owner: 'blitzcc' }).length === 1`) &&
		await js(win, `window.__ide.app.documents.list().every((d) => !/untitled\\.bb$/.test(d.uri || ''))`));
	await waitFor(win, `window.__ide.app.context.get('build.compiling') === false`, 20000);

	// --- Kommandozeile über den Dialog ---------------------------------------------------------------------------------------
	await js(win, `void window.__ide.app.run('program.commandLine'); 0`);
	await waitFor(win, `document.querySelector('.dialog-input') !== null`, 3000);
	check('Kommandozeile: Dialog mit Eingabefeld und Beschriftung',
		await js(win, `document.querySelector('.dialog-label').textContent === 'Program command line:' && document.activeElement === document.querySelector('.dialog-input')`));
	await js(win, `(() => { const i = document.querySelector('.dialog-input'); i.value = '-w level1'; })()`);
	// der Dialog hört am document, nicht am window
	await js(win, `document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }))`);
	await sleep(200);
	check('Kommandozeile: Enter übernimmt den Text', await js(win, `window.__ide.app.settings.get('program.commandLine') === '-w level1'`));
	await js(win, `window.__ide.app.settings.set('program.commandLine', '')`);

	// --- Stop: ein Programm, das nicht von allein endet ---------------------------------------------------------------------------
	dialogStub.openAnswers.push([slow]);
	await key(win, { key: 'o', ctrlKey: true });
	await waitFor(win, `window.__ide.app.documents.active?.title === 'lang.bb'`, 5000);
	await key(win, { key: 'F5' });
	check('Stop: das lange Programm läuft', await waitFor(win, `window.__ide.app.context.get('program.running') === true`, 60000));
	check('Stop: Statuszeile "Program running", Stop-Befehl frei',
		await statusText() === 'Program running' && await js(win, `window.__ide.app.commands.isEnabled('program.stop')`));
	await key(win, { key: 'F5', shiftKey: true });
	check('Umschalt+F5 beendet es', await waitFor(win, `window.__ide.app.context.get('program.running') === false`, 15000));
	const afterStop = await outputLines();
	check('Stop: "Program stopped.", kein Fehler in der Ausgabe',
		afterStop.at(-1) === 'Program stopped.' && await js(win, `document.querySelectorAll('.output-error').length === 0`), afterStop.join(' | '));

	// --- F6 ----------------------------------------------------------------------------------------------------------------------------
	await js(win, `window.__ide.app.documents.close(window.__ide.app.documents.find(${JSON.stringify(slow)}).id)`);
	await key(win, { key: 'F6' });
	check('F6: baut die zuletzt gebaute Datei noch einmal (öffnet sie wieder)',
		await waitFor(win, `window.__ide.app.documents.find(${JSON.stringify(slow)}) !== undefined && window.__ide.app.context.get('program.running') === true`, 60000));
	await js(win, `void window.__ide.app.run('program.stop'); 0`);
	await waitFor(win, `window.__ide.app.context.get('program.running') === false`, 15000);

	// --- Programm erstellen ----------------------------------------------------------------------------------------------------------------
	const exe = path.join(dir, 'fertig');
	await js(win, `window.__ide.app.documents.activate(window.__ide.app.documents.find(${JSON.stringify(good)}).id)`);
	dialogStub.saveAnswers.push(exe);
	await js(win, `void window.__ide.app.run('program.publish'); 0`);
	await waitFor(win, `[...document.querySelectorAll('.output-line')].some((e) => /^Creating executable/.test(e.textContent)) && window.__ide.app.context.get('build.compiling') === false`, 60000);
	check('Programm erstellen: fertig.exe entsteht (Endung ergänzt)', fs.existsSync(`${exe}.exe`), fs.readdirSync(dir).join());

	check('Keine Fehler auf der Konsole der Seite', errors.length === 0, errors.slice(0, 3).join(' || '));
	win.destroy();
	hostService.setLaunchArgs(['electron', 'app'], false, dir);
	fs.rmSync(dir, { recursive: true, force: true });
}

async function scenarioSidebar() {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bltznxt-ui-'));
	const game = path.join(dir, 'spiel');
	const other = path.join(dir, 'anders');
	fs.mkdirSync(path.join(game, 'daten'), { recursive: true });
	fs.mkdirSync(other);
	fs.writeFileSync(path.join(game, 'haupt.bb'), 'Print 1\r\n');
	fs.writeFileSync(path.join(game, 'karte.bb'), 'Print 2\r\n');
	fs.writeFileSync(path.join(game, 'spiel.exe'), 'MZ');
	fs.writeFileSync(path.join(game, 'SDL3.dll'), 'MZ');
	fs.writeFileSync(path.join(game, 'haupt.bb_bak1'), 'alt');
	fs.writeFileSync(path.join(game, 'daten', 'level.bb'), 'Print 3\r\n');
	fs.writeFileSync(path.join(other, 'fremd.bb'), 'Print 4\r\n');
	hostService.setLaunchArgs(['electron', 'app', path.join(game, 'haupt.bb')], false, dir);
	const { win, errors, started } = await openWindow(dir);
	check('Seitenleiste: IDE gestartet', started);
	if (!started) return win.destroy();
	await waitFor(win, `document.querySelector('.explorer-row') !== null`, 8000);

	const rows = () => js(win, `[...document.querySelectorAll('.explorer-row')].map((r) => (r.classList.contains('dir') ? '/' : '') + r.querySelector('.explorer-name').textContent + (r.classList.contains('active') ? '*' : ''))`);
	check('Seitenleiste sichtbar, zeigt den Ordner der geöffneten Datei',
		await js(win, `!document.querySelector('.sidebar').hidden && document.querySelector('.explorer-title').textContent === 'spiel'`));
	const first = await rows();
	check('Liste: Ordner zuerst, Programme/DLLs/Sicherungskopien ausgeblendet, die aktive Datei markiert',
		first.join() === '/daten,haupt.bb*,karte.bb', first.join());

	// Klick auf eine Datei öffnet sie
	await js(win, `[...document.querySelectorAll('.explorer-row')].find((r) => r.querySelector('.explorer-name').textContent === 'karte.bb').click()`);
	await waitFor(win, `window.__ide.app.documents.active?.title === 'karte.bb'`, 3000);
	check('Klick auf eine Datei öffnet sie in einem Tab und markiert sie',
		await js(win, `document.querySelectorAll('.tab').length === 2`) && (await rows()).includes('karte.bb*'));

	// Klick auf einen Ordner klappt ihn auf und zu
	await js(win, `document.querySelector('.explorer-row.dir').click()`);
	await waitFor(win, `document.querySelectorAll('.explorer-row').length === 4`, 3000);
	const opened = await rows();
	check('Klick auf einen Ordner klappt ihn auf (Inhalt eingerückt)', opened.join() === '/daten,level.bb,haupt.bb,karte.bb*', opened.join());
	check('aufgeklappt: Pfeil nach unten, Level-Datei tiefer eingerückt',
		await js(win, `document.querySelector('.explorer-row.dir').getAttribute('aria-expanded') === 'true' && document.querySelectorAll('.explorer-row')[1].getAttribute('aria-level') === '2'`));
	await js(win, `document.querySelector('.explorer-row.dir').click()`);
	check('zweiter Klick klappt zu', (await rows()).join() === '/daten,haupt.bb,karte.bb*');

	// Eine Datei aus einem Unterordner öffnen: die Liste bleibt im Ordner
	await js(win, `document.querySelector('.explorer-row.dir').click()`);
	await waitFor(win, `document.querySelectorAll('.explorer-row').length === 4`, 3000);
	await js(win, `[...document.querySelectorAll('.explorer-row')].find((r) => r.querySelector('.explorer-name').textContent === 'level.bb').click()`);
	await waitFor(win, `window.__ide.app.documents.active?.title === 'level.bb'`, 3000);
	check('Datei im Unterordner: die Liste bleibt beim Ordner spiel', await js(win, `document.querySelector('.explorer-title').textContent === 'spiel'`));

	// Tastatur: Pfeile und Enter
	await js(win, `document.querySelectorAll('.explorer-row')[2].focus()`);
	await js(win, `document.activeElement.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true }))`);
	check('Pfeil nach unten bewegt den Fokus', await js(win, `document.activeElement.querySelector('.explorer-name').textContent === 'karte.bb'`));
	await js(win, `document.activeElement.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }))`);
	await waitFor(win, `window.__ide.app.documents.active?.title === 'karte.bb'`, 3000);
	check('Enter öffnet die Datei', await js(win, `window.__ide.app.documents.active.title === 'karte.bb'`));

	// Ordner fest wählen
	dialogStub.folderAnswers.push(other);
	await js(win, `void window.__ide.app.run('explorer.openFolder'); 0`);
	await waitFor(win, `document.querySelector('.explorer-title').textContent === 'anders'`, 3000);
	check('Open Folder…: die Liste zeigt den gewählten Ordner, ein Pin-Knopf erscheint',
		(await rows()).join() === 'fremd.bb' && await js(win, `document.querySelector('.explorer-button[data-command="explorer.followActive"]') !== null`));
	await js(win, `document.querySelector('.explorer-button[data-command="explorer.followActive"]').click()`);
	await waitFor(win, `document.querySelector('.explorer-title').textContent !== 'anders'`, 3000);
	check('Pin lösen: die Liste folgt wieder der aktiven Datei', await js(win, `document.querySelector('.explorer-title').textContent === 'spiel'`));

	// Neue Datei im Ordner erscheint nach dem Speichern
	fs.writeFileSync(path.join(game, 'neu.bb'), 'Print 5\r\n');
	await js(win, `window.__ide.app.documents.markSaved(window.__ide.app.documents.active.id)`);
	check('Nach dem Speichern liest die Liste neu (neue Datei erscheint)',
		await waitFor(win, `[...document.querySelectorAll('.explorer-name')].some((n) => n.textContent === 'neu.bb')`, 3000));

	// Seitenleiste aus/an (Strg+B), Breite ziehen und merken
	await js(win, `${edApi}.editor.focus()`);
	await key(win, { key: 'b', ctrlKey: true });
	await sleep(150);
	check('Strg+B blendet die Seitenleiste aus', await js(win, `document.querySelector('.sidebar').hidden && document.querySelector('.ide').dataset.sidebar === 'off'`));
	await key(win, { key: 'b', ctrlKey: true });
	await sleep(150);
	check('Strg+B blendet sie wieder ein', await js(win, `!document.querySelector('.sidebar').hidden`));

	const before = await js(win, `document.querySelector('.sidebar').getBoundingClientRect().width`);
	await js(win, `(() => {
		const sash = document.querySelector('.sash');
		const x = sash.getBoundingClientRect().left + 2;
		sash.dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true, clientX: x, button: 0 }));
		document.dispatchEvent(new MouseEvent('mousemove', { bubbles: true, clientX: x + 60 }));
		document.dispatchEvent(new MouseEvent('mouseup', { bubbles: true, clientX: x + 60 }));
	})()`);
	await sleep(150);
	const after = await js(win, `document.querySelector('.sidebar').getBoundingClientRect().width`);
	check('Trenner ziehen ändert die Breite (und merkt sie sich)', Math.abs(after - before - 60) <= 2 && await js(win, `window.__ide.app.state.get('workbench.sidebarWidth') === ${Math.round(after)}`), `${before} -> ${after}`);

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
		await scenarioEditor();
		await scenarioBuild();
		await scenarioSidebar();
		await scenarioCorruptSettings();
	} catch (err) {
		console.log('FAIL  Testlauf abgebrochen:', err && err.stack ? err.stack : err);
		failures++;
	}
	console.log(`\n${failures === 0 ? 'Alle Prüfungen bestanden.' : failures + ' fehlgeschlagen.'}`);
	app.exit(failures === 0 ? 0 : 1);
});
