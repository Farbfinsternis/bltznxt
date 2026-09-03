import './style.css';
import * as monaco from 'monaco-editor';
import './userWorker'; // Importiert unsere Worker-Konfiguration
import { setCommands } from './blitz3d'; // Importiert unsere Sprachdefinition
import platform from './platform';

const app = document.querySelector('#app');

// Editor erstellen
const editor = monaco.editor.create(app, {
	value: `; Beispiel für Blitz3D Code mit Keywords und Commands
Function Main()
	Graphics 800, 600 ; Das ist ein Command
	Print "Hallo Welt aus meiner IDE!"
End Function`,
	language: 'blitz3d', // Unsere neue Sprache verwenden
	theme: 'vs-dark',
	automaticLayout: true
});

// Befehlsliste und Compiler-Status beim Start vom Compiler holen.
// Ohne Backend (reiner Browser via `npm run vite`) läuft der Editor weiter,
// nur ohne Befehls-Vervollständigung.
async function connectCompiler() {
	if (!platform.hasBackend) {
		console.info('[platform] Kein Backend — Editor läuft ohne Compiler-Anbindung.');
		return;
	}

	const info = await platform.getInfo();
	if (!info.available) {
		console.warn('[platform] blitzcc nicht gefunden. Pfad setzen oder BLITZPATH belegen.');
		return;
	}
	console.info(`[platform] blitzcc v${info.version ?? '?'} — ${info.path} (via ${info.source})`);

	const count = setCommands(await platform.listCommands());
	console.info(`[platform] ${count} eingebaute Befehle geladen.`);
}

/**
 * Kompiliert den aktuellen Editorinhalt und setzt die Fehler als Marker.
 * Noch ohne Dateiverwaltung: der Pfad kommt von außen herein.
 */
export async function compileCurrentFile(file) {
	const result = await platform.compile(file);
	monaco.editor.setModelMarkers(
		editor.getModel(),
		'blitzcc',
		platform.toMonacoMarkers(monaco, result.diagnostics)
	);
	return result;
}

connectCompiler();
