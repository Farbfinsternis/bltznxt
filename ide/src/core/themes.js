// Themes.
//
// Ein Theme ist ein Beitrag zum Punkt "themes" — reine Daten:
//
//   {
//     id: 'blitz-classic',
//     label: 'Blitz3D Classic',
//     dark: true,
//     // die sieben Farben der Original-IDE (blitzide.prefs: rgb_*)
//     editor: { background, default, identifier, keyword, comment, string, number },
//     // optional: Überschreibungen für Monaco-Farben ('editor.selectionBackground', ...)
//     monaco: { ... },
//     // optional: CSS-Variablen der Oberfläche ('--bg': '#1e1e1e', ...)
//     ui: { ... }
//   }
//
// Wer ein Theme braucht (der Editor für Monaco, der Workbench für die
// Oberfläche), löst es hier auf. Ein unbekannter Name fällt auf das erste
// vorhandene Theme zurück — nie auf "kein Theme".

export const EDITOR_COLORS = ['background', 'default', 'identifier', 'keyword', 'comment', 'string', 'number'];

/**
 * @param {Array<any>} themes    Beiträge zu "themes"
 * @param {string | undefined} id  gewünschter Name (Einstellung `workbench.theme`)
 * @returns {any | null}
 */
export function resolveTheme(themes, id) {
	if (!Array.isArray(themes) || themes.length === 0) return null;
	// Später beigetragene Themes gewinnen bei gleicher id
	for (let i = themes.length - 1; i >= 0; i--) if (themes[i].id === id) return themes[i];
	return themes[0];
}

/**
 * Prüft ein Theme auf Vollständigkeit der sieben Editorfarben.
 * @returns {string[]} Fehlermeldungen, leer wenn in Ordnung
 */
export function validateTheme(theme) {
	const problems = [];
	if (!theme || typeof theme.id !== 'string' || !theme.id) return ['Theme ohne id'];
	if (!theme.editor) return [`Theme "${theme.id}" hat keine Editorfarben`];
	for (const name of EDITOR_COLORS) {
		if (!/^#[0-9a-fA-F]{6}$/.test(theme.editor[name] || '')) {
			problems.push(`Theme "${theme.id}": Farbe "${name}" fehlt oder ist kein #RRGGBB`);
		}
	}
	return problems;
}

/**
 * Übersetzt ein Theme in die Form, die `monaco.editor.defineTheme` erwartet.
 * Rein — Monaco selbst kommt nicht vor, damit es sich ohne Editor prüfen lässt.
 *
 * Die Zuordnung folgt der Original-IDE: Schlüsselwörter *und* Befehle haben
 * eine Farbe (bei uns getrennte Token, beide `keyword`), Bezeichner eine
 * eigene, alles Übrige (Operatoren, Klammern) die Standardfarbe.
 */
export function themeToMonaco(theme) {
	const c = theme.editor;
	const hex = (color) => color.replace('#', '');
	const rule = (token, color) => ({ token, foreground: hex(color) });
	return {
		base: theme.dark === false ? 'vs' : 'vs-dark',
		inherit: false,
		rules: [
			rule('', c.default),
			rule('source', c.default),
			rule('operator', c.default),
			rule('delimiter', c.default),
			rule('identifier', c.identifier),
			rule('keyword', c.keyword),
			rule('type.identifier', c.keyword), // eingebaute Befehle
			rule('comment', c.comment),
			rule('string', c.string),
			rule('number', c.number)
		],
		colors: {
			'editor.background': c.background,
			'editor.foreground': c.default,
			'editorCursor.foreground': c.default,
			// Zeilennummern und Auswahl aus der Standardfarbe abgeleitet (Alpha), sofern
			// das Theme nichts Eigenes vorgibt
			'editorLineNumber.foreground': `${c.default}99`,
			'editorLineNumber.activeForeground': c.default,
			'editor.selectionBackground': `${c.default}40`,
			'editor.inactiveSelectionBackground': `${c.default}25`,
			'editor.lineHighlightBackground': `${c.default}14`,
			'editorIndentGuide.background1': `${c.default}20`,
			...(theme.monaco || {})
		}
	};
}
