// Plattform-Schnittstelle der IDE.
//
// Der gesamte Renderer-Code spricht nur über dieses Modul mit der Außenwelt.
// Wer hier `window.bltznxt` umgeht und direkt Node benutzt, macht aus dem
// späteren Backend-Wechsel (Electron -> Tauri) einen Umbau der ganzen IDE
// statt eines Austauschs dieser Datei.
//
// Zweite Regel, siehe README.md "Repository Layout": die IDE weiß nichts über
// den Compiler außer seiner Kommandozeile. Insbesondere wird die Liste der
// eingebauten Befehle bei jedem Start frisch per `blitzcc +k` erfragt und
// nicht hier einkopiert.

const bridge = typeof window !== 'undefined' ? window.bltznxt : undefined;

/** Läuft die IDE mit angebundenem Backend? Im reinen Browser (vite dev) nicht. */
export const hasBackend = Boolean(bridge);

function requireBridge(fn) {
	if (!bridge) {
		throw new Error(
			`platform.${fn}() ist ohne Electron-Backend nicht verfügbar. ` +
			`Vorher hasBackend prüfen.`
		);
	}
	return bridge;
}

/**
 * Wo liegt der Compiler, welche Version, ist er überhaupt da?
 * @returns {Promise<{path: string|null, source: string, available: boolean, version: string|null}>}
 *   source ist einer von: 'setting' | 'BLITZPATH' | 'PATH' | 'dev-fallback' | 'none'
 */
export function getInfo() {
	return requireBridge('getInfo').getInfo();
}

/**
 * Die eingebauten Befehle des angebundenen Compilers, für Autovervollständigung
 * und Hover. Ohne Compiler eine leere Liste — nie ein Fehler, die IDE muss
 * auch ohne Compiler bedienbar bleiben.
 * @returns {Promise<Array<{name: string, signature: string, params: string[]}>>}
 */
export function listCommands() {
	if (!bridge) return Promise.resolve([]);
	return bridge.listCommands();
}

/**
 * Kompiliert eine .bb-Datei.
 *
 * `diagnostics` enthält nur Meldungen zur .bb-Quelle und darf als Editor-Marker
 * gesetzt werden. `toolchain` enthält g++-Meldungen zur generierten .cpp — deren
 * Zeilennummern gehören zu einer anderen Datei, sie gehören in ein
 * Ausgabefenster und niemals an den Editorrand.
 *
 * @param {string} file absoluter Pfad
 * @param {{debug?: boolean, transpileOnly?: boolean, outputName?: string}} [opts]
 * @returns {Promise<{
 *   status: 'ok'|'parse-error'|'compile-error'|'no-compiler'|'failed',
 *   exitCode: number,
 *   diagnostics: Array<{file: string, line: number, column: number, severity: string, message: string, raw: string}>,
 *   toolchain: Array<object>,
 *   unparsed: string[],
 *   stdout: string,
 *   stderr: string,
 *   outputPath: string|null
 * }>}
 */
export function compile(file, opts = {}) {
	return requireBridge('compile').compile(file, opts);
}

/** Compiler-Pfad setzen (null löscht die Einstellung wieder). */
export function setCompilerPath(path) {
	return requireBridge('setCompilerPath').setCompilerPath(path);
}

/**
 * Diagnosen in Monaco-Marker übersetzen.
 *
 * Bewusst hier und nicht im Editor-Code: die Form der Diagnosen gehört zur
 * Plattform-Schnittstelle, und Monaco ist die einzige Stelle, die davon
 * abhängt. Der Aufrufer übergibt `monaco`, damit dieses Modul keine
 * Abhängigkeit auf den Editor bekommt.
 */
export function toMonacoMarkers(monaco, diagnostics) {
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
	hasBackend,
	getInfo,
	listCommands,
	compile,
	setCompilerPath,
	toMonacoMarkers
};
