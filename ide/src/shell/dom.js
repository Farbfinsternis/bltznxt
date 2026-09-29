// Kleiner DOM-Helfer. Bewusst kein Framework: die Shell ist dünn und soll
// ersetzbar bleiben (PLAN.md, "Was wir bewusst nicht festlegen").

/**
 * @param {string} tag
 * @param {Record<string, any> | null} [props]  `class`, `text`, `on*`, `data-*`, sonst Attribute
 * @param {...(Node | string | null | undefined | false)} children
 */
export function h(tag, props, ...children) {
	const el = document.createElement(tag);
	for (const [key, value] of Object.entries(props || {})) {
		if (value === undefined || value === null || value === false) continue;
		if (key === 'class') el.className = value;
		else if (key === 'text') el.textContent = value;
		else if (key.startsWith('on') && typeof value === 'function') el.addEventListener(key.slice(2), value);
		else el.setAttribute(key, value === true ? '' : String(value));
	}
	for (const child of children) {
		if (child === null || child === undefined || child === false) continue;
		el.append(child instanceof Node ? child : document.createTextNode(String(child)));
	}
	return el;
}

/** Icon aus einem Beitrag: SVG-Text der Erweiterung oder ein kurzes Zeichen. */
export function iconNode(icon) {
	const span = document.createElement('span');
	span.className = 'icon';
	if (typeof icon === 'string' && icon.trimStart().startsWith('<svg')) span.innerHTML = icon;
	else span.textContent = icon || '';
	return span;
}

/** Fasst mehrere Aufrufe in einem Tick zusammen. */
export function batch(fn) {
	let queued = false;
	return () => {
		if (queued) return;
		queued = true;
		queueMicrotask(() => {
			queued = false;
			fn();
		});
	};
}
