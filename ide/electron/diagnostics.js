// Parser für die Compiler-Ausgabe von blitzcc.
//
// blitzcc meldet Fehler im GCC-Format auf stderr:
//   datei.bb:12:5: error: unexpected token 'THEN'
//   datei.bb:0:0: error: compilation failed
//
// Das Format ist der einzige Kontrakt zwischen IDE und Compiler — es ist in
// Phase A der Compiler-Roadmap bewusst so festgelegt worden, damit beliebige
// Editoren es lesen können. Dieser Parser darf deshalb nichts über den
// Compiler wissen außer diesem einen Muster.

'use strict';

// datei:zeile:spalte: schweregrad: nachricht
const DIAG_RE = /^(.*?):(\d+):(\d+):\s*(error|warning|note):\s*(.*)$/;

/**
 * Zerlegt stderr in strukturierte Diagnosen.
 *
 * Zeilen, die dem Muster nicht entsprechen (z.B. "[runtime] ..."-Meldungen
 * oder g++-Ausgaben), werden in `unparsed` gesammelt statt verworfen — beim
 * Debuggen ist die Rohausgabe wertvoller als eine leere Liste.
 *
 * @param {string} stderr
 * @returns {{ diagnostics: Array, unparsed: string[] }}
 */
function parseDiagnostics(stderr) {
	const diagnostics = [];
	const unparsed = [];

	for (const raw of String(stderr || '').split(/\r?\n/)) {
		const line = raw.trimEnd();
		if (!line) continue;

		const m = DIAG_RE.exec(line);
		if (!m) {
			unparsed.push(line);
			continue;
		}

		const [, file, lineNo, colNo, severity, message] = m;

		diagnostics.push({
			file,
			// blitzcc meldet 0:0 wenn der Fehler keiner Quellzeile zuzuordnen
			// ist (z.B. "compilation failed"). Monaco-Marker sind 1-basiert,
			// darum hier auf 1 anheben.
			line: Math.max(1, parseInt(lineNo, 10)),
			column: Math.max(1, parseInt(colNo, 10)),
			severity,
			message,
			raw: line
		});
	}

	return { diagnostics, unparsed };
}

/**
 * Stammt eine Diagnose aus der Blitz3D-Quelle oder aus der Toolchain?
 *
 * Wichtig, weil blitzcc zwei Sorten Fehler im selben Format ausgibt:
 *
 *   parse.bb:3:9: error: unexpected token 'THEN'        <- Quelle, Zeile stimmt
 *   parse.cpp:6:10: error: 'var_x' was not declared      <- generiertes C++
 *
 * Die zweite Sorte entsteht, wenn der Transpiler durchläuft und erst g++
 * scheitert (Exit-Code 2). Ihre Zeilennummern gehören zur generierten .cpp und
 * haben mit der .bb nichts zu tun — als Editor-Marker gesetzt zeigen sie auf
 * die falsche Zeile. Sie gehören in ein Ausgabefenster, nicht an den Rand.
 */
function isSourceDiagnostic(d) {
	return /\.bb$/i.test(d.file);
}

/**
 * Übersetzt einen blitzcc-Exit-Code in einen sprechenden Status.
 * 0 = ok, 1 = Parse-/Lex-/Semantikfehler, 2 = C++-Compile-Fehler.
 */
function statusFromExitCode(code) {
	if (code === 0) return 'ok';
	if (code === 1) return 'parse-error';
	if (code === 2) return 'compile-error';
	return 'failed';
}

module.exports = { parseDiagnostics, isSourceDiagnostic, statusFromExitCode, DIAG_RE };
