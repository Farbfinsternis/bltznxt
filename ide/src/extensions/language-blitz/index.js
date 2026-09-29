// Erweiterung "language-blitz": die Sprache Blitz3D für den Editor.
//
// Färbung, Vervollständigung und die Schreibweise der Schlüsselwörter. Die
// Liste der eingebauten Befehle und Schlüsselwörter kommt vom Compiler (Dienst
// "toolchain", `blitzcc +k`) und wird bei jedem Start und nach jedem Wechsel
// des Compilers neu geholt — nie hier einkopiert.
//
// Das ist heute die einzige Sprache; eine weitere (etwa für Blitz-Includes
// oder Shader) wäre eine weitere Erweiterung dieser Form.

import { install, setCommands } from './blitz3d.js';
import { buildCaseMap, casingEdits } from './casing.js';

export default {
	id: 'language-blitz',
	dependsOn: ['toolchain'],

	activate(ctx) {
		ctx.subscriptions.add(install());

		/** klein -> Schreibweise; leer, solange kein Compiler geantwortet hat */
		let caseMap = new Map();

		const refresh = async () => {
			try {
				const symbols = await ctx.services.get('toolchain').symbols();
				const count = setCommands(symbols.commands);
				caseMap = buildCaseMap([...symbols.keywords, ...symbols.commands.map((c) => c.name)]);
				console.info(`[language-blitz] ${count} eingebaute Befehle, ${symbols.keywords.length} Schlüsselwörter geladen.`);
			} catch (err) {
				console.warn('[language-blitz] Befehlsliste nicht verfügbar:', err.message);
			}
		};

		// Nicht abwarten: die Aktivierung soll nicht am Compiler hängen.
		refresh();
		ctx.subscriptions.add(ctx.events.on('toolchain:changed', refresh));

		// Schreibweise korrigieren, sobald der Editor da ist
		ctx.services.waitFor('editor').then((editorService) => {
			const off = editorService.onDidMount((editor, monaco) => attachCasing(editor, monaco, () => caseMap));
			ctx.subscriptions.add(off);
		});
	}
};

/**
 * Wie die Original-IDE: ein getipptes Wort wird zur Schreibweise der
 * Befehlsliste, sobald der Cursor es verlässt. Angefasst werden nur Zeilen,
 * die der Benutzer selbst bearbeitet hat — eine geöffnete Datei bleibt, wie
 * sie ist, auch wenn man durch sie hindurchklickt.
 *
 * Eine bearbeitete Zeile ist "offen", bis der Cursor sie verlässt: solange er
 * darin steht, wird jedes Wort außer dem am Cursor schon korrigiert, das
 * Wort am Cursor erst, wenn er weiterrückt.
 */
function attachCasing(editor, monaco, getMap) {
	/** Eigene Änderung: löst keine neue Korrektur aus. */
	let applying = false;
	/** Rückgängig/Wiederholen/Laden: die folgende Cursorbewegung soll nichts korrigieren. */
	let skip = false;
	/** @type {Set<number>} bearbeitete Zeilen, die noch nicht abgeschlossen sind */
	let open = new Set();

	function fix(lines, cursor) {
		const model = editor.getModel();
		const map = getMap();
		if (!model || !map.size) return;
		const edits = [];
		for (const line of lines) {
			if (line < 1 || line > model.getLineCount()) continue;
			const column = cursor && cursor.lineNumber === line ? cursor.column : null;
			for (const e of casingEdits(model.getLineContent(line), map, column)) {
				edits.push({ range: new monaco.Range(line, e.startColumn, line, e.endColumn), text: e.text });
			}
		}
		if (!edits.length) return;
		applying = true;
		try {
			// executeEdits legt keinen eigenen Rückgängig-Schritt an: die Korrektur
			// gehört zur Eingabe, die sie ausgelöst hat.
			editor.executeEdits('blitz-casing', edits);
		} finally {
			applying = false;
		}
	}

	const subs = [
		editor.onDidChangeModel(() => {
			open = new Set();
		}),
		editor.onDidChangeModelContent((e) => {
			if (applying) return;
			if (e.isFlush || e.isUndoing || e.isRedoing) {
				skip = true;
				open = new Set();
				// Falls danach keine Cursorbewegung kommt, das Merken nicht hängen lassen
				setTimeout(() => { skip = false; }, 0);
				return;
			}
			for (const change of e.changes) {
				const extra = change.text.split('\n').length - 1;
				for (let i = 0; i <= extra; i++) open.add(change.range.startLineNumber + i);
			}
		}),
		editor.onDidChangeCursorPosition((e) => {
			if (applying) return;
			if (skip) {
				skip = false;
				return;
			}
			if (!open.size) return;
			fix([...open].sort((a, b) => a - b), e.position);
			// Nur die Zeile mit dem Cursor bleibt offen — dort kann noch getippt werden
			open = open.has(e.position.lineNumber) ? new Set([e.position.lineNumber]) : new Set();
		})
	];

	return () => {
		for (const s of subs) s.dispose();
	};
}
