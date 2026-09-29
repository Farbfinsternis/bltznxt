// Die reinen Teile von P2: Schreibweise der Schlüsselwörter, Gliederung, Themes,
// passive Kürzel.

import { test } from 'node:test';
import assert from 'node:assert/strict';

import { buildCaseMap, casingEdits } from '../../src/extensions/language-blitz/casing.js';
import { parseOutline } from '../../src/extensions/outline/parse.js';
import { resolveTheme, validateTheme, themeToMonaco } from '../../src/core/themes.js';
import { createContext } from '../../src/core/context.js';
import { createContributions } from '../../src/core/contributions.js';
import { createKeybindings } from '../../src/core/keybindings.js';

// ------------------------------------------------------------- Schreibweise

const map = buildCaseMap(['Graphics', 'Print', 'If', 'Then', 'EndIf', 'Left', 'Function', 'Data', 'Repeat']);

/** Wendet die Änderungen an — von hinten, damit die Spalten stimmen. */
function apply(line, cursor = null) {
	let out = line;
	for (const e of casingEdits(line, map, cursor).reverse()) {
		out = out.slice(0, e.startColumn - 1) + e.text + out.slice(e.endColumn - 1);
	}
	return out;
}

test('casing: Wörter aus der Befehlsliste bekommen ihre Schreibweise', () => {
	assert.equal(apply('graphics 800,600'), 'Graphics 800,600');
	assert.equal(apply('IF x = 1 THEN print "a"'), 'If x = 1 Then Print "a"');
	assert.equal(apply('endif'), 'EndIf');
	assert.equal(apply('a$ = left$(b$, 2)'), 'a$ = Left$(b$, 2)'); // Typkennzeichen gehört nicht zum Wort
});

test('casing: Kommentare, Zeichenketten, Hexzahlen und Zahlen bleiben unberührt', () => {
	assert.equal(apply('print "if then print" ; graphics'), 'Print "if then print" ; graphics');
	assert.equal(apply('x = $ff00 : print x'), 'x = $ff00 : Print x');
	assert.equal(apply('x = 12if'), 'x = 12if'); // Zahl mit Suffix ist kein Wort
	assert.equal(apply('x = 1e5 : print x'), 'x = 1e5 : Print x');
	assert.equal(apply('print "offen'), 'Print "offen'); // nicht geschlossene Zeichenkette bis Zeilenende
});

test('casing: Bezeichner mit Schlüsselwort als Teil bleiben, ganze Wörter zählen', () => {
	assert.equal(apply('iffy = printer'), 'iffy = printer');
	assert.equal(apply('my_if = 1'), 'my_if = 1');
	assert.equal(apply('data1 = 2'), 'data1 = 2');
});

test('casing: das Wort am Cursor wird nicht angefasst (auch direkt dahinter), alle anderen schon', () => {
	// "graphics" steht in den Spalten 1..8; Cursor in der Mitte, am Ende, dahinter
	assert.equal(apply('graphics 800', 4), 'graphics 800');
	assert.equal(apply('graphics 800', 9), 'graphics 800'); // direkt hinter dem Wort: noch am Tippen
	assert.equal(apply('graphics 800', 10), 'Graphics 800'); // Cursor weiter weg: korrigieren
	assert.equal(apply('print graphics', 3), 'print Graphics'); // anderes Wort in der Zeile wird korrigiert
	assert.equal(apply('graphics', 1), 'graphics'); // Cursor davor
});

test('casing: richtige Schreibweise bleibt, unbekanntes Wort auch, leere Tabelle tut nichts', () => {
	assert.deepEqual(casingEdits('Graphics 800', map), []);
	assert.deepEqual(casingEdits('foo bar', map), []);
	assert.deepEqual(casingEdits('graphics', new Map()), []);
});

test('casing: erste Schreibweise gewinnt bei Doppelten', () => {
	const m = buildCaseMap(['Print', 'PRINT']);
	assert.equal(m.get('print'), 'Print');
});

// ---------------------------------------------------------------- Gliederung

test('outline: Funktionen, Typen und Marken in Dateireihenfolge', () => {
	const text = [
		'; Kopf',
		'Type Player',
		'\tField x#',
		'End Type',
		'',
		'.start',
		'Function Main()',
		'\tPrint 1',
		'End Function',
		'function Helper%(a)',
		'\t.inner',
		'EndFunction'
	].join('\n');
	const o = parseOutline(text);
	assert.deepEqual(o.types, [{ name: 'Player', line: 2 }]);
	assert.deepEqual(o.funcs, [{ name: 'Main', line: 7 }, { name: 'Helper', line: 10 }]);
	assert.deepEqual(o.labels, [{ name: 'start', line: 6 }, { name: 'inner', line: 11 }]);
});

test('outline: nur ganze Wörter, keine Kommentare, keine Endmarken', () => {
	const o = parseOutline('typeName = 3\nfunctional = 1\n; Function Auskommentiert\nEnd Function\nEndType\nType\nFunction\n.5\n');
	assert.deepEqual(o, { funcs: [], types: [], labels: [] });
});

test('outline: alle Zeilenenden, leerer Text', () => {
	assert.equal(parseOutline('Function A()\r\nFunction B()\rFunction C()\n').funcs.length, 3);
	assert.deepEqual(parseOutline(''), { funcs: [], types: [], labels: [] });
});

// -------------------------------------------------------------------- Themes

const classic = {
	id: 'classic', label: 'Classic', dark: true,
	editor: { background: '#225588', default: '#eeeeee', identifier: '#ffffff', keyword: '#aaffff', comment: '#ffee00', string: '#00ff66', number: '#33ffdd' }
};

test('themes: auflösen mit Rückfall auf das erste, später beigetragene gewinnt', () => {
	const light = { ...classic, id: 'light', dark: false };
	assert.equal(resolveTheme([classic, light], 'light').id, 'light');
	assert.equal(resolveTheme([classic, light], 'gibt-es-nicht').id, 'classic');
	assert.equal(resolveTheme([], 'x'), null);
	const override = { ...classic, label: 'Neu' };
	assert.equal(resolveTheme([classic, override], 'classic').label, 'Neu');
});

test('themes: Prüfung der sieben Farben', () => {
	assert.deepEqual(validateTheme(classic), []);
	assert.match(validateTheme({ ...classic, editor: { ...classic.editor, keyword: 'blau' } })[0], /keyword/);
	assert.match(validateTheme({ id: 'x' })[0], /keine Editorfarben/);
	assert.match(validateTheme({})[0], /ohne id/);
});

test('themes: Übersetzung für Monaco — Schlüsselwort und Befehl gleiche Farbe, Bezeichner eigene', () => {
	const m = themeToMonaco(classic);
	assert.equal(m.base, 'vs-dark');
	const color = (token) => m.rules.find((r) => r.token === token).foreground;
	assert.equal(color('keyword'), 'aaffff');
	assert.equal(color('type.identifier'), 'aaffff'); // eingebaute Befehle wie im Original
	assert.equal(color('identifier'), 'ffffff');
	assert.equal(color('comment'), 'ffee00');
	assert.equal(color('string'), '00ff66');
	assert.equal(color('number'), '33ffdd');
	assert.equal(m.colors['editor.background'], '#225588');
	assert.equal(themeToMonaco({ ...classic, dark: false }).base, 'vs');
	assert.equal(themeToMonaco({ ...classic, monaco: { 'editor.background': '#000000' } }).colors['editor.background'], '#000000');
});

// ---------------------------------------------------------- passive Kürzel

test('keybindings: passive Kürzel erscheinen im Menü, lösen aber nie etwas aus', () => {
	const context = createContext();
	const contributions = createContributions();
	const kb = createKeybindings({ contributions, context });
	contributions.contribute('keybindings', [
		{ command: 'edit.copy', key: 'Ctrl+C', passive: true },
		{ command: 'edit.find', key: 'Ctrl+F' }
	], 'e');
	assert.equal(kb.keyFor('edit.copy'), 'Ctrl+C');
	assert.equal(kb.resolve('Ctrl+C'), null);
	assert.equal(kb.resolve('Ctrl+F').command, 'edit.find');
});
