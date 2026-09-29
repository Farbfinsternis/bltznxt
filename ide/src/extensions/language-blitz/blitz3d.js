import * as monaco from 'monaco-editor';

// Die Sprache wird nicht beim Import registriert, sondern in install() — die
// Erweiterung "language-blitz" schaltet sich damit sauber an und ab.
const LANGUAGE_ID = 'blitz3d';

// Keywords sind für die Ablaufsteuerung (control flow) — die stehen fest,
// sie gehören zur Sprache und nicht zur Runtime.
const KEYWORDS = [
	'Function', 'End', 'End Function', 'EndFunction',
	'If', 'Then', 'Else', 'ElseIf', 'End If', 'EndIf',
	'While', 'Wend', 'For', 'To', 'Next', 'Step', 'Each',
	'Repeat', 'Until', 'Forever', 'Exit',
	'Global', 'Local', 'Const', 'Dim',
	'Select', 'Case', 'Default', 'End Select', 'EndSelect',
	'Type', 'End Type', 'EndType', 'Field', 'New', 'Delete',
	'First', 'Last', 'Before', 'After', 'Insert',
	'Data', 'Read', 'Restore', 'Goto', 'Gosub', 'Return',
	'True', 'False', 'Null',
	'And', 'Or', 'Xor', 'Not', 'Mod', 'Shl', 'Shr', 'Sar'
];

// Die eingebauten Befehle kommen vom Compiler, nicht von hier. Bis
// setCommands() sie nachliefert, bleibt die Liste leer — lieber keine
// Vervollständigung als eine falsche.
//
// Diese Liste NICHT mit einer Kopie von kCommands[] füllen: sie würde still
// veralten, sobald der Compiler neue Befehle bekommt. Siehe README.md,
// Abschnitt "Repository Layout".
let commands = [];
/** Solange nicht installiert, ist `setCommands` nur ein Merker. */
let installed = false;
/** @type {monaco.IDisposable | null} */
let tokensProvider = null;

// 2. Syntax Highlighting mit Monarch definieren
function registerTokens() {
	if (!installed) return;
	if (tokensProvider) tokensProvider.dispose();
	tokensProvider = monaco.languages.setMonarchTokensProvider(LANGUAGE_ID, {
		ignoreCase: true,
		keywords: KEYWORDS,
		commands: commands.map((c) => c.name),

		tokenizer: {
			root: [
				// Kommentare (beginnen mit ;)
				[/;.*/, 'comment'],

				// Preprocessor
				[/^\s*#\w+/, 'keyword.directive'],

				// Label: .name am Zeilenanfang
				[/^\s*\.[a-zA-Z_]\w*/, 'type.identifier'],

				// Hex- und Binärliterale ($FF, %1010) vor den Type-Hints
				[/\$[0-9a-fA-F]+/, 'number.hex'],
				[/%[01]+/, 'number.binary'],

				// Identifier mit optionalem Type-Hint (%, #, !, $)
				[/[a-zA-Z_]\w*[%#!$]?/, {
					cases: {
						'@keywords': 'keyword',
						'@commands': 'type.identifier',
						'@default': 'identifier'
					}
				}],

				// Feldzugriff auf Typen
				[/\\/, 'operator'],

				// Strings
				[/"([^"\\]|\\.)*$/, 'string.invalid'], // nicht geschlossener String
				[/"/, 'string', '@string'],

				// Zahlen
				[/\d*\.\d+([eE][\-+]?\d+)?/, 'number.float'],
				[/\d+/, 'number']
			],

			string: [
				[/[^\\"]+/, 'string'],
				[/\\./, 'string.escape.invalid'],
				[/"/, 'string', '@pop']
			]
		}
	});
}

// 3. Autovervollständigung
function registerCompletions() {
	return monaco.languages.registerCompletionItemProvider(LANGUAGE_ID, {
		provideCompletionItems: (model, position) => {
			// Hole das Wort an der aktuellen Position, um es zu ersetzen
			const word = model.getWordUntilPosition(position);
			const range = {
				startLineNumber: position.lineNumber,
				endLineNumber: position.lineNumber,
				startColumn: word.startColumn,
				endColumn: word.endColumn
			};

			const keywordSuggestions = KEYWORDS.map((k) => ({
				label: k,
				kind: monaco.languages.CompletionItemKind.Keyword,
				insertText: k,
				range
			}));

			// Signatur aus `blitzcc +k` als Detailzeile; die Parameter werden zu
			// Tabstops, damit man nach dem Einfügen direkt weitertippen kann.
			const commandSuggestions = commands.map((c) => ({
				label: c.name,
				kind: monaco.languages.CompletionItemKind.Function,
				detail: c.signature ? `${c.name}(${c.signature})` : `${c.name}()`,
				insertText: c.params.length
					? `${c.name} ${c.params.map((p, i) => `\${${i + 1}:${p}}`).join(', ')}`
					: c.name,
				insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
				range
			}));

			return { suggestions: [...keywordSuggestions, ...commandSuggestions] };
		}
	});
}

/**
 * Befehlsliste aus `blitzcc +k` übernehmen.
 * Registriert das Highlighting neu, damit die Befehle auch eingefärbt werden.
 *
 * @param {Array<{name: string, signature: string, params: string[]}>} list
 */
export function setCommands(list) {
	commands = Array.isArray(list) ? list : [];
	registerTokens();
	return commands.length;
}

/**
 * Registriert Sprache, Färbung und Vervollständigung bei Monaco.
 * @returns {() => void} macht alles wieder rückgängig
 */
export function install() {
	monaco.languages.register({ id: LANGUAGE_ID });
	installed = true;
	registerTokens();
	const completions = registerCompletions();
	// Kommentare mit ";", Klammern; nichts wird von selbst geschlossen (wie im
	// Original). Ein Wort ist ein Bezeichner mit optionalem Typkennzeichen.
	const configuration = monaco.languages.setLanguageConfiguration(LANGUAGE_ID, {
		comments: { lineComment: ';' },
		brackets: [['(', ')'], ['[', ']']],
		wordPattern: /[A-Za-z_][A-Za-z0-9_]*[%#$]?/
	});
	return () => {
		installed = false;
		completions.dispose();
		configuration.dispose();
		if (tokensProvider) tokensProvider.dispose();
		tokensProvider = null;
	};
}

export { LANGUAGE_ID };
