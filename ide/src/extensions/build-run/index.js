// Erweiterung "build-run": Programm-Menü der Original-IDE.
//
//   Run program (F5), Run program again (F6), Check for errors (F7),
//   Create Executable..., Program Command Line..., Debug Enabled?, dazu
//   Stop program (Umschalt+F5) — das Original stoppt nur im Debugger.
//
// Ablauf wie in blitzide/mainframe.cpp (MainFrame::build): geänderte Dateien
// mit Namen werden gespeichert, ein namenloser Tab geht in eine Datei im
// Temp-Ordner, blitzcc läuft mit `blitzide=1` (siehe electron/build-protocol.js),
// ein Fehler öffnet die Datei an der Stelle und wird gemeldet — hier zusätzlich
// als Marker im Editor und als Zeile im Ausgabe-Panel.
//
// Dienst "build":
//   run(), rerun(), check(), publish()   -> Promise
//   stop()
//   lines                                 Zeilen des Ausgabe-Panels
//
// Kontext: build.compiling (blitzcc arbeitet), program.running (mindestens ein
// Programm läuft), build.hasLast (es gibt etwas für F6), output.hasContent,
// program.debugOn.
// Ereignisse: build:started, build:finished ({ mode, ok })

import { icons } from './icons.js';

const OWNER = 'blitzcc'; // Absender der Marker im Editor
const basename = (p) => String(p).split(/[\\/]/).pop();
const sameFile = (a, b) => String(a).toLowerCase() === String(b).toLowerCase();

/** Controller je Aktivierung, damit `activate` an den der `contributes`-Funktion kommt. */
const controllers = new WeakMap();

export default {
	id: 'build-run',
	dependsOn: ['files', 'toolchain', 'editor'],

	messages: {
		en: {
			'program.run': 'Run program',
			'program.rerun': 'Run program again',
			'program.check': 'Check for errors',
			'program.publish': 'Create Executable...',
			'program.commandLine': 'Program Command Line...',
			'program.debug': 'Debug Enabled?',
			'program.stop': 'Stop program',

			'program.commandLine.title': 'Program Command Line',
			'program.commandLine.label': 'Program command line:',
			'program.publish.title': 'Select executable filename',
			'output.title': 'Output',
			'build.start.run': 'Building {name}',
			'build.start.check': 'Checking {name}',
			'build.start.publish': 'Creating executable from {name}',
			'build.status.compiling': 'Compiling...',
			'build.status.elapsed': '{phase} ({seconds} s)',
			'build.done.built': 'Built in {seconds} s.',
			'build.status.running': 'Program running',
			'build.error.title': 'Compilation error',
			'build.failed.title': 'Cannot build',
			'build.done.check': 'No errors found.',
			'build.done.exit': 'blitzcc exited with code {code}.',
			'build.done.stopped': 'Program stopped.',
			'build.noCompiler': 'blitzcc not found. Set the compiler path in the settings or BLITZPATH.'
		}
	},

	contributes: (ctx) => {
		const controller = createController(ctx);
		controllers.set(ctx, controller);

		const ready = 'toolchain.available && !build.compiling';
		const canBuild = `document.active && files.available && ${ready}`;

		return {
			settings: {
				// Original: prg_debug (Vorgabe an) — bei uns ohne Wirkung, siehe KNOWN_ISSUES
				'program.debug': { type: 'boolean', default: true, description: 'Compile with debugging enabled (-d).' },
				// Original: cmd_line
				'program.commandLine': { type: 'string', default: '', description: 'Command line passed to the program.' },
				'build.errorDialog': {
					type: 'boolean',
					default: true,
					description: 'Show a message box when the compiler reports an error (the original IDE does).'
				}
			},

			commands: [
				{ id: 'program.run', title: 'program.run', icon: icons.run, enabledWhen: canBuild, run: () => controller.build('run') },
				{ id: 'program.rerun', title: 'program.rerun', enabledWhen: `build.hasLast && files.available && ${ready}`, run: () => controller.rerun() },
				{ id: 'program.check', title: 'program.check', enabledWhen: canBuild, run: () => controller.build('check') },
				{ id: 'program.publish', title: 'program.publish', enabledWhen: canBuild, run: () => controller.build('publish') },
				{ id: 'program.commandLine', title: 'program.commandLine', run: () => controller.editCommandLine() },
				{
					id: 'program.debug',
					title: 'program.debug',
					run: () => ctx.settings.set('program.debug', !ctx.settings.get('program.debug'))
				},
				{ id: 'program.stop', title: 'program.stop', icon: icons.stop, enabledWhen: 'program.running', run: () => controller.stop() }
			],

			// Programm-Menü des Originals; "Stop" ist neu
			menus: [
				{ menu: 'program', command: 'program.run', group: '1_run', order: 1 },
				{ menu: 'program', command: 'program.rerun', group: '1_run', order: 2 },
				{ menu: 'program', command: 'program.check', group: '1_run', order: 3 },
				{ menu: 'program', command: 'program.publish', group: '1_run', order: 4 },
				{ menu: 'program', command: 'program.stop', group: '2_stop', order: 1 },
				{ menu: 'program', command: 'program.commandLine', group: '3_options', order: 1 },
				{ menu: 'program', command: 'program.debug', group: '3_options', order: 2, checkedWhen: 'program.debugOn' }
			],

			keybindings: [
				{ command: 'program.run', key: 'F5' },
				{ command: 'program.rerun', key: 'F6' },
				{ command: 'program.check', key: 'F7' },
				{ command: 'program.stop', key: 'Shift+F5' }
			],

			// Symbolleiste des Originals: Programm starten
			toolbar: [{ command: 'program.run', icon: icons.run, group: '4_run', order: 1 }],

			statusItems: [{ id: 'build.status', align: 'left', priority: 10 }],

			views: [
				{
					id: 'output',
					location: 'panel',
					title: 'output.title',
					order: 20,
					when: 'output.hasContent',
					mount: (el) => mountOutput(el, controller)
				}
			]
		};
	},

	activate(ctx) {
		const controller = controllers.get(ctx);
		controller.start();
		ctx.subscriptions.add(() => controller.dispose());
	}
};

// ---------------------------------------------------------------------------

function createController(ctx) {
	const { documents, settings, state, context, platform, i18n } = ctx;
	const t = i18n.t;

	/** @type {Array<{ text: string, kind: 'info' | 'error' | 'output', target?: { file: string, docId?: number, line: number, column: number } }>} */
	const lines = [];
	const listeners = new Set();
	/** @type {Map<string, object>} laufende Bauvorgänge nach token */
	const sessions = new Map();
	let programs = 0;
	let compiling = 0;
	let counter = 0;
	let offEvents = () => {};

	const files = () => ctx.services.get('files');
	const dialogs = () => ctx.services.get('dialogs');
	const statusbar = () => ctx.services.tryGet('statusbar');

	function syncContext() {
		context.set('build.compiling', compiling > 0);
		context.set('program.running', programs > 0);
		context.set('build.hasLast', Boolean(state.get('build.lastFile')));
		context.set('output.hasContent', lines.length > 0);
		context.set('program.debugOn', Boolean(settings.get('program.debug')));
	}

	function status(text) {
		const bar = statusbar();
		if (!bar) return;
		if (text) bar.setText('build.status', text);
		else bar.clear('build.status');
	}

	function addLine(line) {
		lines.push(line);
		for (const fn of listeners) fn(line);
		syncContext();
	}

	function clearOutput() {
		lines.length = 0;
		for (const fn of listeners) fn(null);
		syncContext();
	}

	// ---- Fehler zeigen ---------------------------------------------------------------
	async function jumpTo(target) {
		let doc = target.docId !== undefined ? documents.get(target.docId) : null;
		if (!doc) doc = await files().openPath(target.file);
		if (!doc) return;
		documents.activate(doc.id);
		const editor = ctx.services.tryGet('editor');
		if (editor) editor.reveal(target.line, target.column);
	}

	async function reportError(session, ev) {
		const message = ev.message;
		let target = null;
		if (ev.file) {
			const isScratch = session.scratchFile && sameFile(ev.file, session.scratchFile);
			target = {
				file: isScratch ? session.doc.uri || session.doc.title : ev.file,
				docId: isScratch ? session.doc.id : undefined,
				line: ev.line,
				column: ev.column
			};
		}

		addLine({
			kind: 'error',
			text: target ? `${basename(target.file)}:${target.line}:${target.column}: ${message}` : message,
			target: target || undefined
		});
		ctx.services.tryGet('panel')?.show('output');

		// Datei öffnen, Cursor setzen, Marker — wie im Original springt die IDE an die Stelle
		if (target) {
			let doc = target.docId !== undefined ? documents.get(target.docId) : documents.find(target.file);
			if (!doc) doc = await files().openPath(ev.file);
			if (doc) {
				target.docId = doc.id;
				documents.activate(doc.id);
				const editor = ctx.services.tryGet('editor');
				if (editor) {
					editor.setDiagnostics(doc.id, OWNER, [
						{ line: ev.line, column: ev.column, endLine: ev.endLine, endColumn: ev.endColumn, severity: 'error', message }
					]);
					editor.reveal(ev.line, ev.column);
				}
			}
		}

		if (settings.get('build.errorDialog')) {
			await dialogs().message({
				title: t('build.error.title'),
				text: message,
				buttons: [{ id: 'ok', label: t('button.ok'), default: true }]
			});
		}
	}

	// ---- Ereignisse von blitzcc -----------------------------------------------------------
	async function onEvent(ev) {
		const session = sessions.get(ev.token);
		if (!session) return;

		switch (ev.type) {
			case 'progress':
				addLine({ kind: 'info', text: ev.text });
				session.phase = ev.text;
				showPhase(session);
				break;

			case 'error':
				session.failed = true;
				await reportError(session, ev);
				break;

			case 'running':
				addLine({ kind: 'info', text: t('build.done.built', { seconds: seconds(session) }) });
				addLine({ kind: 'info', text: 'Executing...' });
				session.running = true;
				programs++;
				endCompiling(session);
				status(t('build.status.running'));
				break;

			case 'output':
				addLine({ kind: 'output', text: ev.text });
				break;

			case 'failed':
				endCompiling(session);
				sessions.delete(ev.token);
				addLine({ kind: 'error', text: ev.message });
				status('');
				await dialogs().message({
					title: t('build.failed.title'),
					text: ev.message,
					buttons: [{ id: 'ok', label: t('button.ok'), default: true }]
				});
				finished(session, false);
				break;

			case 'exit': {
				endCompiling(session);
				sessions.delete(ev.token);
				if (session.running) programs = Math.max(0, programs - 1);
				const ok = !session.failed && ev.code === 0;
				if (ok && session.mode === 'check') addLine({ kind: 'info', text: t('build.done.check') });
				else if (ok && session.mode === 'publish') addLine({ kind: 'info', text: t('build.done.built', { seconds: seconds(session) }) });
				else if (!ok && !session.failed && !session.stopped) addLine({ kind: 'error', text: t('build.done.exit', { code: ev.code }) });
				if (session.stopped) addLine({ kind: 'info', text: t('build.done.stopped') });
				// Der Text der Statuszeile gilt nur für das jüngste, noch laufende Programm
				if (programs === 0) status('');
				finished(session, ok);
				break;
			}
		}
		syncContext();
	}

	function endCompiling(session) {
		if (session.compiling) {
			session.compiling = false;
			compiling = Math.max(0, compiling - 1);
			if (session.timer) clearInterval(session.timer);
			session.timer = null;
		}
	}

	/** Sekunden seit dem Start, eine Nachkommastelle. */
	const seconds = (session) => ((Date.now() - session.startedAt) / 1000).toFixed(1);

	/**
	 * Statuszeile: die jetzige Phase, dazu die verstrichene Zeit. Der g++-Lauf
	 * gibt sonst lange kein Lebenszeichen von sich.
	 */
	function showPhase(session) {
		const elapsed = Math.floor((Date.now() - session.startedAt) / 1000);
		const phase = session.phase || t('build.status.compiling');
		status(elapsed >= 1 ? t('build.status.elapsed', { phase, seconds: elapsed }) : phase);
	}

	function finished(session, ok) {
		ctx.events.emit('build:finished', { mode: session.mode, ok, file: session.file });
		syncContext();
	}

	// ---- Bauen ----------------------------------------------------------------------------------
	/**
	 * @param {'run' | 'check' | 'publish'} mode
	 * @param {{ doc?: object }} [opts]
	 * @returns {Promise<boolean>} `true`: der Vorgang wurde gestartet
	 */
	async function build(mode, { doc = documents.active } = {}) {
		if (!doc) return false;
		if (!(await files().saveNamed())) return false;

		let output;
		if (mode === 'publish') {
			const exe = await askExecutable(doc);
			if (!exe) return false;
			output = exe;
		}

		let file = doc.uri;
		let scratchFile = null;
		try {
			if (!file) file = scratchFile = await files().writeScratch(doc);
		} catch (err) {
			await dialogs().message({ title: t('build.failed.title'), text: String(err.message || err), buttons: [{ id: 'ok', label: t('button.ok'), default: true }] });
			return false;
		}
		if (doc.uri) state.set('build.lastFile', doc.uri);

		clearOutput();
		ctx.services.tryGet('editor')?.clearDiagnostics(OWNER);

		const token = `b${Date.now().toString(36)}${++counter}`;
		const session = {
			token, mode, doc, file, scratchFile,
			compiling: true, running: false, failed: false, stopped: false,
			startedAt: Date.now(), phase: '', timer: null
		};
		// Die Ausgabe zeigt vom ersten Augenblick an, was passiert
		addLine({ kind: 'info', text: t(`build.start.${mode}`, { name: doc.uri ? basename(doc.uri) : doc.title }) });
		ctx.services.tryGet('panel')?.show('output');
		showPhase(session);
		session.timer = setInterval(() => showPhase(session), 1000);
		if (typeof session.timer.unref === 'function') session.timer.unref(); // hält unter Node (Tests) den Prozess nicht fest
		sessions.set(token, session);
		compiling++;
		syncContext();
		ctx.events.emit('build:started', { mode, file });

		try {
			const result = await platform.invoke('build', 'start', {
				token,
				file,
				mode,
				debug: settings.get('program.debug'),
				output,
				commandLine: settings.get('program.commandLine'),
				compilerPath: settings.get('toolchain.compilerPath') || undefined
			});
			if (!result.started) return false; // 'failed' ist schon unterwegs
		} catch (err) {
			endCompiling(session);
			sessions.delete(token);
			status('');
			syncContext();
			await dialogs().message({ title: t('build.failed.title'), text: String(err.message || err), buttons: [{ id: 'ok', label: t('button.ok'), default: true }] });
			return false;
		}
		return true;
	}

	async function askExecutable(doc) {
		const base = doc.uri ? basename(doc.uri).replace(/\.[^.]*$/, '') : 'untitled';
		const dir = doc.uri ? String(doc.uri).replace(/[\\/][^\\/]*$/, '') : state.get('files.lastDir', '');
		const sep = dir.includes('\\') ? '\\' : '/';
		const chosen = await platform.invoke('dialog', 'save', {
			title: t('program.publish.title'),
			defaultPath: dir ? `${dir}${sep}${base}.exe` : `${base}.exe`,
			filters: [{ name: 'Executable files (*.exe)', extensions: ['exe'] }]
		});
		if (!chosen) return null;
		return /\.exe$/i.test(chosen) ? chosen : `${chosen}.exe`;
	}

	/** F6: die zuletzt gebaute Datei öffnen und starten. */
	async function rerun() {
		const last = state.get('build.lastFile');
		if (!last) return false;
		const doc = await files().openPath(last);
		return doc ? build('run', { doc }) : false;
	}

	async function editCommandLine() {
		const value = await dialogs().prompt({
			title: t('program.commandLine.title'),
			label: t('program.commandLine.label'),
			value: settings.get('program.commandLine'),
			okLabel: t('button.ok'),
			cancelLabel: t('button.cancel')
		});
		if (value !== null) settings.set('program.commandLine', value);
	}

	async function stopPrograms() {
		for (const session of sessions.values()) session.stopped = true;
		await platform.invoke('build', 'stop');
	}

	function start() {
		syncContext();
		// Ereignisse der Reihe nach verarbeiten: während eines Fehlerdialogs darf
		// das folgende "exit" nicht überholen.
		let queue = Promise.resolve();
		offEvents = platform.on('build:event', (ev) => {
			queue = queue
				.then(() => onEvent(ev))
				.catch((err) => console.error('[build-run] Ereignis nicht verarbeitet:', err));
		});
		ctx.subscriptions.add(settings.onDidChange((e) => e.key === 'program.debug' && syncContext()));
		ctx.subscriptions.add(state.onDidChange((e) => e.key === 'build.lastFile' && syncContext()));
		ctx.services.provide('build', {
			run: () => build('run'),
			check: () => build('check'),
			publish: () => build('publish'),
			rerun,
			stop: stopPrograms,
			get lines() { return lines; }
		});
	}

	return {
		start,
		dispose: () => {
			offEvents();
			for (const session of sessions.values()) if (session.timer) clearInterval(session.timer);
		},
		build,
		rerun,
		editCommandLine,
		stop: stopPrograms,
		lines,
		jumpTo,
		onLine: (fn) => {
			listeners.add(fn);
			return () => listeners.delete(fn);
		}
	};
}

// ---------------------------------------------------------------------------
// Ausgabe-Panel
// ---------------------------------------------------------------------------

function mountOutput(el, controller) {
	el.classList.add('output');
	const list = document.createElement('div');
	list.className = 'output-list';
	list.setAttribute('role', 'log');
	el.append(list);

	function row(line) {
		const div = document.createElement('div');
		div.className = `output-line output-${line.kind}` + (line.target ? ' link' : '');
		div.textContent = line.text;
		if (line.target) div.addEventListener('click', () => controller.jumpTo(line.target));
		return div;
	}

	const toBottom = () => {
		list.scrollTop = list.scrollHeight;
	};
	// Steht man (fast) unten, läuft die Ausgabe mit; hat man nach oben gescrollt,
	// um etwas zu lesen, bleibt sie stehen. Der Zustand wird beim Scrollen
	// gemerkt, denn in einem verborgenen Reiter gibt es keine Maße.
	let follow = true;
	list.addEventListener('scroll', () => {
		if (list.clientHeight > 0) follow = list.scrollHeight - list.scrollTop - list.clientHeight < 24;
	});
	// Wird der Reiter eingeblendet oder das Panel größer, ans Ende gehen (falls man dort war)
	const observer = new ResizeObserver(() => {
		if (follow) toBottom();
	});
	observer.observe(list);

	function render() {
		list.replaceChildren(...controller.lines.map(row));
		follow = true;
		toBottom();
	}
	render();

	const off = controller.onLine((line) => {
		if (line === null) {
			list.replaceChildren();
			follow = true;
			return;
		}
		list.append(row(line));
		if (follow) toBottom();
	});

	return () => {
		observer.disconnect();
		off();
	};
}
