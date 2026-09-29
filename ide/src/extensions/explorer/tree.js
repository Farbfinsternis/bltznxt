// Die reine Logik der Dateiliste: Pfade, Sortierung, Ausblenden, Wurzelordner.
// Kein DOM, kein Backend — damit sie sich unter Node prüfen lässt.

const norm = (p) => String(p).replace(/\//g, '\\').replace(/\\+$/, '').toLowerCase();

/** Ordner einer Datei (Trenner `\` oder `/`); leer, wenn es keinen gibt. */
export function dirname(p) {
	const s = String(p);
	const i = Math.max(s.lastIndexOf('\\'), s.lastIndexOf('/'));
	if (i < 0) return '';
	// "C:\" bleibt "C:\", nicht "C:"
	if (i === 2 && /^[A-Za-z]:/.test(s)) return s.slice(0, 3);
	return i === 0 ? s.slice(0, 1) : s.slice(0, i);
}

export function basename(p) {
	const s = String(p).replace(/[\\/]+$/, '');
	return s.slice(Math.max(s.lastIndexOf('\\'), s.lastIndexOf('/')) + 1) || s;
}

/** Pfad eines Eintrags: dieselbe Schreibweise des Trenners wie der Ordner. */
export function joinPath(dir, name) {
	if (!dir) return name;
	const sep = dir.includes('\\') || !dir.includes('/') ? '\\' : '/';
	return dir.endsWith('\\') || dir.endsWith('/') ? `${dir}${name}` : `${dir}${sep}${name}`;
}

/** Liegt `path` in `root` (oder ist es `root`)? Ohne Rücksicht auf Groß-/Kleinschreibung und Trenner. */
export function isInside(root, path) {
	if (!root || !path) return false;
	const r = norm(root);
	const p = norm(path);
	return p === r || p.startsWith(`${r}\\`);
}

/** Einfaches Muster: `*` beliebig viele Zeichen, `?` eines; ohne Rücksicht auf Groß-/Kleinschreibung. */
function globToRegExp(pattern) {
	const escaped = String(pattern).replace(/[.+^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*').replace(/\?/g, '.');
	return new RegExp(`^${escaped}$`, 'i');
}

/** Soll ein Eintrag ausgeblendet werden? */
export function isHidden(name, patterns) {
	return (patterns || []).some((p) => typeof p === 'string' && p && globToRegExp(p).test(name));
}

/**
 * Ordner zuerst, dann nach Namen; Zahlen im Namen zählen als Zahlen
 * (`level2` vor `level10`), ohne Rücksicht auf Groß-/Kleinschreibung.
 */
export function sortEntries(entries) {
	const collator = new Intl.Collator('en', { numeric: true, sensitivity: 'base' });
	return [...entries].sort((a, b) => (a.dir === b.dir ? collator.compare(a.name, b.name) : a.dir ? -1 : 1));
}

/** Aufbereiteter Ordnerinhalt: ohne ausgeblendete Einträge, sortiert. */
export function visibleEntries(entries, hidePatterns) {
	return sortEntries(entries.filter((e) => !isHidden(e.name, hidePatterns)));
}

const MEDIA_EXTENSIONS = new Set([
	'png', 'jpg', 'jpeg', 'bmp', 'tga', 'gif', 'pcx', 'iff', 'webp',
	'wav', 'mp3', 'ogg', 'mid', 'midi', 'mod', 's3m', 'xm', 'it', 'rmi', 'sgt', 'flac',
	'x', '3ds', 'md2', 'b3d', 'glb', 'gltf'
]);

/** Ein Bild, ein Klang oder ein Modell — kein Text, der in den Editor gehört. */
export function isMedia(name) {
	const m = /\.([^.\\/]+)$/.exec(String(name));
	return Boolean(m) && MEDIA_EXTENSIONS.has(m[1].toLowerCase());
}

/**
 * Welcher Ordner steht oben in der Liste?
 *
 *   * Hat der Benutzer einen Ordner gewählt, gilt der.
 *   * Sonst folgt die Liste der aktiven Datei — wie im Original, wo ein
 *     Programm an den Ordner seiner Quelle gebunden ist. Liegt die Datei
 *     schon im gezeigten Ordner (etwa ein Include in einem Unterordner),
 *     bleibt alles, wie es ist.
 *   * Ohne Datei bleibt der bisherige Ordner.
 *
 * @param {{ explicitRoot?: string | null, activePath?: string | null, currentRoot?: string | null }} o
 * @returns {string | null}
 */
export function decideRoot({ explicitRoot = null, activePath = null, currentRoot = null } = {}) {
	if (explicitRoot) return explicitRoot;
	if (activePath) {
		if (currentRoot && isInside(currentRoot, activePath)) return currentRoot;
		return dirname(activePath) || currentRoot;
	}
	return currentRoot;
}
