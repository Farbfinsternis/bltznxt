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

import * as monaco from 'monaco-editor';
import './workers.js';
import { LANGUAGE_ID } from '../language-blitz/blitz3d.js';

const LF = () => monaco.editor.EndOfLinePreference.LF;

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
		endLineNumber: d.line,
		// blitzcc meldet nur den Startpunkt, keine Spanne — bis zum Zeilenende
		// markieren ist die brauchbarste Näherung.
		endColumn: d.column + 1
	}));
}

export default {
	id: 'editor',

	contributes: {
		settings: {
			'editor.fontSize': { type: 'number', default: 14, description: 'Editor font size in pixels.' },
			// Original: edit_tabs 4
			'editor.tabSize': { type: 'number', default: 4, description: 'Width of a tab in spaces.' }
		},
		views: [
			{
				id: 'editor',
				location: 'editor',
				when: 'document.active',
				mount: (el, ctx) => mountEditor(el, ctx)
			}
		]
	}
};

function mountEditor(el, ctx) {
	const { documents, settings } = ctx;

	const editor = monaco.editor.create(el, {
		model: null,
		theme: 'vs-dark',
		automaticLayout: true,
		fontSize: settings.get('editor.fontSize'),
		tabSize: settings.get('editor.tabSize'),
		insertSpaces: false // Blitz-Quelltexte sind mit Tabs eingerückt
	});

	/** @type {Map<number, monaco.editor.ITextModel>} */
	const models = new Map();
	/** Verhindert, dass ein von außen gesetzter Text als Eingabe zurückläuft. */
	let applying = false;
	const subs = [];

	function modelFor(doc) {
		let model = models.get(doc.id);
		if (model) return model;
		model = monaco.editor.createModel(doc.text, LANGUAGE_ID);
		model.setEOL(doc.eol === '\r\n' ? monaco.editor.EndOfLineSequence.CRLF : monaco.editor.EndOfLineSequence.LF);
		model.onDidChangeContent(() => {
			if (applying) return;
			documents.setText(doc.id, model.getValue(LF()));
		});
		models.set(doc.id, model);
		return model;
	}

	function show(doc) {
		editor.setModel(doc ? modelFor(doc) : null);
	}

	subs.push(
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
		}),
		documents.on('closed', (doc) => {
			const model = models.get(doc.id);
			if (model) {
				if (editor.getModel() === model) editor.setModel(null);
				model.dispose();
				models.delete(doc.id);
			}
		}),
		documents.on('renamed', (doc) => {
			// Endung und Zeilenende können sich beim Speichern ändern
			const model = models.get(doc.id);
			if (model) model.setEOL(doc.eol === '\r\n' ? monaco.editor.EndOfLineSequence.CRLF : monaco.editor.EndOfLineSequence.LF);
		}),
		settings.onDidChange((e) => {
			if (e.key === 'editor.fontSize') editor.updateOptions({ fontSize: e.value });
			if (e.key === 'editor.tabSize') editor.updateOptions({ tabSize: e.value });
		})
	);

	// Der Dienst für alle, die mit dem Editor reden wollen
	ctx.services.provide('editor', {
		focus: () => editor.focus(),
		/** Cursor auf eine Stelle setzen und sichtbar machen (1-basiert). */
		reveal(line, column = 1) {
			editor.setPosition({ lineNumber: line, column });
			editor.revealPositionInCenterIfOutsideViewport({ lineNumber: line, column });
			editor.focus();
		},
		/** Diagnosen eines Absenders (etwa 'blitzcc') als Marker am Dokument anzeigen. */
		setDiagnostics(docId, owner, diagnostics) {
			const model = models.get(docId) || (documents.get(docId) ? modelFor(documents.get(docId)) : null);
			if (model) monaco.editor.setModelMarkers(model, owner, toMonacoMarkers(diagnostics));
		},
		clearDiagnostics(owner) {
			for (const model of models.values()) monaco.editor.setModelMarkers(model, owner, []);
		},
		/** Der rohe Monaco-Editor — nur für Erweiterungen, die wirklich an Monaco hängen. */
		get monaco() {
			return { editor, api: monaco };
		}
	});

	if (documents.active) show(documents.active);

	return () => {
		for (const off of subs) off();
		for (const model of models.values()) model.dispose();
		models.clear();
		editor.dispose();
	};
}
