// Erweiterung "editor": zeigt das aktive Dokument in Monaco.
//
// Monaco steckt ganz in dieser Erweiterung. Alle anderen sprechen mit dem
// Editor über den Dienst "editor" und über das Dokumentmodell — dadurch bleibt
// der Editor austauschbar (PLAN.md, "Was wir bewusst nicht festlegen").
//
// Dokument und Editor bleiben so verbunden:
//   * je Dokument ein Monaco-Modell, angelegt beim ersten Anzeigen
//   * Eingaben im Modell -> `documents.setText` (Text immer mit LF)
//   * Änderungen am Dokument von außen -> ins Modell
//   * Dokument geschlossen -> Modell verworfen
//
// Dazu, wie im Bearbeiten-Menü der Original-IDE: Ausschneiden, Kopieren,
// Einfügen, Alles auswählen, Suchen, Weitersuchen, Ersetzen; das Rechtsklick-
// menü ist das Bearbeiten-Menü; die Statuszeile zeigt "Row:12 Col:4 *".
//
// Dienst "editor":
//   onDidMount(fn)                   fn(editor, monaco) sobald der Editor da ist (sofort, wenn schon)
//   focus(), reveal(line, column)
//   trigger(actionId)                eine Monaco-Aktion ausführen
//   setDiagnostics(docId, owner, list), clearDiagnostics(owner)
//   monaco                           { editor, api } oder null, solange nichts gemountet ist

import * as monaco from 'monaco-editor';
import './workers.js';
import { LANGUAGE_ID } from '../language-blitz/blitz3d.js';
import { resolveTheme, themeToMonaco, validateTheme } from '../../core/themes.js';
import { icons } from './icons.js';

const LF = () => monaco.editor.EndOfLinePreference.LF;
const eolOf = (doc) => (doc.eol === '\r\n' ? monaco.editor.EndOfLineSequence.CRLF : monaco.editor.EndOfLineSequence.LF);

function toMonacoMarkers(diagnostics) {
	const severityOf = (s) =>
		s === 'warning' ? monaco.MarkerSeverity.Warning
		: s === 'note' ? monaco.MarkerSeverity.Info
		: monaco.MarkerSeverity.Error;

	return diagnostics.map((d) => ({
		severity: severityOf(d.severity),
		message: d.message,
		startLineNumber: d.line,
		startColumn: d.column,
		// Der Compiler im IDE-Format meldet Anfang und Ende; im GCC-Format nur den
		// Anfang — dann wird ein Zeichen markiert, das ist die brauchbarste Näherung.
		endLineNumber: d.endLine ?? d.line,
		endColumn: d.endColumn ?? d.column + 1
	}));
}

/** Controller je Aktivierung, damit `activate` an den der `contributes`-Funktion kommt. */
const controllers = new WeakMap();

export default {
	id: 'editor',

	messages: {
		en: {
			'edit.undo': 'Undo',
			'edit.redo': 'Redo',
			'edit.cut': 'Cut',
			'edit.copy': 'Copy',
			'edit.paste': 'Paste',
			'edit.selectAll': 'Select All',
			'edit.find': 'Find...',
			'edit.findNext': 'Find Next',
			'edit.findPrevious': 'Find Previous',
			'edit.replace': 'Replace...'
		}
	},

	contributes: (ctx) => {
		const controller = createController(ctx);
		controllers.set(ctx, controller);
		const action = (id) => () => controller.trigger(id);
		const inDoc = 'document.active';

		return {
			settings: {
				'editor.fontSize': { type: 'number', default: 14, description: 'Editor font size in pixels.' },
				'editor.fontFamily': {
					type: 'string',
					default: "Consolas, 'Courier New', monospace",
					description: 'Editor font.'
				},
				// Original: edit_tabs 4
				'editor.tabSize': { type: 'number', default: 4, description: 'Width of a tab in spaces.' },
				// Original: edit_blkcursor
				'editor.blockCursor': { type: 'boolean', default: false, description: 'Use a block cursor.' }
			},

			commands: [
				{ id: 'edit.undo', title: 'edit.undo', enabledWhen: inDoc, run: action('undo') },
				{ id: 'edit.redo', title: 'edit.redo', enabledWhen: inDoc, run: action('redo') },
				{ id: 'edit.cut', title: 'edit.cut', enabledWhen: 'editor.hasSelection', run: () => controller.clipboard('cut', 'editor.action.clipboardCutAction') },
				{ id: 'edit.copy', title: 'edit.copy', enabledWhen: 'editor.hasSelection', run: () => controller.clipboard('copy', 'editor.action.clipboardCopyAction') },
				{ id: 'edit.paste', title: 'edit.paste', enabledWhen: inDoc, run: () => controller.clipboard('paste', 'editor.action.clipboardPasteAction') },
				{ id: 'edit.selectAll', title: 'edit.selectAll', enabledWhen: inDoc, run: action('editor.action.selectAll') },
				{ id: 'edit.find', title: 'edit.find', enabledWhen: inDoc, run: action('actions.find') },
				{ id: 'edit.findNext', title: 'edit.findNext', enabledWhen: inDoc, run: action('editor.action.nextMatchFindAction') },
				{ id: 'edit.findPrevious', title: 'edit.findPrevious', enabledWhen: inDoc, run: action('editor.action.previousMatchFindAction') },
				{ id: 'edit.replace', title: 'edit.replace', enabledWhen: inDoc, run: action('editor.action.startFindReplace') }
			],

			// Anordnung wie das Bearbeiten-Menü des Originals; Rückgängig/Wiederholen
			// kennt das Original nur als Tastenkürzel des Textfelds.
			menus: [
				{ menu: 'edit', command: 'edit.undo', group: '0_undo', order: 1 },
				{ menu: 'edit', command: 'edit.redo', group: '0_undo', order: 2 },
				{ menu: 'edit', command: 'edit.cut', group: '1_clipboard', order: 1 },
				{ menu: 'edit', command: 'edit.copy', group: '1_clipboard', order: 2 },
				{ menu: 'edit', command: 'edit.paste', group: '1_clipboard', order: 3 },
				{ menu: 'edit', command: 'edit.selectAll', group: '2_select' },
				{ menu: 'edit', command: 'edit.find', group: '3_find', order: 1 },
				{ menu: 'edit', command: 'edit.findNext', group: '3_find', order: 2 },
				{ menu: 'edit', command: 'edit.replace', group: '3_find', order: 3 },
				{ menu: 'edit', command: 'edit.findPrevious', group: '3_find', order: 4 }
			],

			keybindings: [
				// Nur zur Anzeige: das Textfeld des Editors behandelt diese Tasten selbst
				{ command: 'edit.undo', key: 'Ctrl+Z', passive: true },
				{ command: 'edit.redo', key: 'Ctrl+Y', passive: true },
				{ command: 'edit.cut', key: 'Ctrl+X', passive: true },
				{ command: 'edit.copy', key: 'Ctrl+C', passive: true },
				{ command: 'edit.paste', key: 'Ctrl+V', passive: true },
				{ command: 'edit.selectAll', key: 'Ctrl+A', passive: true },
				{ command: 'edit.find', key: 'Ctrl+F' },
				{ command: 'edit.findNext', key: 'F3' },
				{ command: 'edit.findPrevious', key: 'Shift+F3' },
				{ command: 'edit.replace', key: 'Ctrl+R' }
			],

			// Symbolleiste des Originals: Ausschneiden, Kopieren, Einfügen | Suchen
			toolbar: [
				{ command: 'edit.cut', icon: icons.cut, group: '2_edit', order: 1 },
				{ command: 'edit.copy', icon: icons.copy, group: '2_edit', order: 2 },
				{ command: 'edit.paste', icon: icons.paste, group: '2_edit', order: 3 },
				{ command: 'edit.find', icon: icons.find, group: '3_find', order: 1 }
			],

			statusItems: [{ id: 'editor.position', align: 'right', priority: 50 }],

			views: [
				{
					id: 'editor',
					location: 'editor',
					when: 'document.active',
					mount: (el) => controller.mount(el)
				}
			]
		};
	},

	activate(ctx) {
		const controller = controllers.get(ctx);
		controller.start();
		ctx.subscriptions.add(() => controller.stop());
	}
};

// ---------------------------------------------------------------------------

function createController(ctx) {
	const { documents, settings, context, contributions } = ctx;

	/** @type {monaco.editor.IStandaloneCodeEditor | null} */
	let editor = null;
	/** @type {Map<number, monaco.editor.ITextModel>} */
	const models = new Map();
	/** Verhindert, dass ein von außen gesetzter Text als Eingabe zurückläuft. */
	let applying = false;
	const mountListeners = new Set();
	const subs = [];
	const themeIds = new Set();

	// ---- Themes -------------------------------------------------------------
	function applyTheme() {
		const themes = contributions.get('themes');
		for (const theme of themes) {
			const problems = validateTheme(theme);
			if (problems.length) {
				console.error(`[editor] ${problems.join('; ')}`);
				continue;
			}
			monaco.editor.defineTheme(theme.id, themeToMonaco(theme));
			themeIds.add(theme.id);
		}
		const chosen = resolveTheme(themes.filter((t) => themeIds.has(t.id)), settings.get('workbench.theme'));
		monaco.editor.setTheme(chosen ? chosen.id : 'vs-dark');
	}

	// ---- Dokument <-> Modell ---------------------------------------------------
	function modelFor(doc) {
		let model = models.get(doc.id);
		if (model) return model;
		model = monaco.editor.createModel(doc.text, LANGUAGE_ID);
		model.setEOL(eolOf(doc));
		model.onDidChangeContent(() => {
			if (applying) return;
			documents.setText(doc.id, model.getValue(LF()));
		});
		models.set(doc.id, model);
		return model;
	}

	function show(doc) {
		if (!editor) return;
		editor.setModel(doc ? modelFor(doc) : null);
		updateStatus();
	}

	// ---- Statuszeile: "Row:12 Col:4 *" wie im Original -----------------------------
	function updateStatus() {
		const statusbar = ctx.services.tryGet('statusbar');
		if (!statusbar) return;
		const doc = documents.active;
		if (!editor || !doc || !editor.getModel()) {
			statusbar.clear('editor.position');
			return;
		}
		const p = editor.getPosition();
		if (!p) return;
		statusbar.setText('editor.position', `Row:${p.lineNumber} Col:${p.column} ${doc.dirty ? '*' : ' '}`);
	}

	function options() {
		return {
			fontSize: settings.get('editor.fontSize'),
			fontFamily: settings.get('editor.fontFamily'),
			tabSize: settings.get('editor.tabSize'),
			cursorStyle: settings.get('editor.blockCursor') ? 'block' : 'line'
		};
	}

	// ---- Einhängen ------------------------------------------------------------------------
	function mount(el) {
		editor = monaco.editor.create(el, {
			model: null,
			automaticLayout: true,
			...options(),
			// Wie das RichEdit-Feld des Originals: Tabs bleiben Tabs, Enter übernimmt die
			// Einrückung der Zeile, nichts wird von selbst geschlossen oder umgebrochen.
			insertSpaces: false,
			detectIndentation: false,
			autoIndent: 'keep',
			autoClosingBrackets: 'never',
			autoClosingQuotes: 'never',
			autoSurround: 'never',
			// Das Original kennt nur die sieben Farben des Themes, keine bunten Klammerpaare
			bracketPairColorization: { enabled: false },
			// Das Rechtsklickmenü ist das Bearbeiten-Menü der IDE (siehe unten)
			contextmenu: false
		});

		editor.onDidChangeCursorPosition(updateStatus);
		editor.onDidChangeCursorSelection((e) => context.set('editor.hasSelection', !e.selection.isEmpty()));
		editor.onDidChangeModel(() => context.set('editor.hasSelection', false));
		editor.onContextMenu((e) => {
			const menus = ctx.services.tryGet('menus');
			if (menus) menus.popup('edit', e.event.browserEvent.clientX, e.event.browserEvent.clientY);
		});

		const subsMount = [
			documents.on('activated', show),
			documents.on('changed', (doc) => {
				const model = models.get(doc.id);
				if (model && model.getValue(LF()) !== doc.text) {
					applying = true;
					try {
						model.setValue(doc.text);
					} finally {
						applying = false;
					}
				}
				updateStatus();
			}),
			documents.on('saved', updateStatus),
			documents.on('closed', (doc) => {
				const model = models.get(doc.id);
				if (model) {
					if (editor && editor.getModel() === model) editor.setModel(null);
					model.dispose();
					models.delete(doc.id);
				}
				updateStatus();
			}),
			documents.on('renamed', (doc) => {
				// Zeilenende kann sich beim Speichern ändern
				const model = models.get(doc.id);
				if (model) model.setEOL(eolOf(doc));
			})
		];

		if (documents.active) show(documents.active);
		for (const fn of mountListeners) fn(editor, monaco);

		return () => {
			for (const off of subsMount) off();
			for (const model of models.values()) model.dispose();
			models.clear();
			editor.dispose();
			editor = null;
			context.set('editor.hasSelection', false);
			const statusbar = ctx.services.tryGet('statusbar');
			if (statusbar) statusbar.clear('editor.position');
		};
	}

	function start() {
		context.set('editor.hasSelection', false);
		applyTheme();
		subs.push(
			contributions.onDidChange('themes', applyTheme),
			settings.onDidChange((e) => {
				if (e.key === 'workbench.theme') applyTheme();
				if (e.key.startsWith('editor.') && editor) editor.updateOptions(options());
			})
		);

		// Der Dienst für alle, die mit dem Editor reden wollen
		ctx.services.provide('editor', {
			onDidMount(fn) {
				mountListeners.add(fn);
				if (editor) fn(editor, monaco);
				return () => mountListeners.delete(fn);
			},
			focus: () => editor && editor.focus(),
			/** Cursor auf eine Stelle setzen und sichtbar machen (1-basiert). */
			reveal(line, column = 1) {
				if (!editor) return;
				editor.setPosition({ lineNumber: line, column });
				editor.revealPositionInCenterIfOutsideViewport({ lineNumber: line, column });
				editor.focus();
			},
			/** Eine Monaco-Aktion ausführen; der Editor bekommt vorher den Fokus. */
			trigger: (id) => controller.trigger(id),
			/** Diagnosen eines Absenders (etwa 'blitzcc') als Marker am Dokument anzeigen. */
			setDiagnostics(docId, owner, diagnostics) {
				const doc = documents.get(docId);
				const model = models.get(docId) || (doc ? modelFor(doc) : null);
				if (model) monaco.editor.setModelMarkers(model, owner, toMonacoMarkers(diagnostics));
			},
			clearDiagnostics(owner) {
				for (const model of models.values()) monaco.editor.setModelMarkers(model, owner, []);
			},
			/** Der rohe Monaco-Editor — nur für Erweiterungen, die wirklich an Monaco hängen. */
			get monaco() {
				return editor ? { editor, api: monaco } : null;
			}
		});
	}

	function stop() {
		for (const off of subs) off();
		subs.length = 0;
	}

	function trigger(actionId) {
		if (!editor) return;
		editor.focus();
		editor.trigger('menu', actionId, null);
	}

	/**
	 * Zwischenablage: über das Fenster (Electron), sonst über die Aktion von
	 * Monaco — die im Browser nur auf eine echte Benutzergeste hin klappt.
	 */
	async function clipboard(nativeAction, monacoAction) {
		if (!editor) return;
		editor.focus();
		if (ctx.platform.hasBackend) await ctx.platform.invoke('window', 'edit', nativeAction);
		else editor.trigger('menu', monacoAction, null);
	}

	const controller = { mount, start, stop, trigger, clipboard };
	return controller;
}
