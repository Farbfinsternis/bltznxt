import * as monaco from 'monaco-editor';

// 1. Eigene Sprache registrieren
monaco.languages.register({ id: 'blitz3d' });

// 2. Syntax Highlighting mit Monarch definieren
monaco.languages.setMonarchTokensProvider('blitz3d', {
	// Keywords sind für die Ablaufsteuerung (control flow)
	keywords: [
		'Function', 'End', 'End Function', 'EndFunction',
		'If', 'Then', 'Else', 'End If', 'EndIf',
		'While', 'Wend', 'For', 'To', 'Next', 'Step',
		'Global', 'Local', 'Const',
		'Select', 'Case', 'Default', 'End Select', 'EndSelect',
		'Type', 'End Type', 'EndType', 'Field'
	],
	// Commands sind eingebaute Befehle/Funktionen
	commands: [
		'Print', 'Graphics', 'Graphics3D', 'RenderWorld', 'Flip', 'Cls'
	],

	tokenizer: {
		root: [
			// Identifier, Keywords und Commands
			[/[a-zA-Z_][\w]*/, {
				cases: {
					'@keywords': 'keyword',
					'@commands': 'type.identifier', // Ein üblicher Token für eingebaute Funktionen
					'@default': 'identifier'
				}
			}],

			// Kommentare (beginnen mit ;)
			[/;.*/, 'comment'],

			// Strings
			[/"([^"\\]|\\.)*$/, 'string.invalid'], // nicht geschlossener String
			[/"/, 'string', '@string'],

			// Zahlen
			[/\d*\.\d+([eE][\-+]?\d+)?/, 'number.float'],
			[/\d+/, 'number'],
		],

		string: [
			[/[^\\"]+/, 'string'],
			[/\\./, 'string.escape.invalid'],
			[/"/, 'string', '@pop']
		],
	},
});

// 3. (Optional) Einfache Autovervollständigung bereitstellen
monaco.languages.registerCompletionItemProvider('blitz3d', {
	provideCompletionItems: (model, position) => {
		// Hole das Wort an der aktuellen Position, um es zu ersetzen
		const word = model.getWordUntilPosition(position);
		const range = {
			startLineNumber: position.lineNumber,
			endLineNumber: position.lineNumber,
			startColumn: word.startColumn,
			endColumn: word.endColumn
		};

		// Vorschläge für Keywords
		const keywordSuggestions = [
			'Function', 'End', 'End Function', 'EndFunction',
			'If', 'Then', 'Else', 'End If', 'EndIf',
			'While', 'Wend', 'For', 'To', 'Next', 'Step',
			'Global', 'Local', 'Const',
			'Select', 'Case', 'Default', 'End Select', 'EndSelect',
			'Type', 'End Type', 'EndType', 'Field'
		].map(k => ({
			label: k,
			kind: monaco.languages.CompletionItemKind.Keyword,
			insertText: k,
			range: range
		}));

		// Vorschläge für Commands
		const commandSuggestions = [
			'Print', 'Graphics', 'Graphics3D', 'RenderWorld', 'Flip', 'Cls'
		].map(c => ({
			label: c,
			kind: monaco.languages.CompletionItemKind.Function, // 'Function' für Befehle
			insertText: c,
			range: range
		}));

		return {
			suggestions: [...keywordSuggestions, ...commandSuggestions]
		};
	}
});