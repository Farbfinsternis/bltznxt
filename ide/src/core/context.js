// Kontextschlüssel und "when"-Ausdrücke.
//
// Ob ein Menüpunkt sichtbar, ein Befehl freigegeben oder eine Ansicht
// eingeblendet ist, steht als kleiner Ausdruck in den Daten der Erweiterung
// ("document.active && !build.running") und nicht als Code. So kann jede
// Erweiterung den Zustand anderer lesen, ohne sie zu kennen.
//
// Grammatik:
//   or      := and ('||' and)*
//   and     := unary ('&&' unary)*
//   unary   := '!' unary | primary
//   primary := '(' or ')' | key [('==' | '!=') literal]
//   literal := 'text' | "text" | Zahl | true | false

import { createEmitter } from './events.js';

function tokenize(src) {
	const tokens = [];
	let i = 0;
	while (i < src.length) {
		const c = src[i];
		if (/\s/.test(c)) {
			i++;
		} else if (src.startsWith('&&', i) || src.startsWith('||', i) || src.startsWith('==', i) || src.startsWith('!=', i)) {
			tokens.push({ t: src.slice(i, i + 2) });
			i += 2;
		} else if (c === '!' || c === '(' || c === ')') {
			tokens.push({ t: c });
			i++;
		} else if (c === "'" || c === '"') {
			const end = src.indexOf(c, i + 1);
			if (end < 0) throw new Error(`when: Zeichenkette nicht geschlossen in "${src}"`);
			tokens.push({ t: 'lit', v: src.slice(i + 1, end) });
			i = end + 1;
		} else if (/[0-9]/.test(c)) {
			const m = /^[0-9]+(\.[0-9]+)?/.exec(src.slice(i));
			tokens.push({ t: 'lit', v: Number(m[0]) });
			i += m[0].length;
		} else if (/[A-Za-z_]/.test(c)) {
			const m = /^[A-Za-z_][\w.\-]*/.exec(src.slice(i));
			tokens.push({ t: 'id', v: m[0] });
			i += m[0].length;
		} else {
			throw new Error(`when: unerwartetes Zeichen "${c}" in "${src}"`);
		}
	}
	return tokens;
}

/**
 * Zerlegt einen Ausdruck. Wirft bei Syntaxfehlern — Aufrufer, die zur Laufzeit
 * auswerten, benutzen `evaluate`, das nie wirft.
 * @param {string} src
 * @returns {{ type: string, [k: string]: any }}
 */
export function parseWhen(src) {
	const tokens = tokenize(src);
	let pos = 0;
	const peek = () => tokens[pos];
	const take = () => tokens[pos++];

	function parseOr() {
		let left = parseAnd();
		while (peek() && peek().t === '||') {
			take();
			left = { type: 'or', left, right: parseAnd() };
		}
		return left;
	}
	function parseAnd() {
		let left = parseUnary();
		while (peek() && peek().t === '&&') {
			take();
			left = { type: 'and', left, right: parseUnary() };
		}
		return left;
	}
	function parseUnary() {
		if (peek() && peek().t === '!') {
			take();
			return { type: 'not', operand: parseUnary() };
		}
		return parsePrimary();
	}
	function parsePrimary() {
		const tok = take();
		if (!tok) throw new Error(`when: Ausdruck endet zu früh in "${src}"`);
		if (tok.t === '(') {
			const inner = parseOr();
			if (!peek() || take().t !== ')') throw new Error(`when: ")" fehlt in "${src}"`);
			return inner;
		}
		if (tok.t === 'id') {
			const next = peek();
			if (next && (next.t === '==' || next.t === '!=')) {
				take();
				const rhs = take();
				if (!rhs || (rhs.t !== 'lit' && rhs.t !== 'id')) {
					throw new Error(`when: Vergleichswert fehlt in "${src}"`);
				}
				let value = rhs.v;
				if (rhs.t === 'id') {
					if (value === 'true') value = true;
					else if (value === 'false') value = false;
					else throw new Error(`when: Vergleichswert "${value}" muss in Anführungszeichen stehen ("${src}")`);
				}
				return { type: 'cmp', key: tok.v, op: next.t, value };
			}
			return { type: 'key', key: tok.v };
		}
		throw new Error(`when: unerwartetes "${tok.t}" in "${src}"`);
	}

	const tree = parseOr();
	if (pos < tokens.length) throw new Error(`when: Überschuss nach dem Ausdruck in "${src}"`);
	return tree;
}

function run(node, get) {
	switch (node.type) {
		case 'or': return run(node.left, get) || run(node.right, get);
		case 'and': return run(node.left, get) && run(node.right, get);
		case 'not': return !run(node.operand, get);
		case 'key': return Boolean(get(node.key));
		case 'cmp': {
			const eq = get(node.key) === node.value;
			return node.op === '==' ? eq : !eq;
		}
		default: throw new Error(`when: unbekannter Knoten ${node.type}`);
	}
}

export function createContext() {
	/** @type {Map<string, any>} */
	const values = new Map();
	/** @type {Map<string, any>} */
	const cache = new Map();
	const warned = new Set();
	const events = createEmitter();

	function set(key, value) {
		if (values.get(key) === value) return;
		values.set(key, value);
		events.emit('change', { key, value });
	}

	function get(key) {
		return values.get(key);
	}

	/**
	 * Wertet eine Bedingung aus. Ohne Bedingung: wahr. Eine Funktion wird
	 * aufgerufen; ein Ausdruck mit Syntaxfehler ist falsch und wird genau einmal
	 * gemeldet — ein kaputter Menüpunkt darf die Oberfläche nicht lahmlegen.
	 * @param {string | (() => boolean) | null | undefined} when
	 */
	function evaluate(when) {
		if (when === undefined || when === null || when === '') return true;
		if (typeof when === 'function') {
			try {
				return Boolean(when());
			} catch (err) {
				console.error('[context] when-Funktion hat geworfen:', err);
				return false;
			}
		}
		try {
			let tree = cache.get(when);
			if (!tree) cache.set(when, (tree = parseWhen(when)));
			return run(tree, get);
		} catch (err) {
			if (!warned.has(when)) {
				warned.add(when);
				console.error(`[context] ${err.message}`);
			}
			return false;
		}
	}

	return {
		set,
		get,
		evaluate,
		/** @param {(e: {key: string, value: any}) => void} fn */
		onDidChange: (fn) => events.on('change', fn)
	};
}
